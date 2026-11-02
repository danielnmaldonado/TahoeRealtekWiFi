/*	$OpenBSD: if_urtwn.c,v 1.114 2025/06/18 13:48:23 jcs Exp $	*/

/*-
 * Copyright (c) 2010 Damien Bergamini <damien.bergamini@free.fr>
 * Copyright (c) 2014 Kevin Lo <kevlo@FreeBSD.org>
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

/* CU/88E/92E USB HAL adapted from the pinned OpenBSD urtwn attachment.
 * Transport operations are synchronous and serialized by the controller.
 * This file never owns a USB device or opens an endpoint.
 */
#include "RTW8192EUHAL.hpp"
#include <sys/errno.h>

#define R92C_HQ_NPAGES 12
#define R92C_LQ_NPAGES 2
#define R92C_NQ_NPAGES 2
#define R92C_TX_PAGE_COUNT 248
#define R92C_MAX_RX_DMA_SIZE 0x2800
#define R88E_HQ_NPAGES 0
#define R88E_LQ_NPAGES 9
#define R88E_NQ_NPAGES 0
#define R88E_TX_PAGE_COUNT 168
#define R88E_MAX_RX_DMA_SIZE 0x2400
#define R88F_HQ_NPAGES 12
#define R88F_LQ_NPAGES 2
#define R88F_NQ_NPAGES 2
#define R88F_TX_PAGE_COUNT 247
#define R88F_MAX_RX_DMA_SIZE 0x3f80
#define R92E_HQ_NPAGES 16
#define R92E_LQ_NPAGES 16
#define R92E_NQ_NPAGES 16
#define R92E_TX_PAGE_COUNT 248
#define R92E_MAX_RX_DMA_SIZE 0x3fc0

static uint8_t urtwn_read_1(rtwn_softc *sc,uint16_t a) { return sc->sc_ops.read_1(sc->sc_ops.cookie,a); }
static uint16_t urtwn_read_2(rtwn_softc *sc,uint16_t a) { return sc->sc_ops.read_2(sc->sc_ops.cookie,a); }
static uint32_t urtwn_read_4(rtwn_softc *sc,uint16_t a) { return sc->sc_ops.read_4(sc->sc_ops.cookie,a); }
static void urtwn_write_1(rtwn_softc *sc,uint16_t a,uint8_t v) { sc->sc_ops.write_1(sc->sc_ops.cookie,a,v); }
static void urtwn_write_2(rtwn_softc *sc,uint16_t a,uint16_t v) { sc->sc_ops.write_2(sc->sc_ops.cookie,a,v); }
static void urtwn_write_4(rtwn_softc *sc,uint16_t a,uint32_t v) { sc->sc_ops.write_4(sc->sc_ops.cookie,a,v); }
#define urtwn_bb_read urtwn_read_4
#define urtwn_bb_write urtwn_write_4
static bool valid(rtwn_softc *sc) { return sc && (sc->chip & RTWN_CHIP_USB) && (sc->chip & (RTWN_CHIP_92C | RTWN_CHIP_88C | RTWN_CHIP_88E | RTWN_CHIP_92E)) && sc->sc_ops.read_1 && sc->sc_ops.read_2 && sc->sc_ops.read_4 && sc->sc_ops.write_1 && sc->sc_ops.write_2 && sc->sc_ops.write_4 && sc->sc_ops.delay_us && sc->sc_ops.io_error; }

/* GPL-2.0-only shutdown adaptation from Linux rtl8xxxu, commit
 * f47d223627f80cb7a1eb14c585fe6cb35de5e984:
 * 8192c.c:rtl8192cu_power_off; 8188e.c:rtl8188eu_power_off and transitions;
 * 8192e.c:rtl8192eu_power_off and transitions; core.c:flush_fifo,
 * firmware_self_reset, reset_8051, read_rfreg, write_rfreg_mask.
 * Copyright (c) 2014-2017 Jes Sorensen <Jes.Sorensen@gmail.com>
 * Portions Copyright (c) 2007-2011 Realtek Corporation.
 *
 * Keep a bootstrap failure separate from cleanup. Every callback is checked
 * through the transport's sticky error, never rtwn_io_error (which also sees
 * the original core_error). A failed transport ends I/O immediately. Bounded
 * FIFO/TX/MAC poll failures remain cleanup failures, while independent reset
 * steps still execute. A graceful firmware-reset timeout uses the upstream
 * forced reset fallback. No error state is cleared by this interface.
 */
class RTWLegacyPowerIO {
    rtwn_softc *sc_;
    int result_ = 0;
    bool transportOK() {
        int error = sc_->sc_ops.io_error(sc_->sc_ops.cookie);
        if (error && !result_) result_ = error;
        return error == 0;
    }
public:
    explicit RTWLegacyPowerIO(rtwn_softc *sc) : sc_(sc) {}
    int result() const { return result_; }
    bool read(uint16_t address, unsigned width, uint32_t &value) {
        if (!transportOK()) return false;
        if (width == 1) value = sc_->sc_ops.read_1(sc_->sc_ops.cookie, address);
        else if (width == 2) value = sc_->sc_ops.read_2(sc_->sc_ops.cookie, address);
        else value = sc_->sc_ops.read_4(sc_->sc_ops.cookie, address);
        return transportOK();
    }
    bool write(uint16_t address, unsigned width, uint32_t value) {
        if (!transportOK()) return false;
        if (width == 1) sc_->sc_ops.write_1(sc_->sc_ops.cookie, address, uint8_t(value));
        else if (width == 2) sc_->sc_ops.write_2(sc_->sc_ops.cookie, address, uint16_t(value));
        else sc_->sc_ops.write_4(sc_->sc_ops.cookie, address, value);
        return transportOK();
    }
    bool update(uint16_t address, unsigned width, uint32_t clear, uint32_t set) {
        uint32_t value;
        return read(address, width, value) && write(address, width, (value & ~clear) | set);
    }
    void delay(uint32_t us) {
        if (transportOK()) sc_->sc_ops.delay_us(sc_->sc_ops.cookie, us);
    }
    bool poll(uint16_t address, unsigned width, uint32_t mask,
              uint32_t expected, unsigned count, uint32_t delayUS,
              bool required = true) {
        for (unsigned i = 0; i < count; ++i) {
            uint32_t value;
            if (!read(address, width, value)) return false;
            if ((value & mask) == expected) return true;
            if (delayUS) delay(delayUS);
        }
        if (required && !result_) result_ = ETIMEDOUT;
        return false;
    }
};

