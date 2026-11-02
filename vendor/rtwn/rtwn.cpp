/*	$OpenBSD: rtwn.c,v 1.61 2025/08/20 14:24:05 kevlo Exp $	*/

/*-
 * Copyright (c) 2010 Damien Bergamini <damien.bergamini@free.fr>
 * Copyright (c) 2015 Stefan Sperling <stsp@openbsd.org>
 * Copyright (c) 2016 Nathanial Sloss <nathanialsloss@yahoo.com.au>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Driver for Realtek 802.11b/g/n chipsets.
 */

/* macOS transport and mbuf adaptation; radio/state algorithms remain OpenBSD. */
#include "rtwnvar.h"
#include "rtw/Efuse.h"
#include "rtw/RTL8192CDataPath.h"
#include <sys/sockio.h>
#include <sys/errno.h>
#include <net/if.h>
#include <libkern/libkern.h>
#define NBPFILTER 0
#define DVACT_QUIESCE 2
#define DVACT_WAKEUP 5

void rtwn_delay(struct rtwn_softc *sc, uint32_t us)
{
    sc->sc_ops.delay_us(sc->sc_ops.cookie, us);
}
int rtwn_io_error(struct rtwn_softc *sc)
{
    return sc->core_error ? sc->core_error : sc->sc_ops.io_error(sc->sc_ops.cookie);
}

#define RTWN_RIDX_CCK1		0
#define RTWN_RIDX_CCK2		1
#define RTWN_RIDX_CCK11		3
#define RTWN_RIDX_OFDM6		4
#define RTWN_RIDX_OFDM54	11
#define RTWN_RIDX_MCS0		12
#define RTWN_RIDX_MCS8		(RTWN_RIDX_MCS0 + 8)
#define RTWN_RIDX_MCS15		27
#define RTWN_RIDX_MAX		27

#define RTWN_POWER_CCK1		0
#define RTWN_POWER_CCK2		1
#define RTWN_POWER_CCK55	2
#define RTWN_POWER_CCK11	3
#define RTWN_POWER_OFDM6	4
#define RTWN_POWER_OFDM9	5
#define RTWN_POWER_OFDM12	6
#define RTWN_POWER_OFDM18	7
#define RTWN_POWER_OFDM24	8
#define RTWN_POWER_OFDM36	9
#define RTWN_POWER_OFDM48	10
#define RTWN_POWER_OFDM54	11
#define RTWN_POWER_MCS(mcs)	(12 + (mcs))
#define RTWN_POWER_COUNT	28


#ifdef RTWN_DEBUG
#define DPRINTF(x)	do { if (rtwn_debug) printf x; } while (0)
#define DPRINTFN(n, x)	do { if (rtwn_debug >= (n)) printf x; } while (0)
int rtwn_debug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n, x)
#endif

/* Registers to save and restore during IQ calibration. */
struct rtwn_iq_cal_regs {
	uint32_t	adda[16];
	uint8_t		txpause;
	uint8_t		bcn_ctrl;
	uint8_t		bcn_ctrl1;
	uint32_t	gpio_muxcfg;
	uint32_t	ofdm0_trxpathena;
	uint32_t	ofdm0_trmuxpar;
	uint32_t	fpga0_rfifacesw0;
	uint32_t	fpga0_rfifacesw1;
	uint32_t	fpga0_rfifaceoe0;
	uint32_t	fpga0_rfifaceoe1;
	uint32_t	config_ant_a;
	uint32_t	config_ant_b;
	uint32_t	cck0_afesetting;
};

void		rtwn_write_1(struct rtwn_softc *, uint16_t, uint8_t);
void		rtwn_write_2(struct rtwn_softc *, uint16_t, uint16_t);
void		rtwn_write_4(struct rtwn_softc *, uint16_t, uint32_t);
uint8_t		rtwn_read_1(struct rtwn_softc *, uint16_t);
uint16_t	rtwn_read_2(struct rtwn_softc *, uint16_t);
uint32_t	rtwn_read_4(struct rtwn_softc *, uint16_t);
int		rtwn_fw_cmd(struct rtwn_softc *, uint8_t, const void *, int);
void		rtwn_rf_write(struct rtwn_softc *, int, uint16_t, uint32_t);
uint32_t	rtwn_rf_read(struct rtwn_softc *, int, uint8_t);
void		rtwn_cam_write(struct rtwn_softc *, uint32_t, uint32_t);
uint8_t		rtwn_efuse_read_1(struct rtwn_softc *, uint16_t);
void		rtwn_efuse_read(struct rtwn_softc *, uint8_t *, size_t);
void		rtwn_efuse_switch_power(struct rtwn_softc *);
int		rtwn_read_chipid(struct rtwn_softc *);
void		rtwn_read_rom(struct rtwn_softc *);
void		rtwn_r92c_read_rom(struct rtwn_softc *);
void		rtwn_r92e_read_rom(struct rtwn_softc *);
void		rtwn_r88e_read_rom(struct rtwn_softc *);
void		rtwn_r88f_read_rom(struct rtwn_softc *);
void		rtwn_r23a_read_rom(struct rtwn_softc *);
int		rtwn_media_change(struct _ifnet *);
int		rtwn_ra_init(struct rtwn_softc *);
int		rtwn_r92c_ra_init(struct rtwn_softc *, u_int8_t, u_int32_t,
		    int, uint32_t, int);
int		rtwn_r88e_ra_init(struct rtwn_softc *, u_int8_t, u_int32_t,
		    int, uint32_t, int);
void		rtwn_tsf_sync_enable(struct rtwn_softc *);
void		rtwn_set_led(struct rtwn_softc *, int, int);
void		rtwn_set_nettype(struct rtwn_softc *, enum ieee80211_opmode);
void		rtwn_update_short_preamble(struct ieee80211com *);
void		rtwn_r92c_update_short_preamble(struct rtwn_softc *);
void		rtwn_r88e_update_short_preamble(struct rtwn_softc *);
int8_t		rtwn_r88e_get_rssi(struct rtwn_softc *, int, void *);
int8_t		rtwn_r88f_get_rssi(struct rtwn_softc *, int, void *);
void		rtwn_watchdog(struct _ifnet *);
void		rtwn_fw_reset(struct rtwn_softc *);
void		rtwn_r92c_fw_reset(struct rtwn_softc *);
void		rtwn_r88e_fw_reset(struct rtwn_softc *);
int		rtwn_load_firmware(struct rtwn_softc *);
void		rtwn_rf_init(struct rtwn_softc *);
void		rtwn_cam_init(struct rtwn_softc *);
void		rtwn_pa_bias_init(struct rtwn_softc *);
void		rtwn_rxfilter_init(struct rtwn_softc *);
void		rtwn_edca_init(struct rtwn_softc *);
void		rtwn_rate_fallback_init(struct rtwn_softc *);
void		rtwn_write_txpower(struct rtwn_softc *, int, uint16_t *);
void		rtwn_get_txpower(struct rtwn_softc *sc, int,
		    struct ieee80211_channel *, struct ieee80211_channel *,
		    uint16_t *);
void		rtwn_r92c_get_txpower(struct rtwn_softc *, int,
		    struct ieee80211_channel *, struct ieee80211_channel *,
		    uint16_t *);
void		rtwn_r92e_get_txpower(struct rtwn_softc *, int,
		    struct ieee80211_channel *,
		    struct ieee80211_channel *, uint16_t *);
void		rtwn_r88e_get_txpower(struct rtwn_softc *, int,
		    struct ieee80211_channel *,
		    struct ieee80211_channel *, uint16_t *);
void		rtwn_set_txpower(struct rtwn_softc *,
		    struct ieee80211_channel *, struct ieee80211_channel *);
void		rtwn_set_chan(struct rtwn_softc *,
		    struct ieee80211_channel *, struct ieee80211_channel *);
int		rtwn_chan2group(int);
int		rtwn_iq_calib_chain(struct rtwn_softc *, int, uint16_t[2],
		    uint16_t[2]);
void		rtwn_iq_calib_run(struct rtwn_softc *, int, uint16_t[2][2],
    		    uint16_t rx[2][2], struct rtwn_iq_cal_regs *);
int		rtwn_iq_calib_compare_results(uint16_t[2][2], uint16_t[2][2],
		    uint16_t[2][2], uint16_t[2][2], int);
void		rtwn_iq_calib_write_results(struct rtwn_softc *, uint16_t[2],
		    uint16_t[2], int);
void		rtwn_iq_calib(struct rtwn_softc *);
void		rtwn_lc_calib(struct rtwn_softc *);
void		rtwn_temp_calib(struct rtwn_softc *);
void		rtwn_enable_intr(struct rtwn_softc *);
void		rtwn_disable_intr(struct rtwn_softc *);
int		rtwn_init(struct _ifnet *);
void		rtwn_init_task(void *);
void		rtwn_stop(struct _ifnet *);

/* Aliases. */
#define	rtwn_bb_write	rtwn_write_4
#define rtwn_bb_read	rtwn_read_4

/*
 * Macro to convert 4-bit signed integer to 8-bit signed integer.
 */
#define RTWN_SIGN4TO8(val)	(((val) & 0x08) ? (val) | 0xf0 : (val))

void
rtwn_ht_capabilities(struct rtwn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	ic->ic_htcaps = 0;
	memset(ic->ic_sup_mcs, 0, sizeof(ic->ic_sup_mcs));
	if (!(sc->chip & RTWN_CHIP_92E) &&
	    ((sc->chip & RTWN_CHIP_PCI) ||
	     !(sc->chip & (RTWN_CHIP_92C | RTWN_CHIP_88C | RTWN_CHIP_88E)))) return;
	ic->ic_caps |= IEEE80211_C_QOS | IEEE80211_C_TX_AMPDU;
	ic->ic_flags |= IEEE80211_F_QOS;
	ic->ic_htcaps = IEEE80211_HTCAP_SGI20 |
	    (IEEE80211_HTCAP_SMPS_DIS << IEEE80211_HTCAP_SMPS_SHIFT);
	if (sc->chip & RTWN_CHIP_92E)
		ic->ic_htcaps |= IEEE80211_HTCAP_CBW20_40 | IEEE80211_HTCAP_SGI40;
	/* Legacy DMA is 9/10KiB: bound the advertised RX aggregate to 8KiB.
	 * net80211 owns reorder windows and software CCMP for all generations. */
	ic->ic_ampdu_params = ((sc->chip & RTWN_CHIP_92E) ?
	    IEEE80211_AMPDU_PARAM_LE : 0) | IEEE80211_AMPDU_PARAM_SS_16;
	ic->ic_sup_mcs[0] = 0xff;
	if (sc->nrxchains > 1) ic->ic_sup_mcs[1] = 0xff;
	ic->ic_tx_mcs_set = IEEE80211_TX_MCS_SET_DEFINED;
	if (sc->ntxchains < sc->nrxchains)
		ic->ic_tx_mcs_set |= IEEE80211_TX_RX_MCS_NOT_EQUAL |
		    ((sc->ntxchains - 1) << 2);
}

int
rtwn_attach(struct device *pdev, struct rtwn_softc *sc, IOEthernetController *controller)
{
	if (!sc || !pdev || !controller || !sc->sc_ops.read_1 ||
	    !sc->sc_ops.read_2 || !sc->sc_ops.read_4 || !sc->sc_ops.write_1 ||
	    !sc->sc_ops.write_2 || !sc->sc_ops.write_4 || !sc->sc_ops.delay_us ||
	    !sc->sc_ops.io_error || !sc->sc_ops.tx || !sc->sc_ops.power_on ||
	    !sc->sc_ops.dma_init || !sc->sc_ops.fw_loadpage ||
	    !sc->sc_ops.load_firmware || !sc->sc_ops.aggr_init ||
	    !sc->sc_ops.mac_init || !sc->sc_ops.bb_init ||
	    !sc->sc_ops.alloc_buffers || !sc->sc_ops.init || !sc->sc_ops.stop ||
	    !sc->sc_ops.is_oactive || !sc->sc_ops.next_calib ||
	    !sc->sc_ops.cancel_calib || !sc->sc_ops.next_scan ||
	    !sc->sc_ops.cancel_scan || !sc->sc_ops.wait_async) return EINVAL;
	struct ieee80211com *ic = &sc->sc_ic;
	struct _ifnet *ifp = &ic->ic_if;
	int i, error;
	sc->sc_pdev = pdev;

	task_set(&sc->init_task, rtwn_init_task, sc, "rtwn_init");

	error = rtwn_read_chipid(sc);
	if (error != 0) {
		printf("%s: chip identification failed (%d)\n", sc->sc_pdev->dv_xname, error);
		return (error);
	}

	/* Determine number of Tx/Rx chains. */
	if (sc->chip & (RTWN_CHIP_92C | RTWN_CHIP_92E)) {
		sc->ntxchains = (sc->chip & RTWN_CHIP_92C_1T2R) ? 1 : 2;
		sc->nrxchains = 2;
	} else {
		sc->ntxchains = 1;
		sc->nrxchains = 1;
	}

	rtwn_read_rom(sc);
	if (rtwn_io_error(sc)) return rtwn_io_error(sc);
	if ((sc->chip & (RTWN_CHIP_92E | RTWN_CHIP_88E | RTWN_CHIP_92C | RTWN_CHIP_88C)) &&
	    (letoh16((sc->chip & RTWN_CHIP_92E) ? sc->sc_r92e_rom.id :
	             (sc->chip & RTWN_CHIP_88E) ? sc->sc_r88e_rom.id : sc->sc_r92c_rom.id) != 0x8129 ||
	     (ic->ic_myaddr[0] & 1) ||
	     (ic->ic_myaddr[0] == 0 && ic->ic_myaddr[1] == 0 &&
	      ic->ic_myaddr[2] == 0 && ic->ic_myaddr[3] == 0 &&
	      ic->ic_myaddr[4] == 0 && ic->ic_myaddr[5] == 0))) return ENXIO;

	if (sc->chip & RTWN_CHIP_PCI) {
		printf("%s: MAC/BB RTL%s, RF 6052 %dT%dR, address %s\n",
		    sc->sc_pdev->dv_xname,
		    (sc->chip & RTWN_CHIP_92C) ? "8192CE" :
		    (sc->chip & RTWN_CHIP_88E) ? "8188EE" :
		    (sc->chip & RTWN_CHIP_92E) ? "8192EE" :
		    (sc->chip & RTWN_CHIP_23A) ? "8723AE" :
		    (sc->chip & RTWN_CHIP_23B) ? "8723BE" : "8188CE",
		    sc->ntxchains, sc->nrxchains,
		    ether_sprintf(ic->ic_myaddr));
	} else if (sc->chip & RTWN_CHIP_USB) {
		printf("%s: MAC/BB RTL%s, RF 6052 %dT%dR, address %s\n",
		    sc->sc_pdev->dv_xname,
		    (sc->chip & RTWN_CHIP_92C) ? "8192CU" :
		    (sc->chip & RTWN_CHIP_92E) ? "8192EU" :
		    (sc->chip & RTWN_CHIP_88E) ? "8188EU" :
		    (sc->chip & RTWN_CHIP_88F) ? "8188FTV" :
		    (sc->board_type == R92C_BOARD_TYPE_HIGHPA) ? "8188RU" :
		    (sc->board_type == R92C_BOARD_TYPE_MINICARD) ?
		    "8188CE-VAU" : "8188CUS",
		    sc->ntxchains, sc->nrxchains,
		    ether_sprintf(ic->ic_myaddr));
	} else {
		printf("%s: unsupported chip\n", sc->sc_pdev->dv_xname);
		return (ENXIO);
	}

	ic->ic_phytype = IEEE80211_T_OFDM;	/* Not only, but not used. */
	ic->ic_opmode = IEEE80211_M_STA;	/* Default to BSS mode. */
	ic->ic_state = IEEE80211_S_INIT;

	/* Set device capabilities. */
	ic->ic_caps =
	    IEEE80211_C_SHPREAMBLE |	/* Short preamble supported. */
	    IEEE80211_C_SHSLOT |	/* Short slot time supported. */
	    IEEE80211_C_RSN;		/* WPA/RSN. */

	rtwn_ht_capabilities(sc);

	/* Set supported .11b and .11g rates. */
	ic->ic_sup_rates[IEEE80211_MODE_11B] = ieee80211_std_rateset_11b;
	ic->ic_sup_rates[IEEE80211_MODE_11G] = ieee80211_std_rateset_11g;

	/* Initial conservative 2.4 GHz domain: channels 1 through 11. */
	for (i = 1; i <= 11; i++) {
		ic->ic_channels[i].ic_freq =
		    ieee80211_ieee2mhz(i, IEEE80211_CHAN_2GHZ);
		ic->ic_channels[i].ic_flags =
		    IEEE80211_CHAN_CCK | IEEE80211_CHAN_OFDM |
		    IEEE80211_CHAN_DYN | IEEE80211_CHAN_2GHZ;
		if (ic->ic_htcaps) {
			ic->ic_channels[i].ic_flags |= IEEE80211_CHAN_HT20;
			/* Keep both primary and secondary inside channels 1..11. */
			if ((sc->chip & RTWN_CHIP_92E) && i <= 7)
				ic->ic_channels[i].ic_flags |= IEEE80211_CHAN_HT40U;
			if ((sc->chip & RTWN_CHIP_92E) && i >= 5)
				ic->ic_channels[i].ic_flags |= IEEE80211_CHAN_HT40D;
		}
	}

	ic->ic_ibss_chan = &ic->ic_channels[1];

#ifdef notyet
	/*
	 * The number of STAs that we can support is limited by the number
	 * of CAM entries used for hardware crypto.
	 */
	ic->ic_max_nnodes = R92C_CAM_ENTRY_COUNT - 4;
	if (ic->ic_max_nnodes > IEEE80211_CACHE_SIZE)
		ic->ic_max_nnodes = IEEE80211_CACHE_SIZE;
#endif

	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = rtwn_ioctl;
	ifp->if_start = rtwn_start;
	ifp->if_watchdog = rtwn_watchdog;
	memcpy(ifp->if_xname, sc->sc_pdev->dv_xname, IFNAMSIZ);

	if_attach(ifp);
	ieee80211_ifattach(ifp, controller);
	ic->ic_updateslot = rtwn_updateslot;
	ic->ic_updateedca = rtwn_updateedca;
#ifdef notyet
	ic->ic_set_key = rtwn_set_key;
	ic->ic_delete_key = rtwn_delete_key;
#endif
	/* Override state transition machine. */
	sc->sc_newstate = ic->ic_newstate;
	ic->ic_newstate = rtwn_newstate;
	ieee80211_media_init(ifp);

	return (0);
}

int
rtwn_detach(struct rtwn_softc *sc, int flags)
{
	struct _ifnet *ifp = &sc->sc_ic.ic_if;
	int s;

	s = splnet();

	task_del(systq, &sc->init_task);

	if (ifp->if_softc != NULL) {
		ieee80211_ifdetach(ifp);
		if_detach(ifp);
	}

	splx(s);

	return (0);
}

int
rtwn_activate(struct rtwn_softc *sc, int act)
{
	struct _ifnet *ifp = &sc->sc_ic.ic_if;

	switch (act) {
	case DVACT_QUIESCE: /* rtwn_stop() may sleep */
		if (ifp->if_flags & IFF_RUNNING)
			rtwn_stop(ifp);
		break;
	case DVACT_WAKEUP:
		rtwn_init_task(sc);
		break;
	}
	return (0);
}

void
rtwn_write_1(struct rtwn_softc *sc, uint16_t addr, uint8_t val)
{
	sc->sc_ops.write_1(sc->sc_ops.cookie, addr, val);
}

void
rtwn_write_2(struct rtwn_softc *sc, uint16_t addr, uint16_t val)
{
	sc->sc_ops.write_2(sc->sc_ops.cookie, addr, val);
}

void
rtwn_write_4(struct rtwn_softc *sc, uint16_t addr, uint32_t val)
{
	sc->sc_ops.write_4(sc->sc_ops.cookie, addr, val);
}

uint8_t
rtwn_read_1(struct rtwn_softc *sc, uint16_t addr)
{
	return sc->sc_ops.read_1(sc->sc_ops.cookie, addr);
}

uint16_t
rtwn_read_2(struct rtwn_softc *sc, uint16_t addr)
{
	return sc->sc_ops.read_2(sc->sc_ops.cookie, addr);
}

uint32_t
rtwn_read_4(struct rtwn_softc *sc, uint16_t addr)
{
	return sc->sc_ops.read_4(sc->sc_ops.cookie, addr);
}

