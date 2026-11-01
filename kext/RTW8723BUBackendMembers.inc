/* SPDX-License-Identifier: GPL-2.0-only */
    rtw_8723bu r23Core_{};
    rtw_8723bu_rates r23Rates_{};
    void*r23Storage_=nullptr;size_t r23PrivateSize_=0;
    bool r23BoardKnown_=false,r23RateReady_=false;
    uint8_t r23MACAddress_[6]{};
    bool is8723Family()const,supports8723Band(bool)const,permits8723Channel(uint8_t)const,uses8723FirmwareRate()const;
    static int r23Read(void*,uint16_t,unsigned,uint32_t*),r23Write(void*,uint16_t,unsigned,uint32_t);
    static int r23Region(void*,uint16_t,const uint8_t*,size_t);
    static unsigned r23RadioState(void*,uint8_t);
    int bootstrap8723(bool),attach8723(),start8723(),set8723Channel(uint8_t,bool),state8723(ieee80211com*,ieee80211_state,int),station8723Hardware(ieee80211_state),tick8723();
    void stop8723(),release8723Storage();
    uint16_t station8723Rates(bool)const;
    static void update8723EDCA(ieee80211com*),watchdog8723(_ifnet*),initTask8723(void*);
    static int ioctl8723(_ifnet*,u_long,caddr_t);
    int send8723Frame(mbuf_t,ieee80211_node*),sample8723RSSI(const rtw_rx_packet&);
    rtw_packet_error decode8723USBFrame(rtw_rx_iterator*,rtw_8192eu_rx_frame*,uint8_t*)const;
    void handle8723C2H(const uint8_t*,size_t);