static void rtwLegacyResetFirmware(RTWLegacyPowerIO &io) {
    io.write(R92C_HMETFR + 3, 1, 0x20);
    if (!io.poll(R92C_SYS_FUNC_EN, 2, R92C_SYS_FUNC_EN_CPUEN, 0, 100, 50, false))
        io.update(R92C_SYS_FUNC_EN, 2, R92C_SYS_FUNC_EN_CPUEN, 0);
}

static void rtwLegacyCUPowerOff(rtwn_softc *sc, RTWLegacyPowerIO &io) {
    uint32_t value;
    /* Linux identifies the 8188RU high-PA board from ROM RF option bit5. */
    if (!(sc->chip & RTWN_CHIP_92C) && sc->board_type == R92C_BOARD_TYPE_HIGHPA)
        io.update(R92C_FPGA0_RFPARAM(1), 4, 0, 2);
    io.write(R92C_TXPAUSE, 1, 0xff);

    /* RF path A AC low byte = 0, preserving its remaining 12 bits. */
    if (io.read(R92C_HSSI_PARAM2(0), 4, value)) {
        const uint32_t edge = value | R92C_HSSI_PARAM2_READ_EDGE;
        io.write(R92C_HSSI_PARAM2(0), 4, value & ~R92C_HSSI_PARAM2_READ_EDGE);
        io.delay(10);
        io.write(R92C_HSSI_PARAM2(0), 4, edge & ~R92C_HSSI_PARAM2_READ_ADDR_M);
        io.delay(100);
        io.write(R92C_HSSI_PARAM2(0), 4, edge);
        io.delay(10);
        if (io.read(R92C_HSSI_PARAM1(0), 4, value)) {
            const uint16_t readback = (value & R92C_HSSI_PARAM1_PI) ?
                R92C_HSPI_READBACK(0) : R92C_LSSI_READBACK(0);
            if (io.read(readback, 4, value)) {
                io.write(R92C_LSSI_PARAM(0), 4, value & 0xfff00U);
                io.delay(1);
            }
        }
    }
    io.update(R92C_APSD_CTRL, 1, 0, R92C_APSD_CTRL_OFF);
    io.update(R92C_FPGA0_RFPARAM(1), 4, 0, 0x80000000U);
    io.write(R92C_SYS_FUNC_EN, 1, R92C_SYS_FUNC_EN_USBA |
        R92C_SYS_FUNC_EN_USBD | R92C_SYS_FUNC_EN_BB_GLB_RST);
    io.write(R92C_SYS_FUNC_EN, 1, R92C_SYS_FUNC_EN_USBA | R92C_SYS_FUNC_EN_USBD);
    if (io.read(R92C_MCUFWDL, 1, value) && (value & R92C_MCUFWDL_RDY)) {
        io.write(R92C_MCUFWDL, 1, 0);
        io.write(R92C_FWIMR, 1, 0x20);
        io.write(R92C_HMETFR + 3, 1, 0x20);
        if (!io.poll(R92C_SYS_FUNC_EN, 2, R92C_SYS_FUNC_EN_CPUEN, 0, 100, 50, false)) {
            io.write(R92C_SYS_FUNC_EN + 1, 1, 0x50);
            io.delay(10000);
        }
    }
    io.write(R92C_SYS_FUNC_EN + 1, 1, 0x54);

    io.write(R92C_GPIO_PIN_CTRL + 2, 2, 0);
    if (io.read(R92C_GPIO_PIN_CTRL, 4, value)) {
        value &= 0xffff00ffU;
        io.write(R92C_GPIO_PIN_CTRL, 4, value | ((value & 0xff) << 8) | 0x00ff0000U);
    }
    io.write(R92C_GPIO_MUXCFG + 3, 1, 0);
    if (io.read(R92C_GPIO_MUXCFG + 2, 2, value)) {
        value &= 0xff0fU;
        io.write(R92C_GPIO_MUXCFG + 2, 2, value | ((value & 15) << 4) | 0x0780);
    }
    /* Upstream's UMC B-cut SPS workaround needs the actual cut, rather than
     * treating every non-A UMC device as cut1. */
    if (io.read(R92C_SYS_CFG, 4, value)) {
        const bool umcB = (value & R92C_SYS_CFG_VENDOR_UMC) &&
            MS(value, R92C_SYS_CFG_CHIP_VER_RTL) == 1;
        io.write(R92C_SPS0_CTRL, 1, umcB ? 0x2b : 0x23);
    }
    io.write(R92C_APS_FSMCO, 2, 0x4802);
    io.write(R92C_RSV_CTRL, 1, 0x0e);
}