int
rtwn_fw_cmd(struct rtwn_softc *sc, uint8_t id, const void *buf, int len)
{
	uint8_t message[8] = { 0 };
	uint32_t main_word, ext_word;
	int ntries, error, gen2;

	if (sc == NULL || sc->fwcur < 0 || sc->fwcur >= R92C_H2C_NBOX)
		return (EINVAL);
	gen2 = (sc->chip & RTWN_CHIP_92E) != 0;
	if (len < 0 || len > (gen2 ? 7 : 5) || (len != 0 && buf == NULL))
		return (EINVAL);
	if ((error = rtwn_io_error(sc)) != 0)
		return (error);

	/* Wait for current FW box to be empty. */
	for (ntries = 0; ntries < 100; ntries++) {
		uint8_t busy = rtwn_read_1(sc, R92C_HMETFR);
		if ((error = rtwn_io_error(sc)) != 0)
			return (error);
		if (!(busy & (1U << sc->fwcur)))
			break;
		rtwn_delay(sc, 10);
	}
	if (ntries == 100) {
		printf("%s: could not send firmware command %d\n",
		    sc->sc_pdev->dv_xname, id);
		return (ETIMEDOUT);
	}
	/* 92E gen2 has seven payload bytes and a 32-bit extension at 0x1f0.
	 * Unlike gen1, its command ID must not acquire the 0x80 EXT flag.
	 * See Linux rtl8xxxu_gen2_h2c_cmd and vendor FillH2CCmd_8192E. */
	message[0] = id;
	if (!gen2 && len > 3)
		message[0] |= R92C_CMD_FLAG_EXT;
	if (len != 0)
		memcpy(message + 1, buf, (size_t)len);
	/* Byte assembly is safe for unaligned buffers and independent of host
	 * endian order. Register callbacks encode these scalar values as LE. */
	main_word = (uint32_t)message[0] | (uint32_t)message[1] << 8 |
	    (uint32_t)message[2] << 16 | (uint32_t)message[3] << 24;
	ext_word = (uint32_t)message[4] | (uint32_t)message[5] << 8 |
	    (uint32_t)message[6] << 16 | (uint32_t)message[7] << 24;

	/* Write the first word last since that will trigger the FW. */
	if (len > 3) {
		if (gen2)
			rtwn_write_4(sc, R88E_HMEBOX_EXT(sc->fwcur), ext_word);
		else
			rtwn_write_2(sc, R92C_HMEBOX_EXT(sc->fwcur),
			    (uint16_t)ext_word);
		if ((error = rtwn_io_error(sc)) != 0)
			return (error);
	}
	rtwn_write_4(sc, R92C_HMEBOX(sc->fwcur), main_word);
	if ((error = rtwn_io_error(sc)) != 0)
		return (error);

	sc->fwcur = (sc->fwcur + 1) % R92C_H2C_NBOX;

	if (sc->chip & RTWN_CHIP_PCI) {
		/* Give firmware some time for processing. */
		rtwn_delay(sc, 2000);
	}

	return (0);
}

void
rtwn_rf_write(struct rtwn_softc *sc, int chain, uint16_t addr, uint32_t val)
{
	uint32_t param_addr;

	if (sc->chip & RTWN_CHIP_92E) {
		rtwn_write_4(sc, R92C_FPGA0_POWER_SAVE,
		    rtwn_read_4(sc, R92C_FPGA0_POWER_SAVE) & ~0x20000);
	}

	if (sc->chip & (RTWN_CHIP_88E | RTWN_CHIP_88F | RTWN_CHIP_92E))
		param_addr = SM(R88E_LSSI_PARAM_ADDR, addr);
	else
		param_addr = SM(R92C_LSSI_PARAM_ADDR, addr);

	rtwn_bb_write(sc, R92C_LSSI_PARAM(chain),
	    param_addr | SM(R92C_LSSI_PARAM_DATA, val));

	rtwn_delay(sc, 1);

	if (sc->chip & RTWN_CHIP_92E) {
		rtwn_write_4(sc, R92C_FPGA0_POWER_SAVE,
		    rtwn_read_4(sc, R92C_FPGA0_POWER_SAVE) | 0x20000);
	}
}

uint32_t
rtwn_rf_read(struct rtwn_softc *sc, int chain, uint8_t addr)
{
	uint32_t reg[R92C_MAX_CHAINS], val;

	reg[0] = rtwn_bb_read(sc, R92C_HSSI_PARAM2(0));
	if (chain != 0)
		reg[chain] = rtwn_bb_read(sc, R92C_HSSI_PARAM2(chain));

	rtwn_bb_write(sc, R92C_HSSI_PARAM2(0),
	    reg[0] & ~R92C_HSSI_PARAM2_READ_EDGE);
	rtwn_delay(sc, 1000);

	rtwn_bb_write(sc, R92C_HSSI_PARAM2(chain),
	    RW(reg[chain], R92C_HSSI_PARAM2_READ_ADDR, addr) |
	    R92C_HSSI_PARAM2_READ_EDGE);
	rtwn_delay(sc, 1000);

	if (!(sc->chip & RTWN_CHIP_88E)) {
		rtwn_bb_write(sc, R92C_HSSI_PARAM2(0),
		    reg[0] | R92C_HSSI_PARAM2_READ_EDGE);
		rtwn_delay(sc, 1000);
	}

	if (rtwn_bb_read(sc, R92C_HSSI_PARAM1(chain)) & R92C_HSSI_PARAM1_PI)
		val = rtwn_bb_read(sc, R92C_HSPI_READBACK(chain));
	else
		val = rtwn_bb_read(sc, R92C_LSSI_READBACK(chain));
	return (MS(val, R92C_LSSI_READBACK_DATA));
}

void
rtwn_cam_write(struct rtwn_softc *sc, uint32_t addr, uint32_t data)
{
	rtwn_write_4(sc, R92C_CAMWRITE, data);
	rtwn_write_4(sc, R92C_CAMCMD,
	    R92C_CAMCMD_POLLING | R92C_CAMCMD_WRITE |
	    SM(R92C_CAMCMD_ADDR, addr));
}

uint8_t
rtwn_efuse_read_1(struct rtwn_softc *sc, uint16_t addr)
{
	uint32_t reg;
	int ntries;

	reg = rtwn_read_4(sc, R92C_EFUSE_CTRL);
	reg = RW(reg, R92C_EFUSE_CTRL_ADDR, addr);
	reg &= ~R92C_EFUSE_CTRL_VALID;
	rtwn_write_4(sc, R92C_EFUSE_CTRL, reg);
	/* Wait for read operation to complete. */
	for (ntries = 0; ntries < 100; ntries++) {
		reg = rtwn_read_4(sc, R92C_EFUSE_CTRL);
		if (rtwn_io_error(sc)) return 0xff;
		if (reg & R92C_EFUSE_CTRL_VALID)
			return (MS(reg, R92C_EFUSE_CTRL_DATA));
		rtwn_delay(sc, 5);
	}
	printf("%s: could not read efuse byte at address 0x%x\n",
	    sc->sc_pdev->dv_xname, addr);
	if (!sc->core_error) sc->core_error = ETIMEDOUT;
	return (0xff);
}

void
rtwn_efuse_read(struct rtwn_softc *sc, uint8_t *rom, size_t size)
{
    uint8_t physical[RTW_EFUSE_BYTES];
    size_t words = 0;
    const bool gen2 = (sc->chip & RTWN_CHIP_92E) != 0;
    const bool eu = (sc->chip & RTWN_CHIP_88E) != 0;
    const bool cu = (sc->chip & (RTWN_CHIP_92C | RTWN_CHIP_88C)) != 0;
    const size_t physical_size = eu ? 256U : sizeof(physical);
    const size_t logical_size = cu ? sizeof(struct r92c_rom) : RTW_EFUSE_BYTES;
    if (!rom || !size) { sc->core_error = EINVAL; return; }
    memset(rom, 0xff, size);
    if ((!gen2 && !eu && !cu) || size != logical_size) {
        sc->core_error = EINVAL;
        return;
    }
    /* CU has neither the access gate nor extended OTP headers. 88E has a
     * 256-byte physical OTP aperture and a 512-byte extended logical map.
     * The 92E path retains its existing complete 512-byte bounded decode. */
    if (!cu) rtwn_write_1(sc, R92C_EFUSE_ACCESS, R92C_EFUSE_ACCESS_ON);
    rtwn_efuse_switch_power(sc);
    for (unsigned addr = 0; addr < physical_size; ++addr) {
        if (rtwn_io_error(sc)) break;
        physical[addr] = rtwn_efuse_read_1(sc, (uint16_t)addr);
    }
    if (!rtwn_io_error(sc) && gen2) {
        if (rtw_efuse_decode(physical, sizeof(physical), rom, size, &words) != RTW_EFUSE_OK)
            sc->core_error = EINVAL;
    } else if (!rtwn_io_error(sc)) {
        size_t pos = 0;
        while (pos < physical_size && !sc->core_error) {
            unsigned header = physical[pos++];
            if (header == 0xffU) break;
            unsigned block = header >> 4;
            unsigned mask = header & 15U;
            if (eu && (header & 0x1fU) == 0x0fU) {
                if (pos == physical_size) { sc->core_error = EINVAL; break; }
                unsigned ext = physical[pos++];
                if ((ext & 15U) == 15U) continue;
                block = ((ext & 0xf0U) >> 1) | (header >> 5);
                mask = ext & 15U;
            }
            for (unsigned word = 0; word < 4; ++word) {
                if (mask & (1U << word)) continue;
                size_t offset = (size_t)block * 8U + word * 2U;
                if (physical_size - pos < 2U || offset >= size || size - offset < 2U) {
                    sc->core_error = EINVAL;
                    break;
                }
                rom[offset] = physical[pos++];
                rom[offset + 1] = physical[pos++];
            }
        }
    }
    /* Always close the gated families, including logical-error cleanup. */
    if (!cu) rtwn_write_1(sc, R92C_EFUSE_ACCESS, R92C_EFUSE_ACCESS_OFF);
    if (rtwn_io_error(sc)) memset(rom, 0xff, size);
    explicit_bzero(physical, sizeof(physical));
}

void
rtwn_efuse_switch_power(struct rtwn_softc *sc)
{
	uint16_t reg;

	if (!(sc->chip & (RTWN_CHIP_88F | RTWN_CHIP_92E))) {
		reg = rtwn_read_2(sc, R92C_SYS_ISO_CTRL);
		if (!(reg & R92C_SYS_ISO_CTRL_PWC_EV12V)) {
			rtwn_write_2(sc, R92C_SYS_ISO_CTRL,
			    reg | R92C_SYS_ISO_CTRL_PWC_EV12V);
		}
	}
	reg = rtwn_read_2(sc, R92C_SYS_FUNC_EN);
	if (!(reg & R92C_SYS_FUNC_EN_ELDR)) {
		rtwn_write_2(sc, R92C_SYS_FUNC_EN,
		    reg | R92C_SYS_FUNC_EN_ELDR);
	}
	reg = rtwn_read_2(sc, R92C_SYS_CLKR);
	if ((reg & (R92C_SYS_CLKR_LOADER_EN | R92C_SYS_CLKR_ANA8M)) !=
	    (R92C_SYS_CLKR_LOADER_EN | R92C_SYS_CLKR_ANA8M)) {
		rtwn_write_2(sc, R92C_SYS_CLKR,
		    reg | R92C_SYS_CLKR_LOADER_EN | R92C_SYS_CLKR_ANA8M);
	}
}

int
rtwn_read_chipid(struct rtwn_softc *sc)
{
	uint32_t reg;
	/* Do not select firmware/RF chains from a failed USB register read. */
	if (rtwn_io_error(sc)) return rtwn_io_error(sc);

	if (sc->chip & RTWN_CHIP_92E) {
		/* Linux rtl8192eu_identify_chip: 8191EU bonds one TX path
		 * while retaining both RX paths (HPON_FSM bits 23:22 == 1). */
		reg = rtwn_read_4(sc, R92C_SYS_CFG);
		if (rtwn_io_error(sc)) return rtwn_io_error(sc);
		if (reg & R92C_SYS_CFG_TRP_VAUX_EN)
			return EIO;
		sc->chip &= ~RTWN_CHIP_92C_1T2R;
		reg = rtwn_read_4(sc, R92C_HPON_FSM);
		if (rtwn_io_error(sc)) return rtwn_io_error(sc);
		if (MS(reg, R92C_HPON_FSM_CHIP_BONDING_ID) ==
		    R92C_HPON_FSM_CHIP_BONDING_ID_92C_1T2R)
			sc->chip |= RTWN_CHIP_92C_1T2R;
		sc->sc_flags |= RTWN_FLAG_EXT_HDR;
		return rtwn_io_error(sc);
	}
	if (sc->chip & RTWN_CHIP_88E) {
		reg = rtwn_read_4(sc, R92C_SYS_CFG);
		if (rtwn_io_error(sc)) return rtwn_io_error(sc);
		/* Linux rtl8188eu_identify_chip rejects I-cut (8): its firmware,
		 * radio tables and rate control differ from this 8188EU backend. */
		if (reg == UINT32_MAX || (reg & R92C_SYS_CFG_TRP_VAUX_EN) ||
		    MS(reg, R92C_SYS_CFG_CHIP_VER_RTL) == 8) return ENXIO;
		sc->sc_flags |= RTWN_FLAG_EXT_HDR;
		return 0;
	}
	if (sc->chip & RTWN_CHIP_88F) {
		sc->sc_flags |= RTWN_FLAG_EXT_HDR;
		return (0);
	}

	reg = rtwn_read_4(sc, R92C_SYS_CFG);
	if (rtwn_io_error(sc)) return rtwn_io_error(sc);
	if (reg & R92C_SYS_CFG_TRP_VAUX_EN)
		/* Unsupported test chip. */
		return (EIO);

	if ((sc->chip & (RTWN_CHIP_92C | RTWN_CHIP_88C)) != 0) {
		if (reg & R92C_SYS_CFG_TYPE_92C) {
			sc->chip &= ~RTWN_CHIP_88C;
			/* Check if it is a castrated 8192C. */
			const uint32_t bonding = rtwn_read_4(sc, R92C_HPON_FSM);
			if (rtwn_io_error(sc)) return rtwn_io_error(sc);
			if (MS(bonding, R92C_HPON_FSM_CHIP_BONDING_ID) ==
			    R92C_HPON_FSM_CHIP_BONDING_ID_92C_1T2R)
				sc->chip |= RTWN_CHIP_92C_1T2R;
		} else
			sc->chip &= ~RTWN_CHIP_92C;

		if (reg & R92C_SYS_CFG_VENDOR_UMC) {
			sc->chip |= RTWN_CHIP_UMC;
			if (MS(reg, R92C_SYS_CFG_CHIP_VER_RTL) == 0)
				sc->chip |= RTWN_CHIP_UMC_A_CUT;
		}

		return (0);
	} else if (sc->chip & RTWN_CHIP_23A) {
		sc->sc_flags |= RTWN_FLAG_EXT_HDR;

		if ((reg & 0xf000) == 0)
			sc->chip |= RTWN_CHIP_UMC_A_CUT;
		return (0);
	}

	return (ENXIO); /* unsupported chip */
}

void
rtwn_read_rom(struct rtwn_softc *sc)
{
	if (sc->chip & RTWN_CHIP_88E)
		rtwn_r88e_read_rom(sc);
	else if (sc->chip & RTWN_CHIP_88F)
		rtwn_r88f_read_rom(sc);
	else if (sc->chip & RTWN_CHIP_92E)
		rtwn_r92e_read_rom(sc);
	else if (sc->chip & RTWN_CHIP_23A)
		rtwn_r23a_read_rom(sc);
	else
		rtwn_r92c_read_rom(sc);
}

void
rtwn_r92c_read_rom(struct rtwn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct r92c_rom *rom = &sc->sc_r92c_rom;

	/* Read full ROM image. */
	rtwn_efuse_read(sc, (uint8_t *)&sc->sc_r92c_rom,
	    sizeof(sc->sc_r92c_rom));
	if (rtwn_io_error(sc)) return;

	/* XXX Weird but this is what the vendor driver does. */
	sc->pa_setting = rtwn_efuse_read_1(sc, 0x1fa);
	DPRINTF(("PA setting=0x%x\n", sc->pa_setting));

	sc->board_type = MS(rom->rf_opt1, R92C_ROM_RF1_BOARD_TYPE);
	DPRINTF(("board type=%d\n", sc->board_type));

	sc->regulatory = MS(rom->rf_opt1, R92C_ROM_RF1_REGULATORY);
	DPRINTF(("regulatory type=%d\n", sc->regulatory));

	IEEE80211_ADDR_COPY(ic->ic_myaddr, rom->macaddr);
}

void
rtwn_r92e_read_rom(struct rtwn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct r92e_rom *rom = &sc->sc_r92e_rom;

	/* Read full ROM image. */
	rtwn_efuse_read(sc, (uint8_t *)&sc->sc_r92e_rom,
	    sizeof(sc->sc_r92e_rom));

	if (rtwn_io_error(sc)) return;
	const struct r92e_tx_pwr *paths[2] = { &rom->txpwr_a, &rom->txpwr_b };
	for (unsigned chain = 0; chain < 2; ++chain) {
		for (unsigned group = 0; group < 4; ++group) {
			if (paths[chain]->cck_tx_pwr[group] > R92C_MAX_TX_PWR ||
			    paths[chain]->ht40_tx_pwr[group] > R92C_MAX_TX_PWR) {
				sc->core_error = EINVAL;
				return;
			}
		}
		if (paths[chain]->ht20_ofdm_tx_pwr_diff == 0xff) {
			sc->core_error = EINVAL;
			return;
		}
	}
	/* FreeBSD r92e_rom_image.h documents 0x20 for an erased crystal cap. */
	sc->crystal_cap = rom->xtal_k == 0xff ? 0x20 : rom->xtal_k;
	DPRINTF(("crystal cap=0x%x\n", sc->crystal_cap));

	sc->regulatory = MS(rom->rf_board_opt, R92C_ROM_RF1_REGULATORY);
	DPRINTF(("regulatory type=%d\n", sc->regulatory));

	IEEE80211_ADDR_COPY(ic->ic_myaddr, rom->macaddr);
}

void
rtwn_r88e_read_rom(struct rtwn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct r88e_rom *rom = &sc->sc_r88e_rom;

	/* Read full ROM image. */
	rtwn_efuse_read(sc, (uint8_t *)&sc->sc_r88e_rom,
	    sizeof(sc->sc_r88e_rom));
	if (rtwn_io_error(sc)) return;

	sc->crystal_cap = (sc->chip & RTWN_CHIP_PCI) ? 0x20 : rom->xtal;
	DPRINTF(("Crystal cap=0x%x\n", sc->crystal_cap));

	sc->regulatory = MS(rom->rf_board_opt, R92C_ROM_RF1_REGULATORY);
	DPRINTF(("regulatory type=%d\n", sc->regulatory));

	if (sc->chip & RTWN_CHIP_PCI)
		IEEE80211_ADDR_COPY(ic->ic_myaddr, rom->r88ee_rom.macaddr);
	else
		IEEE80211_ADDR_COPY(ic->ic_myaddr, rom->r88eu_rom.macaddr);
}

void
rtwn_r88f_read_rom(struct rtwn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct r88f_rom *rom = &sc->sc_r88f_rom;

	/* Read full ROM image. */
	rtwn_efuse_read(sc, (uint8_t *)&sc->sc_r88f_rom,
	    sizeof(sc->sc_r88f_rom));

	sc->crystal_cap = rom->xtal;

	IEEE80211_ADDR_COPY(ic->ic_myaddr, rom->macaddr);
}

void
rtwn_r23a_read_rom(struct rtwn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct r23a_rom *rom = &sc->sc_r23a_rom;

	/* Read full ROM image. */
	rtwn_efuse_read(sc, (uint8_t *)&sc->sc_r23a_rom,
	    sizeof(sc->sc_r23a_rom));

	IEEE80211_ADDR_COPY(ic->ic_myaddr, rom->macaddr);
}

int
rtwn_media_change(struct _ifnet *ifp)
{
	int error;

	error = ieee80211_media_change(ifp);
	if (error != ENETRESET)
		return (error);

	if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) ==
	    (IFF_UP | IFF_RUNNING)) {
		rtwn_stop(ifp);
		error = rtwn_init(ifp);
	}
	return (error);
}

/*
 * Initialize rate adaptation.
 */
#include "r92e_ht.inc"

