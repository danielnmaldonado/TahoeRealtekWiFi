/* SPDX-License-Identifier: GPL-2.0-only
 * Read-only runtime discovery for the exact conflicting Linksys13b1:0043 ID.
 * This does not change catalog provenance or authorize other USB identities.
 */
#ifndef RTW_CHIP_IDENTITY_H
#define RTW_CHIP_IDENTITY_H
#include <stddef.h>
#include <stdint.h>
#include "rtw/USBWiFiProfiles.h"
#include "rtw/RTW88Firmware.h"
#ifdef __cplusplus
extern "C" {
#endif
#define RTW_CHIP_IDENTITY_STORAGE_BYTES 512U
typedef struct rtw_chip_identity rtw_chip_identity;
typedef struct {
 uint16_t vendor,product,bcd_device;
 uint8_t configuration,interface_number,interface_class,interface_subclass,interface_protocol,alternate_setting;
} rtw_chip_identity_descriptor;
size_t rtw_chip_identity_size(void);
/* Pure catalog/descriptor predicate. Only the immutable conflicting row,
 * configuration1/interface0 and the primary ff/ff/ff USB interface qualify. */
int rtw_chip_identity_candidate(const rtw_usb_wifi_profile*,const rtw_chip_identity_descriptor*);
/* Zeroed, pointer-aligned caller storage. One checked8-bit SYS_CFG2 read;
 * never writes, powers, downloads, delays or retries. Every other code and
 * read error fails closed. The copied profile retains actual VID/PID, name,
 * constraints, band limit and primary_verified=0; only family is resolved.
 * Caller serializes all operations and forgets proof before reusing storage. */
int rtw_chip_identity_init(void*,size_t,const rtw_usb_wifi_profile*,
 const rtw_chip_identity_descriptor*,const rtw88_fw_bus*,rtw_chip_identity**);
const rtw_usb_wifi_profile *rtw_chip_identity_profile(const rtw_chip_identity*);
uint8_t rtw_chip_identity_code(const rtw_chip_identity*);
/* Pure ownership validation: copies, altered profiles and other bus cookies/
 * callbacks do not inherit the proof. No catalog metadata API is relaxed. */
int rtw_chip_identity_valid(const rtw_chip_identity*,const rtw_usb_wifi_profile*,const rtw88_fw_bus*);
/* One Boot context may consume the proof; restarting that same context is
 * allowed, while copying/rebinding it to another object is rejected. */
int rtw_chip_identity_bind_context(rtw_chip_identity*,const void*);
int rtw_chip_identity_bound_to(const rtw_chip_identity*,const void*);
int rtw_chip_identity_descriptor_matches(const rtw_chip_identity*,const rtw_chip_identity_descriptor*);
/* Re-read the same code before a power/FW stage. A changed chip, transport
 * error or ownership mismatch irreversibly revokes this proof; zero writes. */
int rtw_chip_identity_revalidate(rtw_chip_identity*,const rtw88_fw_bus*);
void rtw_chip_identity_forget(rtw_chip_identity*);
#ifdef __cplusplus
}
#endif
#endif