static void rtwLegacyEUPowerOff(rtwn_softc *sc, RTWLegacyPowerIO &io) {
    const bool e92 = (sc->chip & RTWN_CHIP_92E) != 0;
    uint32_t value;
    io.write(R92C_TXPAUSE, 1, 0xff);
    io.update(R92C_RXPKT_NUM, 4, 0, 0x00040000);
    io.poll(R92C_RXPKT_NUM, 4, 0x00020000, 0x00020000, 100, 0);
    io.write(R92C_RQPN_NPQ, 2, 0);
    io.write(R92C_RQPN, 4, 0x80000000U);
    io.delay(2000);
    io.update(R88E_TX_RPT_CTRL, 1, R88E_TX_RPT_CTRL_TIMER_EN, 0);
    io.write(R92C_RF_CTRL, 1, 0);

    /* active_to_lps. Linux uses 0x5f8 for this 32-bit scheduler poll;
     * OpenBSD's similarly named R92C_SCH_TXCMD is a different register. */
    io.write(R92C_TXPAUSE, 1, e92 ? 0xff : 0x7f);
    if (io.poll(0x05f8, 4, 0xffffffffU, 0, 100, 0)) {
        io.update(R92C_SYS_FUNC_EN, 1, R92C_SYS_FUNC_EN_BBRSTB, 0);
        io.delay(2);
        if (e92) {
            io.update(R92C_SYS_FUNC_EN, 1, R92C_SYS_FUNC_EN_BB_GLB_RST, 0);
            io.update(R92C_CR, 2, 0xff, R92C_CR_HCI_TXDMA_EN | R92C_CR_HCI_RXDMA_EN);
            io.update(R92C_CR, 2, R92C_CR_ENSEC, 0);
        } else {
            io.update(R92C_CR, 2, R92C_CR_MACTXEN | R92C_CR_MACRXEN | R92C_CR_ENSEC,
                0xff & ~(R92C_CR_MACTXEN | R92C_CR_MACRXEN | R92C_CR_ENSEC));
        }
        io.update(R92C_DUAL_TSF_RST, 1, 0, 0x20);
    }
    if (io.read(R92C_MCUFWDL, 1, value) && (value & R92C_MCUFWDL_RAM_DL_SEL))
        rtwLegacyResetFirmware(io);
    io.update(R92C_SYS_FUNC_EN, 2, R92C_SYS_FUNC_EN_CPUEN, 0);
    io.write(R92C_MCUFWDL, 1, 0);

    if (e92) {
        /* reset_8051: toggle the I/O wrapper and CPU exactly as upstream. */
        io.update(R92C_RSV_CTRL + 1, 1, 1, 0);
        io.update(R92C_SYS_FUNC_EN, 2, R92C_SYS_FUNC_EN_CPUEN, 0);
        io.update(R92C_RSV_CTRL + 1, 1, 0, 1);
        io.update(R92C_SYS_FUNC_EN, 2, 0, R92C_SYS_FUNC_EN_CPUEN);
        io.update(R92C_RF_CTRL, 1, R92C_RF_CTRL_EN, 0);
        io.update(R92C_LEDCFG2, 1, 0x80, 0);
        io.update(R92C_APS_FSMCO + 1, 1, 0, 2);
        io.poll(R92C_APS_FSMCO + 1, 1, 2, 0, 500, 10);
        io.update(R92C_APS_FSMCO + 1, 1, 0x18, 8);
    } else {
        io.update(0x0194, 1, 1, 0); // 8188E 32K_CTRL
        io.update(R92C_RF_CTRL, 1, R92C_RF_CTRL_EN, 0);
        io.update(R92C_LPLDO_CTRL, 1, 0, 0x10);
        io.update(R92C_AFE_XTAL_CTRL, 4, 0, 0x00800000);
        io.update(R92C_APS_FSMCO, 2, R92C_APS_FSMCO_AFSM_PCIE, R92C_APS_FSMCO_AFSM_HSUS);
        io.write(R92C_APS_FSMCO + 3, 1, 0);
        io.update(R92C_GPIO_MUXCFG + 1, 1, 0x10, 0);
        io.update(0xfe10, 1, 0, 0x10);
        io.update(R92C_RSV_CTRL + 1, 1, 8, 0);
        io.update(R92C_RSV_CTRL + 1, 1, 0, 8);
        if (io.read(R92C_GPIO_PIN_CTRL, 1, value)) io.write(R92C_GPIO_PIN_CTRL + 1, 1, value);
        io.write(R92C_GPIO_PIN_CTRL + 2, 1, 0xff);
        if (io.read(R92C_GPIO_IO_SEL, 1, value)) io.write(R92C_GPIO_IO_SEL, 1, value << 4);
        io.update(R92C_GPIO_IO_SEL + 1, 1, 0, 15);
        io.write(0x0064, 4, 0x00080808); // PAD_CTRL1 LNA/TRSW/EX_PA output
        io.write(R92C_RSV_CTRL, 1, 0);
        io.write(R92C_GPIO_MUXCFG, 4, 0);
    }
}

int rtwLegacyPowerOffChecked(rtwn_softc *sc) {
    if (!valid(sc) || !sc->sc_ops.cookie) return EINVAL;
    const uint32_t families = sc->chip & (RTWN_CHIP_92C | RTWN_CHIP_88C | RTWN_CHIP_88E | RTWN_CHIP_92E);
    const uint32_t allowed = RTWN_CHIP_USB | RTWN_CHIP_92C | RTWN_CHIP_88C |
        RTWN_CHIP_88E | RTWN_CHIP_92E | RTWN_CHIP_UMC | RTWN_CHIP_UMC_A_CUT | RTWN_CHIP_92C_1T2R;
    if ((sc->chip & ~allowed) || (families != RTWN_CHIP_92C && families != RTWN_CHIP_88C &&
        families != RTWN_CHIP_88E && families != RTWN_CHIP_92E)) return EINVAL;
    const int transportError = sc->sc_ops.io_error(sc->sc_ops.cookie);
    if (transportError) return transportError;
    RTWLegacyPowerIO io(sc);
    if (families & (RTWN_CHIP_92C | RTWN_CHIP_88C)) rtwLegacyCUPowerOff(sc, io);
    else rtwLegacyEUPowerOff(sc, io);
    return io.result();
}