int
rtwn_ra_init(struct rtwn_softc *sc)
{
	static const uint8_t map[] =
	    { 2, 4, 11, 22, 12, 18, 24, 36, 48, 72, 96, 108 };
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni = ic->ic_bss;
	struct ieee80211_rateset *rs;
	uint32_t rates, basicrates;
	uint8_t mode;
	int maxrate, maxbasicrate, i, j;
	int error = 0;
	rtw_92e_rate_config legacy_config;
	const int legacy_usb = !(sc->chip & RTWN_CHIP_PCI) &&
	    (sc->chip & (RTWN_CHIP_92C | RTWN_CHIP_88C | RTWN_CHIP_88E));
	if (legacy_usb) memset(&sc->rate_config, 0, sizeof(sc->rate_config));

	if (!ni || !ni->ni_rates.rs_nrates ||
	    ni->ni_rates.rs_nrates > nitems(ni->ni_rates.rs_rates)) return EINVAL;
	rs = &ni->ni_rates;

	/* Get normal and basic rates mask. */
	rates = basicrates = 0;
	maxrate = maxbasicrate = 0;
	for (i = 0; i < rs->rs_nrates; i++) {
		/* Convert 802.11 rate to HW rate index. */
		for (j = 0; j < nitems(map); j++)
			if ((rs->rs_rates[i] & IEEE80211_RATE_VAL) == map[j])
				break;
		if (j == nitems(map))	/* Unknown rate, skip. */
			continue;
		rates |= 1 << j;
		if (j > maxrate)
			maxrate = j;
		if (rs->rs_rates[i] & IEEE80211_RATE_BASIC) {
			basicrates |= 1 << j;
			if (j > maxbasicrate)
				maxbasicrate = j;
		}
	}
	if (!rates) return EINVAL;
	if (legacy_usb) {
		/* Static SMPS permits one spatial stream. Dynamic SMPS requires
		 * MIMO-specific RTS protection which this gen1 path does not emit;
		 * keep both cases at one TX stream without reducing local RX MCS. */
		unsigned peer_tx_chains = sc->ntxchains;
		if ((ni->ni_flags & IEEE80211_NODE_HT) &&
		    ((ni->ni_htcaps & IEEE80211_HTCAP_SMPS_MASK) >>
		    IEEE80211_HTCAP_SMPS_SHIFT) != IEEE80211_HTCAP_SMPS_DIS)
			peer_tx_chains = 1;
		if (!rtw_92c_build_rate_config((uint16_t)rates, ni->ni_rxmcs,
		    peer_tx_chains, (ni->ni_flags & IEEE80211_NODE_HT) != 0,
		    (ic->ic_htcaps & IEEE80211_HTCAP_SGI20) != 0,
		    (ni->ni_flags & IEEE80211_NODE_HT_SGI20) != 0,
		    &legacy_config)) return EINVAL;
		/* Response and RTS control frames always use a legacy rate. */
		if (!(sc->chip & RTWN_CHIP_88E)) {
			rtwn_write_1(sc, R92C_INIRTS_RATE_SEL, maxbasicrate);
			rtwn_write_4(sc, R92C_RRSR,
			    RW(rtwn_read_4(sc, R92C_RRSR), R92C_RRSR_RATE_BITMAP, rates));
			if ((error = rtwn_io_error(sc)) != 0) return error;
		}
	}
	if (ic->ic_curmode == IEEE80211_MODE_11B)
		mode = R92C_RAID_11B;
	else
		mode = (rates & 0x0fU) ? R92C_RAID_11BG : R92C_RAID_11G;
	DPRINTF(("mode=0x%x rates=0x%08x, basicrates=0x%08x\n",
	    mode, rates, basicrates));

	if (sc->chip & RTWN_CHIP_PCI) {
		/* Configure Automatic Rate Fallback Register. */
		if (ic->ic_curmode == IEEE80211_MODE_11B) {
			if (rates & 0x0c)
				rtwn_write_4(sc, R92C_ARFR(0), rates & 0x05);
			else
				rtwn_write_4(sc, R92C_ARFR(0), rates & 0x07);
		} else
			rtwn_write_4(sc, R92C_ARFR(0), rates & 0x07f5);
	}

	if (sc->chip & RTWN_CHIP_92E) {
		error = rtwn_r92e_ra_init(sc, rates, maxbasicrate);
		ni->ni_txrate = 0;
	} else if (sc->chip & (RTWN_CHIP_88E | RTWN_CHIP_88F)) {
		error = rtwn_r88e_ra_init(sc, mode, rates,
		    legacy_usb ? legacy_config.initial_rate : maxrate,
		    basicrates, maxbasicrate);
		/* We use AMRR with this chip. Start with the lowest rate. */
		ni->ni_txrate = 0;
	} else {
		if (sc->chip & RTWN_CHIP_PCI) {
			ni->ni_txrate = 0; /* AMRR will raise. */
			/* Set initial MRR rates. */
			rtwn_write_1(sc,
			    R92C_INIDATA_RATE_SEL(R92C_MACID_BC), maxbasicrate);
			rtwn_write_1(sc,
			    R92C_INIDATA_RATE_SEL(R92C_MACID_BSS), 0);
		} else {
			error = rtwn_r92c_ra_init(sc,
			    legacy_usb ? legacy_config.raid : mode,
			    legacy_usb ? legacy_config.rate_mask : rates,
			    legacy_usb ? legacy_config.initial_rate : maxrate,
			    basicrates, maxbasicrate);
			/* No AMRR support. Indicate highest supported rate. */
			if (!error) ni->ni_txrate = rs->rs_nrates - 1;
		}
	}
	if (!error && legacy_usb) sc->rate_config = legacy_config;
	return (error);
}

/*
 * Initialize rate adaptation in firmware.
 */
int
rtwn_r92c_ra_init(struct rtwn_softc *sc, u_int8_t mode, u_int32_t rates,
    int maxrate, uint32_t basicrates, int maxbasicrate)
{
	uint8_t cmd[5];
	uint32_t mask;
	int error;

	/* Gen1 RAMASK is not a sequential little-endian mask plus MACID.
	 * Its wire layout is mask_hi16, MACID, mask_lo16: the last two bytes
	 * are written to HMEBOX_EXT before the four-byte main mailbox.
	 * Cross-check: pinned Linux rtl8xxxu h2c_cmd.ramask and
	 * rtl8xxxu_update_rate_mask; command 6 acquires EXT in rtwn_fw_cmd. */
	const uint8_t basicmode = (basicrates & 0xff0U) ?
	    ((basicrates & 15U) ? R92C_RAID_11BG : R92C_RAID_11G) : R92C_RAID_11B;
	mask = ((uint32_t)(mode <= 3 ? basicmode : mode) << 28) | basicrates;
	cmd[0] = (uint8_t)(mask >> 16); cmd[1] = (uint8_t)(mask >> 24);
	cmd[2] = R92C_MACID_BC | R92C_MACID_VALID;
	cmd[3] = (uint8_t)mask; cmd[4] = (uint8_t)(mask >> 8);
	error = rtwn_fw_cmd(sc, R92C_CMD_MACID_CONFIG, cmd, sizeof(cmd));
	if (error != 0) {
		printf("%s: could not add broadcast station\n",
		    sc->sc_pdev->dv_xname);
		return (error);
	}
	/* Set initial MRR rate. */
	DPRINTF(("maxbasicrate=%d\n", maxbasicrate));
	rtwn_write_1(sc, R92C_INIDATA_RATE_SEL(R92C_MACID_BC),
	    maxbasicrate);

	/* Set rates mask for unicast frames. */
	mask = ((uint32_t)mode << 28) | rates;
	cmd[0] = (uint8_t)(mask >> 16); cmd[1] = (uint8_t)(mask >> 24);
	cmd[2] = R92C_MACID_BSS | R92C_MACID_VALID;
	if ((rates & 0x0ffff000U) &&
	    (sc->sc_ic.ic_htcaps & IEEE80211_HTCAP_SGI20) &&
	    (sc->sc_ic.ic_bss->ni_flags & IEEE80211_NODE_HT_SGI20))
		cmd[2] |= R92C_MACID_SHORTGI;
	cmd[3] = (uint8_t)mask; cmd[4] = (uint8_t)(mask >> 8);
	error = rtwn_fw_cmd(sc, R92C_CMD_MACID_CONFIG, cmd, sizeof(cmd));
	if (error != 0) {
		printf("%s: could not add BSS station\n",
		    sc->sc_pdev->dv_xname);
		return (error);
	}
	/* Set initial MRR rate. */
	DPRINTF(("maxrate=%d\n", maxrate));
	rtwn_write_1(sc, R92C_INIDATA_RATE_SEL(R92C_MACID_BSS),
	    maxrate);

	return rtwn_io_error(sc);
}

int
rtwn_r88e_ra_init(struct rtwn_softc *sc, u_int8_t mode, u_int32_t rates,
    int maxrate, uint32_t basicrates, int maxbasicrate)
{
	u_int32_t reg;

	rtwn_write_1(sc, R92C_INIRTS_RATE_SEL, maxbasicrate);

	reg = rtwn_read_4(sc, R92C_RRSR);
	reg = RW(reg, R92C_RRSR_RATE_BITMAP, rates);
	rtwn_write_4(sc, R92C_RRSR, reg);

	/*
	 * Workaround for performance problems with firmware rate adaptation:
	 * If the AP only supports 11b rates, disable mixed B/G mode.
	 */
	if (mode != R92C_RAID_11B && maxrate <= 3 /* 11M */)
		sc->sc_flags |= RTWN_FLAG_FORCE_RAID_11B;

	return rtwn_io_error(sc);
}

void
rtwn_tsf_sync_enable(struct rtwn_softc *sc)
{
	struct ieee80211_node *ni = sc->sc_ic.ic_bss;
	uint64_t tsf;

	/* Enable TSF synchronization. */
	rtwn_write_1(sc, R92C_BCN_CTRL,
	    rtwn_read_1(sc, R92C_BCN_CTRL) & ~R92C_BCN_CTRL_DIS_TSF_UDT0);

	rtwn_write_1(sc, R92C_BCN_CTRL,
	    rtwn_read_1(sc, R92C_BCN_CTRL) & ~R92C_BCN_CTRL_EN_BCN);

	/* Set initial TSF. */
	memcpy(&tsf, ni->ni_tstamp, sizeof(tsf));
	tsf = letoh64(tsf);
	tsf = tsf - (tsf % (ni->ni_intval * IEEE80211_DUR_TU));
	tsf -= IEEE80211_DUR_TU;
	rtwn_write_4(sc, R92C_TSFTR + 0, tsf);
	rtwn_write_4(sc, R92C_TSFTR + 4, tsf >> 32);

	rtwn_write_1(sc, R92C_BCN_CTRL,
	    rtwn_read_1(sc, R92C_BCN_CTRL) | R92C_BCN_CTRL_EN_BCN);
}

void
rtwn_set_led(struct rtwn_softc *sc, int led, int on)
{
	uint8_t reg;

	if (led != RTWN_LED_LINK)
		return; /* not supported */

	if (sc->chip & RTWN_CHIP_PCI) {
		reg = rtwn_read_1(sc, R92C_LEDCFG2) & 0xf0;
		if (!on)
			reg |= R92C_LEDCFG2_DIS;
		else
			reg |= R92C_LEDCFG2_EN;
		rtwn_write_1(sc, R92C_LEDCFG2, reg);
	} else if (sc->chip & RTWN_CHIP_USB) {
		if (sc->chip & RTWN_CHIP_92E) {
			rtwn_write_1(sc, 0x64, rtwn_read_1(sc, 0x64) & 0xfe);
			reg = rtwn_read_1(sc, R92C_LEDCFG1) & R92E_LEDSON;
			rtwn_write_1(sc, R92C_LEDCFG1, reg |
			    (R92C_LEDCFG0_DIS << 1));
			if (on) {
				reg = rtwn_read_1(sc, R92C_LEDCFG1) &
				    R92E_LEDSON;
				rtwn_write_1(sc, R92C_LEDCFG1, reg);
			}
		} else if (sc->chip & RTWN_CHIP_88E) {
			reg = rtwn_read_1(sc, R92C_LEDCFG2) & 0xf0;
			rtwn_write_1(sc, R92C_LEDCFG2, reg | R92C_LEDCFG2_EN);
			if (!on) {
				reg = rtwn_read_1(sc, R92C_LEDCFG2) & 0x90;
				rtwn_write_1(sc, R92C_LEDCFG2,
				    reg | R92C_LEDCFG0_DIS);
				rtwn_write_1(sc, R92C_MAC_PINMUX_CFG,
				    rtwn_read_1(sc, R92C_MAC_PINMUX_CFG) &
				    0xfe);
			}
		} else {
			reg = rtwn_read_1(sc, R92C_LEDCFG0) & 0x70;
			if (!on)
				reg |= R92C_LEDCFG0_DIS;
			rtwn_write_1(sc, R92C_LEDCFG0, reg);
		}
	}
	sc->ledlink = on;	/* Save LED state. */
}

void
rtwn_set_nettype(struct rtwn_softc *sc, enum ieee80211_opmode opmode)
{
	uint8_t msr;

	msr = rtwn_read_1(sc, R92C_MSR) & ~R92C_MSR_NETTYPE_MASK;

	switch (opmode) {
	case IEEE80211_M_MONITOR:
		msr |= R92C_MSR_NETTYPE_NOLINK;
		break;
	case IEEE80211_M_STA:
		msr |= R92C_MSR_NETTYPE_INFRA;
		break;
	default:
		break;
	}

	rtwn_write_1(sc, R92C_MSR, msr);
}

void
rtwn_calib(struct rtwn_softc *sc)
{

	if (sc->avg_pwdb != -1) {
		DPRINTFN(3, ("sending RSSI command avg=%d\n", sc->avg_pwdb));

		/* Indicate Rx signal strength to FW for rate adaptation. */
		if (sc->chip & RTWN_CHIP_92E) {
			struct r92e_fw_cmd_rssi cmd;

			memset(&cmd, 0, sizeof(cmd));
			cmd.macid = 0;	/* BSS. */
			cmd.pwdb = sc->avg_pwdb;
			rtwn_fw_cmd(sc, R92E_CMD_RSSI_REPORT, &cmd,
			    sizeof(cmd));
		} else {
			struct r92c_fw_cmd_rssi cmd;

			memset(&cmd, 0, sizeof(cmd));
			cmd.macid = 0;	/* BSS. */
			cmd.pwdb = sc->avg_pwdb;
			rtwn_fw_cmd(sc, R92C_CMD_RSSI_SETTING, &cmd,
			    sizeof(cmd));
		}
	}

	/* Do temperature compensation. */
	rtwn_temp_calib(sc);

	sc->sc_ops.next_calib(sc->sc_ops.cookie);
}

void
rtwn_next_scan(struct rtwn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	int s;

	s = splnet();
	if (ic->ic_state == IEEE80211_S_SCAN)
		ieee80211_next_scan(&ic->ic_if);
	splx(s);
}

/* Select HT40 only after net80211 has completed the AP channel-width
 * negotiation. Respect both local/peer 40-intolerant flags, the user HT40
 * disable switch, sideband direction and the device's 1..11 domain. */
static struct ieee80211_channel *
rtwn_r92e_run_extension(struct rtwn_softc *sc, struct ieee80211_node *ni)
{
	struct ieee80211com *ic = &sc->sc_ic;
	int primary, secondary;
	unsigned sco;
	struct ieee80211_channel *extc;

	if (!(sc->chip & RTWN_CHIP_92E) || !ni || !ni->ni_chan ||
	    ni->ni_chan == IEEE80211_CHAN_ANYC ||
	    !(ni->ni_flags & IEEE80211_NODE_HT) ||
	    ni->ni_chw != IEEE80211_CHAN_WIDTH_40 ||
	    !(ni->ni_htop0 & IEEE80211_HTOP0_CHW) ||
	    !(ic->ic_htcaps & IEEE80211_HTCAP_CBW20_40) ||
	    !(ni->ni_htcaps & IEEE80211_HTCAP_CBW20_40) ||
	    ((ic->ic_htcaps | ni->ni_htcaps) & IEEE80211_HTCAP_40INTOLERANT) ||
	    (ic->ic_userflags & IEEE80211_F_NOHT40))
		return NULL;
	primary = ieee80211_chan2ieee(ic, ni->ni_chan);
	sco = ni->ni_htop0 & IEEE80211_HTOP0_SCO_MASK;
	if (primary < 1 || primary > 11)
		return NULL;
	if (sco == IEEE80211_HTOP0_SCO_SCA &&
	    (ni->ni_chan->ic_flags & IEEE80211_CHAN_HT40U))
		secondary = primary + 4;
	else if (sco == IEEE80211_HTOP0_SCO_SCB &&
	    (ni->ni_chan->ic_flags & IEEE80211_CHAN_HT40D))
		secondary = primary - 4;
	else
		return NULL;
	if (secondary < 1 || secondary > 11)
		return NULL;
	extc = &ic->ic_channels[secondary];
	if (!(extc->ic_flags & IEEE80211_CHAN_2GHZ) ||
	    extc->ic_freq != ni->ni_chan->ic_freq + (secondary - primary) * 5)
		return NULL;
	return extc;
}

int
rtwn_newstate(struct ieee80211com *ic, enum ieee80211_state nstate, int arg)
{
	struct rtwn_softc *sc = (struct rtwn_softc *)ic->ic_softc;
	struct ieee80211_node *ni;
	enum ieee80211_state ostate;
	uint32_t reg;
	int s, error = 0;

	s = splnet();
	ostate = ic->ic_state;

	if (nstate != ostate)
		DPRINTF(("newstate %s -> %s\n",
		    ieee80211_state_name[ostate],
		    ieee80211_state_name[nstate]));

	if (ostate == IEEE80211_S_RUN) {
		if (!(sc->chip & RTWN_CHIP_92E))
			memset(&sc->rate_config, 0, sizeof(sc->rate_config));
		if (sc->chip & RTWN_CHIP_92E)
			error = rtwn_r92e_media_status(sc, 0);
		/* Stop calibration. */
		sc->sc_ops.cancel_calib(sc->sc_ops.cookie);

		/* Turn link LED off. */
		rtwn_set_led(sc, RTWN_LED_LINK, 0);

		/* Set media status to 'No Link'. */
		rtwn_set_nettype(sc, IEEE80211_M_MONITOR);

		/* Stop Rx of data frames. */
		rtwn_write_2(sc, R92C_RXFLTMAP2, 0);
		rtwn_write_2(sc, R92C_RXFLTMAP1, 0);

		/* Rest TSF. */
		rtwn_write_1(sc, R92C_DUAL_TSF_RST, 0x03);

		/* Disable TSF synchronization. */
		rtwn_write_1(sc, R92C_BCN_CTRL,
		    rtwn_read_1(sc, R92C_BCN_CTRL) |
		    R92C_BCN_CTRL_DIS_TSF_UDT0);

		/* Reset EDCA parameters. */
		rtwn_edca_init(sc);

		rtwn_updateslot(ic);
		rtwn_update_short_preamble(ic);

		/* Disable 11b-only AP workaround (see rtwn_r88e_ra_init). */
		sc->sc_flags &= ~RTWN_FLAG_FORCE_RAID_11B;
		if (sc->chip & RTWN_CHIP_92E)
			sc->channel_bw40 = 0;
	}
	if (error != 0 && nstate != IEEE80211_S_INIT)
		goto done;
	switch (nstate) {
	case IEEE80211_S_INIT:
		/* Turn link LED off. */
		rtwn_set_led(sc, RTWN_LED_LINK, 0);
		break;
	case IEEE80211_S_SCAN:
		if (ostate != IEEE80211_S_SCAN) {
			/* Allow Rx from any BSSID. */
			rtwn_write_4(sc, R92C_RCR,
			    rtwn_read_4(sc, R92C_RCR) &
			    ~(R92C_RCR_CBSSID_DATA | R92C_RCR_CBSSID_BCN));

			/* Set gain for scanning. */
			reg = rtwn_bb_read(sc, R92C_OFDM0_AGCCORE1(0));
			reg = RW(reg, R92C_OFDM0_AGCCORE1_GAIN, 0x20);
			rtwn_bb_write(sc, R92C_OFDM0_AGCCORE1(0), reg);

			if (!(sc->chip & RTWN_CHIP_88E)) {
				reg = rtwn_bb_read(sc, R92C_OFDM0_AGCCORE1(1));
				reg = RW(reg, R92C_OFDM0_AGCCORE1_GAIN, 0x20);
				rtwn_bb_write(sc, R92C_OFDM0_AGCCORE1(1), reg);
			}
		}

		/* Make link LED blink during scan. */
		rtwn_set_led(sc, RTWN_LED_LINK, !sc->ledlink);

		/* Pause AC Tx queues. */
		rtwn_write_1(sc, R92C_TXPAUSE,
		    rtwn_read_1(sc, R92C_TXPAUSE) | R92C_TXPAUSE_AC_VO |
		    R92C_TXPAUSE_AC_VI | R92C_TXPAUSE_AC_BE |
		    R92C_TXPAUSE_AC_BK);

		rtwn_set_chan(sc, ic->ic_bss->ni_chan, NULL);
		sc->sc_ops.next_scan(sc->sc_ops.cookie);
		break;

	case IEEE80211_S_AUTH:
		/* Set initial gain under link. */
		reg = rtwn_bb_read(sc, R92C_OFDM0_AGCCORE1(0));
		reg = RW(reg, R92C_OFDM0_AGCCORE1_GAIN, 0x32);
		rtwn_bb_write(sc, R92C_OFDM0_AGCCORE1(0), reg);

		if (!(sc->chip & RTWN_CHIP_88E)) {
			reg = rtwn_bb_read(sc, R92C_OFDM0_AGCCORE1(1));
			reg = RW(reg, R92C_OFDM0_AGCCORE1_GAIN, 0x32);
			rtwn_bb_write(sc, R92C_OFDM0_AGCCORE1(1), reg);
		}

		rtwn_set_chan(sc, ic->ic_bss->ni_chan, NULL);
		break;
	case IEEE80211_S_ASSOC:
		break;
	case IEEE80211_S_RUN:
		if (ic->ic_opmode == IEEE80211_M_MONITOR) {
			rtwn_set_chan(sc, ic->ic_ibss_chan, NULL);

			/* Enable Rx of data frames. */
			rtwn_write_2(sc, R92C_RXFLTMAP2, 0xffff);

			/* Enable Rx of control frames. */
			rtwn_write_2(sc, R92C_RXFLTMAP1, 0xffff);

			rtwn_write_4(sc, R92C_RCR,
			    rtwn_read_4(sc, R92C_RCR) |
			    R92C_RCR_AAP | R92C_RCR_ADF | R92C_RCR_ACF |
			    R92C_RCR_AMF);

			/* Turn link LED on. */
			rtwn_set_led(sc, RTWN_LED_LINK, 1);
			break;
		}
		ni = ic->ic_bss;

		/* SCAN/AUTH tune HT20. Reconfigure the radio before publishing
		 * HT40 rate adaptation so the H2C width always matches hardware.
		 * All other chipsets retain their existing RUN sequence. */
		if ((sc->chip & (RTWN_CHIP_92E | RTWN_CHIP_92C | RTWN_CHIP_88C | RTWN_CHIP_88E)) && ni && ni->ni_chan &&
		    ni->ni_chan != IEEE80211_CHAN_ANYC) {
			rtwn_set_chan(sc, ni->ni_chan,
			    rtwn_r92e_run_extension(sc, ni));
			if ((error = rtwn_io_error(sc)) != 0)
				break;
		}

		/* Set media status to 'Associated'. */
		rtwn_set_nettype(sc, IEEE80211_M_STA);

		/* Set BSSID. */
		rtwn_write_4(sc, R92C_BSSID + 0, LE_READ_4(&ni->ni_bssid[0]));
		rtwn_write_4(sc, R92C_BSSID + 4, LE_READ_2(&ni->ni_bssid[4]));

		if (ic->ic_curmode == IEEE80211_MODE_11B)
			rtwn_write_1(sc, R92C_INIRTS_RATE_SEL, 0);
		else	/* 802.11b/g */
			rtwn_write_1(sc, R92C_INIRTS_RATE_SEL, 3);

		rtwn_updateslot(ic);
		rtwn_update_short_preamble(ic);

		/* Enable Rx of data frames. */
		rtwn_write_2(sc, R92C_RXFLTMAP2, 0xffff);

		/* Flush all AC queues. */
		rtwn_write_1(sc, R92C_TXPAUSE, 0x00);

		/* Set beacon interval. */
		rtwn_write_2(sc, R92C_BCN_INTERVAL, ni->ni_intval);

		/* Allow Rx from our BSSID only. */
		rtwn_write_4(sc, R92C_RCR,
		    rtwn_read_4(sc, R92C_RCR) |
		    R92C_RCR_CBSSID_DATA | R92C_RCR_CBSSID_BCN);

		/* Enable TSF synchronization. */
		rtwn_tsf_sync_enable(sc);

		/* Firmware must know both the negotiated rate mask and the AP's
		 * connected MACID before any firmware-adapted data is submitted. */
		error = rtwn_ra_init(sc);
		if (!error && (sc->chip & RTWN_CHIP_92E))
			error = rtwn_r92e_media_status(sc, 1);
		if (error) {
			rtwn_write_2(sc, R92C_RXFLTMAP2, 0);
			rtwn_set_nettype(sc, IEEE80211_M_MONITOR);
			break;
		}
		if (sc->rate_config.ht || (sc->chip & RTWN_CHIP_92E)) {
			/* RX BlockAck reorder runs in net80211. Program BAR
			 * (control subtype 8) in the subtype map and keep ACF low:
			 * the RTL8192E RX filter is bypassed by accept-all-control.
			 * BAR does not require ACF (vendor hal_com_reg.h), while
			 * the driver does not consume ACK/BA. */
			rtwn_write_2(sc, R92C_RXFLTMAP1,
			    sc->rate_config.ht ? 0x0100 : 0);
			reg = rtwn_read_4(sc, R92C_RCR) & ~R92C_RCR_ACF;
			rtwn_write_4(sc, R92C_RCR, reg);
		}

		/* Turn link LED on. */
		rtwn_set_led(sc, RTWN_LED_LINK, 1);

		sc->avg_pwdb = -1;	/* Reset average RSSI. */
		/* Reset temperature calibration state machine. */
		sc->thcal_state = 0;
		sc->thcal_lctemp = 0;
		/* Start periodic calibration. */
		sc->sc_ops.next_calib(sc->sc_ops.cookie);
		break;
	}

done:
	if (error == 0)
		error = rtwn_io_error(sc);
	/* Generic INIT always tears down host keys, queues and controlled port,
	 * even when USB is gone and hardware cleanup cannot succeed. */
	if (error == 0 || nstate == IEEE80211_S_INIT) {
		int state_error = sc->sc_newstate(ic, nstate, arg);
		if (error == 0) error = state_error;
	}
	splx(s);

	return (error);
}

