#include "rtwn_ht_host.hpp"
int main() {
    rtwn_softc s; ieee80211_node ni; s.sc_ic.ic_bss=&ni;
    static const uint8_t rates[]={0x82,0x84,0x8b,0x96,12,18,24,36,48,72,96,108};
    ni.ni_rates.rs_nrates=sizeof(rates); std::memcpy(ni.ni_rates.rs_rates,rates,sizeof(rates));
    require(rtwn_ra_init(&s)==0,"RTL8192EU initializes negotiated station rates");
    require(s.commands.size()==1 && s.commands[0].id==0x40,"RTL8192EU configures generation-2 firmware rate adaptation");
    require(s.commands[0].payload==std::vector<uint8_t>({0,6,0,0xff,0x0f,0,0}),"legacy fallback uses generation-2 BG RAID and only peer legacy rates");
    require((s.rrsr&0xfffU)==0xfffU && s.inirts==3,"legacy response rates and basic RTS rate are preserved");
    s.commands.clear();s.sc_ic.ic_softc=&s;
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==0,"successful RUN enters associated state");
    require(s.commands.size()==2 && s.commands[0].id==0x40 && s.commands[1].id==0x01,"RUN installs negotiated rates before enabling firmware connection");
    require(s.commands[1].payload==std::vector<uint8_t>({0x21,0,0}) && s.fw_station_connected,"RUN identifies MACID0 AP peer and enables firmware rate control");
    require(rtwn_read_2(&s,R92C_RXFLTMAP1)==0 && !(rtwn_read_4(&s,R92C_RCR)&R92C_RCR_ACF),"legacy RUN does not open control reception");
    s.commands.clear();
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_INIT,0)==0,"leaving RUN succeeds");
    require(s.commands.size()==1 && s.commands[0].id==0x01 && s.commands[0].payload==std::vector<uint8_t>({0x20,0,0}),"leaving RUN disconnects firmware AP peer");
    require(!s.fw_station_connected && !s.rate_config.rate_mask,"leaving RUN clears firmware rate eligibility");
    ni.ni_flags=IEEE80211_NODE_HT|IEEE80211_NODE_HT_SGI20;ni.ni_rxmcs[0]=0xff;ni.ni_rxmcs[1]=0xff;
    s.commands.clear();
    rtwn_write_4(&s,R92C_RCR,rtwn_read_4(&s,R92C_RCR)|R92C_RCR_ACF);
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==0,"HT20 RUN succeeds");
    require(s.commands[0].payload==std::vector<uint8_t>({0,0x82,0,0xff,0xff,0xff,0x0f}),"HT20 two-stream MCS0-15 and negotiated SGI use exact generation-2 wire bytes");
    require(s.rate_config.ht && s.rate_config.tx_streams==2 && s.rate_config.initial_rate==27,"HT state enables two transmit streams and MCS15");
    require(s.registers[R92C_RXFLTMAP1]==0 && s.registers[R92C_RXFLTMAP1+1]==1,"HT RX admits only BAR control subtype");
    require(!(rtwn_read_4(&s,R92C_RCR)&R92C_RCR_ACF),"HT subtype filter is not bypassed by accepting every control frame");
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_INIT,0)==0,"HT shutdown succeeds");
    require(rtwn_read_2(&s,R92C_RXFLTMAP1)==0 && !(rtwn_read_4(&s,R92C_RCR)&R92C_RCR_ACF),"HT shutdown closes the control subtype filter");
    s.commands.clear();s.ntxchains=1;
    require(rtwn_ra_init(&s)==0 && s.commands[0].payload==std::vector<uint8_t>({0,0x83,0,0xff,0xff,0x0f,0}),"1T2R bond limits transmit rates to MCS0-7 and 1SS RAID");
    s.commands.clear();s.ntxchains=2;ni.ni_rxmcs[0]=0x11;ni.ni_rxmcs[1]=0;
    require(rtwn_ra_init(&s)==0 && s.rate_config.rate_mask==0x00011fffU && s.rate_config.raid==3,"peer receive MCS intersection preserves disabled MCS bits");
    ni.ni_flags=0;s.commands.clear();
    require(rtwn_ra_init(&s)==0 && !s.rate_config.ht && !s.rate_config.sgi20 && s.rate_config.rate_mask==0xfffU,"beacon capabilities without negotiated HT retain legacy rates");
    ni.ni_flags=IEEE80211_NODE_HT|IEEE80211_NODE_HT_SGI20;ni.ni_rxmcs[0]=0xff;ni.ni_rxmcs[1]=0xff;
    for(int failed_id : {0x40,0x01}) {
        s.sc_ic.ic_state=IEEE80211_S_ASSOC;s.ledlink=0;s.fw_station_connected=0;s.fw_error=ETIMEDOUT;s.fw_fail_id=failed_id;s.commands.clear();
        require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==ETIMEDOUT,"mailbox failures propagate from RUN initialization");
        require(s.sc_ic.ic_state==IEEE80211_S_ASSOC && !s.ledlink && !s.fw_station_connected && !s.rate_config.rate_mask,"failed firmware configuration cannot expose a connected HT station");
        require(s.registers[R92C_RXFLTMAP2]==0 && s.registers[R92C_RXFLTMAP2+1]==0,"failed RUN disables data reception");
    }
    s.fw_error=0;s.fw_fail_id=-1;s.ntxchains=0;s.commands.clear();
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==EINVAL && s.commands.empty(),"invalid hardware stream configuration fails before sending firmware commands");
    s.ntxchains=2;s.fw_station_connected=1;s.sc_ic.ic_state=IEEE80211_S_RUN;s.fw_error=ETIMEDOUT;s.fw_fail_id=0x01;
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_INIT,0)==ETIMEDOUT && s.sc_ic.ic_state==IEEE80211_S_INIT,"INIT still tears down host state when firmware disconnect fails");
    require(!s.fw_station_connected && !s.rate_config.rate_mask,"failed disconnect clears host eligibility");
    rtw_92e_rate_config c{};uint8_t mcs[2]={0xff,0xff},wire[7]{};
    require(rtw_92e_build_rate_config(0xff0,mcs,2,1,1,1,&c) && c.raid==4,"OFDM-only HT20 two-stream peer uses GN RAID4");
    require(rtw_92e_build_rate_config(0xff0,mcs,1,1,1,1,&c) && c.raid==5,"OFDM-only HT20 one-stream peer uses GN RAID5");
    require(rtw_92e_build_rate_config(0xff0,nullptr,1,0,1,1,&c) && c.raid==7,"OFDM-only legacy peer uses G RAID7");
    require(rtw_92e_build_rate_config(0x0f,nullptr,2,0,1,1,&c) && c.raid==8,"CCK-only legacy peer uses B RAID8");
    for(unsigned local=0;local<2;local++) for(unsigned peer=0;peer<2;peer++) {
        require(rtw_92e_build_rate_config(0xfff,mcs,2,1,local,peer,&c),"SGI combination has a valid rate mask");
        require(c.sgi20==(local&&peer),"SGI20 requires both hardware and negotiated peer support");
    }
    mcs[0]=0;mcs[1]=0xff;
    require(rtw_92e_build_rate_config(0xfff,mcs,2,1,1,1,&c) && !c.ht,"invalid peer without mandatory first-stream MCS falls back to legacy");
    require(!rtw_92e_build_rate_config(0,mcs,2,1,1,1,&c) && !c.rate_mask,"empty legacy mask fails closed");
    require(!rtw_92e_build_rate_config(0x1000,mcs,2,1,1,1,&c),"unknown legacy rate bits are rejected");
    require(!rtw_92e_build_rate_config(0xfff,nullptr,2,1,1,1,&c),"HT requires peer rate evidence");
    require(!rtw_92e_make_macid_config(nullptr,0,wire) && !rtw_92e_make_macid_config(&c,0,wire),"invalid firmware configuration is rejected");
    require(!rtw_92e_make_media_status(2,0,wire) && !rtw_92e_make_media_status(1,128,wire),"media report rejects invalid state and MACID");
    mcs[0]=0xff;mcs[1]=0xff;
    require(rtw_92e_build_rate_config(0xfff,mcs,2,1,1,1,&c),"valid config exists for serializer validation");
    c.raid=0;
    require(!rtw_92e_make_macid_config(&c,0,wire),"HT40 firmware rate tables cannot enter the HT20 command");
    c.raid=8;
    require(!rtw_92e_make_macid_config(&c,0,wire),"CCK-only firmware table cannot carry negotiated MCS mask");
    s.fw_error=0;s.io_error=0;s.byte_writes.clear();s.registers[R92C_AMPDU_MIN_SPACE]=0xa8;
    const uint8_t expected[4][4]={{0x41,0x44,0x42,0x44},{0x41,0x88,0x72,0x88},{0x41,0xa8,0x72,0xb9},{0x41,0xa8,0x72,0xb9}};
    for(unsigned factor=0;factor<4;factor++) for(unsigned density=0;density<8;density++) {
        require(rtwn_r92e_ampdu_params(&s,factor,density)==0,"valid peer AMPDU parameters configure hardware");
        require(std::memcmp(s.registers+R92C_AGGLEN_LMT,expected[factor],4)==0,"aggregation limit nibbles match pinned Linux factor algorithm");
        require(s.registers[R92C_AMPDU_MIN_SPACE]==(0xa8U|density),"peer minimum spacing preserves unrelated hardware bits");
    }
    auto writes=s.byte_writes.size();
    require(rtwn_r92e_ampdu_params(&s,4,0)==EINVAL && rtwn_r92e_ampdu_params(&s,0,8)==EINVAL && s.byte_writes.size()==writes,"invalid aggregation factors and densities do not touch hardware");
    s.io_error=EIO;
    require(rtwn_r92e_ampdu_params(&s,3,7)==EIO,"AMPDU register failures propagate to BA setup");
    ieee80211_channel primary,extension;extension.ic_freq=2432;
    rtwn_set_chan(&s,&primary,&extension);
    require(!s.extension_power,"unflagged channel prevents extension-channel transmit power programming");
    require(!(rtwn_read_2(&s,R92C_WMAC_TRXPTCL_CTL)&R92C_WMAC_TRXPTCL_CTL_BW_MASK) && !(rtwn_read_4(&s,R92C_FPGA0_RFMOD)&R92C_RFMOD_40MHZ),"invalid channel direction cannot enable HT40");
    require((s.rf_output[0]&R88E_RF_CHNLBW_BW20)==R88E_RF_CHNLBW_BW20 && (s.rf_output[1]&R88E_RF_CHNLBW_BW20)==R88E_RF_CHNLBW_BW20,"both RF paths retain20MHz bandwidth");
    s.io_error=0;
    /* Real RUN negotiation, including channel direction and regulatory guard. */
    for(int ch=1;ch<=11;ch++){
        auto&c=s.sc_ic.ic_channels[ch];c.ic_freq=2407U+5U*ch;
        c.ic_flags=IEEE80211_CHAN_2GHZ|(ch<=7?IEEE80211_CHAN_HT40U:0U)|(ch>=5?IEEE80211_CHAN_HT40D:0U);
    }
    ni.ni_chan=&s.sc_ic.ic_channels[6];ni.ni_htcaps=IEEE80211_HTCAP_CBW20_40;
    ni.ni_flags=IEEE80211_NODE_HT|IEEE80211_NODE_HT_SGI20|IEEE80211_NODE_HT_SGI40;
    ni.ni_chw=IEEE80211_CHAN_WIDTH_40;ni.ni_htop0=IEEE80211_HTOP0_SCO_SCA|IEEE80211_HTOP0_CHW;
    s.sc_ic.ic_state=IEEE80211_S_ASSOC;s.commands.clear();
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==0,"HT40 channel6 extension above associates");
    require(s.channel_bw40&&s.extension_power&&s.rate_config.bw40&&s.rate_config.sgi40&&!s.rate_config.sgi20,
        "negotiated 40 MHz and SGI40 update radio, power and firmware together");
    require((s.rf_output[0]&0xffU)==8U&&(s.rf_output[1]&0xffU)==8U,"40 MHz radio uses RF center channel8 for primary6+secondary10");
    require((s.rf_output[0]&R88E_RF_CHNLBW_BW20)==R92E_RF_CHNLBW_BW40&&
        (s.rf_output[1]&R88E_RF_CHNLBW_BW20)==R92E_RF_CHNLBW_BW40,"RTL8192EU HT40 RF width code=0x400, not HT20 or 80 MHz");
    require(s.commands.size()==2&&s.commands[0].payload==std::vector<uint8_t>({0,0x82,1,0xff,0xff,0xff,0x0f}),
        "Linux gen2 HT40 firmware MACID payload uses BW=1 and SGI40 bit");
    require((rtwn_read_2(&s,R92C_WMAC_TRXPTCL_CTL)&R92C_WMAC_TRXPTCL_CTL_BW_MASK)==R92C_WMAC_TRXPTCL_CTL_BW_40,
        "40MHz MAC width follows the associated AP");
    require(rtwn_read_1(&s,R92E_DATA_SC)==2,
        "HT40+ DATA_SUBCHANNEL matches Linux gen2 secondary above code 2");
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_INIT,0)==0,"HT40 teardown succeeds");
    ni.ni_htop0=IEEE80211_HTOP0_SCO_SCB|IEEE80211_HTOP0_CHW;s.commands.clear();
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==0,"HT40 channel6 extension below associates");
    require(s.channel_bw40&&s.extension_power&&(s.rf_output[0]&0xffU)==4U,
        "below-primary extension tunes radio to center channel4");
    require(rtwn_read_1(&s,R92E_DATA_SC)==1,
        "HT40- DATA_SUBCHANNEL matches Linux gen2 secondary below code 1");
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_INIT,0)==0,"HT40 below teardown succeeds");
    /* Width fallback cases: AP HT20, 40-intolerant, user override, or
     * mismatch between secondary offset and permitted local channel. */
    for(unsigned reason=0;reason<7;reason++){
        ni.ni_chw=IEEE80211_CHAN_WIDTH_40;ni.ni_htcaps=IEEE80211_HTCAP_CBW20_40;
        ni.ni_htop0=IEEE80211_HTOP0_SCO_SCA|IEEE80211_HTOP0_CHW;s.sc_ic.ic_userflags=0;
        s.sc_ic.ic_htcaps=IEEE80211_HTCAP_CBW20_40|IEEE80211_HTCAP_SGI20|IEEE80211_HTCAP_SGI40;
        ni.ni_chan=&s.sc_ic.ic_channels[6];
        if(reason==0)ni.ni_chw=IEEE80211_CHAN_WIDTH_20;
        if(reason==1)ni.ni_htcaps|=IEEE80211_HTCAP_40INTOLERANT;
        if(reason==2)s.sc_ic.ic_userflags=IEEE80211_F_NOHT40;
        if(reason==3)ni.ni_htcaps=0;
        if(reason==4){ni.ni_chan=&s.sc_ic.ic_channels[11];ni.ni_htop0=IEEE80211_HTOP0_SCO_SCA|IEEE80211_HTOP0_CHW;}
        if(reason==5)ni.ni_htop0=0;
        if(reason==6)ni.ni_htop0&=~IEEE80211_HTOP0_CHW;
        s.sc_ic.ic_state=IEEE80211_S_ASSOC;s.commands.clear();
        require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==0,"HT40 unsafe association gracefully falls back to HT20");
        require(!s.channel_bw40&&!s.extension_power&&!s.rate_config.bw40&&!s.rate_config.sgi40&&
                !s.commands[0].payload[2],"HT20 fallback is consistent across firmware and RF");
        require(rtwn_read_1(&s,R92E_DATA_SC)==0,
            "HT20 fallback resets the secondary subchannel register");
        require(rtwn_newstate(&s.sc_ic,IEEE80211_S_INIT,0)==0,"HT20 fallback teardown succeeds");
    }
    ni.ni_htcaps=IEEE80211_HTCAP_CBW20_40;ni.ni_chan=&s.sc_ic.ic_channels[6];
    ni.ni_chw=IEEE80211_CHAN_WIDTH_40;ni.ni_htop0=IEEE80211_HTOP0_SCO_SCA|IEEE80211_HTOP0_CHW;s.sc_ic.ic_userflags=0;
    s.commands.clear();s.sc_ic.ic_state=IEEE80211_S_ASSOC;
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==0&&s.rate_config.bw40,"retry after fallback enables 40MHz");
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_INIT,0)==0,"HT40 last teardown succeeds");
    rtw_92e_rate_config width{};uint8_t widest[2]={0xff,0xff},width_payload[7]{};
    for(int l=0;l<=1;l++)for(int peer=0;peer<=1;peer++){
        require(rtw_92e_build_rate_config_width(0xfff,widest,2,1,1,1,1,l,peer,&width),"all SGI40 combinations are valid");
        require(width.bw40 && width.sgi40==(l&&peer) && !width.sgi20,"SGI40 never leaks SGI20");
        require(rtw_92e_make_macid_config(&width,0,width_payload) && width_payload[2]==1 &&
                bool(width_payload[1]&0x80)==bool(l&&peer),"SGI40 and width encode independently");
    }
    require(rtw_92e_build_rate_config_width(0xfff,widest,2,0,1,1,1,1,1,&width) && !width.bw40,
        "non-HT peer never negotiates HT40");
    require(!rtw_92e_build_rate_config_width(0xfff,widest,2,1,1,1,2,1,1,&width),
        "invalid HT40 selector rejected");
    require(rtw_92e_build_rate_config_width(0xfff,widest,2,1,1,1,1,1,1,&width),
        "valid HT40 config exists");
    width.sgi20=1;
    require(!rtw_92e_make_macid_config(&width,0,width_payload),"firmware rejects conflicting SGI modes");

    /* Power table regression: the first HT40 implementation accidentally
     * left the second spatial stream at zero, despite advertising MCS8-15.
     * Keep HT20 calibration byte-for-byte compatible with the old path. */
    ieee80211_channel pwr_primary{},pwr_secondary{};
    pwr_primary.ic_freq=2412;pwr_secondary.ic_freq=2432;
    s.ntxchains=2;
    s.sc_r92e_rom.txpwr_a.ht40_tx_pwr[0]=36;
    s.sc_r92e_rom.txpwr_a.ht20_ofdm_tx_pwr_diff=0x10;
    s.sc_r92e_rom.txpwr_a.pwr_diff[0].ht40_ht20_tx_pwr_diff=0xe3;
    uint16_t pwr[RTWN_POWER_COUNT]{};
    rtwn_r92e_get_txpower(&s,0,&pwr_primary,&pwr_secondary,pwr);
    require(pwr[RTWN_RIDX_MCS0]==36&&pwr[RTWN_RIDX_MCS8]==34&&
            pwr[RTWN_RIDX_MCS15]==34,
        "HT40 calibrated two-stream power uses signed ROM high-nibble differential, not zero");
    rtwn_r92e_get_txpower(&s,0,&pwr_primary,nullptr,pwr);
    require(pwr[RTWN_RIDX_MCS0]==37&&pwr[RTWN_RIDX_MCS8]==40&&
            pwr[RTWN_RIDX_MCS15]==40,
        "HT20 calibrated one-stream/two-stream power retains the legacy ROM offsets");
    s.sc_r92e_rom.txpwr_a.ht40_tx_pwr[0]=63;
    s.sc_r92e_rom.txpwr_a.pwr_diff[0].ht40_ht20_tx_pwr_diff=0x71;
    rtwn_r92e_get_txpower(&s,0,&pwr_primary,&pwr_secondary,pwr);
    require(pwr[RTWN_RIDX_MCS8]==63,
        "HT40 two-stream positive differential is clamped to radio maximum");

    s.io_error=0;rtwn_write_4(&s,R92C_HPON_FSM,1U<<22);
    require(rtwn_read_chipid(&s)==0 && (s.chip&RTWN_CHIP_92C_1T2R),"8191EU1T2R bonding is detected before chain count and MCS advertisement");
    rtwn_write_4(&s,R92C_HPON_FSM,0);
    require(rtwn_read_chipid(&s)==0 && !(s.chip&RTWN_CHIP_92C_1T2R),"8192EU2T2R bonding clears any stale one-stream flag");
    rtwn_write_4(&s,R92C_SYS_CFG,R92C_SYS_CFG_TRP_VAUX_EN);
    require(rtwn_read_chipid(&s)==EIO,"unsupported8192EU test chips fail before HT advertisement");
    std::puts("RTL8192EU HT core tests passed");
}
