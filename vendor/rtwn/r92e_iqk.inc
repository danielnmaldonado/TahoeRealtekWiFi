// SPDX-License-Identifier: GPL-2.0-only
/*
 * RTL8192EU host IQ calibration, adapted from Linux rtl8xxxu/8192e.c
 * and core.c, commit 0e135910deec43ffe97eca0b6af1f8f847285b59.
 * Copyright (c) 2014 - 2017 Jes Sorensen <Jes.Sorensen@gmail.com>
 * Calibration portions: Copyright (c) 2007 - 2011 Realtek Corporation.
 *
 * Policy: use the pinned Linux TX, TXIMR, RX and gen2 comparison profile
 * together (two retries, three runs, signed Y, tolerance five). In particular
 * RX B retains Linux's PI_A writes. Realtek PHYDM at c629cc972a0d878c930d19c4a9309c27c1e61f92
 * uses PI_B there; neither choice has been verified on this hardware. Linux's
 * corrected B TXIMR failure cleanup addresses RF_B, not RF_A.
 *
 * Port adaptations: explicit phase validity, reject absent all-zero/all-ones
 * measurements, best-effort restoration even on a sticky transport error,
 * and restore snapshots before applying any selected compensation matrix.
 * The RF mode/LUT programming (ef/30/31/32) is kept as in Linux: these are
 * banked tables, and reading those four addresses once cannot snapshot both
 * IQK mode entries. They are deliberately not treated as independent state
 * registers to restore. AC/df/56 controls and RF-helper BB side effects are
 * restored. Plausible stale reports after a completed shot are not detectable
 * by this interface: Linux provides no completion/freshness token here.
 * Include after rtwn_iq_calib_write_results(), with rtwn transports in scope.
 */

struct rtwn_r92e_iqk_result {
	uint16_t value[8];
	uint8_t valid; /* TX A, RX A, TX B, RX B, independently measured */
};

static const uint16_t rtwn_r92e_iqk_adda[] = {
	0x85c, 0xe6c, 0xe70, 0xe74, 0xe78, 0xe7c, 0xe80, 0xe84,
	0xe88, 0xe8c, 0xed0, 0xed4, 0xed8, 0xedc, 0xee0, 0xeec
};
static const uint16_t rtwn_r92e_iqk_bb[] = {
	0xc04, 0xc08, 0x874, 0xb68, 0xb6c, 0x870, 0x860, 0x864, 0xa04,
	0xc50, 0xc58,
	0xe28, 0xe30, 0xe34, 0xe38, 0xe3c, 0xe40, 0xe44, 0xe48,
	0xe4c, 0xe50, 0xe54, 0xe58, 0xe5c,
	/* rtwn_rf_read/write also modify these BB interface registers. */
	0x818, 0x824, 0x82c, 0x840, 0x844
};
static const uint8_t rtwn_r92e_iqk_rf[] = { 0x00, 0xdf, 0x56 };
static const uint16_t rtwn_r92e_iqk_matrix[] = {
    0xc80, 0xc88, 0xc94, 0xc9c, 0xc4c, 0xc14, 0xc1c, 0xca0, 0xc78
};
struct rtwn_r92e_iqk_saved {
	uint32_t adda[nitems(rtwn_r92e_iqk_adda)];
	uint32_t bb[nitems(rtwn_r92e_iqk_bb)];
	uint32_t rf[2][nitems(rtwn_r92e_iqk_rf)];
	uint8_t txpause, beacon[2];
	uint32_t gpio;
	uint32_t matrix[nitems(rtwn_r92e_iqk_matrix)];
};

static void
rtwn_r92e_iqk_lut(struct rtwn_softc *sc, int chain, uint32_t mode,
    uint32_t gain)
{
	rtwn_rf_write(sc, chain, 0xef, 0x800a0);
	rtwn_rf_write(sc, chain, 0x30, mode);
	rtwn_rf_write(sc, chain, 0x31, 0x0000f);
	rtwn_rf_write(sc, chain, 0x32, gain);
}

static void
rtwn_r92e_iqk_tones(struct rtwn_softc *sc, int chain, int rx)
{
	rtwn_bb_write(sc, 0xe30, !chain && !rx ? 0x18008c1c : 0x38008c1c);
	rtwn_bb_write(sc, 0xe34, !chain && rx ? 0x18008c1c : 0x38008c1c);
	rtwn_bb_write(sc, 0xe50, chain && !rx ? 0x18008c1c : 0x38008c1c);
	rtwn_bb_write(sc, 0xe54, chain && rx ? 0x18008c1c : 0x38008c1c);
}