/* Legacy power and manual LLT flows from the same pinned urtwn snapshot. */
static int
rtwlegacy_r92c_power_on(struct rtwn_softc *sc)
{
	uint32_t reg;
	int ntries;

	/* Wait for autoload done bit. */
	for (ntries = 0; ntries < 1000; ntries++) {
		if (rtwn_io_error(sc)) return rtwn_io_error(sc);
		if (urtwn_read_1(sc, R92C_APS_FSMCO) & R92C_APS_FSMCO_PFM_ALDN)
			break;
		rtwn_delay(sc, 5);
	}
	if (ntries == 1000) {
		printf("%s: timeout waiting for chip autoload\n",
		    sc->sc_pdev->dv_xname);
		return (ETIMEDOUT);
	}

	/* Unlock ISO/CLK/Power control register. */
	urtwn_write_1(sc, R92C_RSV_CTRL, 0);
	/* Move SPS into PWM mode. */
	urtwn_write_1(sc, R92C_SPS0_CTRL, 0x2b);
	rtwn_delay(sc, 100);

	reg = urtwn_read_1(sc, R92C_LDOV12D_CTRL);
	if (!(reg & R92C_LDOV12D_CTRL_LDV12_EN)) {
		urtwn_write_1(sc, R92C_LDOV12D_CTRL,
		    reg | R92C_LDOV12D_CTRL_LDV12_EN);
		rtwn_delay(sc, 100);
		urtwn_write_1(sc, R92C_SYS_ISO_CTRL,
		    urtwn_read_1(sc, R92C_SYS_ISO_CTRL) &
		    ~R92C_SYS_ISO_CTRL_MD2PP);
	}

	/* Auto enable WLAN. */
	urtwn_write_2(sc, R92C_APS_FSMCO,
	    urtwn_read_2(sc, R92C_APS_FSMCO) | R92C_APS_FSMCO_APFM_ONMAC);
	for (ntries = 0; ntries < 1000; ntries++) {
		if (rtwn_io_error(sc)) return rtwn_io_error(sc);
		if (!(urtwn_read_2(sc, R92C_APS_FSMCO) &
		    R92C_APS_FSMCO_APFM_ONMAC))
			break;
		rtwn_delay(sc, 5);
	}
	if (ntries == 1000) {
		printf("%s: timeout waiting for MAC auto ON\n",
		    sc->sc_pdev->dv_xname);
		return (ETIMEDOUT);
	}

	/* Enable radio, GPIO and LED functions. */
	urtwn_write_2(sc, R92C_APS_FSMCO,
	    R92C_APS_FSMCO_AFSM_HSUS |
	    R92C_APS_FSMCO_PDN_EN |
	    R92C_APS_FSMCO_PFM_ALDN);
	/* Release RF digital isolation. */
	urtwn_write_2(sc, R92C_SYS_ISO_CTRL,
	    urtwn_read_2(sc, R92C_SYS_ISO_CTRL) & ~R92C_SYS_ISO_CTRL_DIOR);

	/* Enable MAC DMA/WMAC/SCHEDULE/SEC blocks. */
	reg = urtwn_read_2(sc, R92C_CR);
	reg |= R92C_CR_HCI_TXDMA_EN | R92C_CR_HCI_RXDMA_EN |
	    R92C_CR_TXDMA_EN | R92C_CR_RXDMA_EN | R92C_CR_PROTOCOL_EN |
	    R92C_CR_SCHEDULE_EN | R92C_CR_MACTXEN | R92C_CR_MACRXEN |
	    R92C_CR_ENSEC;
	urtwn_write_2(sc, R92C_CR, reg);

	urtwn_write_1(sc, 0xfe10, 0x19);
	return rtwn_io_error(sc);
}

static int
rtwlegacy_r88e_power_on(struct rtwn_softc *sc)
{
	uint32_t reg;
	int ntries;

	/* Wait for power ready bit. */
	for (ntries = 0; ntries < 5000; ntries++) {
		if (rtwn_io_error(sc)) return rtwn_io_error(sc);
		if (urtwn_read_4(sc, R92C_APS_FSMCO) & R92C_APS_FSMCO_SUS_HOST)
			break;
		rtwn_delay(sc, 10);
	}
	if (ntries == 5000) {
		printf("%s: timeout waiting for chip power up\n",
		    sc->sc_pdev->dv_xname);
		return (ETIMEDOUT);
	}

	/* Reset BB. */
	urtwn_write_1(sc, R92C_SYS_FUNC_EN,
	    urtwn_read_1(sc, R92C_SYS_FUNC_EN) & ~(R92C_SYS_FUNC_EN_BBRSTB |
	    R92C_SYS_FUNC_EN_BB_GLB_RST));

	urtwn_write_1(sc, R92C_AFE_XTAL_CTRL + 2,
	    urtwn_read_1(sc, R92C_AFE_XTAL_CTRL + 2) | 0x80);

	/* Disable HWPDN. */
	urtwn_write_2(sc, R92C_APS_FSMCO,
	    urtwn_read_2(sc, R92C_APS_FSMCO) & ~R92C_APS_FSMCO_APDM_HPDN);
	/* Disable WL suspend. */
	urtwn_write_2(sc, R92C_APS_FSMCO,
	    urtwn_read_2(sc, R92C_APS_FSMCO) &
	    ~(R92C_APS_FSMCO_AFSM_HSUS | R92C_APS_FSMCO_AFSM_PCIE));

	/* Auto enable WLAN. */
	urtwn_write_2(sc, R92C_APS_FSMCO,
	    urtwn_read_2(sc, R92C_APS_FSMCO) | R92C_APS_FSMCO_APFM_ONMAC);
	for (ntries = 0; ntries < 5000; ntries++) {
		if (rtwn_io_error(sc)) return rtwn_io_error(sc);
		if (!(urtwn_read_2(sc, R92C_APS_FSMCO) &
		    R92C_APS_FSMCO_APFM_ONMAC))
			break;
		rtwn_delay(sc, 10);
	}
	if (ntries == 5000) {
		printf("%s: timeout waiting for MAC auto ON\n",
		    sc->sc_pdev->dv_xname);
		return (ETIMEDOUT);
	}

	/* Enable LDO normal mode. */
	urtwn_write_1(sc, R92C_LPLDO_CTRL,
	    urtwn_read_1(sc, R92C_LPLDO_CTRL) & ~0x10);

	/* Enable MAC DMA/WMAC/SCHEDULE/SEC blocks. */
	urtwn_write_2(sc, R92C_CR, 0);
	reg = urtwn_read_2(sc, R92C_CR);
	reg |= R92C_CR_HCI_TXDMA_EN | R92C_CR_HCI_RXDMA_EN |
	    R92C_CR_TXDMA_EN | R92C_CR_RXDMA_EN | R92C_CR_PROTOCOL_EN |
	    R92C_CR_SCHEDULE_EN | R92C_CR_ENSEC | R92C_CR_CALTMR_EN;
	urtwn_write_2(sc, R92C_CR, reg);
	return rtwn_io_error(sc);
}

