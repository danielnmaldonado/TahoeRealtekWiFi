#ifndef RTW_PHY_REGISTERS_H
#define RTW_PHY_REGISTERS_H
#include "rtw/PowerSequence.h"
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RTW_PHY_OK=0, RTW_PHY_BAD_ARGUMENT, RTW_PHY_UNSUPPORTED,
    RTW_PHY_BAD_TABLE, RTW_PHY_USB_ERROR, RTW_PHY_DELAY_FAILED
} rtw_phy_error;
typedef enum {
    RTW_PHY_MAC8=1,RTW_PHY_BB32=2,RTW_PHY_RF_A=3,RTW_PHY_RF_B=4,
    RTW_PHY_DELAY_US=5
} rtw_phy_operation;
typedef struct {
    uint16_t address; /* PHY_RF_* uses 8-bit RF address (not USB address). */
    uint8_t operation;
    uint32_t value;   /* full 32-bit BB, <=20-bit RF, <=50ms delay */
} rtw_phy_entry;
/* Pure whitelist/shape preflight, no I/O; 4 control registers <0x0100
 * are explicitly allowed for the RTL8812AU MAC vendor reference table. */
rtw_phy_error rtw_phy_validate_8812au(const rtw_phy_entry* entries,size_t count);
/* Table programming is prepared for externally reviewed vendor tables.
   The source does NOT ship a complete valid MAC/BB/AGC/RF/RFE profile.
   All entries are validated BEFORE the first hardware write; no rollback
   is possible on register I/O errors. Not run automatically by the kext. */
rtw_phy_error rtw_phy_apply_8812au(const rtw_usb_rw_transport* transport,
                                    const rtw_waiter* waiter,
                                    const rtw_phy_entry* entries,size_t count,
                                    size_t* applied);
/* Vendor 8812AU B-cut/TSMC USB BB/RF enable, including RF path B reset.
   Should ONLY be called after complete PHY init sequencing is verified. */
rtw_phy_error rtw_phy_enable_rf_8812au(const rtw_usb_rw_transport* transport);
/* Only RFE 3, band 2.4GHz as measured by xen-proc. No 5GHz/other boards. */
rtw_phy_error rtw_phy_config_rfe3_24ghz(const rtw_usb_rw_transport* transport);
const char* rtw_phy_error_string(rtw_phy_error err);
#ifdef __cplusplus
}
#endif
#endif