static int
rtwn_r92e_iqk_shot(struct rtwn_softc *sc, int chain, uint32_t wait_us)
{
	if (rtwn_io_error(sc))
		return (0);
	rtwn_bb_write(sc, 0xe48, chain ? 0xfa000000 : 0xf9000000);
	rtwn_bb_write(sc, 0xe48, 0xf8000000);
	rtwn_delay(sc, wait_us);
	return (!rtwn_io_error(sc));
}

static int
rtwn_r92e_iqk_measure(struct rtwn_softc *sc, int chain, int rx,
    uint16_t out[2])
{
	uint32_t status, x, y;
	const uint16_t base = (rx ? 0xea4 : 0xe94) + chain * 0x20;

	status = rtwn_bb_read(sc, 0xeac);
	x = rtwn_bb_read(sc, base);
	y = rtwn_bb_read(sc, base + 8);
	if (rtwn_io_error(sc) || status == 0xffffffff ||
	    x == 0xffffffff || y == 0xffffffff ||
	    (status & (1U << ((rx ? 27 : 28) + chain * 3))))
		return (0);
	const uint16_t a = (x >> 16) & 0x3ff;
	const uint16_t b = (y >> 16) & 0x3ff;
	if ((!a && !b) || a == (rx ? 0x132 : 0x142) ||
	    b == (rx ? 0x036 : 0x042))
		return (0);
	out[0] = a;
	out[1] = b;
	return (1);
}

/* The main TX measurement and the TXIMR prerequisite are distinct shots. */
static int
rtwn_r92e_iqk_tx(struct rtwn_softc *sc, int chain, int imr,
    uint16_t out[2])
{
	const uint16_t pi = 0xe38 + chain * 0x20;

	rtwn_bb_write(sc, 0xe28, 0);
	if (imr) {
		rtwn_r92e_iqk_lut(sc, chain, 0x30000, 0xf1173);
		rtwn_r92e_iqk_lut(sc, !chain, 0x30000, 0xf1173);
		rtwn_rf_write(sc, chain, 0xdf, 0x00980);
		rtwn_rf_write(sc, chain, 0x56, 0x511e0);
	} else {
		rtwn_rf_write(sc, chain, 0xdf, 0x00180);
		rtwn_r92e_iqk_lut(sc, chain, 0x20000, 0x07f77);
	}
	rtwn_bb_write(sc, 0xe28, 0x80800000);
	if (imr) {
		rtwn_bb_write(sc, 0xe40, 0x01007c00);
		rtwn_bb_write(sc, 0xe44, 0x01004800);
	}
	rtwn_r92e_iqk_tones(sc, chain, 0);
	rtwn_bb_write(sc, pi, imr ? 0x8216031f : 0x82140303);
	rtwn_bb_write(sc, pi + 4, imr ? 0x6816031f : 0x68160000);
	rtwn_bb_write(sc, 0xe4c, imr ? 0x0046a911 : 0x00462911);
	if (!rtwn_r92e_iqk_shot(sc, chain, chain && !imr ? 1000 : 10000))
		return (0);
	return (rtwn_r92e_iqk_measure(sc, chain, 0, out));
}

static int
rtwn_r92e_iqk_rx(struct rtwn_softc *sc, int chain, uint16_t out[2])
{
	uint16_t imr[2];
	int ok = 0;

	if (!rtwn_r92e_iqk_tx(sc, chain, 1, imr))
		goto done;
	rtwn_bb_write(sc, 0xe40, 0x80007c00 | (uint32_t(imr[0]) << 16) | imr[1]);
	rtwn_bb_write(sc, 0xe28, 0);
	rtwn_r92e_iqk_lut(sc, chain, 0x30000, 0xf7ff2);
	rtwn_r92e_iqk_lut(sc, !chain, 0x30000, 0xf7ff2);
	rtwn_rf_write(sc, chain, 0xdf, 0x00980);
	rtwn_rf_write(sc, chain, 0x56, 0x510e0);
	rtwn_bb_write(sc, 0xe28, 0x80800000);
	rtwn_bb_write(sc, 0xe44, 0x01004800);
	rtwn_r92e_iqk_tones(sc, chain, 1);
	/* Preserve pinned Linux RX-B PI_A writes; PHYDM uses PI_B. */
	rtwn_bb_write(sc, 0xe38, 0x821608ff);
	rtwn_bb_write(sc, 0xe3c, 0x281608ff);
	rtwn_bb_write(sc, 0xe4c, 0x0046a891);
	if (rtwn_r92e_iqk_shot(sc, chain, 10000))
		ok = rtwn_r92e_iqk_measure(sc, chain, 1, out);
 done:
	/* Also executed after failed TXIMR and sticky USB errors. */
	rtwn_bb_write(sc, 0xe28, 0);
	rtwn_rf_write(sc, chain, 0xdf, 0x180);
	return (ok && !rtwn_io_error(sc));
}

