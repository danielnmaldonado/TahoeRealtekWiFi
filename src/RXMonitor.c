/* SPDX-License-Identifier: GPL-2.0-only
 * Passive monitor 8812AU register profile derived from GPL-2.0 reference
 * xen-proc/rtl8812au-macos rtl8812au/monitor.py.
 */
#include "rtw/RXMonitor.h"
#include "rtw/FCS.h"
#include <string.h>

#define RTW_MONITOR_RCR (0xF0000000UL | (1U<<14U) | (1U<<13U) | \
     (1U<<12U) | (1U<<11U) | (1U<<9U) | (1U<<8U) | 0x3FU)
#define RTW_MONITOR_CR 0x06FFU
static int write_checked(const rtw_usb_rw_transport* t,uint16_t a,uint8_t n,
                         uint32_t v,size_t* ops) {
    if(rtw_usb_write_register(t,a,n,v)!=RTW_USB_OK)return 0;
    ++*ops;
    return 1;
}
static int config_rx(const rtw_usb_rw_transport* bus,size_t* writes) {
    /* Accept monitor frames from RF only; never exposes a host data interface.
     * CRC/ICV errors are flagged by descriptor and discarded by receiver. */
    return write_checked(bus,0x0620U,4,0xFFFFFFFFU,writes) &&
           write_checked(bus,0x0624U,4,0xFFFFFFFFU,writes) &&
           write_checked(bus,0x0608U,4,RTW_MONITOR_RCR,writes) &&
           write_checked(bus,0x06A0U,2,0xFFFFU,writes) &&
           write_checked(bus,0x06A2U,2,0xFFFFU,writes) &&
           write_checked(bus,0x06A4U,2,0xFFFFU,writes) &&
           write_checked(bus,0x0100U,2,RTW_MONITOR_CR,writes);
}
rtw_monitor_error rtw_monitor_prepare_channel6(
    const rtw_usb_rw_transport* bus,const rtw_waiter* waiter,
    const rtw_hal_report* boot,int allow_hardware_writes,
    rtw_monitor_report* report) {
    rtw_monitor_report state;
    const rtw_mac_options mac={3,4,0};
    uint32_t cfg=0;
    size_t n=0;
    memset(&state,0,sizeof(state));
    if(report) memset(report,0,sizeof(*report));
    if(!report || !boot || !bus || !bus->control_in || !bus->control_out ||
       !waiter || !waiter->wait_us) return RTW_MONITOR_BAD_ARGUMENT;
    if(!allow_hardware_writes)return RTW_MONITOR_NOT_AUTHORIZED;
    if(boot->last_success!=RTW_HAL_MAC_PREPARED || boot->radio_ready ||
       boot->ethernet_authorized || boot->identity.eeprom_vendor!=0x0BDAU ||
       boot->identity.eeprom_product!=0x8812U || boot->identity.rfe_type!=3U ||
       boot->sys_cfg!=0x04411137U)
        return RTW_MONITOR_UNSUPPORTED;
    if(rtw_phy_profile_preflight_8812au(&state.phy)!=RTW_PHY_OK)
        return RTW_MONITOR_PHY_FAILED;
    /* Confirm the same chip remains attached after prior USB writes. */
    if(rtw_usb_rw_read(bus,0x00F0U,4,&cfg)!=RTW_USB_OK)
        return RTW_MONITOR_USB_FAILED;
    if(cfg!=boot->sys_cfg)return RTW_MONITOR_UNSUPPORTED;
    if(rtw_phy_profile_apply_8812au(bus,waiter,&state.phy)!=RTW_PHY_OK){
        *report=state;return RTW_MONITOR_PHY_FAILED;
    }
    /* Our pre-RF stage initially wrote the queue boundaries. Applying
     * vendor MAC table afterwards can alter some of them; program again. */
    if(rtw_mac_dma_prepare_8812au(bus,&mac,&n)!=RTW_MAC_OK){
        *report=state;return RTW_MONITOR_MAC_FAILED;
    }
    state.mac_operations=n;
    if(!config_rx(bus,&state.rx_register_operations)){
        *report=state;return RTW_MONITOR_USB_FAILED;
    }
    state.monitor_registers_written=1;
    *report=state;
    return RTW_MONITOR_OK;
}
rtw_monitor_error rtw_monitor_verify_channel6(
    const rtw_usb_rw_transport* bus, const rtw_monitor_report* report) {
    typedef struct {
        uint16_t address;
        uint8_t width;
        uint32_t mask;
        uint32_t expected;
    } check;
    /* These values are written in the fixed RFE3/20MHz monitor sequence.
       RF-path 3-wire LSSI readback is NOT verified by these BB checks. */
    static const check checks[] = {
        {0x00F0U, 4U, 0xFFFFFFFFU, 0x04411137U},
        {0x0100U, 2U, 0x0000FFFFU, RTW_MONITOR_CR},
        {0x0608U, 4U, 0xFFFFFFFFU, RTW_MONITOR_RCR},
        {0x0800U, 4U, 0x03000000U, 0x03000000U},
        {0x0808U, 4U, 0x30000000U, 0x30000000U},
        {0x0834U, 4U, 0x00000003U, 0x00000001U}
    };
    size_t i;
    uint32_t value = 0U;
    if (!bus || !bus->control_in || !report ||
        !report->monitor_registers_written || report->ethernet_authorized)
        return RTW_MONITOR_NOT_AUTHORIZED;
    for (i = 0U; i < sizeof(checks) / sizeof(checks[0]); ++i) {
        if (rtw_usb_rw_read(bus, checks[i].address, checks[i].width,
                            &value) != RTW_USB_OK)
            return RTW_MONITOR_USB_FAILED;
        if ((value & checks[i].mask) != checks[i].expected)
            return RTW_MONITOR_REGISTER_MISMATCH;
    }
    return RTW_MONITOR_OK;
}
rtw_monitor_error rtw_monitor_quiesce_rx_channel6(
    const rtw_usb_rw_transport *bus, const rtw_monitor_report *report) {
    uint32_t cr = 0U;
    /* 0x0080 = MACRXEN, 0x0008 = RXDMA_EN, 0x0002 = HCI_RXDMA_EN.
       Never clear the USB system clock, TX state, or unrelated CR bits. */
    const uint32_t rx_enable_mask = 0x008AU;
    if (!bus || !bus->control_in || !bus->control_out || !report ||
        (!report->monitor_registers_written &&
         report->rx_register_operations == 0U) ||
        report->ethernet_authorized)
        return RTW_MONITOR_NOT_AUTHORIZED;
    if (rtw_usb_rw_read(bus,0x0100U,2U,&cr) != RTW_USB_OK)
        return RTW_MONITOR_USB_FAILED;
    if (rtw_usb_write_register(bus,0x0100U,2U,
                               cr & ~rx_enable_mask) != RTW_USB_OK)
        return RTW_MONITOR_USB_FAILED;
    if (rtw_usb_rw_read(bus,0x0100U,2U,&cr) != RTW_USB_OK)
        return RTW_MONITOR_USB_FAILED;
    return (cr & rx_enable_mask) ?
        RTW_MONITOR_REGISTER_MISMATCH : RTW_MONITOR_OK;
}

