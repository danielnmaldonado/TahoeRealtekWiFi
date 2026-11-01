// SPDX-License-Identifier: GPL-2.0-only
bool RTWUSBController::usesLegacyFirmwareRate() const {
    const auto&ic=core_.sc_ic;
    /* CU RUN is entered only after both firmware MACID masks and initial
     * rates have been programmed successfully. It has no CCX/AMRR path. */
    return family_==RTW_WIFI_RTL8192CU&&running_&&!stopping_&&
        ic.ic_state==IEEE80211_S_RUN&&ic.ic_bss;
}
rtw_packet_error RTWUSBController::decodeUSBFrame(rtw_rx_iterator*it,
    rtw_8192eu_rx_frame*out,uint8_t*reportType) const {
    if(out)memset(out,0,sizeof(*out));
    if(reportType)*reportType=0;
    if(!out||!reportType)return RTW_PACKETS_BAD_ARGUMENT;
    if(is8723Family())return decode8723USBFrame(it,out,reportType);
    if(is8710BUFamily())return decode8710BUUSBFrame(it,out,reportType);
    if(is8192FUFamily())return decode8192FUUSBFrame(it,out,reportType);
    if(family_==RTW_WIFI_RTL8192DU)return decode8192DUUSBFrame(it,out,reportType);
    if(is88Family())return decode88USBFrame(it,out,reportType);
    if(is8812Family())return decode8812USBFrame(it,out,reportType);
    if(family_==RTW_WIFI_RTL8192EU){
        auto result=rtw_8192eu_rx_next(it,out);
        if(result==RTW_PACKETS_OK)*reportType=out->report_is_c2h?1:0;
        return result;
    }
    if(family_==RTW_WIFI_RTL8814AU){
        rtw_8814au_rx_frame ac{};auto result=rtw_8814au_rx_next(it,&ac);
        if(result!=RTW_PACKETS_OK)return result;
        out->payload=ac.packet;out->report_is_c2h=ac.is_c2h;
        /* The8814 station RCR includes FCS. net80211 expects the MPDU
         * without those four bytes; firmware events have no802.11 FCS. */
        if(!ac.is_c2h){if(out->payload.mpdu_length<4)return RTW_PACKETS_MALFORMED;out->payload.mpdu_length-=4;}
        out->security_code=ac.security_code;out->sw_decrypt_required=ac.sw_decrypt_required;
        *reportType=ac.is_c2h?4:0;return result;
    }
    if(family_!=RTW_WIFI_RTL8192CU&&family_!=RTW_WIFI_RTL8188EU)return RTW_PACKETS_BAD_ARGUMENT;
    rtw_92c_rx_frame old{};
    auto result=rtw_92c_rx_next(it,family_==RTW_WIFI_RTL8188EU,&old);
    if(result!=RTW_PACKETS_OK)return result;
    /* Preserve structural decoding of foreign HT traffic during scanning.
     * The receive policy drops that MPDU after validation, keeping beacons
     * in the same USB aggregate available to net80211. */
    out->payload=old.packet;
    if(old.ht&&!old.report_type)out->payload.rate_index=(uint8_t)(12U+old.packet.rate_index);out->report_is_c2h=old.report_type!=0;
    out->security_code=old.security_code;out->sw_decrypt_required=old.sw_decrypt_required;
    *reportType=old.report_type;return RTW_PACKETS_OK;
}
bool RTWUSBController::uses8814FirmwareRate() const {
    return family_==RTW_WIFI_RTL8814AU&&acRateReady_&&acMediaConnected_&&
        running_&&!stopping_&&core_.sc_ic.ic_state==IEEE80211_S_RUN&&core_.sc_ic.ic_bss;
}
void RTWUSBController::handle8814C2H(const uint8_t*bytes,size_t length) {
    rtw_8814au_c2h_event event{};
    auto result=rtw_8814au_parse_c2h(bytes,length,acRateReady_?&acRateConfig_:nullptr,&event);
    if(result==RTW_8814AU_C2H_MALFORMED){++rateControl_.invalid;return;}
    if(result==RTW_8814AU_C2H_IGNORED){++rateControl_.ignored;return;}
    if(event.kind==RTW_8814AU_EVENT_RA&&uses8814FirmwareRate()){
        ++rateControl_.reports;rateControl_.finalRate=event.rate;
        /* Firmware rate telemetry carries no per-packet ACK/retry outcome. */
        rtw_8814au_phy_set_tx_rate(&acCore_,event.rate);
    }
}