static void
rtwn_r92e_iqk_restore(struct rtwn_softc *sc,
    const struct rtwn_r92e_iqk_saved *saved, int rf_saved)
{
	rtwn_bb_write(sc, 0xe28, 0);
	if (rf_saved) {
		for (int chain = 0; chain < 2; chain++)
			for (unsigned i = 0; i < nitems(rtwn_r92e_iqk_rf); i++)
				rtwn_rf_write(sc, chain, rtwn_r92e_iqk_rf[i], saved->rf[chain][i]);
	}
	for (unsigned i = 0; i < nitems(rtwn_r92e_iqk_adda); i++)
		rtwn_bb_write(sc, rtwn_r92e_iqk_adda[i], saved->adda[i]);
	rtwn_write_1(sc, R92C_TXPAUSE, saved->txpause);
	rtwn_write_1(sc, R92C_BCN_CTRL, saved->beacon[0]);
	rtwn_write_1(sc, R92C_BCN_CTRL1, saved->beacon[1]);
	rtwn_write_4(sc, R92C_GPIO_MUXCFG, saved->gpio);
	/* Pulse RX initial gain before restoring the exact BB snapshot. */
	rtwn_bb_write(sc, 0xc50, (saved->bb[9] & ~0xffU) | 0x50);
	rtwn_bb_write(sc, 0xc58, (saved->bb[10] & ~0xffU) | 0x50);
	for (unsigned i = 0; i < nitems(rtwn_r92e_iqk_bb); i++)
		rtwn_bb_write(sc, rtwn_r92e_iqk_bb[i], saved->bb[i]);
}

/* Linux gen2 compares signed ten-bit Y, unsigned X, within five. Keep
 * independent validity for each measured pair instead of Linux's zero row
 * sentinel, so two failed phases never become a stable candidate. */
static int
rtwn_r92e_iqk_compare(const struct rtwn_r92e_iqk_result *a,
    const struct rtwn_r92e_iqk_result *b,
    struct rtwn_r92e_iqk_result *partial, int chains)
{
	uint8_t same = 0;
	for (int pair = 0; pair < chains * 2; pair++) {
		const uint8_t bit = 1U << pair;
		if (!(a->valid & b->valid & bit))
			continue;
		int x = int(a->value[pair * 2]) - int(b->value[pair * 2]);
		int ya = a->value[pair * 2 + 1];
		int yb = b->value[pair * 2 + 1];
		if (ya & 0x200) ya -= 0x400;
		if (yb & 0x200) yb -= 0x400;
		int y = ya - yb;
		if (x < -5 || x > 5 || y < -5 || y > 5)
			continue;
		same |= bit;
		partial->value[pair * 2] = a->value[pair * 2];
		partial->value[pair * 2 + 1] = a->value[pair * 2 + 1];
		partial->valid |= bit;
	}
	return (same == ((1U << (chains * 2)) - 1));
}