void
rtwn_update_short_preamble(struct ieee80211com *ic)
{
	struct rtwn_softc *sc = (struct rtwn_softc *)ic->ic_softc;

	if (sc->chip & (RTWN_CHIP_88E | RTWN_CHIP_92E))
		rtwn_r88e_update_short_preamble(sc);
	else
		rtwn_r92c_update_short_preamble(sc);
}

void
rtwn_r92c_update_short_preamble(struct rtwn_softc *sc)
{
	uint32_t reg;

	reg = rtwn_read_4(sc, R92C_RRSR);
	if (sc->sc_ic.ic_flags & IEEE80211_F_SHPREAMBLE)
		reg |= R92C_RRSR_SHORT;
	else
		reg &= ~R92C_RRSR_SHORT;
	rtwn_write_4(sc, R92C_RRSR, reg);
}

void
rtwn_r88e_update_short_preamble(struct rtwn_softc *sc)
{
	uint32_t reg;

	reg = rtwn_read_4(sc, R92C_WMAC_TRXPTCL_CTL);
	if (sc->sc_ic.ic_flags & IEEE80211_F_SHPREAMBLE)
		reg |= R92C_WMAC_TRXPTCL_CTL_SHORT;
	else
		reg &= ~R92C_WMAC_TRXPTCL_CTL_SHORT;
	rtwn_write_4(sc, R92C_WMAC_TRXPTCL_CTL, reg);
}

void
rtwn_updateslot(struct ieee80211com *ic)
{
	struct rtwn_softc *sc = (struct rtwn_softc *)ic->ic_softc;
	int s;

	s = splnet();
	if (ic->ic_flags & IEEE80211_F_SHSLOT)
		rtwn_write_1(sc, R92C_SLOT, IEEE80211_DUR_DS_SHSLOT);
	else
		rtwn_write_1(sc, R92C_SLOT, IEEE80211_DUR_DS_SLOT);
	splx(s);
}

void
rtwn_updateedca(struct ieee80211com *ic)
{
	struct rtwn_softc *sc = (struct rtwn_softc *)ic->ic_softc;
	const uint16_t aci2reg[EDCA_NUM_AC] = {
		R92C_EDCA_BE_PARAM,
		R92C_EDCA_BK_PARAM,
		R92C_EDCA_VI_PARAM,
		R92C_EDCA_VO_PARAM
	};
	struct ieee80211_edca_ac_params *ac;
	int s, aci, aifs, slottime;
	uint8_t acm = 0;

	if (ic->ic_flags & IEEE80211_F_SHSLOT)
		slottime = IEEE80211_DUR_DS_SHSLOT;
	else
		slottime = IEEE80211_DUR_DS_SLOT;
	s = splnet();
	for (aci = 0; aci < EDCA_NUM_AC; aci++) {
		ac = &ic->ic_edca_ac[aci];
		/* AIFS[AC] = AIFSN[AC] * aSlotTime + aSIFSTime. */
		aifs = ac->ac_aifsn * slottime + IEEE80211_DUR_DS_SIFS;
		rtwn_write_4(sc, aci2reg[aci],
		    SM(R92C_EDCA_PARAM_TXOP, ac->ac_txoplimit) |
		    SM(R92C_EDCA_PARAM_ECWMIN, ac->ac_ecwmin) |
		    SM(R92C_EDCA_PARAM_ECWMAX, ac->ac_ecwmax) |
		    SM(R92C_EDCA_PARAM_AIFS, aifs));

		/* Is admission control mandatory for this queue? */
		if (ac->ac_acm) {
			switch (aci) {
			case EDCA_AC_BE:
				acm |= R92C_ACMHW_BEQEN;
				break;
			case EDCA_AC_VI:
				acm |= R92C_ACMHW_VIQEN;
				break;
			case EDCA_AC_VO:
				acm |= R92C_ACMHW_VOQEN;
				break;
			default:
				break;
			}
		}
	}
	splx(s);

	/* Enable hardware admission control. */
	rtwn_write_1(sc, R92C_ACMHWCTRL, R92C_ACMHW_HWEN | acm);
}

int
rtwn_set_key(struct ieee80211com *ic, struct ieee80211_node *ni,
    struct ieee80211_key *k)
{
	struct rtwn_softc *sc = (struct rtwn_softc *)ic->ic_softc;
	static const uint8_t etherzeroaddr[6] = { 0 };
	const uint8_t *macaddr;
	uint8_t keybuf[16], algo;
	int i, entry;

	/* Defer setting of WEP keys until interface is brought up. */
	if ((ic->ic_if.if_flags & (IFF_UP | IFF_RUNNING)) !=
	    (IFF_UP | IFF_RUNNING))
		return (0);

	/* Map net80211 cipher to HW crypto algorithm. */
	switch (k->k_cipher) {
	case IEEE80211_CIPHER_WEP40:
		algo = R92C_CAM_ALGO_WEP40;
		break;
	case IEEE80211_CIPHER_WEP104:
		algo = R92C_CAM_ALGO_WEP104;
		break;
	case IEEE80211_CIPHER_TKIP:
		algo = R92C_CAM_ALGO_TKIP;
		break;
	case IEEE80211_CIPHER_CCMP:
		algo = R92C_CAM_ALGO_AES;
		break;
	default:
		/* Fallback to software crypto for other ciphers. */
		return (ieee80211_set_key(ic, ni, k));
	}
	if (k->k_flags & IEEE80211_KEY_GROUP) {
		macaddr = etherzeroaddr;
		entry = k->k_id;
	} else {
		macaddr = ic->ic_bss->ni_macaddr;
		entry = 4;
	}
	/* Write key. */
	memset(keybuf, 0, sizeof(keybuf));
	memcpy(keybuf, k->k_key, MIN(k->k_len, sizeof(keybuf)));
	for (i = 0; i < 4; i++) {
		rtwn_cam_write(sc, R92C_CAM_KEY(entry, i),
		    LE_READ_4(&keybuf[i * 4]));
	}
	/* Write CTL0 last since that will validate the CAM entry. */
	rtwn_cam_write(sc, R92C_CAM_CTL1(entry),
	    LE_READ_4(&macaddr[2]));
	rtwn_cam_write(sc, R92C_CAM_CTL0(entry),
	    SM(R92C_CAM_ALGO, algo) |
	    SM(R92C_CAM_KEYID, k->k_id) |
	    SM(R92C_CAM_MACLO, LE_READ_2(&macaddr[0])) |
	    R92C_CAM_VALID);

	return (0);
}

void
rtwn_delete_key(struct ieee80211com *ic, struct ieee80211_node *ni,
    struct ieee80211_key *k)
{
	struct rtwn_softc *sc = (struct rtwn_softc *)ic->ic_softc;
	int i, entry;

	if (!(ic->ic_if.if_flags & IFF_RUNNING) ||
	    ic->ic_state != IEEE80211_S_RUN)
		return;	/* Nothing to do. */

	if (k->k_flags & IEEE80211_KEY_GROUP)
		entry = k->k_id;
	else
		entry = 4;
	rtwn_cam_write(sc, R92C_CAM_CTL0(entry), 0);
	rtwn_cam_write(sc, R92C_CAM_CTL1(entry), 0);
	/* Clear key. */
	for (i = 0; i < 4; i++)
		rtwn_cam_write(sc, R92C_CAM_KEY(entry, i), 0);
}

void
rtwn_update_avgrssi(struct rtwn_softc *sc, int rate, int8_t rssi)
{
	int pwdb;

	/* Convert antenna signal to percentage. */
	if (rssi <= -100 || rssi >= 20)
		pwdb = 0;
	else if (rssi >= 0)
		pwdb = 100;
	else
		pwdb = 100 + rssi;
	if (sc->chip & (RTWN_CHIP_92C | RTWN_CHIP_88C)) {
		if (rate <= 3) {
			/* CCK gain is smaller than OFDM/MCS gain. */
			pwdb += 6;
			if (pwdb > 100)
				pwdb = 100;
			if (pwdb <= 14)
				pwdb -= 4;
			else if (pwdb <= 26)
				pwdb -= 8;
			else if (pwdb <= 34)
				pwdb -= 6;
			else if (pwdb <= 42)
				pwdb -= 2;
		}
	}
	if (sc->avg_pwdb == -1)	/* Init. */
		sc->avg_pwdb = pwdb;
	else if (sc->avg_pwdb < pwdb)
		sc->avg_pwdb = ((sc->avg_pwdb * 19 + pwdb) / 20) + 1;
	else
		sc->avg_pwdb = ((sc->avg_pwdb * 19 + pwdb) / 20);
	DPRINTFN(4, ("PWDB=%d EMA=%d\n", pwdb, sc->avg_pwdb));
}

int8_t
rtwn_get_rssi(struct rtwn_softc *sc, int rate, void *physt)
{
	static const int8_t cckoff[] = { 16, -12, -26, -46 };
	struct r92c_rx_phystat *phy;
	struct r92c_rx_cck *cck;
	uint8_t rpt;
	int8_t rssi;

	if (sc->chip & (RTWN_CHIP_88E | RTWN_CHIP_92E))
		return rtwn_r88e_get_rssi(sc, rate, physt);
	else if (sc->chip & RTWN_CHIP_88F)
		return rtwn_r88f_get_rssi(sc, rate, physt);

	if (rate <= 3) {
		cck = (struct r92c_rx_cck *)physt;
		if (sc->sc_flags & RTWN_FLAG_CCK_HIPWR) {
			rpt = (cck->agc_rpt >> 5) & 0x3;
			rssi = (cck->agc_rpt & 0x1f) << 1;
		} else {
			rpt = (cck->agc_rpt >> 6) & 0x3;
			rssi = cck->agc_rpt & 0x3e;
		}
		rssi = cckoff[rpt] - rssi;
	} else {	/* OFDM/HT. */
		phy = (struct r92c_rx_phystat *)physt;
		rssi = ((letoh32(phy->phydw1) >> 1) & 0x7f) - 110;
	}
	return (rssi);
}

int8_t
rtwn_r88e_get_rssi(struct rtwn_softc *sc, int rate, void *physt)
{
	static const int8_t cckoff[] = { 20, 14, 10, -4, -16, -22, -38, -40 };
	struct r88e_rx_phystat *phy;
	uint8_t rpt;
	int8_t rssi;

	phy = (struct r88e_rx_phystat *)physt;

	if (rate <= 3) {
		rpt = (phy->agc_rpt >> 5) & 0x7;
		rssi = (phy->agc_rpt & 0x1f) << 1;
		if (sc->sc_flags & RTWN_FLAG_CCK_HIPWR) {
			if (rpt == 2)
				rssi -= 6;
		}
		/* The VGA saturation floor belongs to LNA index 7 only.
		 * Applying it to every LNA index caused artificial jumps to -94 dBm.
		 * Reference: FreeBSD rtl8188e/r88e_rx.c, r88e_get_rssi_cck(). */
		rssi = (rpt == 7 && (phy->agc_rpt & 0x1f) > 27) ?
		    -94 : cckoff[rpt] - rssi;
	} else {	/* OFDM/HT. */
		rssi = ((le32toh(phy->sq_rpt) >> 1) & 0x7f) - 110;
	}
	return (rssi);
}

int8_t
rtwn_r88f_get_rssi(struct rtwn_softc *sc, int rate, void *physt)
{
	struct r88e_rx_phystat *phy;
	uint8_t lna_idx, vga_idx;
	int8_t rssi;

	phy = (struct r88e_rx_phystat *)physt;
	lna_idx = (phy->agc_rpt & 0xe0) >> 5;
	vga_idx = (phy->agc_rpt & 0x1f);
	rssi = -(2 * vga_idx);

	if (rate <= 3) {
		switch (lna_idx) {
		case 7:
			if (vga_idx > 27)
				rssi = -100;
			else
				rssi += -46;
			break;
		case 5:
			rssi += -32;
			break;
		case 3:
			rssi += -20;
			break;
		case 1:
			rssi += -6;
			break;
		default:
			rssi = 0;
			break;
		}
	} else {	/* OFDM/HT. */
		rssi = ((le32toh(phy->sq_rpt) >> 1) & 0x7f) - 110;
	}
	return (rssi);
}

void
rtwn_start(struct _ifnet *ifp)
{
	struct rtwn_softc *sc = (struct rtwn_softc *)ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni;
	mbuf_t m;

	if (!(ifp->if_flags & IFF_RUNNING) || ifq_is_oactive(&ifp->if_snd))
		return;

	for (;;) {
		if (sc->sc_ops.is_oactive(sc->sc_ops.cookie)) {
			ifq_set_oactive(&ifp->if_snd);
			break;
		}
		/* Send pending management frames first. */
		m = mq_dequeue(&ic->ic_mgtq);
		if (m != NULL) {
			ni = (struct ieee80211_node *)mbuf_pkthdr_rcvif(m);
			goto sendit;
		}
		if (ic->ic_state != IEEE80211_S_RUN)
			break;

		/* Encapsulate and send data frames. */
		m = ifq_dequeue(&ifp->if_snd);
		if (m == NULL)
			break;
#if NBPFILTER > 0
		if (ifp->if_bpf != NULL)
			bpf_mtap(ifp->if_bpf, m, BPF_DIRECTION_OUT);
#endif
		if ((m = ieee80211_encap(ifp, m, &ni)) == NULL)
			continue;
sendit:
#if NBPFILTER > 0
		if (ic->ic_rawbpf != NULL)
			bpf_mtap(ic->ic_rawbpf, m, BPF_DIRECTION_OUT);
#endif
		if (sc->sc_ops.tx(sc->sc_ops.cookie, m, ni) != 0) {
			ieee80211_release_node(ic, ni);
			ifp->if_oerrors++;
			continue;
		}

		sc->sc_tx_timer = 5;
		ifp->if_timer = 1;
	}
}

void
rtwn_watchdog(struct _ifnet *ifp)
{
	struct rtwn_softc *sc = (struct rtwn_softc *)ifp->if_softc;

	ifp->if_timer = 0;

	if (sc->sc_tx_timer > 0) {
		if (--sc->sc_tx_timer == 0) {
			printf("%s: device timeout\n", sc->sc_pdev->dv_xname);
			task_add(systq, &sc->init_task);
			ifp->if_oerrors++;
			return;
		}
		ifp->if_timer = 1;
	}
	ieee80211_watchdog(ifp);
}

int
rtwn_ioctl(struct _ifnet *ifp, u_long cmd, caddr_t data)
{
	struct rtwn_softc *sc = (struct rtwn_softc *)ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	int s, error = 0;

	s = splnet();
	/*
	 * Prevent processes from entering this function while another
	 * process is tsleep'ing in it.
	 */
	if (sc->sc_flags & RTWN_FLAG_BUSY)
		error = EBUSY;
	if (error != 0) {
		splx(s);
		return error;
	}
	sc->sc_flags |= RTWN_FLAG_BUSY;

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		/* FALLTHROUGH */
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			if (!(ifp->if_flags & IFF_RUNNING))
				error = rtwn_init(ifp);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				rtwn_stop(ifp);
		}
		break;
	case SIOCS80211CHANNEL:
		error = ieee80211_ioctl(ifp, cmd, data);
		if (error == ENETRESET &&
		    ic->ic_opmode == IEEE80211_M_MONITOR) {
			if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) ==
			    (IFF_UP | IFF_RUNNING))
				rtwn_set_chan(sc, ic->ic_ibss_chan, NULL);
			error = 0;
		}
		break;
	default:
		error = ieee80211_ioctl(ifp, cmd, data);
	}

	if (error == ENETRESET) {
		if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) ==
		    (IFF_UP | IFF_RUNNING)) {
			rtwn_stop(ifp);
			error = rtwn_init(ifp);
		} else
			error = 0;
	}
	sc->sc_flags &= ~RTWN_FLAG_BUSY;

	splx(s);

	return (error);
}

void
rtwn_fw_reset(struct rtwn_softc *sc)
{
	if (sc->chip & (RTWN_CHIP_88E | RTWN_CHIP_88F | RTWN_CHIP_92E))
		rtwn_r88e_fw_reset(sc);
	else
		rtwn_r92c_fw_reset(sc);
}

void
rtwn_r92c_fw_reset(struct rtwn_softc *sc)
{
	uint16_t reg = 0;
	int ntries;

	/* Tell 8051 to reset itself. */
	rtwn_write_1(sc, R92C_HMETFR + 3, 0x20);

	/* Wait until 8051 resets by itself. */
	for (ntries = 0; ntries < 100; ntries++) {
		reg = rtwn_read_2(sc, R92C_SYS_FUNC_EN);
		if (!(reg & R92C_SYS_FUNC_EN_CPUEN))
			goto sleep;
		rtwn_delay(sc, 50);
	}
	/* Force 8051 reset. */
	rtwn_write_2(sc, R92C_SYS_FUNC_EN, reg & ~R92C_SYS_FUNC_EN_CPUEN);
sleep:
	if (sc->chip & RTWN_CHIP_PCI) {
		/*
		 * We must sleep for one second to let the firmware settle.
		 * Accessing registers too early will hang the whole system.
		 */
		rtwn_delay(sc, 1000000);
	}
}