static int
rtwlegacy_llt_write(struct rtwn_softc *sc, uint32_t addr, uint32_t data)
{
	int ntries;

	urtwn_write_4(sc, R92C_LLT_INIT,
	    SM(R92C_LLT_INIT_OP, R92C_LLT_INIT_OP_WRITE) |
	    SM(R92C_LLT_INIT_ADDR, addr) |
	    SM(R92C_LLT_INIT_DATA, data));
	/* Wait for write operation to complete. */
	for (ntries = 0; ntries < 20; ntries++) {
		if (rtwn_io_error(sc)) return rtwn_io_error(sc);
		if (MS(urtwn_read_4(sc, R92C_LLT_INIT), R92C_LLT_INIT_OP) ==
		    R92C_LLT_INIT_OP_NO_ACTIVE)
			return rtwn_io_error(sc);
		rtwn_delay(sc, 5);
	}
	return (ETIMEDOUT);
}

static int
rtwlegacy_llt_init(struct rtwn_softc *sc, int page_count)
{
	int i, error, pktbuf_count;

	pktbuf_count = (sc->chip & RTWN_CHIP_88E) ?
	    177 : 256;

	/* Reserve pages [0; page_count]. */
	for (i = 0; i < page_count; i++) {
		if ((error = rtwlegacy_llt_write(sc, i, i + 1)) != 0)
			return (error);
	}
	/* NB: 0xff indicates end-of-list. */
	if ((error = rtwlegacy_llt_write(sc, i, 0xff)) != 0)
		return (error);
	/*
	 * Use pages [page_count + 1; pktbuf_count - 1]
	 * as ring buffer.
	 */
	for (++i; i < pktbuf_count - 1; i++) {
		if ((error = rtwlegacy_llt_write(sc, i, i + 1)) != 0)
			return (error);
	}
	/* Make the last page point to the beginning of the ring buffer. */
	error = rtwlegacy_llt_write(sc, i, page_count + 1);
	return (error);
}

static void rtw92eu_burstlen_init(struct rtwn_softc *sc, bool highSpeed)
{
    uint8_t reg = urtwn_read_1(sc, R92E_RXDMA_PRO);
    reg &= ~0x30;
    urtwn_write_1(sc, R92E_RXDMA_PRO, reg | (highSpeed ? 0x1e : 0x2e));
}
static int
rtw92eu_auto_llt_init(struct rtwn_softc *sc)
{
	int ntries;

	urtwn_write_4(sc, R92E_AUTO_LLT,
	    urtwn_read_4(sc, R92E_AUTO_LLT) | R92E_AUTO_LLT_EN);
	for (ntries = 0; ntries < 1000; ntries++) {
		if (!(urtwn_read_4(sc, R92E_AUTO_LLT) & R92E_AUTO_LLT_EN))
			return rtwn_io_error(sc);
		rtwn_delay(sc, 2);
	}

	return (ETIMEDOUT);
}

int
rtw92eu_power_on(struct rtwn_softc *sc)
{
	if (!valid(sc)) return EINVAL;
	if (rtwn_io_error(sc)) return rtwn_io_error(sc);
	if (sc->chip & RTWN_CHIP_88E) return rtwlegacy_r88e_power_on(sc);
	if (!(sc->chip & RTWN_CHIP_92E)) return rtwlegacy_r92c_power_on(sc);
	uint32_t reg;
	int ntries;

	if (urtwn_read_4(sc, R92C_SYS_CFG) & R92E_SYS_CFG_SPSLDO_SEL) {
		/* LDO. */
		urtwn_write_1(sc, R92E_LDO_SWR_CTRL, 0xc3);
	} else {
		reg = urtwn_read_4(sc, R92C_SYS_SWR_CTRL2);
		reg &= 0xff0fffff;
		reg |= 0x00500000;
		urtwn_write_4(sc, R92C_SYS_SWR_CTRL2, reg);
		urtwn_write_1(sc, R92E_LDO_SWR_CTRL, 0x83);
	}

	/* 40MHz crystal source */
	urtwn_write_1(sc, R92C_AFE_PLL_CTRL,
	    urtwn_read_1(sc, R92C_AFE_PLL_CTRL) & 0xfb);
	urtwn_write_4(sc, R92C_AFE_XTAL_CTRL_EXT,
	    urtwn_read_4(sc, R92C_AFE_XTAL_CTRL_EXT) & 0xfffffc7f);

	urtwn_write_1(sc, R92C_AFE_PLL_CTRL,
	    urtwn_read_1(sc, R92C_AFE_PLL_CTRL) & 0xbf);
	urtwn_write_4(sc, R92C_AFE_XTAL_CTRL_EXT,
	    urtwn_read_4(sc, R92C_AFE_XTAL_CTRL_EXT) & 0xffdfffff);

	/* Disable HWPDN. */
	urtwn_write_2(sc, R92C_APS_FSMCO,
	    urtwn_read_2(sc, R92C_APS_FSMCO) & ~R92C_APS_FSMCO_APDM_HPDN);
	for (ntries = 0; ntries < 5000; ntries++) {
		if (urtwn_read_4(sc, R92C_APS_FSMCO) & R92C_APS_FSMCO_SUS_HOST)
			break;
		rtwn_delay(sc, 10);
	}
	if (ntries == 5000) {
		printf("%s: timeout waiting for chip power up\n",
		    sc->sc_pdev->dv_xname);
		return (ETIMEDOUT);
	}

	/* Disable WL suspend. */
	urtwn_write_2(sc, R92C_APS_FSMCO,
	    urtwn_read_2(sc, R92C_APS_FSMCO) &
	    ~(R92C_APS_FSMCO_AFSM_HSUS | R92C_APS_FSMCO_AFSM_PCIE));

	/* Auto enable WLAN. */
	urtwn_write_4(sc, R92C_APS_FSMCO,
	    urtwn_read_4(sc, R92C_APS_FSMCO) | R92C_APS_FSMCO_RDY_MACON);
	urtwn_write_2(sc, R92C_APS_FSMCO,
	    urtwn_read_2(sc, R92C_APS_FSMCO) | R92C_APS_FSMCO_APFM_ONMAC);
	for (ntries = 0; ntries < 5000; ntries++) {
		if (!(urtwn_read_2(sc, R92C_APS_FSMCO) &
		    R92C_APS_FSMCO_APFM_ONMAC))
			break;
		rtwn_delay(sc, 10);
	}
	if (ntries == 5000) {
		printf("%s: timeout waiting for MAC auto ON\n",
		    sc->sc_pdev->dv_xname);
		return (ETIMEDOUT);
	}

	/* Enable MAC DMA/WMAC/SCHEDULE/SEC blocks. */
	urtwn_write_2(sc, R92C_CR, 0);
	reg = urtwn_read_2(sc, R92C_CR);
	reg |= R92C_CR_HCI_TXDMA_EN | R92C_CR_HCI_RXDMA_EN |
	    R92C_CR_TXDMA_EN | R92C_CR_RXDMA_EN | R92C_CR_PROTOCOL_EN |
	    R92C_CR_SCHEDULE_EN | R92C_CR_ENSEC | R92C_CR_CALTMR_EN;
	urtwn_write_2(sc, R92C_CR, reg);
	return rtwn_io_error(sc);
}