rtw_packet_error rtw_monitor_process_bulk(
    const uint8_t* bulk,size_t length,rtw_monitor_beacon_cb on_beacon,
    void* context,rtw_monitor_rx_stats* stats) {
    rtw_rx_iterator it;
    rtw_rx_packet packet;
    rtw_packet_error err;
    size_t seen=0;
    if(!stats || !on_beacon || (!bulk && length) || length>65536U)
        return RTW_PACKETS_BAD_ARGUMENT;
    rtw_rx_iterator_init(&it,bulk,length);
    while((err=rtw_rx_next(&it,&packet))==RTW_PACKETS_OK){
        ++stats->packets_seen;
        ++seen;
        if(packet.crc_error){++stats->crc_dropped;continue;}
        if(packet.icv_error){++stats->icv_dropped;continue;}
        /* Only 802.11 beacon (8) / probe response (5), no data traffic.
           We intentionally do NOT distribute 802.11 data to macOS. */
        if(packet.mpdu_length<40U || (packet.mpdu[0]!=0x80U &&
           packet.mpdu[0]!=0x50U) || (packet.mpdu[1]&0x47U) ||
           (packet.mpdu[22]&0x0FU)){
            ++stats->non_beacons;continue;
        }
        /* Descriptor CRC status alone is insufficient: reject corrupted
         * MPDUs using the trailing on-air FCS (APPFCS=1). */
        if(!rtw_fcs_valid(packet.mpdu,packet.mpdu_length)) {
            ++stats->fcs_dropped;
            continue;
        }
        /* Monitor RCR APPFCS=1; exclude last four bytes before parser. */
        if(!on_beacon(context,packet.mpdu,packet.mpdu_length-4U,&packet)){
            return RTW_PACKETS_BAD_ARGUMENT;
        }
        ++stats->beacons;
    }
    if(err!=RTW_PACKETS_DONE){
        ++stats->malformed;
        return err;
    }
    (void)seen;
    return RTW_PACKETS_OK;
}
const char* rtw_monitor_error_string(rtw_monitor_error e) {
    switch(e){
        case RTW_MONITOR_OK:return "passive RX registers written; unverified hardware";
        case RTW_MONITOR_BAD_ARGUMENT:return "invalid monitor arguments";
        case RTW_MONITOR_NOT_AUTHORIZED:return "hardware register writes not authorized";
        case RTW_MONITOR_UNSUPPORTED:return "unknown cut/RFE/firmware or USB identity";
        case RTW_MONITOR_PHY_FAILED:return "PHY register programming failed";
        case RTW_MONITOR_MAC_FAILED:return "MAC DMA programming failed";
        case RTW_MONITOR_USB_FAILED:return "monitor RX USB register write/read failed";
        case RTW_MONITOR_REGISTER_MISMATCH:return "monitor register readback mismatch";
        default:return "unknown monitor error";
    }
}