void
rtwn_r88e_fw_reset(struct rtwn_softc *sc)
{
	/* Reset MCU IO wrapper. */
	if (!(sc->chip & RTWN_CHIP_88F)) {
		rtwn_write_1(sc, R92C_RSV_CTRL,
		    rtwn_read_1(sc, R92C_RSV_CTRL) & ~R92C_RSV_CTRL_WLOCK_00);
	}
	if (sc->chip & RTWN_CHIP_88E) {
		rtwn_write_2(sc, R92C_RSV_CTRL,
		    rtwn_read_2(sc, R92C_RSV_CTRL) & ~R88E_RSV_CTRL_MCU_RST);
	} else {
		rtwn_write_2(sc, R92C_RSV_CTRL,
		    rtwn_read_2(sc, R92C_RSV_CTRL) & ~R88E_RSV_CTRL_MIO_EN);
	}
	rtwn_write_2(sc, R92C_SYS_FUNC_EN,
	    rtwn_read_2(sc, R92C_SYS_FUNC_EN) & ~R92C_SYS_FUNC_EN_CPUEN);

	/* Enable MCU IO wrapper. */
	if (!(sc->chip & RTWN_CHIP_88F)) {
		rtwn_write_1(sc, R92C_RSV_CTRL,
		    rtwn_read_1(sc, R92C_RSV_CTRL) & ~R92C_RSV_CTRL_WLOCK_00);
	}
	if (sc->chip & RTWN_CHIP_88E) {
		rtwn_write_2(sc, R92C_RSV_CTRL,
		    rtwn_read_2(sc, R92C_RSV_CTRL) | R88E_RSV_CTRL_MCU_RST);
	} else {
		rtwn_write_2(sc, R92C_RSV_CTRL,
		    rtwn_read_2(sc, R92C_RSV_CTRL) | R88E_RSV_CTRL_MIO_EN);
	}
	rtwn_write_2(sc, R92C_SYS_FUNC_EN,
	    rtwn_read_2(sc, R92C_SYS_FUNC_EN) | R92C_SYS_FUNC_EN_CPUEN);
}

int
rtwn_load_firmware(struct rtwn_softc *sc)
{
	const struct r92c_fw_hdr *hdr;
	u_char *fw, *ptr;
	size_t len0, len;
	uint32_t reg;
	int mlen, ntries, page, error;

	/* Read firmware image from the filesystem. */
	error = sc->sc_ops.load_firmware(sc->sc_ops.cookie, &fw, &len0);
	if (error)
		return (error);
	len = len0;
	if (len < sizeof(*hdr)) {
		printf("%s: firmware too short\n", sc->sc_pdev->dv_xname);
		error = EINVAL;
		goto fail;
	}
	ptr = fw;
	hdr = (const struct r92c_fw_hdr *)ptr;
	/* Check if there is a valid FW header and skip it. */
	if ((letoh16(hdr->signature) >> 4) == 0x230 ||
	    (letoh16(hdr->signature) >> 4) == 0x88c ||
	    (letoh16(hdr->signature) >> 4) == 0x88e ||
	    (letoh16(hdr->signature) >> 4) == 0x88f ||
	    (letoh16(hdr->signature) >> 4) == 0x92c ||
	    (letoh16(hdr->signature) >> 4) == 0x92e) {
		DPRINTF(("FW V%d.%d %02d-%02d %02d:%02d\n",
		    letoh16(hdr->version), letoh16(hdr->subversion),
		    hdr->month, hdr->date, hdr->hour, hdr->minute));
		ptr += sizeof(*hdr);
		len -= sizeof(*hdr);
	}

	if (rtwn_read_1(sc, R92C_MCUFWDL) & R92C_MCUFWDL_RAM_DL_SEL) {
		rtwn_write_1(sc, R92C_MCUFWDL, 0);
		rtwn_fw_reset(sc);
	}

	if ((sc->chip & RTWN_CHIP_PCI) || (sc->chip & RTWN_CHIP_88F)) {
		rtwn_write_2(sc, R92C_SYS_FUNC_EN,
		    rtwn_read_2(sc, R92C_SYS_FUNC_EN) | R92C_SYS_FUNC_EN_CPUEN);
	}

	/* Enable FW download. */
	rtwn_write_1(sc, R92C_MCUFWDL,
	    rtwn_read_1(sc, R92C_MCUFWDL) | R92C_MCUFWDL_EN);
	rtwn_write_4(sc, R92C_MCUFWDL,
	    rtwn_read_4(sc, R92C_MCUFWDL) & ~R92C_MCUFWDL_ROM_DLEN);

	/* Reset the FWDL checksum. */
	rtwn_write_1(sc, R92C_MCUFWDL,
	    rtwn_read_1(sc, R92C_MCUFWDL) | R92C_MCUFWDL_CHKSUM_RPT);

	rtwn_delay(sc, 50);
	for (page = 0; len > 0; page++) {
		mlen = MIN(len, R92C_FW_PAGE_SIZE);
		error = sc->sc_ops.fw_loadpage(sc->sc_ops.cookie, page, ptr,
		    mlen);
		if (error != 0) {
			printf("%s: could not load firmware page %d\n",
			    sc->sc_pdev->dv_xname, page);
			goto fail;
		}
		ptr += mlen;
		len -= mlen;
	}

	/* Wait for checksum report. */
	for (ntries = 0; ntries < 1000; ntries++) {
		if (rtwn_read_4(sc, R92C_MCUFWDL) & R92C_MCUFWDL_CHKSUM_RPT)
			break;
		rtwn_delay(sc, 5);
	}
	if (ntries == 1000) {
		printf("%s: timeout waiting for checksum report\n",
		    sc->sc_pdev->dv_xname);
		error = ETIMEDOUT;
		goto fail;
	}

	/* Disable FW download. */
	rtwn_write_1(sc, R92C_MCUFWDL,
	    rtwn_read_1(sc, R92C_MCUFWDL) & ~R92C_MCUFWDL_EN);

	/* Reserved for fw extension. */
	if (!(sc->chip & (RTWN_CHIP_88F | RTWN_CHIP_92E)))
		rtwn_write_1(sc, R92C_MCUFWDL + 1, 0);

	reg = rtwn_read_4(sc, R92C_MCUFWDL);
	reg = (reg & ~R92C_MCUFWDL_WINTINI_RDY) | R92C_MCUFWDL_RDY;
	rtwn_write_4(sc, R92C_MCUFWDL, reg);
	if (sc->chip & (RTWN_CHIP_92C | RTWN_CHIP_88C | RTWN_CHIP_23A)) {
		reg = rtwn_read_2(sc, R92C_SYS_FUNC_EN);
		rtwn_write_2(sc, R92C_SYS_FUNC_EN,
		    reg & ~R92C_SYS_FUNC_EN_CPUEN);
		rtwn_write_2(sc, R92C_SYS_FUNC_EN,
		    reg | R92C_SYS_FUNC_EN_CPUEN);
	} else
		rtwn_fw_reset(sc);
	/* Wait for firmware readiness. */
	for (ntries = 0; ntries < 1000; ntries++) {
		if (rtwn_read_4(sc, R92C_MCUFWDL) & R92C_MCUFWDL_WINTINI_RDY)
			break;
		rtwn_delay(sc, 10);
	}
	if (ntries == 1000) {
		printf("%s: timeout waiting for firmware readiness\n",
		    sc->sc_pdev->dv_xname);
		error = ETIMEDOUT;
		goto fail;
	}
fail:
	free(fw);
	/* Init H2C command. */
	if (sc->chip & RTWN_CHIP_88F)
		rtwn_write_1(sc, R92C_HMETFR, 0xf);
	return (error);
}

void
rtwn_rf_init(struct rtwn_softc *sc)
{
	const struct r92c_rf_prog *prog;
	uint32_t reg, type;
	int i, j, idx, off;

	/* Select RF programming based on board type. */
	if (sc->chip & RTWN_CHIP_88E)
		prog = rtl8188eu_rf_prog;
	else if (sc->chip & RTWN_CHIP_88F)
		prog = rtl8188ftv_rf_prog;
	else if (sc->chip & RTWN_CHIP_92E)
		prog = rtl8192e_rf_prog;
	else if (!(sc->chip & RTWN_CHIP_92C)) {
		if (sc->board_type == R92C_BOARD_TYPE_MINICARD)
			prog = rtl8188ce_rf_prog;
		else if (sc->board_type == R92C_BOARD_TYPE_HIGHPA)
			prog = rtl8188ru_rf_prog;
		else
			prog = rtl8188cu_rf_prog;
	} else
		prog = rtl8192ce_rf_prog;

	for (i = 0; i < sc->nrxchains; i++) {
		/* Save RF_ENV control type. */
		idx = i / 2;
		off = (i % 2) * 16;
		reg = rtwn_bb_read(sc, R92C_FPGA0_RFIFACESW(idx));
		type = (reg >> off) & 0x10;

		/* Set RF_ENV enable. */
		reg = rtwn_bb_read(sc, R92C_FPGA0_RFIFACEOE(i));
		reg |= 0x100000;
		rtwn_bb_write(sc, R92C_FPGA0_RFIFACEOE(i), reg);
		rtwn_delay(sc, 50);
		/* Set RF_ENV output high. */
		reg = rtwn_bb_read(sc, R92C_FPGA0_RFIFACEOE(i));
		reg |= 0x10;
		rtwn_bb_write(sc, R92C_FPGA0_RFIFACEOE(i), reg);
		rtwn_delay(sc, 50);
		/* Set address and data lengths of RF registers. */
		reg = rtwn_bb_read(sc, R92C_HSSI_PARAM2(i));
		reg &= ~R92C_HSSI_PARAM2_ADDR_LENGTH;
		rtwn_bb_write(sc, R92C_HSSI_PARAM2(i), reg);
		rtwn_delay(sc, 50);
		reg = rtwn_bb_read(sc, R92C_HSSI_PARAM2(i));
		reg &= ~R92C_HSSI_PARAM2_DATA_LENGTH;
		rtwn_bb_write(sc, R92C_HSSI_PARAM2(i), reg);
		rtwn_delay(sc, 50);

		/* Write RF initialization values for this chain. */
		for (j = 0; j < prog[i].count; j++) {
			switch (prog[i].regs[j]) {
			case 0xfe:
			case 0xffe:
				rtwn_delay(sc, 50000);
				continue;
			case 0xfd:
				rtwn_delay(sc, 5000);
				continue;
			case 0xfc:
				rtwn_delay(sc, 1000);
				continue;
			case 0xfb:
				rtwn_delay(sc, 50);
				continue;
			case 0xfa:
				rtwn_delay(sc, 5);
				continue;
			case 0xf9:
				rtwn_delay(sc, 1);
				continue;
			}
			rtwn_rf_write(sc, i, prog[i].regs[j],
			    prog[i].vals[j]);
			rtwn_delay(sc, 5);
		}

		/* Restore RF_ENV control type. */
		reg = rtwn_bb_read(sc, R92C_FPGA0_RFIFACESW(idx));
		reg &= ~(0x10 << off) | (type << off);
		rtwn_bb_write(sc, R92C_FPGA0_RFIFACESW(idx), reg);

		/* Cache RF register CHNLBW. */
		sc->rf_chnlbw[i] = rtwn_rf_read(sc, i, R92C_RF_CHNLBW);
	}

	/* magic value for HP 8188EEs */
	if (sc->chip == (RTWN_CHIP_88E | RTWN_CHIP_PCI)) {
		struct r88e_rom *rom = &sc->sc_r88e_rom;
		if (rom->r88ee_rom.svid == 0x103c &&
		    rom->r88ee_rom.smid == 0x197d)
			rtwn_rf_write(sc, 0, 0x52, 0x7e4bd);
	}

	if ((sc->chip & (RTWN_CHIP_UMC_A_CUT | RTWN_CHIP_92C)) ==
	    RTWN_CHIP_UMC_A_CUT) {
		rtwn_rf_write(sc, 0, R92C_RF_RX_G1, 0x30255);
		rtwn_rf_write(sc, 0, R92C_RF_RX_G2, 0x50a00);
	} else if (sc->chip & RTWN_CHIP_23A) {
		rtwn_rf_write(sc, 0, 0x0C, 0x894ae);
		rtwn_rf_write(sc, 0, 0x0A, 0x1af31);
		rtwn_rf_write(sc, 0, R92C_RF_IPA, 0x8f425);
		rtwn_rf_write(sc, 0, R92C_RF_SYN_G(1), 0x4f200);
		rtwn_rf_write(sc, 0, R92C_RF_RCK1, 0x44053);
		rtwn_rf_write(sc, 0, R92C_RF_RCK2, 0x80201);
	}
}

void
rtwn_cam_init(struct rtwn_softc *sc)
{
	/* Invalidate all CAM entries. */
	rtwn_write_4(sc, R92C_CAMCMD,
	    R92C_CAMCMD_POLLING | R92C_CAMCMD_CLR);
}

void
rtwn_pa_bias_init(struct rtwn_softc *sc)
{
	uint8_t reg;
	int i;

	for (i = 0; i < sc->nrxchains; i++) {
		if (sc->pa_setting & (1 << i))
			continue;
		rtwn_rf_write(sc, i, R92C_RF_IPA, 0x0f406);
		rtwn_rf_write(sc, i, R92C_RF_IPA, 0x4f406);
		rtwn_rf_write(sc, i, R92C_RF_IPA, 0x8f406);
		rtwn_rf_write(sc, i, R92C_RF_IPA, 0xcf406);
	}
	if (!(sc->pa_setting & 0x10)) {
		reg = rtwn_read_1(sc, 0x16);
		reg = (reg & ~0xf0) | 0x90;
		rtwn_write_1(sc, 0x16, reg);
	}
}

void
rtwn_rxfilter_init(struct rtwn_softc *sc)
{
	/* Initialize Rx filter. */
	rtwn_write_4(sc, R92C_RCR,
	    R92C_RCR_AAP | R92C_RCR_APM | R92C_RCR_AM | R92C_RCR_AB |
	    R92C_RCR_APP_ICV | R92C_RCR_AMF | R92C_RCR_HTC_LOC_CTRL |
	    R92C_RCR_APP_MIC | R92C_RCR_APP_PHYSTS);
	/* Accept all multicast frames. */
	rtwn_write_4(sc, R92C_MAR + 0, 0xffffffff);
	rtwn_write_4(sc, R92C_MAR + 4, 0xffffffff);
	/* Accept all management frames. */
	rtwn_write_2(sc, R92C_RXFLTMAP0, 0xffff);
	/* Reject all control frames. */
	rtwn_write_2(sc, R92C_RXFLTMAP1, 0x0000);
	/* Accept all data frames. */
	rtwn_write_2(sc, R92C_RXFLTMAP2, 0xffff);
}

void
rtwn_edca_init(struct rtwn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	int mode, aci;

	/* Set SIFS; 0x10 = 16 usec (SIFS 11g), 0x0a = 10 usec (SIFS 11b) */
	rtwn_write_2(sc, R92C_SPEC_SIFS, 0x100a);
	rtwn_write_2(sc, R92C_MAC_SPEC_SIFS, 0x100a);
	rtwn_write_2(sc, R92C_SIFS_CCK, 0x100a);
	rtwn_write_2(sc, R92C_SIFS_OFDM, 0x100a);
	if (!(sc->chip & RTWN_CHIP_88F)) {
		rtwn_write_2(sc, R92C_RESP_SIFS_CCK, 0x100a);
		rtwn_write_2(sc, R92C_RESP_SIFS_OFDM, 0x100a);
	} else {
		rtwn_write_2(sc, R92C_RESP_SIFS_CCK, 0x0808);
		rtwn_write_2(sc, R92C_RESP_SIFS_OFDM, 0x0a0a);
	}

	if (ic->ic_curmode == IEEE80211_MODE_AUTO)
		mode = IEEE80211_MODE_11G; /* XXX */
	else
		mode = ic->ic_curmode;
	for (aci = 0; aci < EDCA_NUM_AC; aci++)
		memcpy(&ic->ic_edca_ac[aci], &ieee80211_edca_table[mode][aci],
		    sizeof(struct ieee80211_edca_ac_params));
	rtwn_updateedca(ic);

	if (sc->chip & RTWN_CHIP_PCI) {
		/* linux magic */
		rtwn_write_4(sc, R92C_FAST_EDCA_CTRL, 0x086666);
	}

	rtwn_write_4(sc, R92C_EDCA_RANDOM_GEN, arc4random());
}

void
rtwn_rate_fallback_init(struct rtwn_softc *sc)
{
	if (!(sc->chip & (RTWN_CHIP_88E | RTWN_CHIP_92E))) {
		if (sc->chip & RTWN_CHIP_PCI) {
			rtwn_write_4(sc, R92C_DARFRC + 0, 0x01000000);
			rtwn_write_4(sc, R92C_DARFRC + 4, 0x07060504);
			rtwn_write_4(sc, R92C_RARFRC + 0, 0x01000000);
			rtwn_write_4(sc, R92C_RARFRC + 4, 0x07060504);
		} else if (sc->chip & RTWN_CHIP_USB) {
			rtwn_write_4(sc, R92C_DARFRC + 0, 0x00000000);
			rtwn_write_4(sc, R92C_DARFRC + 4, 0x10080404);
			rtwn_write_4(sc, R92C_RARFRC + 0, 0x04030201);
			rtwn_write_4(sc, R92C_RARFRC + 4, 0x08070605);
		}
	}
}

void
rtwn_write_txpower(struct rtwn_softc *sc, int chain,
    uint16_t power[RTWN_POWER_COUNT])
{
	uint32_t reg;

	/* Write per-CCK rate Tx power. */
	if (chain == 0) {
		reg = rtwn_bb_read(sc, R92C_TXAGC_A_CCK1_MCS32);
		reg = RW(reg, R92C_TXAGC_A_CCK1,  power[RTWN_POWER_CCK1]);
		rtwn_bb_write(sc, R92C_TXAGC_A_CCK1_MCS32, reg);
		reg = rtwn_bb_read(sc, R92C_TXAGC_B_CCK11_A_CCK2_11);
		reg = RW(reg, R92C_TXAGC_A_CCK2,  power[RTWN_POWER_CCK2]);
		reg = RW(reg, R92C_TXAGC_A_CCK55, power[RTWN_POWER_CCK55]);
		reg = RW(reg, R92C_TXAGC_A_CCK11, power[RTWN_POWER_CCK11]);
		rtwn_bb_write(sc, R92C_TXAGC_B_CCK11_A_CCK2_11, reg);
	} else {
		reg = rtwn_bb_read(sc, R92C_TXAGC_B_CCK1_55_MCS32);
		reg = RW(reg, R92C_TXAGC_B_CCK1,  power[RTWN_POWER_CCK1]);
		reg = RW(reg, R92C_TXAGC_B_CCK2,  power[RTWN_POWER_CCK2]);
		reg = RW(reg, R92C_TXAGC_B_CCK55, power[RTWN_POWER_CCK55]);
		rtwn_bb_write(sc, R92C_TXAGC_B_CCK1_55_MCS32, reg);
		reg = rtwn_bb_read(sc, R92C_TXAGC_B_CCK11_A_CCK2_11);
		reg = RW(reg, R92C_TXAGC_B_CCK11, power[RTWN_POWER_CCK11]);
		rtwn_bb_write(sc, R92C_TXAGC_B_CCK11_A_CCK2_11, reg);
	}
	/* Write per-OFDM rate Tx power. */
	rtwn_bb_write(sc, R92C_TXAGC_RATE18_06(chain),
	    SM(R92C_TXAGC_RATE06, power[RTWN_POWER_OFDM6]) |
	    SM(R92C_TXAGC_RATE09, power[RTWN_POWER_OFDM9]) |
	    SM(R92C_TXAGC_RATE12, power[RTWN_POWER_OFDM12]) |
	    SM(R92C_TXAGC_RATE18, power[RTWN_POWER_OFDM18]));
	rtwn_bb_write(sc, R92C_TXAGC_RATE54_24(chain),
	    SM(R92C_TXAGC_RATE24, power[RTWN_POWER_OFDM24]) |
	    SM(R92C_TXAGC_RATE36, power[RTWN_POWER_OFDM36]) |
	    SM(R92C_TXAGC_RATE48, power[RTWN_POWER_OFDM48]) |
	    SM(R92C_TXAGC_RATE54, power[RTWN_POWER_OFDM54]));
	/* Write per-MCS Tx power. */
	rtwn_bb_write(sc, R92C_TXAGC_MCS03_MCS00(chain),
	    SM(R92C_TXAGC_MCS00,  power[RTWN_POWER_MCS( 0)]) |
	    SM(R92C_TXAGC_MCS01,  power[RTWN_POWER_MCS( 1)]) |
	    SM(R92C_TXAGC_MCS02,  power[RTWN_POWER_MCS( 2)]) |
	    SM(R92C_TXAGC_MCS03,  power[RTWN_POWER_MCS( 3)]));
	rtwn_bb_write(sc, R92C_TXAGC_MCS07_MCS04(chain),
	    SM(R92C_TXAGC_MCS04,  power[RTWN_POWER_MCS( 4)]) |
	    SM(R92C_TXAGC_MCS05,  power[RTWN_POWER_MCS( 5)]) |
	    SM(R92C_TXAGC_MCS06,  power[RTWN_POWER_MCS( 6)]) |
	    SM(R92C_TXAGC_MCS07,  power[RTWN_POWER_MCS( 7)]));
	if (sc->ntxchains > 1) {
		rtwn_bb_write(sc, R92C_TXAGC_MCS11_MCS08(chain),
		    SM(R92C_TXAGC_MCS08,  power[RTWN_POWER_MCS( 8)]) |
		    SM(R92C_TXAGC_MCS09,  power[RTWN_POWER_MCS( 9)]) |
		    SM(R92C_TXAGC_MCS10,  power[RTWN_POWER_MCS(10)]) |
		    SM(R92C_TXAGC_MCS11,  power[RTWN_POWER_MCS(11)]));
		rtwn_bb_write(sc, R92C_TXAGC_MCS15_MCS12(chain),
		    SM(R92C_TXAGC_MCS12,  power[RTWN_POWER_MCS(12)]) |
		    SM(R92C_TXAGC_MCS13,  power[RTWN_POWER_MCS(13)]) |
		    SM(R92C_TXAGC_MCS14,  power[RTWN_POWER_MCS(14)]) |
		    SM(R92C_TXAGC_MCS15,  power[RTWN_POWER_MCS(15)]));
	}
}