int
rtw92eu_dma_init(struct rtwn_softc *sc, unsigned outputEndpoints)
{
	if (!valid(sc) || outputEndpoints < 1 || outputEndpoints > 3) return EINVAL;
	if (rtwn_io_error(sc)) return rtwn_io_error(sc);
	uint32_t reg;
	uint16_t dmasize;
	int hqpages, lqpages, nqpages, pagecnt, boundary;
	int error, hashq, haslq, hasnq;

	/* Default initialization of chipset values. */
	if (sc->chip & RTWN_CHIP_88E) {
		hqpages = R88E_HQ_NPAGES;
		lqpages = R88E_LQ_NPAGES;
		nqpages = R88E_NQ_NPAGES;
		pagecnt = R88E_TX_PAGE_COUNT;
		dmasize = R88E_MAX_RX_DMA_SIZE;
	} else if (sc->chip & RTWN_CHIP_88F) {
		hqpages = R88F_HQ_NPAGES;
		lqpages = R88F_LQ_NPAGES;
		nqpages = R88F_NQ_NPAGES;
		pagecnt = R88F_TX_PAGE_COUNT;
		dmasize = R88F_MAX_RX_DMA_SIZE;
	} else if (sc->chip & RTWN_CHIP_92E) {
		hqpages = R92E_HQ_NPAGES;
		lqpages = R92E_LQ_NPAGES;
		nqpages = R92E_NQ_NPAGES;
		pagecnt = R92E_TX_PAGE_COUNT;
		dmasize = R92E_MAX_RX_DMA_SIZE;
	} else {
		hqpages = R92C_HQ_NPAGES;
		lqpages = R92C_LQ_NPAGES;
		nqpages = R92C_NQ_NPAGES;
		pagecnt = R92C_TX_PAGE_COUNT;
		dmasize = R92C_MAX_RX_DMA_SIZE;
	}
	boundary = pagecnt + 1;

	/* Initialize LLT table. */
	error = (sc->chip & RTWN_CHIP_92E) ?
	    rtw92eu_auto_llt_init(sc) : rtwlegacy_llt_init(sc, pagecnt);
	if (error != 0)
		return (error);

	/* Get Tx queues to USB endpoints mapping. */
	hashq = hasnq = haslq = 0;
	switch (outputEndpoints) {
	case 3:
		haslq = 1;
		pagecnt -= lqpages;
		/* FALLTHROUGH */
	case 2:
		hasnq = 1;
		pagecnt -= nqpages;
		/* FALLTHROUGH */
	case 1:
		hashq = 1;
		pagecnt -= hqpages;
		break;
	}

	/* Set number of pages for normal priority queue. */
	urtwn_write_1(sc, R92C_RQPN_NPQ, hasnq ? nqpages : 0);
	urtwn_write_4(sc, R92C_RQPN,
	    /* Set number of pages for public queue. */
	    SM(R92C_RQPN_PUBQ, pagecnt) |
	    /* Set number of pages for high priority queue. */
	    SM(R92C_RQPN_HPQ, hashq ? hqpages : 0) |
	    /* Set number of pages for low priority queue. */
	    SM(R92C_RQPN_LPQ, haslq ? lqpages : 0) |
	    /* Load values. */
	    R92C_RQPN_LD);

	urtwn_write_1(sc, R92C_TXPKTBUF_BCNQ_BDNY, boundary);
	urtwn_write_1(sc, R92C_TXPKTBUF_MGQ_BDNY, boundary);
	urtwn_write_1(sc, R92C_TXPKTBUF_WMAC_LBK_BF_HD, boundary);
	urtwn_write_1(sc, R92C_TRXFF_BNDY, boundary);
	urtwn_write_1(sc, R92C_TDECTRL + 1, boundary);

	/* Set queue to USB pipe mapping. */
	reg = urtwn_read_2(sc, R92C_TRXDMA_CTRL);
	reg &= ~R92C_TRXDMA_CTRL_QMAP_M;
	if (haslq)
		reg |= R92C_TRXDMA_CTRL_QMAP_3EP;
	else if (hashq) {
		if (!hasnq)
			reg |= R92C_TRXDMA_CTRL_QMAP_HQ;
		else
			reg |= R92C_TRXDMA_CTRL_QMAP_HQ_NQ;
	}
	urtwn_write_2(sc, R92C_TRXDMA_CTRL, reg);

	/* Set Tx/Rx transfer page boundary. */
	urtwn_write_2(sc, R92C_TRXFF_BNDY + 2, dmasize - 1);

	if (!(sc->chip & RTWN_CHIP_92E)) {
		/* Set Tx/Rx transfer page size. */
		if (sc->chip & RTWN_CHIP_88F) {
			urtwn_write_1(sc, R92C_PBP,
			    SM(R92C_PBP_PSRX, R92C_PBP_256) |
			    SM(R92C_PBP_PSTX, R92C_PBP_256));
		} else {
			urtwn_write_1(sc, R92C_PBP,
			    SM(R92C_PBP_PSRX, R92C_PBP_128) |
			    SM(R92C_PBP_PSTX, R92C_PBP_128));
		}
	}
	return error ? error : rtwn_io_error(sc);
}

