/* SPDX-License-Identifier: GPL-2.0-only */
#include "RTL8723BUInternal.h"
static const struct rtl8xxxu_fileops *r23_ops(void);
size_t rtw_8723bu_private_size(void){return sizeof(struct rtw_8723bu_private);}
int rtw_8723bu_init(rtw_8723bu*c,uint16_t vid,uint16_t pid,const rtw_8723bu_bus*bus,const rtw_8723bu_usb*usb,void*storage,size_t n){
 struct rtw_8723bu_private*p;rtw_8723bu_bus saved_bus;rtw_8723bu_usb saved_usb;
 if(!c)return EINVAL;if(bus)saved_bus=*bus;else memset(&saved_bus,0,sizeof(saved_bus));if(usb)saved_usb=*usb;else memset(&saved_usb,0,sizeof(saved_usb));memset(c,0,sizeof(*c));
 bus=&saved_bus;usb=&saved_usb;
 if(!bus->read||!bus->write||!bus->write_region||!bus->delay_us||!storage||n<sizeof(*p)||((uintptr_t)storage%_Alignof(struct rtw_8723bu_private)))return EINVAL;
 {uintptr_t x=(uintptr_t)c,y=(uintptr_t)storage;if(x<=y?y-x<sizeof(*c):x-y<sizeof(*p))return EINVAL;}
 if(!((vid==0x0bda&&pid==0xb720)||(vid==0x7392&&pid==0xa611)))return ENODEV;
 if(!(usb->bulk_in&0x80)||!(usb->bulk_in&15)||usb->bulk_out_count<1||usb->bulk_out_count>3||usb->high_speed>1||usb->bulk_max_packet!=(usb->high_speed?512:64))return EINVAL;
 for(unsigned i=0;i<usb->bulk_out_count;i++){if(!usb->bulk_out[i]||(usb->bulk_out[i]&0xf0))return EINVAL;for(unsigned j=0;j<i;j++)if(usb->bulk_out[i]==usb->bulk_out[j])return EINVAL;}
 memset(c,0,sizeof(*c));memset(storage,0,sizeof(*p));c->bus=*bus;c->usb=*usb;c->private_state=storage;c->vid=vid;c->pid=pid;p=storage;p->owner=c;p->rf_paths=p->tx_paths=p->rx_paths=1;p->rtl_chip=RTL8723B;p->fops=r23_ops();return 0;
}
static int valid(const rtw_8723bu*c){return c&&c->private_state&&((const struct rtw_8723bu_private*)c->private_state)->owner==c;}
static int fail(rtw_8723bu*c,int e){if(e<0)e=-e;if(e&&c){c->calibrated=0;if(!c->last_error)c->last_error=e;}return e;}
static int readreg(rtw_8723bu*c,u16 a,unsigned w,u32*v){int e;*v=0;if(c->last_error)return c->last_error;e=c->bus.read(c->bus.cookie,a,w,v);if(e)*v=0;return fail(c,e);}
int rtw_8723bu_identify(rtw_8723bu*c){
 u32 cfg,multi,gpio;int e;if(!valid(c))return EINVAL;c->identified=c->board_valid=c->calibrated=0;++c->epoch;
 if((e=readreg(c,REG_SYS_CFG,4,&cfg))||(e=readreg(c,REG_MULTI_FUNC_CTRL,4,&multi))||(e=readreg(c,REG_GPIO_OUTSTS,4,&gpio)))return e;
 if(cfg==0xffffffffU||(cfg&SYS_CFG_TRP_VAUX_EN)||!(multi&MULTI_WIFI_FUNC_EN))return fail(c,ENODEV);
 c->board.cut=(u8)((cfg&SYS_CFG_CHIP_VERSION_MASK)>>12);c->board.vendor=(u8)((cfg&SYS_CFG_VENDOR_EXT_MASK)>>18);c->board.rom_revision=(u8)((gpio&GPIO_RF_RL_ID)>>28);
 c->board.has_wifi=(multi&MULTI_WIFI_FUNC_EN)!=0;c->board.has_bluetooth=(multi&MULTI_BT_FUNC_EN)!=0;c->board.has_gps=(multi&MULTI_GPS_FUNC_EN)!=0;c->identified=1;return 0;
}
static int overlaps(const void*a,size_t an,const void*b,size_t bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<an:x-y<bn;}
int rtw_8723bu_decode_efuse(const u8*in,size_t n,u8*out,size_t capacity){
 size_t p=0;if(!in||!out||n!=512||capacity!=512||overlaps(in,n,out,capacity))return EINVAL;memset(out,255,capacity);
 while(p<n){unsigned section,mask;u8 h=in[p++];if(h==255)return 0;
  if((h&31U)==15U){u8 h2;if(p==n)return EINVAL;h2=in[p++];if((h2&15U)==15U)continue;section=((h2&0xf0U)>>1U)|((h>>5U)&7U);mask=h2&15U;}
  else{section=h>>4U;mask=h&15U;}
  for(unsigned word=0;word<4;word++)if(!(mask&(1U<<word))){size_t at=(size_t)section*8U+word*2U;if(n-p<2||at>=capacity||capacity-at<2)return EINVAL;out[at]=in[p++];out[at+1]=in[p++];}
 }return 0;
}
static int signed4(u8 v){return(v&8U)?(int)(v&15U)-16:(int)(v&15U);}
int rtw_8723bu_parse_board(rtw_8723bu*c,const u8*b,size_t n){
 rtw_8723bu_board board;struct rtw_8723bu_private*p;unsigned any=0,allff=1;
 if(!valid(c)||!b||n!=512)return EINVAL;c->board_valid=c->calibrated=0;if(!c->identified)return ENXIO;
 if(b[0]!=0x29||b[1]!=0x81)return EINVAL;board=c->board;memmove(board.logical,b,n);memcpy(board.mac,b+0x107,6);
 for(unsigned i=0;i<6;i++){any|=board.mac[i];if(board.mac[i]!=255)allff=0;}if(!any||allff||(board.mac[0]&1U))return EINVAL;
 board.crystal_cap=b[0xb9]&63U;board.thermal_meter=b[0xba];board.channel_plan=b[0xb8];board.rf_board_option=b[0xc1];board.bt_setting=b[0xc3];board.antenna_option=b[0xc9];board.rfe_option=b[0xca];board.package_type=b[0x1fb];
 memcpy(board.cck_power,b+0x3a,6);memcpy(board.ht40_power,b+0x40,5);board.ofdm_delta=(s8)signed4(b[0x45]);board.ht20_delta=(s8)signed4(b[0x45]>>4);
 p=c->private_state;memmove(p->efuse_wifi.raw,board.logical,512);p->default_crystal_cap=board.crystal_cap;p->chip_cut=board.cut;
 memcpy(p->cck_tx_power_index_B,board.cck_power,6);memcpy(p->ht40_1s_tx_power_index_B,board.ht40_power,5);p->ofdm_tx_power_diff[0].b=board.ofdm_delta;p->ht20_tx_power_diff[0].b=board.ht20_delta;
 c->board=board;c->board_valid=1;return 0;
}
int rtw_8723bu_channel_supported(const rtw_8723bu*c,u8 ch,unsigned bw){unsigned group;if(!valid(c)||!c->identified||!c->board_valid||bw!=20||ch<1||ch>11)return 0;group=ch<=2?0:ch<=5?1:ch<=8?2:3;return c->board.cck_power[group]<=63&&c->board.ht40_power[group]<=63;}
int rtw_8723bu_inspect_firmware(const u8*b,size_t n){return !b||n<=32||n-32>RTW_8723BU_FW_MAX_BYTES||(((unsigned)b[0]|((unsigned)b[1]<<8))&0xfff0U)!=0x5300U?EINVAL:0;}

