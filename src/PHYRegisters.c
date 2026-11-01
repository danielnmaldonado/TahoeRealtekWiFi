/* SPDX-License-Identifier: GPL-2.0-only
   RTL8812AU interoperability; implementation based on vendor sequences and
   GPL-2.0 reference xen-proc/rtl8812au-macos and aircrack-ng/rtl8812au.
   This does NOT attest to validation on the user's physical hardware. */
#include "rtw/PHYRegisters.h"

static int valid_entry(const rtw_phy_entry* e) {
    switch(e->operation) {
        case RTW_PHY_MAC8:
            return ((e->address>=0x0100U && e->address<0x0800U) ||
                    e->address==0x0010U || e->address==0x0011U ||
                    e->address==0x0025U || e->address==0x0072U) &&
                   e->value<=0xFFU;
        case RTW_PHY_BB32:
            return e->address>=0x0800U && e->address<0x0F00U &&
                   !(e->address & 3U);
        case RTW_PHY_RF_A:case RTW_PHY_RF_B:
            return e->address<=0xFFU && e->value<=0xFFFFFU;
        case RTW_PHY_DELAY_US:
            return e->address==0 && e->value>0 && e->value<=50000U;
        default:return 0;
    }
}
rtw_phy_error rtw_phy_validate_8812au(const rtw_phy_entry* entries,size_t count) {
    size_t i;
    if(!entries || !count || count>4096U) return RTW_PHY_BAD_ARGUMENT;
    for(i=0;i<count;++i) if(!valid_entry(entries+i)) return RTW_PHY_BAD_TABLE;
    return RTW_PHY_OK;
}
rtw_phy_error rtw_phy_apply_8812au(const rtw_usb_rw_transport* t,
                                    const rtw_waiter* waiter,
                                    const rtw_phy_entry* entries,size_t count,
                                    size_t* applied) {
    size_t i;
    if(applied) *applied=0;
    if(!t || !t->control_out || !t->control_in ||
       !waiter || !waiter->wait_us || !entries || !count || count>4096U)
        return RTW_PHY_BAD_ARGUMENT;
    { const rtw_phy_error preflight=rtw_phy_validate_8812au(entries,count);
      if(preflight!=RTW_PHY_OK) return preflight; }
    for(i=0;i<count;++i) {
        const rtw_phy_entry* e=entries+i;
        if(e->operation==RTW_PHY_DELAY_US) {
            uint32_t remaining=e->value;
            while(remaining>0) {
                const uint32_t delay=remaining>10000U?10000U:remaining;
                if(!waiter->wait_us(waiter->context,delay))return RTW_PHY_DELAY_FAILED;
                remaining-=delay;
            }
        } else {
            uint16_t address=e->address;
            uint8_t width=4;
            uint32_t value=e->value;
            if(e->operation==RTW_PHY_MAC8)width=1;
            if(e->operation==RTW_PHY_RF_A || e->operation==RTW_PHY_RF_B) {
                address=e->operation==RTW_PHY_RF_A ? 0x0C90U : 0x0E90U;
                value=(((uint32_t)e->address & 0xFFU)<<20U)|e->value;
            }
            if(rtw_usb_write_register(t,address,width,value)!=RTW_USB_OK)
                return RTW_PHY_USB_ERROR;
        }
        if(applied) *applied=i+1U;
    }
    return RTW_PHY_OK;
}
rtw_phy_error rtw_phy_enable_rf_8812au(const rtw_usb_rw_transport* t) {
    uint32_t sys=0;
    if(!t || !t->control_out || !t->control_in)return RTW_PHY_BAD_ARGUMENT;
    if(rtw_usb_rw_read(t,0x0002U,2,&sys)!=RTW_USB_OK ||
       rtw_usb_write_register(t,0x0002U,2,sys|0x2003U)!=RTW_USB_OK ||
       rtw_usb_write_register(t,0x001FU,1,0x07U)!=RTW_USB_OK ||
       /* Explicitly preserve USBA, USBD and USB PLL (FEN_UPLL=0x08). */
       rtw_usb_write_register(t,0x0002U,1,0x1FU)!=RTW_USB_OK ||
       rtw_usb_write_register(t,0x001FU,1,0x05U)!=RTW_USB_OK ||
       rtw_usb_write_register(t,0x001FU,1,0x07U)!=RTW_USB_OK ||
       rtw_usb_write_register(t,0x0076U,1,0x05U)!=RTW_USB_OK ||
       rtw_usb_write_register(t,0x0076U,1,0x07U)!=RTW_USB_OK)
        return RTW_PHY_USB_ERROR;
    return RTW_PHY_OK;
}
static int masked_write(const rtw_usb_rw_transport* t,uint16_t addr,uint32_t mask,uint32_t bits) {
    uint32_t v=0;
    return rtw_usb_rw_read(t,addr,4,&v)==RTW_USB_OK &&
           rtw_usb_write_register(t,addr,4,(v&~mask)|(bits&mask))==RTW_USB_OK;
}
rtw_phy_error rtw_phy_config_rfe3_24ghz(const rtw_usb_rw_transport* t) {
    if(!t || !t->control_in || !t->control_out)return RTW_PHY_BAD_ARGUMENT;
    if(rtw_usb_write_register(t,0x0CB0U,4,0x54337770U)!=RTW_USB_OK ||
       rtw_usb_write_register(t,0x0EB0U,4,0x54337770U)!=RTW_USB_OK ||
       !masked_write(t,0x0CB4U,0x3FF00000U,0x01000000U) ||
       !masked_write(t,0x0EB4U,0x3FF00000U,0x01000000U) ||
       !masked_write(t,0x0900U,0x00000303U,0x00000001U))
       return RTW_PHY_USB_ERROR;
    return RTW_PHY_OK;
}
const char* rtw_phy_error_string(rtw_phy_error e) {
    switch(e) {
        case RTW_PHY_OK:return "ok";
        case RTW_PHY_BAD_ARGUMENT:return "bad argument";
        case RTW_PHY_UNSUPPORTED:return "unsupported hardware profile";
        case RTW_PHY_BAD_TABLE:return "bad or unsafe PHY table";
        case RTW_PHY_USB_ERROR:return "USB PHY write/read failed";
        case RTW_PHY_DELAY_FAILED:return "PHY delay failed";
        default:return "unknown";
    }
}