static void
rtwn_r92e_iq_calib(struct rtwn_softc *sc)
{
	struct rtwn_r92e_iqk_saved saved;
	struct rtwn_r92e_iqk_result result[4] = {};
	int candidate = -1;
	const int chains = sc->ntxchains;

	sc->iq_calibration_mask = 0;
	if (rtwn_io_error(sc) || chains < 1 || chains > 2)
		return;
	for (unsigned i = 0; i < nitems(rtwn_r92e_iqk_adda); i++)
		saved.adda[i] = rtwn_bb_read(sc, rtwn_r92e_iqk_adda[i]);
	for (unsigned i = 0; i < nitems(rtwn_r92e_iqk_bb); i++)
		saved.bb[i] = rtwn_bb_read(sc, rtwn_r92e_iqk_bb[i]);
	for (unsigned i = 0; i < nitems(rtwn_r92e_iqk_matrix); i++)
		saved.matrix[i] = rtwn_bb_read(sc, rtwn_r92e_iqk_matrix[i]);
	saved.txpause = rtwn_read_1(sc, R92C_TXPAUSE);
	saved.beacon[0] = rtwn_read_1(sc, R92C_BCN_CTRL);
	saved.beacon[1] = rtwn_read_1(sc, R92C_BCN_CTRL1);
	saved.gpio = rtwn_read_4(sc, R92C_GPIO_MUXCFG);
	/* No hardware writes have occurred until the RF read helper below. */
	if (rtwn_io_error(sc))
		return;
	for (int chain = 0; chain < 2; chain++)
		for (unsigned i = 0; i < nitems(rtwn_r92e_iqk_rf); i++)
			saved.rf[chain][i] = rtwn_rf_read(sc, chain, rtwn_r92e_iqk_rf[i]);
	if (rtwn_io_error(sc)) {
		rtwn_r92e_iqk_restore(sc, &saved, 0);
		return;
	}
	for (int run = 0; run < 3 && !rtwn_io_error(sc); run++) {
		for (unsigned i = 0; i < nitems(rtwn_r92e_iqk_adda); i++)
			rtwn_bb_write(sc, rtwn_r92e_iqk_adda[i], 0x0fc01616);
		rtwn_write_1(sc, R92C_TXPAUSE, 0x3f);
		rtwn_write_1(sc, R92C_BCN_CTRL, saved.beacon[0] & ~(1U << 3));
		rtwn_write_1(sc, R92C_BCN_CTRL1, saved.beacon[1] & ~(1U << 3));
		rtwn_write_1(sc, R92C_GPIO_MUXCFG, uint8_t(saved.gpio) & ~(1U << 5));
		rtwn_bb_write(sc, 0xa04, saved.bb[8] | 0x0f000000);
		rtwn_bb_write(sc, 0xc04, 0x03a05600);
		rtwn_bb_write(sc, 0xc08, 0x000800e4);
		rtwn_bb_write(sc, 0x874, 0x22208200);
		rtwn_bb_write(sc, 0x870, saved.bb[5] | (1U << 10) | (1U << 26));
		rtwn_bb_write(sc, 0x860, saved.bb[6] | (1U << 10));
		rtwn_bb_write(sc, 0x864, saved.bb[7] | (1U << 10));
		for (int chain = 0; chain < chains && !rtwn_io_error(sc); chain++) {
			if (chain) {
				rtwn_bb_write(sc, 0xe28, 0);
				rtwn_rf_write(sc, 0, 0x00, 0x10000);
			}
			rtwn_bb_write(sc, 0xe28, 0x80800000);
			rtwn_bb_write(sc, 0xe40, 0x01007c00);
			rtwn_bb_write(sc, 0xe44, 0x01004800);
			for (int retry = 0; retry < 2 && !rtwn_io_error(sc); retry++) {
				if (rtwn_r92e_iqk_tx(sc, chain, 0, &result[run].value[chain * 4])) {
					result[run].valid |= 1U << (chain * 2);
					break;
				}
			}
			for (int retry = 0; retry < 2 && !rtwn_io_error(sc); retry++) {
				if (rtwn_r92e_iqk_rx(sc, chain, &result[run].value[chain * 4 + 2])) {
					result[run].valid |= 1U << (chain * 2 + 1);
					break;
				}
			}
		}
		if (run == 1 && rtwn_r92e_iqk_compare(&result[0], &result[1], &result[3], chains)) {
			candidate = 0;
			break;
		}
		if (run == 1) {
			/* Linux reloads ADDA/MAC/BB/AGC after t>0. Keep the
			 * inter-run RF state, including A standby, as upstream. */
			rtwn_r92e_iqk_restore(sc, &saved, 0);
			rtwn_bb_write(sc, 0xe28, 0);
			rtwn_bb_write(sc, 0xe30, 0x01008c00);
			rtwn_bb_write(sc, 0xe34, 0x01008c00);
		}
		if (run == 2) {
			if (rtwn_r92e_iqk_compare(&result[0], &result[2], &result[3], chains))
				candidate = 0;
			else if (rtwn_r92e_iqk_compare(&result[1], &result[2], &result[3], chains))
				candidate = 1;
			else
				candidate = 3;
		}
	}
	rtwn_r92e_iqk_restore(sc, &saved, 1);
	if (rtwn_io_error(sc) || candidate < 0)
		return;
	for (int chain = 0; chain < chains; chain++) {
		const uint8_t txbit = 1U << (chain * 2);
		const uint8_t rxbit = txbit << 1;
		uint16_t tx[2], rx[2] = { 0xffff, 0xffff };
		if (!(result[candidate].valid & txbit))
			continue;
		tx[0] = result[candidate].value[chain * 4];
		tx[1] = result[candidate].value[chain * 4 + 1];
		if (result[candidate].valid & rxbit) {
			rx[0] = result[candidate].value[chain * 4 + 2];
			rx[1] = result[candidate].value[chain * 4 + 3];
		}
		rtwn_iq_calib_write_results(sc, tx, rx, chain);
		if (rtwn_io_error(sc)) {
			sc->iq_calibration_mask = 0;
			for (unsigned i = 0; i < nitems(rtwn_r92e_iqk_matrix); i++)
				rtwn_bb_write(sc, rtwn_r92e_iqk_matrix[i], saved.matrix[i]);
			return;
		}
		if (result[candidate].valid & rxbit)
			sc->iq_calibration_mask |= 1U << chain;
	}
}