static int r23_power_clamp(int value){return value<0?0:value>63?63:value;}
#include "RTL8723BUPlatform.inc"
#include "RTL8723BUTables.inc"
#include "RTL8723BUAlgorithms.inc"
static const struct rtl8xxxu_fileops rtl8723_ops={
 .reset_8051=rtl8723bu_reset_8051,.adda_1t_init=0x01c00014,.adda_1t_path_on=0x01c00014,
 .adda_2t_path_on_a=0x01c00014,.adda_2t_path_on_b=0x01c00014,
 .max_aggr_num=0x0c14,.ampdu_max_time=0x5e,.has_s0s1=1,.init_reg_hmtfr=1,
 .page_num_hi=TX_PAGE_NUM_HI_PQ_8723B,.page_num_lo=TX_PAGE_NUM_LO_PQ_8723B,
 .page_num_norm=TX_PAGE_NUM_NORM_PQ_8723B,.total_page_num=TX_TOTAL_PAGE_NUM_8723B,.ustime_tsf_edca=0x50
};

static const struct rtl8xxxu_fileops*r23_ops(void){return &rtl8723_ops;}
static int r23_paused(rtw_8723bu*c,int e){struct rtw_8723bu_private*p=c->private_state;c->calibrated=0;fail(c,e);p->cleanup=true;(void)rtl8xxxu_write8(p,REG_TXPAUSE,0xff);p->cleanup=false;return c->last_error?c->last_error:e;}
int rtw_8723bu_h2c(rtw_8723bu*c,const u8*b,size_t n){
 struct rtw_8723bu_private*p;u32 words[2]={0,0};unsigned attempt;u8 box;
 if(!valid(c)||!b||n<1||n>8)return EINVAL;if(!c->firmware_running)return ENXIO;if(c->last_error)return c->last_error;
 p=c->private_state;box=c->mailbox;for(attempt=0;attempt<100;attempt++){if(!(rtl8xxxu_read8(p,REG_HMTFR)&(1U<<box)))break;if(c->last_error)return c->last_error;}
 if(attempt==100)return fail(c,EBUSY);memcpy(words,b,n);
 if(n>4)(void)rtl8xxxu_write32(p,(u16)(REG_HMBOX_EXT0_8723B+box*4U),words[1]);
 (void)rtl8xxxu_write32(p,(u16)(REG_HMBOX_0+box*4U),words[0]);if(c->last_error)return c->last_error;c->mailbox=(u8)((box+1U)&3U);return 0;
}
static int r23_endpoints(rtw_8723bu*c){
 struct rtw_8723bu_private*p=c->private_state;u16 sie=rtl8xxxu_read16(p,REG_NORMAL_SIE_EP_TX);
 p->ep_tx_count=p->ep_tx_high_queue=p->ep_tx_normal_queue=p->ep_tx_low_queue=0;
 if(sie&NORMAL_SIE_EP_TX_HIGH_MASK){p->ep_tx_high_queue=1;p->ep_tx_count++;}
 if(sie&NORMAL_SIE_EP_TX_NORMAL_MASK){p->ep_tx_normal_queue=1;p->ep_tx_count++;}
 if(sie&NORMAL_SIE_EP_TX_LOW_MASK){p->ep_tx_low_queue=1;p->ep_tx_count++;}
 /* Actual rtl8xxxu no-SIE fallback for descriptor-count1/2/3. */
 if(!p->ep_tx_count){p->ep_tx_count=c->usb.bulk_out_count;p->ep_tx_high_queue=1;p->ep_tx_normal_queue=p->ep_tx_count>=2;p->ep_tx_low_queue=p->ep_tx_count==3;}
 if(c->last_error)return c->last_error;if(p->ep_tx_count!=c->usb.bulk_out_count)return EINVAL;memcpy(p->out_ep,c->usb.bulk_out,c->usb.bulk_out_count);return 0;
}
/* The RTL8723B branches of rtl8xxxu_init_device, with software CCMP and a
 * paused transmitter. Other families' branches are deliberately absent. */
