/* SPDX-License-Identifier: BSD-3-Clause */
/* Include inside RTWUSBController's private section, after the AU public
 * headers have been included at file scope. Parent owns controller routing. */
    rtw_r12au_boot auCore_{};
    /* Retained after positive identify for every AU board, including no-BT.
     * Dynamic readiness/mailbox fields in this copy are not identity. */
    rtw_r12au_boot auAuthority_{};
    rtw_wifi_family auAuthorityFamily_=RTW_WIFI_UNKNOWN;
    const void *auAuthorityProvider_=nullptr,*auAuthorityRX_=nullptr,*auAuthorityTX_[4]{};
    uint16_t auAuthorityMPS_[4]{};
    rtw_usb_endpoints auAuthorityEndpoints_{};
    bool auAuthorityBound_=false;
    void remember8812Authority();
    bool owns8812Authority() const;
    void *auPHYStorage_ = nullptr;
    size_t auPHYSize_ = 0;
    rtw_r12au_phy *auPHY_ = nullptr;
    void *auCoexStorage_ = nullptr;
    size_t auCoexSize_ = 0;
    rtw_r21au_coex *auCoex_ = nullptr;
    uint64_t auCoexTxBytes_ = 0,auCoexRxBytes_ = 0;
    uint32_t auCoexTxFrames_ = 0,auCoexRxFrames_ = 0;
    uint8_t auCoexRxRate_ = 2,auCoexTxRate_ = 2;
    rtw_r12au_legacy_rate auRateConfig_{};
    rtw_r12au_operating auOperating_{};
    bool auHTSupported_ = false, auVHTSupported_ = false;
    bool auMediaConnected_ = false, auRateReady_ = false;
    bool auRSSIValid_ = false, auCckHighPower_ = false;
    uint8_t auRSSI_ = 0, auChannel_ = 0;
    bool is8812Family() const;
    bool supports8812Band(bool five) const;
    bool uses8812FirmwareRate() const;
    bool uses8812HighThroughput() const;
    bool accepts8812ReceiveRate(const rtw_rx_packet&) const;
    static int ampdu8812TXStart(ieee80211com*,ieee80211_node*,uint8_t);
    uint32_t channel8812Modes(uint8_t) const;
    void refresh8812Modes();
    int peer8812Operating(rtw_r12au_operating*) const;
    int set8812Operating(const rtw_r12au_operating&);
    static rtw_usb_error auControlIn(void *,const rtw_usb_setup *,uint8_t *,uint16_t *);
    static rtw_usb_error auControlOut(void *,const rtw_usb_setup *,const uint8_t *,uint16_t *);
    static int auWait(void *,uint32_t);
    int initialize8812Core();
    int attach8812(),start8812(),set8812Channel(uint8_t);
    void stop8812(),release8812Storage();
    int state8812(ieee80211com *,ieee80211_state,int);
    int station8812Hardware(ieee80211_state);
    int require8812Hardware();
    uint16_t station8812Rates(bool basicOnly) const;
    static void update8812EDCA(ieee80211com *);
    static void watchdog8812(_ifnet *),initTask8812(void *);
    static int ioctl8812(_ifnet *,u_long,caddr_t);
    int send8812Frame(mbuf_t,ieee80211_node *);
    rtw_packet_error decode8812USBFrame(rtw_rx_iterator *,rtw_8192eu_rx_frame *,uint8_t *) const;
    int sample8812RSSI(const rtw_rx_packet &);
    void handle8812C2H(const uint8_t *,size_t);
    int tick8812();
    rtw_r21au_coex_input input8812Coex() const;
    int notify8812Coex(rtw_r21au_coex_event);