void
rtwn_get_txpower(struct rtwn_softc *sc, int chain, struct ieee80211_channel *c,
    struct ieee80211_channel *extc, uint16_t power[RTWN_POWER_COUNT])
{
	if (sc->chip & (RTWN_CHIP_88E | RTWN_CHIP_88F))
		rtwn_r88e_get_txpower(sc, chain, c, extc, power);
	else if (sc->chip & RTWN_CHIP_92E)
		rtwn_r92e_get_txpower(sc, chain, c, extc, power);
	else
		rtwn_r92c_get_txpower(sc, chain, c, extc, power);
}

void
rtwn_r92c_get_txpower(struct rtwn_softc *sc, int chain,
    struct ieee80211_channel *c, struct ieee80211_channel *extc,
    uint16_t power[RTWN_POWER_COUNT])
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct r92c_rom *rom = &sc->sc_r92c_rom;
	uint16_t cckpow, ofdmpow, htpow, diff, max;
	const struct r92c_txpwr *base;
	int ridx, chan, group;

	/* Determine channel group. */
	chan = ieee80211_chan2ieee(ic, c);	/* XXX center freq! */
	if (chan <= 3)
		group = 0;
	else if (chan <= 9)
		group = 1;
	else
		group = 2;

	/* Get original Tx power based on board type and RF chain. */
	if (!(sc->chip & RTWN_CHIP_92C)) {
		if (sc->board_type == R92C_BOARD_TYPE_HIGHPA)
			base = &rtl8188ru_txagc[chain];
		else
			base = &rtl8192cu_txagc[chain];
	} else
		base = &rtl8192cu_txagc[chain];

	memset(power, 0, RTWN_POWER_COUNT * sizeof(power[0]));
	if (sc->regulatory == 0) {
		for (ridx = RTWN_RIDX_CCK1; ridx <= RTWN_RIDX_CCK11; ridx++)
			power[ridx] = base->pwr[0][ridx];
	}
	for (ridx = RTWN_RIDX_OFDM6; ridx <= RTWN_RIDX_MAX; ridx++) {
		if (sc->regulatory == 3) {
			power[ridx] = base->pwr[0][ridx];
			/* Apply vendor limits. */
			if (extc != NULL)
				max = rom->ht40_max_pwr[group];
			else
				max = rom->ht20_max_pwr[group];
			max = (max >> (chain * 4)) & 0xf;
			if (power[ridx] > max)
				power[ridx] = max;
		} else if (sc->regulatory == 1) {
			if (extc == NULL)
				power[ridx] = base->pwr[group][ridx];
		} else if (sc->regulatory != 2)
			power[ridx] = base->pwr[0][ridx];
	}

	/* Compute per-CCK rate Tx power. */
	cckpow = rom->cck_tx_pwr[chain][group];
	for (ridx = RTWN_RIDX_CCK1; ridx <= RTWN_RIDX_CCK11; ridx++) {
		power[ridx] += cckpow;
		if (power[ridx] > R92C_MAX_TX_PWR)
			power[ridx] = R92C_MAX_TX_PWR;
	}

	htpow = rom->ht40_1s_tx_pwr[chain][group];
	if (sc->ntxchains > 1) {
		/* Apply reduction for 2 spatial streams. */
		diff = rom->ht40_2s_tx_pwr_diff[group];
		diff = (diff >> (chain * 4)) & 0xf;
		htpow = (htpow > diff) ? htpow - diff : 0;
	}

	/* Compute per-OFDM rate Tx power. */
	diff = rom->ofdm_tx_pwr_diff[group];
	diff = (diff >> (chain * 4)) & 0xf;
	ofdmpow = htpow + diff;	/* HT->OFDM correction. */
	for (ridx = RTWN_RIDX_OFDM6; ridx <= RTWN_RIDX_OFDM54; ridx++) {
		power[ridx] += ofdmpow;
		if (power[ridx] > R92C_MAX_TX_PWR)
			power[ridx] = R92C_MAX_TX_PWR;
	}

	/* Compute per-MCS Tx power. */
	if (extc == NULL) {
		diff = rom->ht20_tx_pwr_diff[group];
		diff = (diff >> (chain * 4)) & 0xf;
		htpow += diff;	/* HT40->HT20 correction. */
	}
	for (ridx = RTWN_RIDX_MCS0; ridx <= RTWN_RIDX_MCS15; ridx++) {
		power[ridx] += htpow;
		if (power[ridx] > R92C_MAX_TX_PWR)
			power[ridx] = R92C_MAX_TX_PWR;
	}
#ifdef RTWN_DEBUG
	if (rtwn_debug >= 4) {
		/* Dump per-rate Tx power values. */
		printf("Tx power for chain %d:\n", chain);
		for (ridx = RTWN_RIDX_CCK1; ridx <= RTWN_RIDX_MAX; ridx++)
			printf("Rate %d = %u\n", ridx, power[ridx]);
	}
#endif
}

void
rtwn_r92e_get_txpower(struct rtwn_softc *sc, int chain,
    struct ieee80211_channel *c, struct ieee80211_channel *extc,
    uint16_t power[RTWN_POWER_COUNT])
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct r92e_rom *rom = &sc->sc_r92e_rom;
	struct r92e_tx_pwr *txpwr;
	int cckpow, htpow, htpow2s = 0, ofdmpow;
	int8_t diff;
	int ridx, chan, group;

	/* Determine channel group. */
	chan = ieee80211_chan2ieee(ic, c);	/* XXX center freq! */
	group = rtwn_chan2group(chan);

	memset(power, 0, RTWN_POWER_COUNT * sizeof(power[0]));

	if (chain == 0)
		txpwr = &rom->txpwr_a;
	else
		txpwr = &rom->txpwr_b;

	/* Compute per-CCK rate Tx power. */
	cckpow = txpwr->cck_tx_pwr[group];
	for (ridx = RTWN_RIDX_CCK1; ridx <= RTWN_RIDX_CCK11; ridx++) {
		power[ridx] = cckpow < 0 ? 0 : cckpow;
		if (power[ridx] > R92C_MAX_TX_PWR)
			power[ridx] = R92C_MAX_TX_PWR;
	}

	htpow = txpwr->ht40_tx_pwr[group];

	/* Compute per-OFDM rate Tx power. */
	diff = RTWN_SIGN4TO8(MS(txpwr->ht20_ofdm_tx_pwr_diff,
	    R92E_ROM_TXPWR_OFDM_DIFF));
	ofdmpow = htpow + diff;
	for (ridx = RTWN_RIDX_OFDM6; ridx <= RTWN_RIDX_OFDM54; ridx++) {
		power[ridx] = ofdmpow < 0 ? 0 : ofdmpow;
		if (power[ridx] > R92C_MAX_TX_PWR)
			power[ridx] = R92C_MAX_TX_PWR;
	}

	/* Compute per-MCS Tx power. Base is HT40; HT20 has its own offset. */
	if (extc == NULL) {
		diff = RTWN_SIGN4TO8(MS(txpwr->ht20_ofdm_tx_pwr_diff,
		    R92E_ROM_TXPWR_HT20_DIFF));
		htpow += diff;
		if (sc->ntxchains > 1) {
			diff = RTWN_SIGN4TO8(MS(
			    txpwr->pwr_diff[0].ht40_ht20_tx_pwr_diff,
			    R92E_ROM_TXPWR_HT20_2S_DIFF));
			htpow2s = htpow + diff;
		}
	} else if (sc->ntxchains > 1) {
		/* HT40 MCS8-15 must not use the uninitialized zero base.
		 * The ROM high nibble is the 2-stream HT40 differential. */
		diff = RTWN_SIGN4TO8(MS(
		    txpwr->pwr_diff[0].ht40_ht20_tx_pwr_diff,
		    R92E_ROM_TXPWR_HT40_DIFF));
		htpow2s = htpow + diff;
	}

	for (ridx = RTWN_RIDX_MCS0; ridx <= RTWN_RIDX_MCS15; ridx++) {
		int p = (ridx < RTWN_RIDX_MCS8) ? htpow : htpow2s;
		power[ridx] = p < 0 ? 0 : p;
		if (power[ridx] > R92C_MAX_TX_PWR)
			power[ridx] = R92C_MAX_TX_PWR;
	}
}

void
rtwn_r88e_get_txpower(struct rtwn_softc *sc, int chain,
    struct ieee80211_channel *c, struct ieee80211_channel *extc,
    uint16_t power[RTWN_POWER_COUNT])
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct r88e_rom *rom = &sc->sc_r88e_rom;
	uint8_t cckpow, htpow, ofdmpow;
	int8_t diff;
	int ridx, chan, group;

	/* Determine channel group. */
	chan = ieee80211_chan2ieee(ic, c);	/* XXX center freq! */
	if (sc->chip & RTWN_CHIP_88F)
		group = rtwn_chan2group(chan);
	else {
		if (chan <= 2)
			group = 0;
		else if (chan <= 5)
			group = 1;
		else if (chan <= 8)
			group = 2;
		else if (chan <= 11)
			group = 3;
		else if (chan <= 13)
			group = 4;
		else
			group = 5;
	}

	memset(power, 0, RTWN_POWER_COUNT * sizeof(power[0]));

	/* Compute per-CCK rate Tx power. */
	cckpow = rom->txpwr.cck_tx_pwr[group];
	for (ridx = RTWN_RIDX_CCK1; ridx <= RTWN_RIDX_CCK11; ridx++) {
		if (sc->chip & RTWN_CHIP_88F)
			power[ridx] = cckpow;
		else
			power[ridx] = (ridx == RTWN_RIDX_CCK2) ?
			    cckpow - 9 : cckpow;
		if (power[ridx] > R92C_MAX_TX_PWR)
			power[ridx] = R92C_MAX_TX_PWR;
	}

	if (sc->chip & RTWN_CHIP_88F)
		htpow = rom->txpwr.ht40_tx_pwr[group];
	else
		htpow = (group == 5) ? rom->txpwr.ht40_tx_pwr[group - 1] :
		     rom->txpwr.ht40_tx_pwr[group];

	/* Compute per-OFDM rate Tx power. */
	diff = RTWN_SIGN4TO8(MS(rom->txpwr.ht20_ofdm_tx_pwr_diff,
	    R88E_ROM_TXPWR_OFDM_DIFF));
	ofdmpow = htpow + diff;
	for (ridx = RTWN_RIDX_OFDM6; ridx <= RTWN_RIDX_OFDM54; ridx++) {
		power[ridx] = ofdmpow;
		if (power[ridx] > R92C_MAX_TX_PWR)
			power[ridx] = R92C_MAX_TX_PWR;
	}

	/* Compute per-MCS Tx power. */
	if (extc == NULL) {
		diff = RTWN_SIGN4TO8(MS(rom->txpwr.ht20_ofdm_tx_pwr_diff,
		    R88E_ROM_TXPWR_HT20_DIFF));
		htpow += diff;
	}
	for (ridx = RTWN_RIDX_MCS0; ridx < RTWN_RIDX_MCS8; ridx++) {
		power[ridx] = htpow;
		if (power[ridx] > R92C_MAX_TX_PWR)
			power[ridx] = R92C_MAX_TX_PWR;
	}
}

void
rtwn_set_txpower(struct rtwn_softc *sc, struct ieee80211_channel *c,
    struct ieee80211_channel *extc)
{
	uint16_t power[RTWN_POWER_COUNT];
	int i;

	for (i = 0; i < sc->ntxchains; i++) {
		/* Compute per-rate Tx power values. */
		rtwn_get_txpower(sc, i, c, extc, power);
		/* Write per-rate Tx power values to hardware. */
		rtwn_write_txpower(sc, i, power);
	}
}

void
rtwn_set_chan(struct rtwn_softc *sc, struct ieee80211_channel *c,
    struct ieee80211_channel *extc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	u_int chan;
	uint32_t reg;
	int i;

	chan = ieee80211_chan2ieee(ic, c);	/* Primary channel, not RF center. */
	/* No user-supplied extension may bypass the negotiated AP direction or
	 * the conservative channel range. Invalid parameters fall back to HT20. */
	if (sc->chip & RTWN_CHIP_92E) {
		int secondary = extc ? ieee80211_chan2ieee(ic, extc) : 0;
		if (chan < 1 || chan > 11 || secondary < 1 || secondary > 11 ||
		    (secondary != (int)chan + 4 &&
		     secondary != (int)chan - 4) ||
		    !(c->ic_flags & (secondary > (int)chan ?
		        IEEE80211_CHAN_HT40U : IEEE80211_CHAN_HT40D)))
			extc = NULL;
	}

	/* Set Tx power for this new channel. */
	rtwn_set_txpower(sc, c, extc);

	if (extc != NULL) {
		/* Is secondary channel below or above primary? */
		int prichlo = c->ic_freq < extc->ic_freq;

		if (sc->chip & RTWN_CHIP_92E) {
			reg = rtwn_read_2(sc, R92C_WMAC_TRXPTCL_CTL);
			reg &= ~R92C_WMAC_TRXPTCL_CTL_BW_MASK;
			reg |= R92C_WMAC_TRXPTCL_CTL_BW_40;
			rtwn_write_2(sc, R92C_WMAC_TRXPTCL_CTL, reg);
			/* Gen2 Linux: secondary above -> subchannel 2;
			 * secondary below -> subchannel 1.  Zero means HT20. */
			rtwn_write_1(sc, R92E_DATA_SC, prichlo ? 2 : 1);
		} else {
			rtwn_write_1(sc, R92C_BWOPMODE,
			    rtwn_read_1(sc, R92C_BWOPMODE) &
			    ~R92C_BWOPMODE_20MHZ);
		}

		/* The gen2 RTL8192EU configures 40MHz sideband in BB/FPGA,
		 * not the legacy gen1 response-rate sideband field. */
		if (!(sc->chip & RTWN_CHIP_92E)) {
			reg = rtwn_read_1(sc, R92C_RRSR + 2);
			reg = (reg & ~0x6f) | (prichlo ? 1 : 2) << 5;
			rtwn_write_1(sc, R92C_RRSR + 2, reg);
		}

		rtwn_bb_write(sc, R92C_FPGA0_RFMOD,
		    rtwn_bb_read(sc, R92C_FPGA0_RFMOD) | R92C_RFMOD_40MHZ);
		rtwn_bb_write(sc, R92C_FPGA1_RFMOD,
		    rtwn_bb_read(sc, R92C_FPGA1_RFMOD) | R92C_RFMOD_40MHZ);

		/* Set CCK side band. */
		reg = rtwn_bb_read(sc, R92C_CCK0_SYSTEM);
		reg = (reg & ~0x00000010) | (prichlo ? 0 : 1) << 4;
		rtwn_bb_write(sc, R92C_CCK0_SYSTEM, reg);

		reg = rtwn_bb_read(sc, R92C_OFDM1_LSTF);
		reg = (reg & ~0x00000c00) | (prichlo ? 1 : 2) << 10;
		rtwn_bb_write(sc, R92C_OFDM1_LSTF, reg);

		if (!(sc->chip & (RTWN_CHIP_88E | RTWN_CHIP_92E))) {
			rtwn_bb_write(sc, R92C_FPGA0_ANAPARAM2,
			    rtwn_bb_read(sc, R92C_FPGA0_ANAPARAM2) &
			    ~R92C_FPGA0_ANAPARAM2_CBW20);
		}

		reg = rtwn_bb_read(sc, 0x818);
		reg = (reg & ~0x0c000000) | (prichlo ? 2 : 1) << 26;
		rtwn_bb_write(sc, 0x818, reg);

		/* Generation-2 RF tunes the 40 MHz *center* channel (Linux
		 * rtl8xxxu_gen2_config_channel), two channel numbers from primary. */
		for (i = 0; i < sc->nrxchains; i++) {
			u_int rfchan = (sc->chip & RTWN_CHIP_92E) ?
			    (prichlo ? chan + 2 : chan - 2) : chan;
			rtwn_rf_write(sc, i, R92C_RF_CHNLBW,
			    (sc->rf_chnlbw[i] & ~0xfff) | rfchan |
			    ((sc->chip & RTWN_CHIP_92E) ?
			    R92E_RF_CHNLBW_BW40 : 0));
		}
	} else {
		if (sc->chip & RTWN_CHIP_92E) {
			reg = rtwn_read_2(sc, R92C_WMAC_TRXPTCL_CTL);
			reg &= ~R92C_WMAC_TRXPTCL_CTL_BW_MASK;
			rtwn_write_2(sc, R92C_WMAC_TRXPTCL_CTL, reg);
			rtwn_write_1(sc, R92E_DATA_SC, 0);
		} else if (!(sc->chip & RTWN_CHIP_88F)) {
			rtwn_write_1(sc, R92C_BWOPMODE,
			    rtwn_read_1(sc, R92C_BWOPMODE) |
			    R92C_BWOPMODE_20MHZ);
		}

		rtwn_bb_write(sc, R92C_FPGA0_RFMOD,
		    rtwn_bb_read(sc, R92C_FPGA0_RFMOD) & ~R92C_RFMOD_40MHZ);
		rtwn_bb_write(sc, R92C_FPGA1_RFMOD,
		    rtwn_bb_read(sc, R92C_FPGA1_RFMOD) & ~R92C_RFMOD_40MHZ);

		if (!(sc->chip &
		    (RTWN_CHIP_88E | RTWN_CHIP_88F | RTWN_CHIP_92E))) {
			rtwn_bb_write(sc, R92C_FPGA0_ANAPARAM2,
			    rtwn_bb_read(sc, R92C_FPGA0_ANAPARAM2) |
			    R92C_FPGA0_ANAPARAM2_CBW20);
		} else if (sc->chip & (RTWN_CHIP_88F | RTWN_CHIP_92E)) {
			if (sc->chip & RTWN_CHIP_88F) {
				reg = rtwn_bb_read(sc, R92C_FPGA0_RFMOD);
				reg = (reg & ~0x00000700) | 0x7 << 8;
				rtwn_bb_write(sc, R92C_FPGA0_RFMOD, reg);
				reg = rtwn_bb_read(sc, R92C_FPGA0_RFMOD);
				reg = (reg & ~0x00007000) | 0x5 << 12;
				rtwn_bb_write(sc, R92C_FPGA0_RFMOD, reg);
			}

			reg = rtwn_bb_read(sc, R92C_OFDM0_TX_PSDO_NOISE_WEIGHT);
			reg &= ~0xc0000000;
			rtwn_bb_write(sc, R92C_OFDM0_TX_PSDO_NOISE_WEIGHT, reg);

			if (sc->chip & RTWN_CHIP_88F) {
				/* Small bandwidth */
				reg = rtwn_bb_read(sc,
				    R92C_OFDM0_TX_PSDO_NOISE_WEIGHT);
				reg |= 0x30000000;
				rtwn_bb_write(sc,
				    R92C_OFDM0_TX_PSDO_NOISE_WEIGHT, reg);
				/* ADC buffer clk */
				rtwn_bb_write(sc, R92C_OFDM0_RXAFE,
				    rtwn_bb_read(sc, R92C_OFDM0_RXAFE) |
				    0x30000000);
				/* OFDM Rx DFIR */
				rtwn_bb_write(sc, R88F_RX_DFIR,
				    rtwn_bb_read(sc, R88F_RX_DFIR) &
				    ~0x00080000);
				reg = rtwn_bb_read(sc, R88F_RX_DFIR);
				reg = (reg & ~0x00f00000) | 0x3 << 15;
				rtwn_bb_write(sc, R88F_RX_DFIR, reg);
			}
		}

		/* Select 20MHz bandwidth. */
		for (i = 0; i < sc->nrxchains; i++) {
			rtwn_rf_write(sc, i, R92C_RF_CHNLBW,
			    (sc->rf_chnlbw[i] & ~0xfff) | chan |
			    ((sc->chip &
			    (RTWN_CHIP_88E | RTWN_CHIP_88F | RTWN_CHIP_92E)) ?
			    R88E_RF_CHNLBW_BW20 : R92C_RF_CHNLBW_BW20));

			if (sc->chip & RTWN_CHIP_88F) {
				rtwn_rf_write(sc, i, 0x87, 0x65);
				rtwn_rf_write(sc, i, 0x1c, 0);
				rtwn_rf_write(sc, i, 0xdf, 0x0140);
				rtwn_rf_write(sc, i, 0x1b, 0x1c6c);
			}
		}
	}

	if (sc->chip & RTWN_CHIP_92E)
		sc->channel_bw40 = (extc != NULL);
	if (sc->chip == (RTWN_CHIP_88E | RTWN_CHIP_PCI))
		rtwn_delay(sc, 25000);
}

