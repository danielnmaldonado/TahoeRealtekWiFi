/* SPDX-License-Identifier: GPL-2.0-only
 * RTL8812AU register programming based on GPL-2.0 reference
 * xen-proc/rtl8812au-macos rtl8812au/phy.py and tables.py.
 */
#include "rtw/PHYProfile.h"
#include "RTL8812VendorTables.inc"
#include <string.h>

#define RTW_ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))

static int readreg(const rtw_usb_rw_transport* t, uint16_t reg,
                   uint8_t bytes, uint32_t* value) {
    return rtw_usb_rw_read(t,reg,bytes,value)==RTW_USB_OK;
}
static int write_reg(const rtw_usb_rw_transport* t,uint16_t addr,uint8_t bytes,uint32_t value) {
    return rtw_usb_write_register(t,addr,bytes,value)==RTW_USB_OK;
}
/* values are unshifted, matching mask-based writes in the vendor PHY. */
static int set_bits(const rtw_usb_rw_transport* t,uint16_t addr,uint8_t bytes,
                    uint32_t mask,uint32_t value) {
    uint32_t old=0;
    uint8_t shift=0;
    if(!mask || !readreg(t,addr,bytes,&old)) return 0;
    while(((mask>>shift)&1U)==0U) ++shift;
    return write_reg(t,addr,bytes,(old&~mask)|((value<<shift)&mask));
}
/* Reconstruct the pinned vendor (address,u32) byte stream, including
 * the four 0x0FFE 50ms delay markers. The published FNV-1a digests were
 * independently computed from the exact upstream tables.py blob. */
static uint32_t vendor_table_fnv1a(const rtw_phy_entry* entries,size_t count) {
    size_t i,j;
    uint32_t hash=2166136261U;
    for(i=0;i<count;++i){
        uint32_t a=entries[i].address, v=entries[i].value;
        if(entries[i].operation==RTW_PHY_DELAY_US) {
            if(entries[i].value!=50000U)return 0U;
            a=0x0FFEU;v=0;
        }
        for(j=0;j<6U;++j) {
            const uint8_t octet=(uint8_t)(j<2U ? a>>(j*8U) : v>>((j-2U)*8U));
            hash=(hash^octet)*16777619U;
        }
    }
    return hash;
}
static uint32_t last_rf18(const rtw_phy_entry* entries,size_t count) {
    size_t i;
    uint32_t value=0;
    for(i=0;i<count;++i)
        if(entries[i].address==0x18 && entries[i].operation!=RTW_PHY_DELAY_US)
            value=entries[i].value;
    return value;
}
rtw_phy_error rtw_phy_profile_preflight_8812au(rtw_phy_profile_report* report) {
    rtw_phy_profile_report p;
    memset(&p,0,sizeof(p));
    p.mac_entries=RTW_ARRAY_COUNT(profile_mac_reg);
    p.bb_entries=RTW_ARRAY_COUNT(profile_phy_reg);
    p.agc_entries=RTW_ARRAY_COUNT(profile_agc_tab);
    p.rf_a_entries=RTW_ARRAY_COUNT(profile_radio_a);
    p.rf_b_entries=RTW_ARRAY_COUNT(profile_radio_b);
    if(report) *report=p;
    if(p.mac_entries!=107U || p.bb_entries!=215U || p.agc_entries!=132U ||
       p.rf_a_entries!=210U || p.rf_b_entries!=193U) return RTW_PHY_BAD_TABLE;
    if(rtw_phy_validate_8812au(profile_mac_reg,p.mac_entries)!=RTW_PHY_OK ||
       rtw_phy_validate_8812au(profile_phy_reg,p.bb_entries)!=RTW_PHY_OK ||
       rtw_phy_validate_8812au(profile_agc_tab,p.agc_entries)!=RTW_PHY_OK ||
       rtw_phy_validate_8812au(profile_radio_a,p.rf_a_entries)!=RTW_PHY_OK ||
       rtw_phy_validate_8812au(profile_radio_b,p.rf_b_entries)!=RTW_PHY_OK)
        return RTW_PHY_BAD_TABLE;
    /* Constant table provides last RF channel/band register state. */
    if(vendor_table_fnv1a(profile_mac_reg,p.mac_entries)!=0xD4AE639DU ||
       vendor_table_fnv1a(profile_phy_reg,p.bb_entries)!=0x599F8C51U ||
       vendor_table_fnv1a(profile_agc_tab,p.agc_entries)!=0x507FD42AU ||
       vendor_table_fnv1a(profile_radio_a,p.rf_a_entries)!=0xA08738A2U ||
       vendor_table_fnv1a(profile_radio_b,p.rf_b_entries)!=0xA26163EEU)
        return RTW_PHY_BAD_TABLE;
    if(!last_rf18(profile_radio_a,p.rf_a_entries) ||
       !last_rf18(profile_radio_b,p.rf_b_entries)) return RTW_PHY_BAD_TABLE;
    return RTW_PHY_OK;
}
rtw_phy_error rtw_phy_channel6_8812au(const rtw_usb_rw_transport* t) {
    /* 8812AU B-cut, 2T2R, RFE3. No generic/unknown-band channel change. */
    static const uint32_t mask_band=(1U<<18U)|(1U<<17U)|(1U<<16U)|
                                    (1U<<9U)|(1U<<8U);
    static const uint32_t mask_bw=(1U<<11U)|(1U<<10U);
    const uint32_t rf18a=last_rf18(profile_radio_a,RTW_ARRAY_COUNT(profile_radio_a));
    const uint32_t rf18b=last_rf18(profile_radio_b,RTW_ARRAY_COUNT(profile_radio_b));
    const uint32_t rf_mask=mask_band|mask_bw|0xFFU;
    uint32_t cck=0;
    if(!t || !t->control_in || !t->control_out || !rf18a || !rf18b)
        return RTW_PHY_BAD_ARGUMENT;
    /* Set BB band to 2.4 GHz, preserving USB state and other fields. */
    if(!set_bits(t,0x0808U,4,0x30000000U,3) ||
       !set_bits(t,0x0834U,4,0x00000003U,1) ||
       !set_bits(t,0x0830U,4,0x0003E000U,0x17) ||
       !set_bits(t,0x0830U,4,0x0000000EU,4) ||
       !set_bits(t,0x082CU,4,0x00000003U,0) ||
       !set_bits(t,0x080CU,4,0x000000F0U,1) ||
       !set_bits(t,0x0A04U,4,0x0F000000U,1) ||
       !readreg(t,0x0454U,1,&cck) ||
       !write_reg(t,0x0454U,1,cck&~0x80U) ||
       !set_bits(t,0x0860U,4,0x1FFE0000U,0x96AU) ||
       !write_reg(t,0x0C90U,4,(0x18U<<20U)|((rf18a&~rf_mask)|0x0C00U|6U)) ||
       !write_reg(t,0x0E90U,4,(0x18U<<20U)|((rf18b&~rf_mask)|0x0C00U|6U)) ||
       !set_bits(t,0x0668U,2,0x00000180U,0) ||
       !write_reg(t,0x0483U,1,0) ||
       !set_bits(t,0x08ACU,4,0x003003C3U,0x00300200U) ||
       !set_bits(t,0x08C4U,4,0x40000000U,0) ||
       !set_bits(t,0x0848U,4,0x03C00000U,7))
        return RTW_PHY_USB_ERROR;
    return RTW_PHY_OK;
}

