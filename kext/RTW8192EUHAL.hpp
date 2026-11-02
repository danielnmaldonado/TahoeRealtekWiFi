#ifndef RTW_8192EU_USB_HAL_HPP
#define RTW_8192EU_USB_HAL_HPP
#include "../vendor/rtwn/rtwnvar.h"
/* USB HAL for validated CU (92C/88C), 88E and 92E softc profiles.
 * Historical rtw92eu_* entry names are retained for the controller ABI.
 * Chip-dependent power, LLT, tables and firmware page writes dispatch here.
 * read/write/delay callbacks use sc_ops.cookie and record sticky I/O errors.
 * Functions return errno, including that sticky error. No method claims a
 * working radio without completing its register/table operations.
 */
int rtw92eu_power_on(struct rtwn_softc *);
/* Caller serializes and drains USB ownership first. Returns shutdown status
 * independently of a previous core_error; leaves that original error intact.
 * A nonzero result forbids reinitialization of the persistent core. */
int rtwLegacyPowerOffChecked(struct rtwn_softc *);
int rtw92eu_dma_init(struct rtwn_softc *, unsigned outputEndpoints);
int rtw92eu_mac_init(struct rtwn_softc *);
int rtw92eu_bb_init(struct rtwn_softc *);
int rtw92eu_aggr_init(struct rtwn_softc *, bool highSpeed);
int rtw92eu_fw_loadpage(struct rtwn_softc *, int page, uint8_t *, int length);
#endif