int
rtwn_chan2group(int chan)
{
	int group;

	if (chan <= 2)
		group = 0;
	else if (chan <= 5)
		group = 1;
	else if (chan <= 8)
		group = 2;
	else if (chan <= 11)
		group = 3;
	else
		group = 4;

	return (group);
}

int
rtwn_iq_calib_chain(struct rtwn_softc *sc, int chain, uint16_t tx[2],
    uint16_t rx[2])
{
	uint32_t status;
	int offset = chain * 0x20;
	uint32_t iqk_tone_92c[] = {
		0x10008c1f, 0x10008c1f, 0x82140102, 0x28160202, 0x10008c22
	};
	uint32_t iqk_tone_92e[] = {
		0x18008c1c, 0x38008c1c, 0x82140303, 0x68160000, 0x38008c1c
	};
	uint32_t *iqk_tone;

	if (sc->chip & RTWN_CHIP_92E)
		iqk_tone = iqk_tone_92e;
	else
		iqk_tone = iqk_tone_92c;

	if (chain == 0) {	/* IQ calibration for chain 0. */
		/* IQ calibration settings for chain 0. */
		rtwn_bb_write(sc, R92C_TX_IQK_TONE_A, iqk_tone[0]);
		/* Match TX/RX tones on path A before its IQK one-shot. */
		rtwn_bb_write(sc, R92C_RX_IQK_TONE_A, iqk_tone[1]);
		rtwn_bb_write(sc, R92C_TX_IQK_PI_A, iqk_tone[2]);

		if (sc->ntxchains > 1) {
			rtwn_bb_write(sc, R92C_RX_IQK_PI_A, iqk_tone[3]);
			/* IQ calibration settings for chain 1. */
			rtwn_bb_write(sc, R92C_TX_IQK_TONE_B, iqk_tone[4]);
			rtwn_bb_write(sc, R92C_RX_IQK_TONE_B, iqk_tone[4]);
			rtwn_bb_write(sc, R92C_TX_IQK_PI_B, 0x82140102);
			rtwn_bb_write(sc, R92C_RX_IQK_PI_B, 0x28160202);
		} else
			rtwn_bb_write(sc, R92C_RX_IQK_PI_A, 0x28160502);

		/* LO calibration settings. */
		if (sc->chip & (RTWN_CHIP_88E | RTWN_CHIP_92E))
			rtwn_bb_write(sc, R92C_IQK_AGC_RSP, 0x00462911);
		else
			rtwn_bb_write(sc, R92C_IQK_AGC_RSP, 0x001028d1);
		/* We're doing LO and IQ calibration in one shot. */
		rtwn_bb_write(sc, R92C_IQK_AGC_PTS, 0xf9000000);
		rtwn_bb_write(sc, R92C_IQK_AGC_PTS, 0xf8000000);

	} else {		/* IQ calibration for chain 1. */
		/* We're doing LO and IQ calibration in one shot. */
		rtwn_bb_write(sc, R92C_IQK_AGC_CONT, 0x00000002);
		rtwn_bb_write(sc, R92C_IQK_AGC_CONT, 0x00000000);
	}

	/* Give LO and IQ calibrations the time to complete. */
	rtwn_delay(sc, 1000);

	/* Read IQ calibration status. */
	status = rtwn_bb_read(sc, 0xeac);

	if (status & (1 << (28 + chain * 3)))
		return (0);	/* Tx failed. */
	/* Read Tx IQ calibration results. */
	tx[0] = (rtwn_bb_read(sc, R92C_TX_POWER_BEFORE_IQK_A + offset) >> 16)
	    & 0x3ff;
	tx[1] = (rtwn_bb_read(sc, R92C_TX_POWER_AFTER_IQK_A + offset) >> 16)
	    & 0x3ff;
	if (tx[0] == 0x142 || tx[1] == 0x042)
		return (0);	/* Tx failed. */

	if (status & (1 << (27 + chain * 3)))
		return (1);	/* Rx failed. */
	/* Read Rx IQ calibration results. */
	rx[0] = (rtwn_bb_read(sc, R92C_RX_POWER_BEFORE_IQK_A_2 + offset) >> 16)
	    & 0x3ff;
	rx[1] = (rtwn_bb_read(sc, R92C_RX_POWER_AFTER_IQK_A_2 + offset) >> 16)
	    & 0x3ff;
	if (rx[0] == 0x132 || rx[1] == 0x036)
		return (1);	/* Rx failed. */

	return (3);	/* Both Tx and Rx succeeded. */
}

void
rtwn_iq_calib_run(struct rtwn_softc *sc, int n, uint16_t tx[2][2],
    uint16_t rx[2][2], struct rtwn_iq_cal_regs *iq_cal_regs)
{
	static const uint16_t reg_adda[16] = {
		0x85c, 0xe6c, 0xe70, 0xe74,
		0xe78, 0xe7c, 0xe80, 0xe84,
		0xe88, 0xe8c, 0xed0, 0xed4,
		0xed8, 0xedc, 0xee0, 0xeec
	};
	static const uint32_t adda_92c[] = {
		0x0b1b25a0, 0x0bdb25a0, 0x04db25a4, 0x0b1b25a4
	};
	static const uint32_t adda_92e[] = {
		0x0fc01616, 0x0fc01616, 0x0fc01616, 0x0fc01616
	};
	const uint32_t *adda_vals;

	int i, chain;
	uint32_t hssi_param1 = 0, reg;
	uint8_t xa_agc, xb_agc;

	xa_agc = rtwn_bb_read(sc, R92C_OFDM0_AGCCORE1(0)) & 0xff;
	xb_agc = rtwn_bb_read(sc, R92C_OFDM0_AGCCORE1(1)) & 0xff;

	if (sc->chip & RTWN_CHIP_92E)
		adda_vals = adda_92e;
	else
		adda_vals = adda_92c;

	if (n == 0) {
		for (i = 0; i < nitems(reg_adda); i++)
			iq_cal_regs->adda[i] = rtwn_bb_read(sc, reg_adda[i]);

		iq_cal_regs->txpause = rtwn_read_1(sc, R92C_TXPAUSE);
		iq_cal_regs->bcn_ctrl = rtwn_read_1(sc, R92C_BCN_CTRL);
		iq_cal_regs->bcn_ctrl1 = rtwn_read_1(sc, R92C_BCN_CTRL1);
		iq_cal_regs->gpio_muxcfg = rtwn_read_4(sc, R92C_GPIO_MUXCFG);
	}

	if (sc->ntxchains == 1) {
		rtwn_bb_write(sc, reg_adda[0], adda_vals[0]);
		for (i = 1; i < nitems(reg_adda); i++)
			rtwn_bb_write(sc, reg_adda[i], adda_vals[1]);
	} else {
		for (i = 0; i < nitems(reg_adda); i++)
			rtwn_bb_write(sc, reg_adda[i], adda_vals[2]);
	}

	if (n == 0) {
		iq_cal_regs->ofdm0_trxpathena =
		    rtwn_bb_read(sc, R92C_OFDM0_TRXPATHENA);
		iq_cal_regs->ofdm0_trmuxpar =
		    rtwn_bb_read(sc, R92C_OFDM0_TRMUXPAR);
		iq_cal_regs->fpga0_rfifacesw0 =
		    rtwn_bb_read(sc, R92C_FPGA0_RFIFACESW(0));
		iq_cal_regs->fpga0_rfifacesw1 =
		    rtwn_bb_read(sc, R92C_FPGA0_RFIFACESW(1));
		iq_cal_regs->fpga0_rfifaceoe0 =
		    rtwn_bb_read(sc, R92C_FPGA0_RFIFACEOE(0));
		iq_cal_regs->fpga0_rfifaceoe1 =
		    rtwn_bb_read(sc, R92C_FPGA0_RFIFACEOE(1));
		iq_cal_regs->config_ant_a =
		    rtwn_bb_read(sc, R92C_CONFIG_ANT_A);
		iq_cal_regs->config_ant_b =
		    rtwn_bb_read(sc, R92C_CONFIG_ANT_B);
		iq_cal_regs->cck0_afesetting =
		    rtwn_bb_read(sc, R92C_CCK0_AFESETTING);
	}

	if (sc->chip & RTWN_CHIP_92E) {
		rtwn_write_4(sc, R92C_CCK0_AFESETTING, rtwn_read_4(sc,
		    R92C_CCK0_AFESETTING) | 0x0f000000);
	} else {
		hssi_param1 = rtwn_bb_read(sc, R92C_HSSI_PARAM1(0));
		if (!(hssi_param1 & R92C_HSSI_PARAM1_PI)) {
			rtwn_bb_write(sc, R92C_HSSI_PARAM1(0),
			    hssi_param1 | R92C_HSSI_PARAM1_PI);
			rtwn_bb_write(sc, R92C_HSSI_PARAM1(1),
			    hssi_param1 | R92C_HSSI_PARAM1_PI);
		}
	}

	rtwn_bb_write(sc, R92C_OFDM0_TRXPATHENA, 0x03a05600);
	rtwn_bb_write(sc, R92C_OFDM0_TRMUXPAR, 0x000800e4);

	if (sc->chip & RTWN_CHIP_92E) {
		rtwn_bb_write(sc, R92C_FPGA0_RFIFACESW(1), 0x22208200);
		rtwn_bb_write(sc, R92C_FPGA0_RFIFACESW(0),
		    rtwn_bb_read(sc, R92C_FPGA0_RFIFACESW(0)) | (1 << 10) |
		    (1 << 26));

		rtwn_bb_write(sc, R92C_FPGA0_RFIFACEOE(0), rtwn_bb_read(sc,
		    R92C_FPGA0_RFIFACEOE(0)) | (1 << 10));
		rtwn_bb_write(sc, R92C_FPGA0_RFIFACEOE(1), rtwn_bb_read(sc,
		    R92C_FPGA0_RFIFACEOE(1)) | (1 << 10));
	} else {
		rtwn_bb_write(sc, R92C_FPGA0_RFIFACESW(1), 0x22204000);

		if (sc->ntxchains > 1) {
			rtwn_bb_write(sc, R92C_LSSI_PARAM(0), 0x00010000);
			rtwn_bb_write(sc, R92C_LSSI_PARAM(1), 0x00010000);
		}

		rtwn_write_1(sc, R92C_TXPAUSE, R92C_TXPAUSE_AC_VO |
		    R92C_TXPAUSE_AC_VI | R92C_TXPAUSE_AC_BE |
		    R92C_TXPAUSE_AC_BK | R92C_TXPAUSE_MGNT |
		    R92C_TXPAUSE_HIGH);
	}
	rtwn_write_1(sc, R92C_BCN_CTRL,
	    iq_cal_regs->bcn_ctrl & ~(R92C_BCN_CTRL_EN_BCN));
	rtwn_write_1(sc, R92C_BCN_CTRL1,
	    iq_cal_regs->bcn_ctrl1 & ~(R92C_BCN_CTRL_EN_BCN));
	rtwn_write_1(sc, R92C_GPIO_MUXCFG,
	    iq_cal_regs->gpio_muxcfg & ~(R92C_GPIO_MUXCFG_ENBT));

	rtwn_bb_write(sc, R92C_CONFIG_ANT_A, 0x00080000);
	if (sc->ntxchains > 1)
		rtwn_bb_write(sc, R92C_CONFIG_ANT_B, 0x00080000);

	rtwn_bb_write(sc, R92C_FPGA0_IQK, 0x80800000);
	rtwn_bb_write(sc, R92C_TX_IQK, 0x01007c00);
	rtwn_bb_write(sc, R92C_RX_IQK, 0x01004800);

	rtwn_bb_write(sc, R92C_CONFIG_ANT_A, 0x00080000);

	for (chain = 0; chain < sc->ntxchains; chain++) {
		if (chain > 0) {
			/* Put chain 0 on standby. */
			rtwn_bb_write(sc, R92C_FPGA0_IQK, 0x00);
			rtwn_bb_write(sc, R92C_LSSI_PARAM(0), 0x00010000);
			rtwn_bb_write(sc, R92C_FPGA0_IQK, 0x80800000);

			/* Enable chain 1. */
			for (i = 0; i < nitems(reg_adda); i++)
				rtwn_bb_write(sc, reg_adda[i], adda_vals[3]);
		}

		/* Run IQ calibration twice. */
		for (i = 0; i < 2; i++) {
			int ret;

			ret = rtwn_iq_calib_chain(sc, chain,
			    tx[chain], rx[chain]);
			if (ret == 0) {
				DPRINTF(("%s: chain %d: Tx failed.\n",
				    __func__, chain));
				tx[chain][0] = 0xff;
				tx[chain][1] = 0xff;
				rx[chain][0] = 0xff;
				rx[chain][1] = 0xff;
			} else if (ret == 1) {
				DPRINTF(("%s: chain %d: Rx failed.\n",
				    __func__, chain));
				rx[chain][0] = 0xff;
				rx[chain][1] = 0xff;
			} else if (ret == 3) {
				DPRINTF(("%s: chain %d: Both Tx and Rx "
				    "succeeded.\n", __func__, chain));
			}
		}

		DPRINTF(("%s: results for run %d chain %d: tx[0]=0x%x, "
		    "tx[1]=0x%x rx[0]=0x%x rx[1]=0x%x\n", __func__, n, chain,
		    tx[chain][0], tx[chain][1], rx[chain][0], rx[chain][1]));
	}

	rtwn_bb_write(sc, R92C_FPGA0_IQK, 0x00);

	if (!(sc->chip & RTWN_CHIP_92E)) {
		rtwn_bb_write(sc, R92C_LSSI_PARAM(0), 0x00032ed3);
		if (sc->ntxchains > 1)
			rtwn_bb_write(sc, R92C_LSSI_PARAM(1), 0x00032ed3);
	}

	if (n != 0) {
		if (!(sc->chip & RTWN_CHIP_92E)) {
			if (!(hssi_param1 & R92C_HSSI_PARAM1_PI)) {
				rtwn_bb_write(sc, R92C_HSSI_PARAM1(0),
				    hssi_param1);
				rtwn_bb_write(sc, R92C_HSSI_PARAM1(1),
				    hssi_param1);
			}
		}

		for (i = 0; i < nitems(reg_adda); i++)
			rtwn_bb_write(sc, reg_adda[i], iq_cal_regs->adda[i]);

		rtwn_write_1(sc, R92C_TXPAUSE, iq_cal_regs->txpause);
		rtwn_write_1(sc, R92C_BCN_CTRL, iq_cal_regs->bcn_ctrl);
		rtwn_write_1(sc, R92C_BCN_CTRL1, iq_cal_regs->bcn_ctrl1);
		rtwn_write_4(sc, R92C_GPIO_MUXCFG, iq_cal_regs->gpio_muxcfg);

		rtwn_bb_write(sc, R92C_OFDM0_TRXPATHENA,
		    iq_cal_regs->ofdm0_trxpathena);
		rtwn_bb_write(sc, R92C_FPGA0_RFIFACESW(0),
		    iq_cal_regs->fpga0_rfifacesw0);
		rtwn_bb_write(sc, R92C_FPGA0_RFIFACESW(1),
		    iq_cal_regs->fpga0_rfifacesw1);
		rtwn_bb_write(sc, R92C_FPGA0_RFIFACEOE(0),
		    iq_cal_regs->fpga0_rfifaceoe0);
		rtwn_bb_write(sc, R92C_FPGA0_RFIFACEOE(1),
		    iq_cal_regs->fpga0_rfifaceoe1);
		rtwn_bb_write(sc, R92C_OFDM0_TRMUXPAR,
		    iq_cal_regs->ofdm0_trmuxpar);
		rtwn_bb_write(sc, R92C_CONFIG_ANT_A,
		    iq_cal_regs->config_ant_a);
		rtwn_bb_write(sc, R92C_CONFIG_ANT_B,
		    iq_cal_regs->config_ant_b);
		rtwn_bb_write(sc, R92C_CCK0_AFESETTING,
		    iq_cal_regs->cck0_afesetting);

		reg = rtwn_bb_read(sc, R92C_OFDM0_AGCCORE1(0));
		reg &= ~0xff;
		rtwn_bb_write(sc, R92C_OFDM0_AGCCORE1(0), reg | 0x50);
		rtwn_bb_write(sc, R92C_OFDM0_AGCCORE1(0), reg | xa_agc);

		reg = rtwn_bb_read(sc, R92C_OFDM0_AGCCORE1(1));
		reg &= ~0xff;
		rtwn_bb_write(sc, R92C_OFDM0_AGCCORE1(1), reg | 0x50);
		rtwn_bb_write(sc, R92C_OFDM0_AGCCORE1(1), reg | xb_agc);

		rtwn_bb_write(sc, R92C_TX_IQK_TONE_A, 0x01008c00);
		rtwn_bb_write(sc, R92C_RX_IQK_TONE_A, 0x01008c00);
	}
}

#define RTWN_IQ_CAL_MAX_TOLERANCE 5
int
rtwn_iq_calib_compare_results(uint16_t tx1[2][2], uint16_t rx1[2][2],
    uint16_t tx2[2][2], uint16_t rx2[2][2], int ntxchains)
{
	int chain, i, tx_ok[2], rx_ok[2];

	tx_ok[0] = tx_ok[1] = rx_ok[0] = rx_ok[1] = 0;
	for (chain = 0; chain < ntxchains; chain++) {
		tx_ok[chain] = rx_ok[chain] = 1;
		for (i = 0; i < 2; i++)	{
			if (tx1[chain][i] == 0xff || tx2[chain][i] == 0xff ||
			    rx1[chain][i] == 0xff || rx2[chain][i] == 0xff) {
				tx_ok[chain] = rx_ok[chain] = 0;
				continue;
			}

			/* Both coefficients must be valid and stable. A later
			 * coefficient must never erase an earlier failure. */
			tx_ok[chain] &= (abs(tx1[chain][i] - tx2[chain][i]) <=
			    RTWN_IQ_CAL_MAX_TOLERANCE);

			rx_ok[chain] &= (abs(rx1[chain][i] - rx2[chain][i]) <=
			    RTWN_IQ_CAL_MAX_TOLERANCE);
		}
	}

	if (ntxchains > 1)
		return (tx_ok[0] && tx_ok[1] && rx_ok[0] && rx_ok[1]);
	else
		return (tx_ok[0] && rx_ok[0]);
}
#undef RTWN_IQ_CAL_MAX_TOLERANCE

void
rtwn_iq_calib_write_results(struct rtwn_softc *sc, uint16_t tx[2],
    uint16_t rx[2], int chain)
{
	uint32_t reg, val, x;
	long y, tx_c;
	/* Linux/Realtek: path A uses ECCA bits 31/29; path B uses 27/25. */
	const uint32_t tx_x_bit = 1U << (31 - chain * 4);
	const uint32_t tx_y_bit = 1U << (29 - chain * 4);
	/* The 92E backend tracks missing measurements outside the ten-bit
	 * coefficient range; 0x00ff is a legitimate measured value. */
	const uint16_t invalid = (sc->chip & RTWN_CHIP_92E) ? 0xffff : 0xff;

	if (tx[0] == invalid || tx[1] == invalid)
		return;

	reg = rtwn_bb_read(sc, R92C_OFDM0_TXIQIMBALANCE(chain));
	val = ((reg >> 22) & 0x3ff);
	x = tx[0];
	if (x & 0x00000200)
		x |= 0xfffffc00;
	reg &= ~0x3ff;
	reg |= (((x * val) >> 8) & 0x3ff);
	rtwn_bb_write(sc, R92C_OFDM0_TXIQIMBALANCE(chain), reg);

	reg = rtwn_bb_read(sc, R92C_OFDM0_ECCATHRESHOLD);
	if (((x * val) >> 7) & 0x01)
		reg |= tx_x_bit;
	else
		reg &= ~tx_x_bit;
	rtwn_bb_write(sc, R92C_OFDM0_ECCATHRESHOLD, reg);

	y = tx[1];
	if (y & 0x00000200)
		y |= 0xfffffc00;
	tx_c = (y * val) >> 8;
	reg = rtwn_bb_read(sc, R92C_OFDM0_TXAFE(chain));
	reg &= ~0xf0000000;
	reg |= ((tx_c & 0x3c0) << 22);
	rtwn_bb_write(sc, R92C_OFDM0_TXAFE(chain), reg);

	reg = rtwn_bb_read(sc, R92C_OFDM0_TXIQIMBALANCE(chain));
	reg &= ~0x003f0000;
	reg |= ((tx_c & 0x3f) << 16);
	rtwn_bb_write(sc, R92C_OFDM0_TXIQIMBALANCE(chain), reg);

	reg = rtwn_bb_read(sc, R92C_OFDM0_ECCATHRESHOLD);
	if (((y * val) >> 7) & 0x01)
		reg |= tx_y_bit;
	else
		reg &= ~tx_y_bit;
	rtwn_bb_write(sc, R92C_OFDM0_ECCATHRESHOLD, reg);

	if (rx[0] == invalid || rx[1] == invalid)
		return;

	reg = rtwn_bb_read(sc, R92C_OFDM0_RXIQIMBALANCE(chain));
	reg &= ~0x3ff;
	reg |= (rx[0] & 0x3ff);
	rtwn_bb_write(sc, R92C_OFDM0_RXIQIMBALANCE(chain), reg);

	reg &= ~0xfc00;
	reg |= ((rx[1] & 0x03f) << 10);
	rtwn_bb_write(sc, R92C_OFDM0_RXIQIMBALANCE(chain), reg);

	if (chain == 0) {
		reg = rtwn_bb_read(sc, R92C_OFDM0_RXIQEXTANTA);
		reg &= ~0xf0000000;
		reg |= ((rx[1] & 0x3c0) << 22);
		rtwn_bb_write(sc, R92C_OFDM0_RXIQEXTANTA, reg);
	} else {
		reg = rtwn_bb_read(sc, R92C_OFDM0_AGCRSSITABLE);
		reg &= ~0xf000;
		reg |= ((rx[1] & 0x3c0) << 6);
		rtwn_bb_write(sc, R92C_OFDM0_AGCRSSITABLE, reg);
	}
}

