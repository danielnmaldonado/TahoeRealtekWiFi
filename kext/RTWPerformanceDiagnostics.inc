// SPDX-License-Identifier: GPL-2.0-only
/* Observations only: no rate/radio policy or packet changes. These cumulative
 * counters describe validated associated unicast MPDUs before software CCMP
 * decrypt/replay checks, including their 802.11/crypto overhead and any retries.
 * They are not delivered Ethernet bytes or a measure of application throughput. */
void RTWUSBController::observeReceivedRate(const rtw_rx_packet &p) {
    if(!authorized()||stopping_||!p.mpdu||p.crc_error||p.icv_error||
       p.mpdu_length<sizeof(ieee80211_frame)||p.mpdu_length>4096)return;
    ieee80211_frame header{};
    memcpy(&header,p.mpdu,sizeof(header));
    if((header.i_fc[0]&IEEE80211_FC0_VERSION_MASK)||
       (header.i_fc[0]&IEEE80211_FC0_TYPE_MASK)!=IEEE80211_FC0_TYPE_DATA||
       (header.i_fc[0]&IEEE80211_FC0_SUBTYPE_NODATA)||
       ((header.i_fc[0]&IEEE80211_FC0_SUBTYPE_QOS)&&p.mpdu_length<sizeof(header)+2)||
       (header.i_fc[1]&IEEE80211_FC1_DIR_MASK)!=IEEE80211_FC1_DIR_FROMDS||
       (header.i_addr1[0]&1)||
       !IEEE80211_ADDR_EQ(header.i_addr1,core_.sc_ic.ic_myaddr)||
       !IEEE80211_ADDR_EQ(header.i_addr2,core_.sc_ic.ic_bss->ni_bssid))return;
    static const uint8_t rates[12]={2,4,11,22,12,18,24,36,48,72,96,108};
    perfDiag_.rxLastCode=p.rate_index;
    perfDiag_.rxLastRate500Kbps=0;
    if(p.rate_index<12){
        ++perfDiag_.rxRatePackets[p.rate_index];
        perfDiag_.rxRateBytes[p.rate_index]+=p.mpdu_length;
        perfDiag_.rxLastRate500Kbps=rates[p.rate_index];
    }else if(p.rate_index<28){
        const unsigned mcs=p.rate_index-12;
        ++perfDiag_.rxMCSPackets[mcs];perfDiag_.rxMCSBytes[mcs]+=p.mpdu_length;
        /* Supported native HT descriptor/PHY codes:0=20MHz,1=40MHz. */
        if(is8723Family()){
            if(p.bandwidth_code==0)++perfDiag_.rxHT20Packets;
            else ++perfDiag_.rxHTUnknownBWPackets;
        }else if(family_==RTW_WIFI_RTL8192EU||family_==RTW_WIFI_RTL8814AU||family_==RTW_WIFI_RTL8192DU||is8812Family()||is88Family()||is8192FUFamily()||is8710BUFamily()){
            if(p.bandwidth_code==0)++perfDiag_.rxHT20Packets;
            else if(p.bandwidth_code==1)++perfDiag_.rxHT40Packets;
            else ++perfDiag_.rxHTUnknownBWPackets;
        }
    }else if(is8812Family()&&p.rate_index>=44&&p.rate_index<64){
        const unsigned index=p.rate_index-44;
        ++perfDiag_.rxVHTPackets[index];perfDiag_.rxVHTBytes[index]+=p.mpdu_length;
        if(p.bandwidth_code==0)++perfDiag_.rxVHT20Packets;
        else if(p.bandwidth_code==1)++perfDiag_.rxVHT40Packets;
        else if(p.bandwidth_code==2)++perfDiag_.rxVHT80Packets;
        else ++perfDiag_.rxVHTUnknownBWPackets;
    }else{
        ++perfDiag_.rxUnknownPackets;
        perfDiag_.rxUnknownBytes+=p.mpdu_length;
    }
}

void RTWUSBController::publishPerformanceDiagnostics() {
    auto publish=[this](const char*name,uint64_t value){publishDiagnosticCounter(name,value);};
    for(unsigned i=0;i<12;++i){
        char name[48];
        snprintf(name,sizeof(name),"RTWDiagRXRate%uPackets",i);
        publish(name,perfDiag_.rxRatePackets[i]);
        snprintf(name,sizeof(name),"RTWDiagRXRate%uBytes",i);
        publish(name,perfDiag_.rxRateBytes[i]);
    }
    for(unsigned i=0;i<16;++i){
        char name[48];snprintf(name,sizeof(name),"RTWDiagRXMCS%uPackets",i);
        publish(name,perfDiag_.rxMCSPackets[i]);
        snprintf(name,sizeof(name),"RTWDiagRXMCS%uBytes",i);publish(name,perfDiag_.rxMCSBytes[i]);
    }
    for(unsigned i=0;i<20;i++){
        char name[48];snprintf(name,sizeof(name),"RTWDiagRXVHT%uSSMCS%uPackets",i/10+1,i%10);
        publish(name,perfDiag_.rxVHTPackets[i]);
        snprintf(name,sizeof(name),"RTWDiagRXVHT%uSSMCS%uBytes",i/10+1,i%10);publish(name,perfDiag_.rxVHTBytes[i]);
    }
    const bool vhtLast=is8812Family()&&perfDiag_.rxLastCode>=44&&perfDiag_.rxLastCode<64;
    publish("RTWDiagRXLastVHTNSS",vhtLast?(perfDiag_.rxLastCode-44)/10+1:0);
    publish("RTWDiagRXLastVHTMCS",vhtLast?(perfDiag_.rxLastCode-44)%10:255);
    publish("RTWDiagRXVHT20Packets",perfDiag_.rxVHT20Packets);publish("RTWDiagRXVHT40Packets",perfDiag_.rxVHT40Packets);
    publish("RTWDiagRXVHT80Packets",perfDiag_.rxVHT80Packets);publish("RTWDiagRXVHTUnknownBWPackets",perfDiag_.rxVHTUnknownBWPackets);
    publish("RTWDiagRXHT20Packets",perfDiag_.rxHT20Packets);
    publish("RTWDiagRXHT40Packets",perfDiag_.rxHT40Packets);
    publish("RTWDiagRXHTUnknownBWPackets",perfDiag_.rxHTUnknownBWPackets);
    publish("RTWDiagRXLastMCS",perfDiag_.rxLastCode>=12&&perfDiag_.rxLastCode<28?perfDiag_.rxLastCode-12:255);
    publish("RTWDiagRXRateUnknownPackets",perfDiag_.rxUnknownPackets);
    publish("RTWDiagRXRateUnknownBytes",perfDiag_.rxUnknownBytes);
    publish("RTWDiagRXLastRateCode",perfDiag_.rxLastCode);
    publish("RTWDiagRXLastRate500Kbps",perfDiag_.rxLastRate500Kbps);
    const auto&ic=core_.sc_ic;
    publish("RTWDiagRXUseProtection",(ic.ic_flags&IEEE80211_F_USEPROT)?1:0);
    publish("RTWDiagRXRTSThreshold",ic.ic_rtsthreshold);
    publish("RTWDiagRXNodeERP",ic.ic_bss?ic.ic_bss->ni_erp:0);
}