int
rtw92eu_mac_init(struct rtwn_softc *sc)
{
	if (!valid(sc)) return EINVAL;
	if (rtwn_io_error(sc)) return rtwn_io_error(sc);
	int i;

	/* Write MAC initialization values. */
	if (sc->chip & RTWN_CHIP_88E) {
		for (i = 0; i < nitems(rtl8188eu_mac); i++) {
			urtwn_write_1(sc, rtl8188eu_mac[i].reg,
			    rtl8188eu_mac[i].val);
		}
		urtwn_write_1(sc, R92C_MAX_AGGR_NUM, 0x07);
	} else if (sc->chip & RTWN_CHIP_88F) {
		for (i = 0; i < nitems(rtl8188ftv_mac); i++) {
			urtwn_write_1(sc, rtl8188ftv_mac[i].reg,
			    rtl8188ftv_mac[i].val);
		}
	} else if (sc->chip & RTWN_CHIP_92E) {
		for (i = 0; i < nitems(rtl8192eu_mac); i++) {
			urtwn_write_1(sc, rtl8192eu_mac[i].reg,
			    rtl8192eu_mac[i].val);
		}
	} else {
		for (i = 0; i < nitems(rtl8192cu_mac); i++)
			urtwn_write_1(sc, rtl8192cu_mac[i].reg,
			    rtl8192cu_mac[i].val);
	}
	return rtwn_io_error(sc);
}

int
rtw92eu_bb_init(struct rtwn_softc *sc)
{
	if (!valid(sc)) return EINVAL;
	if (rtwn_io_error(sc)) return rtwn_io_error(sc);
	const struct r92c_bb_prog *prog;
	uint32_t reg;
	uint8_t xtal;
	int i;

	/* Enable BB and RF. */
	urtwn_write_2(sc, R92C_SYS_FUNC_EN,
	    urtwn_read_2(sc, R92C_SYS_FUNC_EN) |
	    R92C_SYS_FUNC_EN_BBRSTB | R92C_SYS_FUNC_EN_BB_GLB_RST |
	    R92C_SYS_FUNC_EN_DIO_RF);

	if (!(sc->chip & (RTWN_CHIP_88E | RTWN_CHIP_88F | RTWN_CHIP_92E)))
		urtwn_write_2(sc, R92C_AFE_PLL_CTRL, 0xdb83);

	urtwn_write_1(sc, R92C_RF_CTRL,
	    R92C_RF_CTRL_EN | R92C_RF_CTRL_RSTB | R92C_RF_CTRL_SDMRSTB);
	urtwn_write_1(sc, R92C_SYS_FUNC_EN,
	    R92C_SYS_FUNC_EN_USBA | R92C_SYS_FUNC_EN_USBD |
	    R92C_SYS_FUNC_EN_BB_GLB_RST | R92C_SYS_FUNC_EN_BBRSTB);

	if (!(sc->chip &
	    (RTWN_CHIP_88E | RTWN_CHIP_88F | RTWN_CHIP_92E))) {
		urtwn_write_1(sc, R92C_LDOHCI12_CTRL, 0x0f);
		urtwn_write_1(sc, 0x15, 0xe9);
		urtwn_write_1(sc, R92C_AFE_XTAL_CTRL + 1, 0x80);
	}

	/* Select BB programming based on board type. */
	if (sc->chip & RTWN_CHIP_88E)
		prog = &rtl8188eu_bb_prog;
	else if (sc->chip & RTWN_CHIP_88F)
		prog = &rtl8188ftv_bb_prog;
	else if (sc->chip & RTWN_CHIP_92E)
		prog = &rtl8192eu_bb_prog;
	else if (!(sc->chip & RTWN_CHIP_92C)) {
		if (sc->board_type == R92C_BOARD_TYPE_MINICARD)
			prog = &rtl8188ce_bb_prog;
		else if (sc->board_type == R92C_BOARD_TYPE_HIGHPA)
			prog = &rtl8188ru_bb_prog;
		else
			prog = &rtl8188cu_bb_prog;
	} else {
		if (sc->board_type == R92C_BOARD_TYPE_MINICARD)
			prog = &rtl8192ce_bb_prog;
		else
			prog = &rtl8192cu_bb_prog;
	}
	/* Write BB initialization values. */
	for (i = 0; i < prog->count; i++) {
		urtwn_bb_write(sc, prog->regs[i], prog->vals[i]);
		rtwn_delay(sc, 1);
	}

	if (sc->chip & RTWN_CHIP_92C_1T2R) {
		/* 8192C 1T only configuration. */
		reg = urtwn_bb_read(sc, R92C_FPGA0_TXINFO);
		reg = (reg & ~0x00000003) | 0x2;
		urtwn_bb_write(sc, R92C_FPGA0_TXINFO, reg);

		reg = urtwn_bb_read(sc, R92C_FPGA1_TXINFO);
		reg = (reg & ~0x00300033) | 0x00200022;
		urtwn_bb_write(sc, R92C_FPGA1_TXINFO, reg);

		reg = urtwn_bb_read(sc, R92C_CCK0_AFESETTING);
		reg = (reg & ~0xff000000) | 0x45 << 24;
		urtwn_bb_write(sc, R92C_CCK0_AFESETTING, reg);

		reg = urtwn_bb_read(sc, R92C_OFDM0_TRXPATHENA);
		reg = (reg & ~0x000000ff) | 0x23;
		urtwn_bb_write(sc, R92C_OFDM0_TRXPATHENA, reg);

		reg = urtwn_bb_read(sc, R92C_OFDM0_AGCPARAM1);
		reg = (reg & ~0x00000030) | 1 << 4;
		urtwn_bb_write(sc, R92C_OFDM0_AGCPARAM1, reg);

		reg = urtwn_bb_read(sc, 0xe74);
		reg = (reg & ~0x0c000000) | 2 << 26;
		urtwn_bb_write(sc, 0xe74, reg);
		reg = urtwn_bb_read(sc, 0xe78);
		reg = (reg & ~0x0c000000) | 2 << 26;
		urtwn_bb_write(sc, 0xe78, reg);
		reg = urtwn_bb_read(sc, 0xe7c);
		reg = (reg & ~0x0c000000) | 2 << 26;
		urtwn_bb_write(sc, 0xe7c, reg);
		reg = urtwn_bb_read(sc, 0xe80);
		reg = (reg & ~0x0c000000) | 2 << 26;
		urtwn_bb_write(sc, 0xe80, reg);
		reg = urtwn_bb_read(sc, 0xe88);
		reg = (reg & ~0x0c000000) | 2 << 26;
		urtwn_bb_write(sc, 0xe88, reg);
	}

	/* Write AGC values. */
	for (i = 0; i < prog->agccount; i++) {
		urtwn_bb_write(sc, R92C_OFDM0_AGCRSSITABLE,
		    prog->agcvals[i]);
		rtwn_delay(sc, 1);
	}

	if (sc->chip & (RTWN_CHIP_88E | RTWN_CHIP_88F)) {
		urtwn_bb_write(sc, R92C_OFDM0_AGCCORE1(0), 0x69553422);
		rtwn_delay(sc, 1);
		urtwn_bb_write(sc, R92C_OFDM0_AGCCORE1(0), 0x69553420);
		rtwn_delay(sc, 1);
	} else if (sc->chip & RTWN_CHIP_92E) {
		urtwn_bb_write(sc, R92C_OFDM0_AGCCORE1(0), 0x00040022);
		rtwn_delay(sc, 1);
		urtwn_bb_write(sc, R92C_OFDM0_AGCCORE1(0), 0x00040020);
		rtwn_delay(sc, 1);
	}

	if (sc->chip & (RTWN_CHIP_88E | RTWN_CHIP_88F)) {
		xtal = sc->crystal_cap & 0x3f;
		reg = urtwn_bb_read(sc, R92C_AFE_XTAL_CTRL);
		urtwn_bb_write(sc, R92C_AFE_XTAL_CTRL,
		    RW(reg, R92C_AFE_XTAL_CTRL_ADDR, xtal | xtal << 6));
	} else if (sc->chip & RTWN_CHIP_92E) {
		xtal = sc->crystal_cap & 0x3f;
		reg = urtwn_bb_read(sc, R92C_AFE_CTRL3);
		urtwn_bb_write(sc, R92C_AFE_CTRL3,
		    RW(reg, R92C_AFE_CTRL3_ADDR, xtal | xtal << 6));
		urtwn_write_4(sc, R92C_AFE_XTAL_CTRL, 0x000f81fb);
	}

	if (urtwn_bb_read(sc, R92C_HSSI_PARAM2(0)) & R92C_HSSI_PARAM2_CCK_HIPWR)
		sc->sc_flags |= RTWN_FLAG_CCK_HIPWR;
	return rtwn_io_error(sc);
}