#include "r92e_iqk.inc"

#define RTWN_IQ_CAL_NRUN	3
void
rtwn_iq_calib(struct rtwn_softc *sc)
{
	if (sc->chip & RTWN_CHIP_92E) {
		rtwn_r92e_iq_calib(sc);
		return;
	}
	uint16_t tx[RTWN_IQ_CAL_NRUN][2][2], rx[RTWN_IQ_CAL_NRUN][2][2];
	int n, valid;
	struct rtwn_iq_cal_regs regs;

	valid = 0;
	memset(&regs, 0, sizeof(regs));
	for (n = 0; n < RTWN_IQ_CAL_NRUN; n++) {
		rtwn_iq_calib_run(sc, n, tx[n], rx[n], &regs);

		if (n == 0)
			continue;

		/* Valid results remain stable after consecutive runs. */
		valid = rtwn_iq_calib_compare_results(tx[n - 1], rx[n - 1],
		    tx[n], rx[n], sc->ntxchains);
		if (valid)
			break;
	}

	sc->iq_calibration_mask = valid ? (uint8_t)((1U << sc->ntxchains) - 1U) : 0;
	if (valid) {
		rtwn_iq_calib_write_results(sc, tx[n][0], rx[n][0], 0);
		if (sc->ntxchains > 1)
			rtwn_iq_calib_write_results(sc, tx[n][1], rx[n][1], 1);
	}
}
#undef RTWN_IQ_CAL_NRUN

void
rtwn_lc_calib(struct rtwn_softc *sc)
{
	uint32_t rf_ac[2];
	uint8_t txmode;
	int i;

	txmode = rtwn_read_1(sc, R92C_OFDM1_LSTF + 3);
	if ((txmode & 0x70) != 0) {
		/* Disable all continuous Tx. */
		rtwn_write_1(sc, R92C_OFDM1_LSTF + 3, txmode & ~0x70);

		/* Set RF mode to standby mode. */
		for (i = 0; i < sc->nrxchains; i++) {
			rf_ac[i] = rtwn_rf_read(sc, i, R92C_RF_AC);
			rtwn_rf_write(sc, i, R92C_RF_AC,
			    RW(rf_ac[i], R92C_RF_AC_MODE,
				R92C_RF_AC_MODE_STANDBY));
		}
	} else {
		/* Block all Tx queues. */
		rtwn_write_1(sc, R92C_TXPAUSE, R92C_TXPAUSE_ALL);
	}
	/* Start calibration. */
	rtwn_rf_write(sc, 0, R92C_RF_CHNLBW,
	    rtwn_rf_read(sc, 0, R92C_RF_CHNLBW) | R92C_RF_CHNLBW_LCSTART);

	/* Wait 100 ms before resuming Tx, as in FreeBSD r92c_lc_calib and
	 * Linux rtl8723a_phy_lc_calibrate (used by RTL8192EU). The OpenBSD
	 * snapshot's 100 us delay does not match either independent source. */
	rtwn_delay(sc, 100000);

	/* Restore configuration. */
	if ((txmode & 0x70) != 0) {
		/* Restore Tx mode. */
		rtwn_write_1(sc, R92C_OFDM1_LSTF + 3, txmode);
		/* Restore RF mode. */
		for (i = 0; i < sc->nrxchains; i++)
			rtwn_rf_write(sc, i, R92C_RF_AC, rf_ac[i]);
	} else {
		/* Unblock all Tx queues. */
		rtwn_write_1(sc, R92C_TXPAUSE, 0x00);
	}
}

void
rtwn_temp_calib(struct rtwn_softc *sc)
{
	int temp, t_meter_reg, t_meter_val, threshold = 1;

	if (sc->chip & RTWN_CHIP_92E) {
		t_meter_reg = R92E_RF_T_METER;
		t_meter_val = 0x37cf8;
		/* RTL8192EU uses the gen2 six-bit meter. FreeBSD r92eu selects
		 * r88e_temp_read and a four-unit calibration threshold. */
		threshold = 4;
	} else {
		t_meter_reg = R92C_RF_T_METER;
		t_meter_val = 0x60;
	}

	if (sc->thcal_state == 0) {
		/* Start measuring temperature. */
		rtwn_rf_write(sc, 0, t_meter_reg, t_meter_val);
		sc->thcal_state = 1;
		return;
	}
	sc->thcal_state = 0;

	/* Read measured temperature. */
	temp = rtwn_rf_read(sc, 0, t_meter_reg);
	if (sc->chip & RTWN_CHIP_92E)
		temp = (temp >> 10) & 0x3f;
	else
		temp &= 0x1f;
	if (temp == 0)	/* Read failed, skip. */
		return;
	DPRINTFN(2, ("temperature=%d\n", temp));

	/*
	 * Redo IQ and LC calibration if temperature changed significantly
	 * since last calibration.
	 */
	if (sc->thcal_lctemp == 0) {
		/* First calibration is performed in rtwn_init(). */
		sc->thcal_lctemp = temp;
	} else if (abs(temp - sc->thcal_lctemp) > threshold) {
		DPRINTF(("IQ/LC calib triggered by temp: %d -> %d\n",
		    sc->thcal_lctemp, temp));
		rtwn_iq_calib(sc);
		rtwn_lc_calib(sc);
		/* Record temperature of last calibration. */
		sc->thcal_lctemp = temp;
	}
}

void
rtwn_enable_intr(struct rtwn_softc *sc)
{
	if (sc->chip & RTWN_CHIP_92E) {
		rtwn_write_4(sc, R88E_HISR, 0xffffffff);
		rtwn_write_4(sc, R88E_HISRE, 0xffffffff);
		rtwn_write_4(sc, R88E_HIMR, 0);
		rtwn_write_4(sc, R88E_HIMRE, 0);
	} else if (sc->chip & RTWN_CHIP_88E) {
		rtwn_write_4(sc, R88E_HISR, 0xffffffff);
		if (sc->chip & RTWN_CHIP_USB) {
			rtwn_write_4(sc, R88E_HIMR, R88E_HIMR_CPWM |
			R88E_HIMR_CPWM2 | R88E_HIMR_TBDER |
			R88E_HIMR_PSTIMEOUT);
			rtwn_write_4(sc, R88E_HIMRE, R88E_HIMRE_RXFOVW |
			    R88E_HIMRE_TXFOVW | R88E_HIMRE_RXERR |
			    R88E_HIMRE_TXERR);
		} else {
			rtwn_write_4(sc, R88E_HIMR,
			    RTWN_88E_INT_ENABLE);
			rtwn_write_4(sc, R88E_HIMRE,
			    R88E_HIMRE_RXFOVW);
			rtwn_write_1(sc, R92C_C2HEVT_CLEAR, 0);
			rtwn_write_4(sc, R92C_HSIMR,
			    R88E_HSIMR_PDN_INT_EN | R88E_HSIMR_RON_INT_EN);
		}

		if (sc->chip & RTWN_CHIP_USB) {
			rtwn_write_1(sc, R92C_USB_SPECIAL_OPTION,
			    rtwn_read_1(sc, R92C_USB_SPECIAL_OPTION) |
			    R92C_USB_SPECIAL_OPTION_INT_BULK_SEL);
		}
	} else {
		uint32_t imask = 0;

		if (sc->chip & RTWN_CHIP_USB)
			imask = 0xffffffff;
		else if (sc->chip & RTWN_CHIP_PCI)
			imask = RTWN_92C_INT_ENABLE;
		else
			panic("unknown chip type 0x%x", sc->chip);

		/* Clear pending interrupts. */
		rtwn_write_4(sc, R92C_HISR, 0xffffffff);

		/* Enable interrupts. */
		rtwn_write_4(sc, R92C_HIMR, imask);
	}
}

void
rtwn_disable_intr(struct rtwn_softc *sc)
{
	if (sc->chip & RTWN_CHIP_88E) {
		rtwn_write_4(sc, R88E_HISR, 0x00000000);
		rtwn_write_4(sc, R88E_HIMR, 0x00000000);
		rtwn_write_4(sc, R88E_HIMRE, 0x00000000);
		if (sc->chip & RTWN_CHIP_USB) {
			rtwn_write_1(sc, R92C_USB_SPECIAL_OPTION,
			    rtwn_read_1(sc, R92C_USB_SPECIAL_OPTION) &
			    ~R92C_USB_SPECIAL_OPTION_INT_BULK_SEL);
		}
	} else {
		rtwn_write_4(sc, R92C_HISR, 0x00000000);
		rtwn_write_4(sc, R92C_HIMR, 0x00000000);
	}
}

int
rtwn_init(struct _ifnet *ifp)
{
	struct rtwn_softc *sc = (struct rtwn_softc *)ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	uint32_t reg;
	int i, error;
	bool hardware_started = false, rx_retry = false;

	/* Init firmware commands ring. */
	sc->fwcur = 0;
	sc->fw_station_connected = 0;
	memset(&sc->rate_config, 0, sizeof(sc->rate_config));
	sc->iq_calibration_mask = 0;

	error = sc->sc_ops.alloc_buffers(sc->sc_ops.cookie);
	if (error)
		goto fail;

	hardware_started = true;
	/* Power on adapter. */
	error = sc->sc_ops.power_on(sc->sc_ops.cookie);
	if (error != 0) {
		printf("%s: could not power on adapter\n",
		    sc->sc_pdev->dv_xname);
		goto fail;
	}

	/* Initialize DMA. */
	error = sc->sc_ops.dma_init(sc->sc_ops.cookie);
	if (error != 0) {
		printf("%s: could not initialize DMA\n",
		    sc->sc_pdev->dv_xname);
		goto fail;
	}

	/* Set info size in Rx descriptors (in 64-bit words). */
	rtwn_write_1(sc, R92C_RX_DRVINFO_SZ, 4);

	if ((sc->chip & RTWN_CHIP_USB) && !(sc->chip & RTWN_CHIP_88F)) {
		/* Init interrupts. */
		rtwn_enable_intr(sc);
	} else if (sc->chip & RTWN_CHIP_PCI) {
		rtwn_disable_intr(sc);
	}

	/* Set MAC address. */
	/* ic_myaddr is initialized from this device's eFuse by rtwn_read_rom(). */
	for (i = 0; i < IEEE80211_ADDR_LEN; i++)
		rtwn_write_1(sc, R92C_MACID + i, ic->ic_myaddr[i]);

	/* Set initial network type. */
	rtwn_set_nettype(sc, IEEE80211_M_MONITOR);

	rtwn_rxfilter_init(sc);

	reg = rtwn_read_4(sc, R92C_RRSR);
	if (sc->chip & RTWN_CHIP_USB) {
		reg = RW(reg, R92C_RRSR_RATE_BITMAP,
		    R92C_RRSR_RATE_CCK_ONLY_1M);
	} else {
		reg = RW(reg, R92C_RRSR_RATE_BITMAP, R92C_RRSR_RATE_ALL);
	}
	rtwn_write_4(sc, R92C_RRSR, reg);

	/* Set short/long retry limits. */
	if (sc->chip & RTWN_CHIP_USB) {
		rtwn_write_2(sc, R92C_RL,
		    SM(R92C_RL_SRL, 0x30) | SM(R92C_RL_LRL, 0x30));
	} else {
		rtwn_write_2(sc, R92C_RL,
		    SM(R92C_RL_SRL, 0x07) | SM(R92C_RL_LRL, 0x07));
	}

	/* Initialize EDCA parameters. */
	rtwn_edca_init(sc);

	/* Set data and response automatic rate fallback retry counts. */
	rtwn_rate_fallback_init(sc);

	if (sc->chip & RTWN_CHIP_USB) {
		rtwn_write_1(sc, R92C_FWHW_TXQ_CTRL,
		    rtwn_read_1(sc, R92C_FWHW_TXQ_CTRL) |
		    R92C_FWHW_TXQ_CTRL_AMPDU_RTY_NEW);
	} else {
		rtwn_write_2(sc, R92C_FWHW_TXQ_CTRL, 0x1f80);
	}

	/* Set ACK timeout. */
	rtwn_write_1(sc, R92C_ACKTO, 0x40);

	/* Setup USB aggregation. */
	if (sc->chip & RTWN_CHIP_USB)
		sc->sc_ops.aggr_init(sc->sc_ops.cookie);

	/* Initialize beacon parameters. */
	rtwn_write_2(sc, R92C_BCN_CTRL,
	    (R92C_BCN_CTRL_DIS_TSF_UDT0 << 8) | R92C_BCN_CTRL_DIS_TSF_UDT0);
	rtwn_write_2(sc, R92C_TBTT_PROHIBIT, 0x6404);
	if (!(sc->chip & RTWN_CHIP_88F))
		rtwn_write_1(sc, R92C_DRVERLYINT, R92C_DRVERLYINT_INIT_TIME);
	rtwn_write_1(sc, R92C_BCNDMATIM, R92C_BCNDMATIM_INIT_TIME);
	rtwn_write_2(sc, R92C_BCNTCFG, 0x660f);

	if (!(sc->chip & (RTWN_CHIP_88E | RTWN_CHIP_88F | RTWN_CHIP_92E))) {
		/* Setup AMPDU aggregation. */
		rtwn_write_4(sc, R92C_AGGLEN_LMT, 0x99997631);	/* MCS7~0 */
		rtwn_write_1(sc, R92C_AGGR_BREAK_TIME, 0x16);
		rtwn_write_2(sc, R92C_MAX_AGGR_NUM, 0x0708);

		rtwn_write_1(sc, R92C_BCN_MAX_ERR, 0xff);
	}

	if (sc->chip & RTWN_CHIP_PCI) {
		/* Reset H2C protection register. */
		rtwn_write_4(sc, R92C_MCUTST_1, 0x0);
	}

	/* Load 8051 microcode. */
	error = rtwn_load_firmware(sc);
	if (error != 0)
		goto fail;

	/* Initialize MAC/BB/RF blocks. */
	sc->sc_ops.mac_init(sc->sc_ops.cookie);
	if ((error = rtwn_io_error(sc)) != 0) goto fail;
	sc->sc_ops.bb_init(sc->sc_ops.cookie);
	if ((error = rtwn_io_error(sc)) != 0) goto fail;
	rtwn_rf_init(sc);
	if ((error = rtwn_io_error(sc)) != 0) goto fail;

	if (sc->chip & (RTWN_CHIP_88E | RTWN_CHIP_88F | RTWN_CHIP_92E)) {
		rtwn_write_2(sc, R92C_CR,
		    rtwn_read_2(sc, R92C_CR) | R92C_CR_MACTXEN |
		    R92C_CR_MACRXEN);
	}

	/* Turn CCK and OFDM blocks on. */
	reg = rtwn_bb_read(sc, R92C_FPGA0_RFMOD);
	reg |= R92C_RFMOD_CCK_EN;
	rtwn_bb_write(sc, R92C_FPGA0_RFMOD, reg);
	reg = rtwn_bb_read(sc, R92C_FPGA0_RFMOD);
	reg |= R92C_RFMOD_OFDM_EN;
	rtwn_bb_write(sc, R92C_FPGA0_RFMOD, reg);

	/* Clear per-station keys table. */
	rtwn_cam_init(sc);

	/* Hardware crypto remains off: net80211 performs CCMP. */
	if (sc->chip & RTWN_CHIP_USB) {
		/* net80211 owns CCMP encryption, MIC and replay verification. */
		rtwn_write_2(sc, R92C_SECCFG, 0);
	}

	/* Enable hardware sequence numbering. */
	rtwn_write_1(sc, R92C_HWSEQ_CTRL, 0xff);

	if (sc->chip & RTWN_CHIP_92E) {
		rtwn_write_1(sc, R92C_QUEUE_CTRL,
		    rtwn_read_1(sc, R92C_QUEUE_CTRL) & ~0x08);
	}

	/* Perform LO and IQ calibrations. */
	rtwn_iq_calib(sc);
	if ((error = rtwn_io_error(sc)) != 0) goto fail;
	/* Perform LC calibration. */
	rtwn_lc_calib(sc);
	if ((error = rtwn_io_error(sc)) != 0) goto fail;

	/* Fix USB interference issue. */
	if (sc->chip & RTWN_CHIP_USB) {
		if (!(sc->chip &
		    (RTWN_CHIP_88E | RTWN_CHIP_88F | RTWN_CHIP_92E))) {
			rtwn_write_1(sc, 0xfe40, 0xe0);
			rtwn_write_1(sc, 0xfe41, 0x8d);
			rtwn_write_1(sc, 0xfe42, 0x80);

			rtwn_pa_bias_init(sc);
		}
	}

	/* Initialize GPIO setting. */
	rtwn_write_1(sc, R92C_GPIO_MUXCFG,
	    rtwn_read_1(sc, R92C_GPIO_MUXCFG) & ~R92C_GPIO_MUXCFG_ENBT);

	/* Fix for lower temperature. */
	if (!(sc->chip & (RTWN_CHIP_88E | RTWN_CHIP_88F | RTWN_CHIP_92E)))
		rtwn_write_1(sc, 0x15, 0xe9);

	/* Set default channel. */
	ic->ic_bss->ni_chan = ic->ic_ibss_chan;
	rtwn_set_chan(sc, ic->ic_ibss_chan, NULL);
	if ((error = rtwn_io_error(sc)) != 0) goto fail;

	if (sc->chip & RTWN_CHIP_PCI)
		rtwn_enable_intr(sc);

	error = sc->sc_ops.init(sc->sc_ops.cookie);
	rx_retry = error == EAGAIN || error == EBUSY;
	if (error)
		goto fail;

	if ((error = rtwn_io_error(sc)) != 0) goto fail;

	/* We're ready to go. */
	ifq_clr_oactive(&ifp->if_snd);
	ifp->if_flags |= IFF_RUNNING;

	if ((ic->ic_flags & IEEE80211_F_WEPON) &&
	    (sc->chip & RTWN_CHIP_USB)) {
		/* Install WEP keys. */
		for (i = 0; i < IEEE80211_WEP_NKID; i++)
			ic->ic_set_key(ic, NULL, &ic->ic_nw_keys[i]);
		sc->sc_ops.wait_async(sc->sc_ops.cookie);
	}

	if (ic->ic_opmode == IEEE80211_M_MONITOR)
		error = ic->ic_newstate(ic, IEEE80211_S_RUN, -1);
	else
		error = ic->ic_newstate(ic, IEEE80211_S_SCAN, -1);
	if (error != 0) goto fail;
	return rtwn_io_error(sc);
fail:
	/* The asynchronous init task cannot return errno to the controller.
	 * Preserve native hardware failures before stop records checked cleanup;
	 * initial RX backpressure remains an explicit, unlatched retry. */
	if (hardware_started && !rx_retry && !sc->core_error)
		sc->core_error = error;
	rtwn_stop(ifp);
	return (error);
}

void
rtwn_init_task(void *arg1)
{
	struct rtwn_softc *sc = (struct rtwn_softc *)arg1;
	struct _ifnet *ifp = &sc->sc_ic.ic_if;
	int s;

	s = splnet();
	if (sc->sc_flags & RTWN_FLAG_BUSY) {
		splx(s);
		return;
	}
	sc->sc_flags |= RTWN_FLAG_BUSY;

	rtwn_stop(ifp);

	if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) == IFF_UP)
		rtwn_init(ifp);

	sc->sc_flags &= ~RTWN_FLAG_BUSY;

	splx(s);
}

void
rtwn_stop(struct _ifnet *ifp)
{
	struct rtwn_softc *sc = (struct rtwn_softc *)ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	int s;

	sc->sc_tx_timer = 0;
	ifp->if_timer = 0;
	ifp->if_flags &= ~IFF_RUNNING;
	ifq_clr_oactive(&ifp->if_snd);

	s = splnet();
	ieee80211_new_state(ic, IEEE80211_S_INIT, -1);
	splx(s);

	sc->sc_ops.wait_async(sc->sc_ops.cookie);

	s = splnet();

	sc->sc_ops.cancel_scan(sc->sc_ops.cookie);
	sc->sc_ops.cancel_calib(sc->sc_ops.cookie);

	task_del(systq, &sc->init_task);

	splx(s);

	sc->sc_ops.stop(sc->sc_ops.cookie);
}