rtw_phy_error rtw_phy_profile_apply_8812au(
    const rtw_usb_rw_transport* t,const rtw_waiter* w,
    rtw_phy_profile_report* report) {
    size_t n=0;
    rtw_phy_profile_report status;
    rtw_phy_error err=rtw_phy_profile_preflight_8812au(&status);
    if(report) memset(report,0,sizeof(*report));
    if(err!=RTW_PHY_OK)return err;
    if(!t || !t->control_in || !t->control_out || !w || !w->wait_us)
        return RTW_PHY_BAD_ARGUMENT;
#define APPLY_PROFILE(array) do { \
    err=rtw_phy_apply_8812au(t,w,array,RTW_ARRAY_COUNT(array),&n); \
    status.executed+=n; \
    if(err!=RTW_PHY_OK)goto failed; \
} while(0)
    /* Required order: MAC table before PHY/AGC and RF tables. Caller
       re-applies MAC DMA boundaries after this function if needed. */
    APPLY_PROFILE(profile_mac_reg);
    err=rtw_phy_enable_rf_8812au(t);
    if(err!=RTW_PHY_OK)goto failed;
    APPLY_PROFILE(profile_phy_reg);
    APPLY_PROFILE(profile_agc_tab);
    /* Gate the demodulators separately: CCK + OFDM need 0x800[25:24]
       enabled even when the BB tables load without USB errors. */
    if(!set_bits(t,0x0800U,4,0x03000000U,3)) {
        err=RTW_PHY_USB_ERROR;goto failed;
    }
    /* The vendor init table leaves RX paths 28/29 clear. */
    if(!set_bits(t,0x0808U,4,0x30000000U,3)) {err=RTW_PHY_USB_ERROR;goto failed;}
    APPLY_PROFILE(profile_radio_a);
    APPLY_PROFILE(profile_radio_b);
    err=rtw_phy_config_rfe3_24ghz(t);
    if(err!=RTW_PHY_OK)goto failed;
    err=rtw_phy_channel6_8812au(t);
failed:
    if(report) *report=status;
    return err;
#undef APPLY_PROFILE
}