static int r23_mac_phy(rtw_8723bu*c){
 struct rtw_8723bu_private*p=c->private_state;u32 value;u8 boundary=TX_TOTAL_PAGE_NUM_8723B+1;
 for(unsigned i=0;rtl8723b_mac_init_table[i].reg!=0xffff;i++)(void)rtl8xxxu_write8(p,rtl8723b_mac_init_table[i].reg,rtl8723b_mac_init_table[i].val);
 rtl8723bu_phy_init_antenna_selection(p);rtl8723bu_init_phy_bb(p);if(c->last_error)return r23_paused(c,c->last_error);
 /* A first crystal-cap write must not be skipped because cached zero equals
  * an actual zero-valued factory cap. */
 p->cfo_tracking.crystal_cap=255;rtl8723a_set_crystal_cap(p,p->default_crystal_cap);
 if(rtl8723bu_init_phy_rf(p))return r23_paused(c,c->last_error?c->last_error:EIO);
 value=FPGA0_RF_TRSW|FPGA0_RF_TRSWB|FPGA0_RF_ANTSW|FPGA0_RF_ANTSWB|((FPGA0_RF_ANTSW|FPGA0_RF_ANTSWB)<<FPGA0_RF_BD_CTRL_SHIFT)|FPGA0_RF_PAPE|(FPGA0_RF_PAPE<<FPGA0_RF_BD_CTRL_SHIFT);
 (void)rtl8xxxu_write32(p,REG_FPGA0_XAB_RF_SW_CTRL,value);(void)rtl8xxxu_write32(p,REG_FPGA0_XA_RF_INT_OE,0x66f60210);
 (void)rtl8xxxu_write8(p,REG_TXPKTBUF_BCNQ_BDNY,boundary);(void)rtl8xxxu_write8(p,REG_TXPKTBUF_MGQ_BDNY,boundary);
 (void)rtl8xxxu_write8(p,REG_TXPKTBUF_WMAC_LBK_BF_HD,boundary);(void)rtl8xxxu_write8(p,REG_TRXFF_BNDY,boundary);(void)rtl8xxxu_write8(p,REG_TDECTRL+1,boundary);
 (void)rtl8xxxu_write8(p,REG_PBP,(PBP_PAGE_SIZE_256<<PBP_PAGE_SIZE_RX_SHIFT)|(PBP_PAGE_SIZE_256<<PBP_PAGE_SIZE_TX_SHIFT));
 if(rtl8xxxu_auto_llt_table(p))return r23_paused(c,c->last_error?c->last_error:ETIMEDOUT);rtl8xxxu_gen2_usb_quirks(p);
 (void)rtl8xxxu_write8(p,REG_TX_REPORT_CTRL,rtl8xxxu_read8(p,REG_TX_REPORT_CTRL)|TX_REPORT_CTRL_TIMER_ENABLE);
 (void)rtl8xxxu_write8(p,REG_TX_REPORT_CTRL+1,2);(void)rtl8xxxu_write16(p,REG_TX_REPORT_TIME,0xcdf0);
 (void)rtl8xxxu_write8(p,0xa3,rtl8xxxu_read8(p,0xa3)&0xf8U);(void)rtl8xxxu_write8(p,REG_RX_DRVINFO_SZ,4);
 (void)rtl8xxxu_write32(p,REG_HISR,0xffffffffU);(void)rtl8xxxu_write32(p,REG_HIMR,0xffffffffU);
 value=RCR_ACCEPT_PHYS_MATCH|RCR_ACCEPT_MCAST|RCR_ACCEPT_BCAST|RCR_ACCEPT_MGMT_FRAME|RCR_HTC_LOC_CTRL|RCR_APPEND_PHYSTAT|RCR_APPEND_ICV|RCR_APPEND_MIC|RCR_APPEND_FCS;
 (void)rtl8xxxu_write32(p,REG_RCR,value);(void)rtl8xxxu_write32(p,REG_MAR,0xffffffffU);(void)rtl8xxxu_write32(p,REG_MAR+4,0xffffffffU);
 value=rtl8xxxu_read32(p,REG_RESPONSE_RATE_SET);value&=~RESPONSE_RATE_BITMAP_ALL;value|=RESPONSE_RATE_RRSR_CCK_ONLY_1M;(void)rtl8xxxu_write32(p,REG_RESPONSE_RATE_SET,value);
 (void)rtl8xxxu_write16(p,REG_SPEC_SIFS,0x100a);(void)rtl8xxxu_write16(p,REG_RETRY_LIMIT,0x3030);
 (void)rtl8xxxu_write16(p,REG_MAC_SPEC_SIFS,0x100a);(void)rtl8xxxu_write16(p,REG_SIFS_CCK,0x100a);(void)rtl8xxxu_write16(p,REG_SIFS_OFDM,0x100a);
 (void)rtl8xxxu_write32(p,REG_EDCA_BE_PARAM,0x005ea42b);(void)rtl8xxxu_write32(p,REG_EDCA_BK_PARAM,0x0000a44f);(void)rtl8xxxu_write32(p,REG_EDCA_VI_PARAM,0x005ea324);(void)rtl8xxxu_write32(p,REG_EDCA_VO_PARAM,0x002fa226);
 (void)rtl8xxxu_write32(p,REG_DARFRC,0);(void)rtl8xxxu_write32(p,REG_DARFRC+4,0x10080404);(void)rtl8xxxu_write32(p,REG_RARFRC,0x04030201);(void)rtl8xxxu_write32(p,REG_RARFRC+4,0x08070605);
 (void)rtl8xxxu_write8(p,REG_FWHW_TXQ_CTRL,rtl8xxxu_read8(p,REG_FWHW_TXQ_CTRL)|FWHW_TXQ_CTRL_AMPDU_RETRY);(void)rtl8xxxu_write8(p,REG_ACKTO,0x40);
 (void)rtl8xxxu_write16(p,REG_BEACON_CTRL,BEACON_DISABLE_TSF_UPDATE|(BEACON_DISABLE_TSF_UPDATE<<8));(void)rtl8xxxu_write16(p,REG_TBTT_PROHIBIT,0x6404);
 (void)rtl8xxxu_write8(p,REG_DRIVER_EARLY_INT,DRIVER_EARLY_INT_TIME);(void)rtl8xxxu_write8(p,REG_BEACON_DMA_TIME,BEACON_DMA_ATIME_INT_TIME);(void)rtl8xxxu_write16(p,REG_BEACON_TCFG,0x660f);
 rtl8xxxu_init_burst(p);rtl8723bu_init_aggregation(p);
 value=rtl8xxxu_read32(p,REG_FPGA0_RF_MODE)|FPGA_RF_MODE_CCK|FPGA_RF_MODE_OFDM;(void)rtl8xxxu_write32(p,REG_FPGA0_RF_MODE,value);
 (void)rtl8xxxu_write32(p,REG_CAM_CMD,CAM_CMD_POLLING|BIT(30));(void)rtl8xxxu_write16(p,REG_SECURITY_CFG,0);
 (void)rtl8xxxu_write8(p,REG_LEDCFG2,rtl8xxxu_read8(p,REG_LEDCFG2)|LEDCFG2_DPDT_SELECT);(void)rtl8xxxu_write8(p,REG_HWSEQ_CTRL,0xff);
 (void)rtl8xxxu_write32(p,REG_BAR_MODE_CTRL,0x0201ffff);(void)rtl8xxxu_write16(p,REG_FAST_EDCA_CTRL,0);rtl8723bu_init_statistics(p);
 (void)rtl8xxxu_write_rfreg(p,RF_A,RF6052_REG_T_METER_8723B,0x37cf8);(void)rtl8xxxu_write8(p,REG_NAV_UPPER,(30000+NAV_UPPER_UNIT-1)/NAV_UPPER_UNIT);
 (void)rtl8xxxu_write32(p,REG_FWHW_TXQ_CTRL,rtl8xxxu_read32(p,REG_FWHW_TXQ_CTRL)|FWHW_TXQ_CTRL_XMIT_MGMT_ACK);
 for(unsigned i=0;i<6;i++)(void)rtl8xxxu_write8(p,(u16)(REG_MACID+i),c->board.mac[i]);
 if(c->last_error)return r23_paused(c,c->last_error);c->mac_initialized=c->phy_loaded=1;return 0;
}
int rtw_8723bu_boot(rtw_8723bu*c,const u8*fw,size_t n){
 struct rtw_8723bu_private*p;int e;if(!valid(c))return EINVAL;if((e=rtw_8723bu_inspect_firmware(fw,n)))return e;
 if(!c->identified||!c->board_valid)return ENXIO;if(c->last_error)return c->last_error;p=c->private_state;
 c->powered=c->firmware_running=c->mac_initialized=c->phy_loaded=c->calibrated=c->associated=0;c->channel=0;++c->epoch;
 if((e=r23_endpoints(c)))return fail(c,e);
 p->power_attempted=true;e=rtl8723bu_power_on(p);if(e||c->last_error)return r23_paused(c,c->last_error?c->last_error:(e<0?-e:e));c->powered=1;
 (void)rtl8xxxu_write8(p,REG_TXPAUSE,0xff);rtl8xxxu_init_queue_reserved_page(p);
 if((e=rtl8xxxu_init_queue_priority(p)))return r23_paused(c,-e);(void)rtl8xxxu_write16(p,REG_TRXFF_BNDY+2,0x3f7f);
 p->fw_data=(const struct rtl8xxxu_firmware_header*)fw;p->fw_size=n-32;
 e=rtl8xxxu_download_firmware(p);if(!e&&!c->last_error)e=rtl8xxxu_start_firmware(p);p->fw_data=NULL;p->fw_size=0;
 if(e||c->last_error){
  int original=c->last_error?c->last_error:(e<0?-e:e);
  /* One bounded idempotent cleanup also covers failure of the loader's own
   * disable write. No firmware payload is retried after an I/O failure. */
  p->cleanup=true;u16 control=rtl8xxxu_read16(p,REG_MCU_FW_DL);(void)rtl8xxxu_write16(p,REG_MCU_FW_DL,control&~MCU_FW_DL_ENABLE);p->cleanup=false;
  return r23_paused(c,original);
 }c->firmware_running=1;c->mailbox=0;
 c->firmware_version=(u16)((unsigned)fw[4]|((unsigned)fw[5]<<8));c->firmware_subversion=fw[6];
 if((e=r23_mac_phy(c)))return e;rtl8723b_enable_rf(p);if(c->last_error)return r23_paused(c,c->last_error);
 (void)rtl8xxxu_write8(p,REG_TXPAUSE,0xff);return c->last_error;
}
int rtw_8723bu_set_channel(rtw_8723bu*c,u8 channel,unsigned bandwidth){
 struct rtw_8723bu_private*p;struct rtw_8723bu_channel ch;struct rtw_8723bu_hw hw;
 if(!valid(c)||channel<1||channel>11||bandwidth!=20)return EINVAL;
 if(!rtw_8723bu_channel_supported(c,channel,bandwidth))return ENOTSUP;
 if(!c->powered||!c->firmware_running||!c->mac_initialized||!c->phy_loaded)return ENXIO;if(c->last_error)return c->last_error;
 p=c->private_state;c->calibrated=0;(void)rtl8xxxu_write8(p,REG_TXPAUSE,0xff);
 memset(&hw,0,sizeof(hw));ch.hw_value=channel;ch.center_freq=2407U+5U*channel;hw.priv=p;hw.conf.chandef.chan=&ch;hw.conf.chandef.width=NL80211_CHAN_WIDTH_20;hw.conf.chandef.center_freq1=ch.center_freq;
 rtl8xxxu_gen2_config_channel(&hw);rtl8723b_set_tx_power(p,channel,false);if(c->last_error)return r23_paused(c,c->last_error);c->channel=channel;return 0;
}
static void r23_release_calibration(struct rtw_8723bu_private*p){int saved=p->owner->last_error;const u8 command[]={H2C_8723B_BT_WLAN_CALIBRATION,0};p->owner->last_error=0;(void)rtw_8723bu_h2c(p->owner,command,2);if(saved)p->owner->last_error=saved;}
int rtw_8723bu_calibrate(rtw_8723bu*c){
 struct rtw_8723bu_private*p;u32 bt,sw;int e;unsigned attempt;
 if(!valid(c))return EINVAL;if(!c->powered||!c->firmware_running||!c->phy_loaded||!rtw_8723bu_channel_supported(c,c->channel,20))return ENXIO;if(c->last_error)return c->last_error;
 p=c->private_state;c->calibrated=0;p->iqk_ok=false;(void)rtl8xxxu_write8(p,REG_TXPAUSE,0xff);
 bt=rtl8xxxu_read32(p,REG_BT_CONTROL_8723BU);sw=rtl8xxxu_read32(p,REG_S0S1_PATH_SWITCH);if(c->last_error)return r23_paused(c,c->last_error);
 rtl8723a_phy_lc_calibrate(p);if(c->last_error)goto failed;
 for(attempt=0;attempt<20;attempt++){if(!(rtl8xxxu_read_rfreg(p,RF_A,RF6052_REG_MODE_AG)&0x8000U))break;if(c->last_error)goto failed;r23_delay(p,5000);}
 if(attempt==20)goto failed;
 rtl8723bu_phy_iq_calibrate(p);if(c->last_error||!p->iqk_ok)goto failed;
 if((rtl8xxxu_read_rfreg(p,RF_A,RF6052_REG_MODE_AG)&(MODE_AG_CHANNEL_MASK|MODE_AG_BW_MASK))!=((u32)c->channel|MODE_AG_BW_20MHZ_8723B)){fail(c,EIO);goto failed;}
 (void)rtl8xxxu_write8(p,REG_TXPAUSE,0xff);if(c->last_error)goto failed;c->calibrated=1;p->last_thermal=c->board.thermal_meter;return 0;
failed:
 e=c->last_error?c->last_error:ETIMEDOUT;p->cleanup=true;
 (void)rtl8xxxu_write32(p,REG_BT_CONTROL_8723BU,bt);(void)rtl8xxxu_write32(p,REG_S0S1_PATH_SWITCH,sw);p->cleanup=false;
 r23_release_calibration(p);if(c->last_error)return r23_paused(c,e);
 /* A completed calibration failure can be retried while the bus remains
  * healthy; a transport/restore failure stays sticky until reinitialization. */
 p->cleanup=true;(void)rtl8xxxu_write8(p,REG_TXPAUSE,0xff);p->cleanup=false;return c->last_error?c->last_error:e;
}
int rtw_8723bu_stop(rtw_8723bu*c){
 struct rtw_8723bu_private*p;int saved;if(!valid(c))return EINVAL;p=c->private_state;saved=c->last_error;c->calibrated=c->associated=0;++c->epoch;
 p->cleanup=true;(void)rtl8xxxu_write8(p,REG_TXPAUSE,0xff);if(c->powered||p->power_attempted)rtl8723bu_power_off(p);p->cleanup=false;p->power_attempted=false;
 c->powered=c->firmware_running=c->mac_initialized=c->phy_loaded=0;c->channel=0;c->rssi_valid=0;return saved?saved:c->last_error;
}
static int r23_otp_byte(struct rtw_8723bu_private*p,unsigned offset,u8*out){
 u32 value;unsigned attempt;if(offset>=512)return EOVERFLOW;
 (void)rtl8xxxu_write8(p,REG_EFUSE_CTRL+1,(u8)offset);
 (void)rtl8xxxu_write8(p,REG_EFUSE_CTRL+2,(u8)((rtl8xxxu_read8(p,REG_EFUSE_CTRL+2)&0xfcU)|((offset>>8)&3U)));
 (void)rtl8xxxu_write8(p,REG_EFUSE_CTRL+3,rtl8xxxu_read8(p,REG_EFUSE_CTRL+3)&0x7fU);
 for(attempt=0;attempt<RTL8XXXU_MAX_REG_POLL;attempt++){value=rtl8xxxu_read32(p,REG_EFUSE_CTRL);if(p->owner->last_error)return p->owner->last_error;if(value&BIT(31))break;}
 if(attempt==RTL8XXXU_MAX_REG_POLL)return ETIMEDOUT;r23_delay(p,50);value=rtl8xxxu_read32(p,REG_EFUSE_CTRL);if(p->owner->last_error)return p->owner->last_error;*out=(u8)value;return 0;
}
int rtw_8723bu_read_board(rtw_8723bu*c){
 struct rtw_8723bu_private*p;u32 bank;u16 value;unsigned offset=0;int e=0;u8 header,ext,byte;
 if(!valid(c))return EINVAL;if(!c->identified)return ENXIO;if(c->powered||c->firmware_running)return EBUSY;if(c->last_error)return c->last_error;p=c->private_state;c->board_valid=c->calibrated=0;
 bank=rtl8xxxu_read32(p,REG_EFUSE_TEST);if(c->last_error)return c->last_error;
 /* WiFi bank only, with the2.5V programming supply explicitly disabled. */
 (void)rtl8xxxu_write32(p,REG_EFUSE_TEST,bank&~(EFUSE_SELECT_MASK|BIT(31)));
 (void)rtl8xxxu_write8(p,REG_EFUSE_ACCESS,EFUSE_ACCESS_ENABLE);
 value=rtl8xxxu_read16(p,REG_SYS_ISO_CTRL);(void)rtl8xxxu_write16(p,REG_SYS_ISO_CTRL,value|SYS_ISO_PWC_EV12V);
 value=rtl8xxxu_read16(p,REG_SYS_FUNC);(void)rtl8xxxu_write16(p,REG_SYS_FUNC,value|SYS_FUNC_ELDR);
 value=rtl8xxxu_read16(p,REG_SYS_CLKR);(void)rtl8xxxu_write16(p,REG_SYS_CLKR,value|SYS_CLK_LOADER_ENABLE|SYS_CLK_ANA8M);
 memset(p->efuse_wifi.raw,255,512);if(c->last_error){e=c->last_error;goto done;}
 while(offset<512){unsigned section,mask;
  if((e=r23_otp_byte(p,offset++,&header)))goto done;if(header==255)break;
  if((header&31U)==15U){if((e=r23_otp_byte(p,offset++,&ext)))goto done;if((ext&15U)==15U)continue;section=((header>>5U)&7U)|((ext&0xf0U)>>1U);mask=ext&15U;}
  else{section=header>>4U;mask=header&15U;}
  for(unsigned word=0;word<4;word++)if(!(mask&(1U<<word))){unsigned at=section*8U+word*2U;if(at>=511){e=EINVAL;goto done;}if((e=r23_otp_byte(p,offset++,&byte)))goto done;p->efuse_wifi.raw[at]=byte;if((e=r23_otp_byte(p,offset++,&byte)))goto done;p->efuse_wifi.raw[at+1]=byte;}
 }
done:
 p->cleanup=true;(void)rtl8xxxu_write8(p,REG_EFUSE_ACCESS,EFUSE_ACCESS_DISABLE);
 /* Restore the bank without enabling OTP programming voltage. */
 (void)rtl8xxxu_write32(p,REG_EFUSE_TEST,bank&~BIT(31));p->cleanup=false;
 if(!e)e=c->last_error;if(e)return fail(c,e);return rtw_8723bu_parse_board(c,p->efuse_wifi.raw,512);
}
int rtw_8723bu_prepare_rates(u16 legacy,u8 mcs,int ht,int sgi,rtw_8723bu_rates*out){
 rtw_8723bu_rates rates;u32 mask;u8 raid;
 if(!out||!legacy||(legacy&0xf000U)||(ht!=0&&ht!=1)||(sgi!=0&&sgi!=1)||(ht&&!mcs)||(!ht&&(mcs||sgi)))return EINVAL;
 memset(&rates,0,sizeof(rates));mask=legacy|((u32)(ht?mcs:0)<<12);raid=ht?((legacy&15U)?3:5):((legacy&15U)?((legacy&0xff0U)?6:8):7);
 rates.mask=mask;rates.raid=raid;rates.sgi=(u8)sgi;rates.command[0]=H2C_8723B_MACID_CFG_RAID;rates.command[1]=2;rates.command[2]=(u8)(raid|(sgi?128U:0U));
 rates.command[4]=(u8)mask;rates.command[5]=(u8)(mask>>8);rates.command[6]=(u8)(mask>>16);rates.command[7]=(u8)(mask>>24);*out=rates;return 0;
}
static int r23_mac_valid(const u8*mac){unsigned any=0;for(unsigned i=0;i<6;i++)any|=mac[i];return any&&!(mac[0]&1U);}
int rtw_8723bu_station_state(rtw_8723bu*c,unsigned state,const rtw_8723bu_station*s){
 struct rtw_8723bu_private*p;rtw_8723bu_rates rates;u32 rcr,rrsr;u8 media[4]={H2C_8723B_MEDIA_STATUS_RPT,0x20,2,0};int e;
 if(!valid(c)||state>3||(s&&s->passive_scan>1))return EINVAL;
 if(state>=2&&(!s||!r23_mac_valid(s->bssid)||s->channel!=c->channel||!s->basic_rates||(s->basic_rates&~s->legacy_rates)||(s->basic_rates&0xf000U)||s->short_slot>1||s->short_preamble>1||s->ht>1||s->sgi20>1))return EINVAL;
 if(state==3&&(!s->aid||s->aid>2007||!s->beacon_tu||(e=rtw_8723bu_prepare_rates(s->legacy_rates,s->mcs,s->ht,s->sgi20,&rates))))return EINVAL;
 if(!c->mac_initialized||!c->firmware_running)return state==0?0:ENXIO;if(c->last_error)return c->last_error;
 if((state>=2||(state==1&&(!s||!s->passive_scan)))&&(!c->calibrated||!rtw_8723bu_channel_supported(c,c->channel,20)))return ENXIO;
 p=c->private_state;
 if(state!=3&&c->associated){e=rtw_8723bu_h2c(c,media,sizeof(media));if(e)return r23_paused(c,e);}c->associated=0;
 rcr=rtl8xxxu_read32(p,REG_RCR)&~(RCR_CHECK_BSSID_MATCH|RCR_CHECK_BSSID_BEACON|RCR_ACCEPT_CTRL_FRAME);
 /* regs.h: CTRL_FRAME would bypass RXFLTMAP1. Keep it clear so only
  * the requested BAR subtype reaches software reorder in HT RUN. */
 if(state==3)rcr|=RCR_CHECK_BSSID_MATCH|RCR_CHECK_BSSID_BEACON;
 (void)rtl8xxxu_write32(p,REG_RCR,rcr);(void)rtl8xxxu_write8(p,REG_MSR,(u8)((rtl8xxxu_read8(p,REG_MSR)&~MSR_LINKTYPE_MASK)|(state==3?MSR_LINKTYPE_STATION:MSR_LINKTYPE_NONE)));
 (void)rtl8xxxu_write16(p,0x6a0,0xffff);(void)rtl8xxxu_write16(p,0x6a2,state==3&&s->ht?0x100:0);(void)rtl8xxxu_write16(p,0x6a4,state==3?0xffff:0);
 if(state==1&&(!s||!s->passive_scan)){
  /* Restore the primary scan response bitmap (CCK1M+OFDM/HT) after an
   * OFDM-only basic-rate AP, before allowing probe traffic. */
  rrsr=rtl8xxxu_read32(p,REG_RESPONSE_RATE_SET);rrsr=(rrsr&~(0xfffffU|BIT(23)))|RESPONSE_RATE_RRSR_CCK_ONLY_1M;(void)rtl8xxxu_write32(p,REG_RESPONSE_RATE_SET,rrsr);
 }
 if(state>=2){
  /* RRSR is written before management TX may resume. Keep upper response
   * controls while replacing all legacy/HT bitmap bits and preamble select. */
  rrsr=rtl8xxxu_read32(p,REG_RESPONSE_RATE_SET);rrsr=(rrsr&~(0xfffffU|BIT(23)))|s->basic_rates|(s->short_preamble?BIT(23):0U);(void)rtl8xxxu_write32(p,REG_RESPONSE_RATE_SET,rrsr);
  for(unsigned i=0;i<6;i++)(void)rtl8xxxu_write8(p,(u16)(REG_BSSID+i),s->bssid[i]);(void)rtl8xxxu_write8(p,REG_SLOT,s->short_slot?9:20);
 }
 if(c->last_error)return r23_paused(c,c->last_error);
 if(state==3){
  (void)rtl8xxxu_write16(p,REG_BCN_PSR_RPT,(u16)(0xc000U|s->aid));(void)rtl8xxxu_write16(p,REG_BCN_INTERVAL,s->beacon_tu);
  if((e=rtw_8723bu_h2c(c,rates.command,8)))return r23_paused(c,e);media[1]=0x21;if((e=rtw_8723bu_h2c(c,media,sizeof(media))))return r23_paused(c,e);
 }
 (void)rtl8xxxu_write8(p,REG_TXPAUSE,state==0||(state==1&&s&&s->passive_scan)?255:state==1?15:0);
 if(c->last_error)return r23_paused(c,c->last_error);c->associated=(u8)(state==3);return 0;
}
int rtw_8723bu_set_edca(rtw_8723bu*c,int short_slot,const rtw_8723bu_edca*ac){
 static const u16 address[4]={REG_EDCA_BE_PARAM,REG_EDCA_BK_PARAM,REG_EDCA_VI_PARAM,REG_EDCA_VO_PARAM};struct rtw_8723bu_private*p;u32 value[4];
 if(!valid(c)||!ac||(short_slot!=0&&short_slot!=1))return EINVAL;
 for(unsigned i=0;i<4;i++){unsigned aifs=10U+(short_slot?9U:20U)*ac[i].aifsn;if(!ac[i].aifsn||ac[i].aifsn>15||ac[i].ecwmin>15||ac[i].ecwmax>15||ac[i].ecwmin>ac[i].ecwmax||aifs>255)return EINVAL;value[i]=((u32)ac[i].txop_32us<<16)|((u32)ac[i].ecwmax<<12)|((u32)ac[i].ecwmin<<8)|aifs;}
 if(!c->calibrated||!c->associated||!c->mac_initialized)return ENXIO;if(c->last_error)return c->last_error;p=c->private_state;
 /* Pinned 8723B HW_VAR_AC_PARAM_* and rtl8xxxu_conf_tx replace the full
  * word using the u16 TXOP at bit16. Do not borrow 3081 reserved-bit masks. */
 for(unsigned i=0;i<4;i++)(void)rtl8xxxu_write32(p,address[i],value[i]);return c->last_error?r23_paused(c,c->last_error):0;
}
int rtw_8723bu_rssi(u8 rate,const u8*phy,size_t n,int*dbm){
 int value;if(!phy||n<28||!dbm||rate>19)return EINVAL;
 if(rate<=3){unsigned lna=phy[5]>>5,vga=phy[5]&31U;switch(lna){case 6:value=-34-2*(int)vga;break;case 4:value=-14-2*(int)vga;break;case 1:value=6-2*(int)vga;break;case 0:value=16-2*(int)vga;break;default:return ENOTSUP;}}
 else value=(int)((phy[4]>>1)&127U)-110;if(value< -110)return ERANGE;*dbm=value>0?0:value;return 0;
}
int rtw_8723bu_c2h(rtw_8723bu*c,const u8*b,size_t n){
 struct rtw_8723bu_private*p;if(!valid(c)||!b||n<2)return EINVAL;if(!c->firmware_running)return ENXIO;if(c->last_error)return c->last_error;p=c->private_state;
 if(b[0]==C2H_8723B_RA_REPORT){if(n<5||b[3]!=2||(b[2]&127U)>19||(n>=9&&b[8]!=0)||!c->associated)return EINVAL;c->tx_rate=b[2]&127U;c->tx_sgi=(u8)(b[2]>>7);return 0;}
 if(b[0]==C2H_8723B_BT_INFO){if(n<9||(b[2]&15U)>2)return EINVAL;rtl8723bu_update_bt_link_info(p,b[3]);if(p->bt_coex.c2h_bt_inquiry)rtl8723bu_handle_bt_inquiry(p);else rtl8723bu_handle_bt_info(p);return c->last_error?r23_paused(c,c->last_error):0;}
 return ENOTSUP;
}
int rtw_8723bu_tick(rtw_8723bu*c){
 struct rtw_8723bu_private*p;u8 command[4]={H2C_8723B_RSSI_SETTING,2,0,0};
 if(!valid(c))return EINVAL;if(!c->calibrated||!c->firmware_running)return ENXIO;if(c->last_error)return c->last_error;p=c->private_state;
 /* Read the actual running8723B sensor. Factory thermal data never marks a
  * channel calibrated and no unported temperature/power slope is invented. */
 p->last_thermal=(u8)((rtl8xxxu_read_rfreg(p,RF_A,RF6052_REG_T_METER_8723B)>>10)&63U);if(c->last_error)return r23_paused(c,c->last_error);
 if(c->associated&&c->rssi_valid){if(c->rssi_percent>100)return EINVAL;command[3]=c->rssi_percent;return rtw_8723bu_h2c(c,command,sizeof(command));}return 0;
}
int rtw_8723bu_tx_endpoint(const rtw_8723bu*c,u8 queue,u8*out){
 static const u8 tid[8]={TXDESC_QUEUE_BE,TXDESC_QUEUE_BK,TXDESC_QUEUE_BK,TXDESC_QUEUE_BE,TXDESC_QUEUE_VI,TXDESC_QUEUE_VI,TXDESC_QUEUE_VO,TXDESC_QUEUE_VO};
 const struct rtw_8723bu_private*p;u8 address;if(!valid(c)||!out||(queue>7&&queue!=TXDESC_QUEUE_MGNT))return EINVAL;
 if(!c->mac_initialized||!c->firmware_running||!c->calibrated)return ENXIO;if(c->last_error)return c->last_error;p=c->private_state;address=p->pipe_out[queue<8?tid[queue]:queue];
 for(unsigned i=0;i<c->usb.bulk_out_count;i++)if(address==c->usb.bulk_out[i]){*out=address;return 0;}return ENODEV;
}