int
rtw92eu_aggr_init(struct rtwn_softc *sc, bool highSpeed)
{
	if (!valid(sc)) return EINVAL;
	if (rtwn_io_error(sc)) return rtwn_io_error(sc);
	uint32_t reg = 0;
	int dmasize, dmatiming, ndesc;

	/* Set burst packet length. */
	if (sc->chip & (RTWN_CHIP_88F | RTWN_CHIP_92E))
		rtw92eu_burstlen_init(sc, highSpeed);

	if (sc->chip & RTWN_CHIP_88F) {
		dmasize = 5;
		dmatiming = 32;
		ndesc = 6;
	} else if (sc->chip & RTWN_CHIP_92E) {
		dmasize = 6;
		dmatiming = 32;
		ndesc = 3;
	} else {
		dmasize = 48;
		dmatiming = 4;
		ndesc = (sc->chip & RTWN_CHIP_88E) ? 1 : 6;
	}

	/* Tx aggregation setting. */
	reg = urtwn_read_4(sc, R92C_TDECTRL);
	reg = RW(reg, R92C_TDECTRL_BLK_DESC_NUM, ndesc);
	urtwn_write_4(sc, R92C_TDECTRL, reg);
	if (sc->chip & (RTWN_CHIP_88F | RTWN_CHIP_92E))
		urtwn_write_1(sc, R92E_DWBCN1_CTRL, ndesc << 1);

	/* Rx aggregation setting. */
	urtwn_write_1(sc, R92C_TRXDMA_CTRL,
	    urtwn_read_1(sc, R92C_TRXDMA_CTRL) | R92C_TRXDMA_CTRL_RXDMA_AGG_EN);

	urtwn_write_1(sc, R92C_RXDMA_AGG_PG_TH, dmasize);
	if (sc->chip & (RTWN_CHIP_92C | RTWN_CHIP_88C))
		urtwn_write_1(sc, R92C_USB_DMA_AGG_TO, dmatiming);
	else
		urtwn_write_1(sc, R92C_RXDMA_AGG_PG_TH + 1, dmatiming);

	if (sc->chip & RTWN_CHIP_88F) {
		urtwn_write_1(sc, R92E_RXDMA_PRO,
		    urtwn_read_1(sc, R92E_RXDMA_PRO) | R92E_RXDMA_PRO_DMA_MODE);
	}

	/* Drop incorrect bulk out. */
	urtwn_write_4(sc, R92C_TXDMA_OFFSET_CHK,
	    urtwn_read_4(sc, R92C_TXDMA_OFFSET_CHK) |
	    R92C_TXDMA_OFFSET_CHK_DROP_DATA_EN);
	return rtwn_io_error(sc);
}

int
rtw92eu_fw_loadpage(struct rtwn_softc *sc, int page, uint8_t *buf, int len)
{
	if (!valid(sc) || !sc->sc_ops.write_region || !buf || page < 0 || page > 7 || len < 1 || len > 4096) return EINVAL;
	if (rtwn_io_error(sc)) return rtwn_io_error(sc);
	uint32_t reg;
	int maxblksz, off, mlen, error = 0;

	reg = urtwn_read_4(sc, R92C_MCUFWDL);
	reg = RW(reg, R92C_MCUFWDL_PAGE, page);
	urtwn_write_4(sc, R92C_MCUFWDL, reg);

	maxblksz = (sc->chip & RTWN_CHIP_92E) ? 254 : 196;

	off = R92C_FW_START_ADDR;
	while (len > 0) {
		if (len > maxblksz)
			mlen = maxblksz;
		else if (len > 4)
			mlen = 4;
		else
			mlen = 1;
		error = sc->sc_ops.write_region(sc->sc_ops.cookie, off, buf, mlen);
		if (error != 0)
			break;
		off += mlen;
		buf += mlen;
		len -= mlen;
	}
	return (error);
}
