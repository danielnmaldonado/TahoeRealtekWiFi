/* SPDX-License-Identifier: BSD-3-Clause */
/* RTL8812AU/RTL8821AU MAC, DMA, LLT and USB configuration. Adapted from
 * pinned Linuxrtw88 rtw88xxa.c/mac.c/usb.c and per-chip tables; see
 * RTL8812AUBootstrap-Upstream.json and RTL8812AUPHY-LICENSE.txt. */
#include "rtw/RTL8812AUMAC.h"
#include "RTL8812AUCoreRegs.h"
#include <string.h>
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
#include "RTL8812AUMACTables.inc"
/* Hardware mapping ordinals: EXTRA0,LOW1,NORMAL2,HIGH3. Entries are
 * VO,VI,BE,BK,MG,HI, as in the separate chip rqpn tables. */
static const uint8_t queue_maps[3][6]={{3,3,2,2,3,3},{3,2,1,1,3,3},{2,2,1,1,0,3}};
static int fault(rtw_r12au_boot*c,int e){if(e&&!c->io_error)c->io_error=e;return e;}
static int poll_clear(rtw_r12au_boot*c,uint16_t a,uint32_t bits,unsigned tries){uint32_t v=0;for(unsigned i=0;i<tries;++i){int e=rtw_r12au_read(c,a,4,&v);if(e)return e;if(!(v&bits))return 0;if(i+1<tries&&(e=rtw_r12au_delay(c,10)))return e;}return fault(c,ETIMEDOUT);}
static int failed(rtw_r12au_boot*c,int e){uint8_t old=c->cleanup;c->cleanup=1;c->mac_initialized=0;c->associated=0;(void)rtw_r12au_write(c,REG_TXPAUSE,1,0x3f);(void)rtw_r12au_update(c,REG_CR,1,BIT_MACTXEN|BIT_MACRXEN,0);c->cleanup=old;return e;}
static int write_llt(rtw_r12au_boot*c,unsigned index,unsigned next){int e=rtw_r12au_write(c,REG_LLT_INIT,4,BIT_LLT_WRITE_ACCESS|(index<<8)|next);return e?e:poll_clear(c,REG_LLT_INIT,BIT(31)|BIT(30),22);}
int rtw_r12au_dma_prepare(rtw_r12au_boot*c){
 if(!c||!c->identified||!c->powered||c->firmware_running||c->mac_initialized)return EINVAL;if(c->io_error)return c->io_error;if(c->dma_prepared)return 0;
 if(c->chip!=RTW_R12AU_CHIP_8812AU&&c->chip!=RTW_R12AU_CHIP_8821AU)return EINVAL;
 c->rsvd_boundary=c->acq_pages=(uint8_t)(256-(c->chip==RTW_R12AU_CHIP_8812AU?9:8));int e;
 for(unsigned i=0;i<255;++i){unsigned next=i==(unsigned)c->rsvd_boundary-1?255:i+1;if((e=write_llt(c,i,next)))return e;}
 if((e=write_llt(c,255,c->rsvd_boundary))||(e=rtw_r12au_update(c,REG_TXDMA_OFFSET_CHK,4,BIT_DROP_DATA_EN,BIT_DROP_DATA_EN)))return e;
 c->dma_prepared=1;return 0;
}
int rtw_r12au_qsel_endpoint(const rtw_r12au_boot*c,uint8_t qsel,uint8_t*out){
 unsigned q;if(out)*out=0;if(!c||!out||c->usb.bulk_out_count<2||c->usb.bulk_out_count>4)return EINVAL;
 if(qsel<8){static const uint8_t tid_queue[8]={2,3,3,2,1,1,0,0};q=tid_queue[qsel];}
 else if(qsel==0x10||qsel==0x11||qsel==0x13)q=5;else if(qsel==0x12)q=4;else return EINVAL;
 uint8_t mapping=queue_maps[c->usb.bulk_out_count-2][q];unsigned ep=3U-mapping;
 if(ep>=c->usb.bulk_out_count||!c->usb.bulk_out[ep]||c->usb.bulk_out[ep]>15)return EINVAL;*out=c->usb.bulk_out[ep];return 0;
}
static int address(rtw_r12au_boot*c,uint16_t a,const uint8_t mac[6]){uint32_t lo=(uint32_t)mac[0]|((uint32_t)mac[1]<<8)|((uint32_t)mac[2]<<16)|((uint32_t)mac[3]<<24);int e=rtw_r12au_write(c,a,4,lo);return e?e:rtw_r12au_write(c,(uint16_t)(a+4),2,(uint32_t)mac[4]|((uint32_t)mac[5]<<8));}
static int station(rtw_r12au_boot*c,const uint8_t*bssid,uint16_t aid,int associated){
 static const uint8_t zero[6]={0};int e;uint32_t filters=BIT_CBSSID_BCN|BIT_CBSSID_DATA,rcr=(c->rx_config&~filters)|(associated?filters:0);
 if((e=rtw_r12au_write(c,REG_TXPAUSE,1,0x3f))||(e=address(c,0x610,c->board.mac))||(e=address(c,0x618,associated?bssid:zero))||(e=rtw_r12au_update(c,0x6a8,2,0x7ff,aid))||(e=rtw_r12au_update(c,REG_CR,4,0x30000,associated?0x20000:0))||(e=rtw_r12au_write(c,REG_RCR,4,rcr)))return e;
 unsigned beacon=associated?BIT_EN_BCN_FUNCTION:BIT_DIS_TSF_UDT|(c->board.phy.btcoex?BIT_EN_BCN_FUNCTION:0);
 if((e=rtw_r12au_update(c,REG_BCN_CTRL,1,BIT_DIS_TSF_UDT|BIT_EN_BCN_FUNCTION,beacon)))return e;c->associated=(uint8_t)associated;c->rx_config=rcr;return 0;
}
int rtw_r12au_mac_station(rtw_r12au_boot*c,const uint8_t*bssid,uint16_t aid,int associated){
 if(!c||!c->mac_initialized||(associated!=0&&associated!=1)||(!associated&&aid)||aid>2007)return EINVAL;if(c->io_error)return c->io_error;
 if(associated){unsigned nonzero=0;if(!bssid||!aid||(bssid[0]&1))return EINVAL;for(unsigned i=0;i<6;++i)nonzero|=bssid[i];if(!nonzero)return EINVAL;}
 int e=station(c,bssid,aid,associated);return e?failed(c,e):0;
}
int rtw_r12au_mac_init(rtw_r12au_boot*c){
 if(!c||!c->identified||!c->powered||!c->dma_prepared||!c->firmware_running||c->usb.bulk_out_count<2||c->usb.bulk_out_count>4)return EINVAL;if(c->io_error)return c->io_error;if(c->mac_initialized)return EBUSY;
 unsigned expected=c->chip==RTW_R12AU_CHIP_8812AU?247:c->chip==RTW_R12AU_CHIP_8821AU?248:0;if(!expected||c->acq_pages!=expected||c->rsvd_boundary!=expected)return EINVAL;
 unsigned packet=c->usb.speed==RTW_R12AU_USB_FULL?64:c->usb.speed==RTW_R12AU_USB_HIGH?512:c->usb.speed==RTW_R12AU_USB_SUPER?1024:0;if(!packet||packet!=c->usb.bulk_max_packet)return EINVAL;
 uint8_t endpoint=0;if(rtw_r12au_qsel_endpoint(c,0x12,&endpoint)||rtw_r12au_qsel_endpoint(c,0,&endpoint))return EINVAL;
 const uint32_t*table;size_t count;unsigned single=c->chip==RTW_R12AU_CHIP_8821AU;
 if(single){table=au_8821a_mac;count=ARRAY_SIZE(au_8821a_mac);}else if(c->chip==RTW_R12AU_CHIP_8812AU){table=au_8812a_mac;count=ARRAY_SIZE(au_8812a_mac);}else return EINVAL;
 int e=rtw_r12au_write(c,REG_TXPAUSE,1,0x3f);if(e)return failed(c,e);
 #define DO(expr) do{if((e=(expr)))return failed(c,e);}while(0)
 /* sec.h:0x680 TX/RX decrypt enables are intentionally clear. */
 DO(rtw_r12au_write(c,0x680,2,0));DO(rtw_r12au_write(c,REG_HMETFR,1,0xf));
 for(size_t i=0;i<count;i+=2)DO(rtw_r12au_write(c,(uint16_t)table[i],1,table[i+1]));
 unsigned hq=single?8:16,nq=c->usb.bulk_out_count>=3?hq:0,eq=single&&c->usb.bulk_out_count==4?4:0,pub=c->acq_pages-hq-nq-eq-1;
 DO(rtw_r12au_write(c,REG_RQPN_NPQ,4,BIT_RQPN_NE(nq,eq)));DO(rtw_r12au_write(c,REG_RQPN,4,BIT_RQPN_HLP(hq,0,pub)));
 const uint16_t boundaries[]={REG_BCNQ_BDNY,REG_MGQ_BDNY,REG_WMAC_LBK_BF_HD,REG_TRXFF_BNDY,REG_DWBCN0_CTRL+1};for(unsigned i=0;i<ARRAY_SIZE(boundaries);++i)DO(rtw_r12au_write(c,boundaries[i],1,c->rsvd_boundary));
 const uint8_t*m=queue_maps[c->usb.bulk_out_count-2];uint32_t map=0;DO(rtw_r12au_read(c,REG_TXDMA_PQ_MAP,2,&map));map&=7;map|=(uint32_t)(BIT_TXDMA_VOQ_MAP(m[0])|BIT_TXDMA_VIQ_MAP(m[1])|BIT_TXDMA_BEQ_MAP(m[2])|BIT_TXDMA_BKQ_MAP(m[3])|BIT_TXDMA_MGQ_MAP(m[4])|BIT_TXDMA_HIQ_MAP(m[5]));DO(rtw_r12au_write(c,REG_TXDMA_PQ_MAP,2,map));if(c->usb.bulk_out_count==4)DO(rtw_r12au_write(c,REG_HIQ_NO_LMT_EN,1,255));
 DO(rtw_r12au_write(c,REG_TRXFF_BNDY+2,2,16128-128-1));if(!single)DO(rtw_r12au_write(c,REG_PBP,1,0x30));
 DO(rtw_r12au_write(c,REG_RX_DRVINFO_SZ,1,4));DO(rtw_r12au_write(c,REG_HIMR0,4,0));DO(rtw_r12au_write(c,REG_HIMR1,4,0));
 DO(rtw_r12au_write(c,REG_RXFLTMAP0,2,0xffff));DO(rtw_r12au_write(c,REG_RXFLTMAP1,2,0x400));DO(rtw_r12au_write(c,REG_RXFLTMAP2,2,0xffff));DO(rtw_r12au_write(c,REG_MAR,4,UINT32_MAX));DO(rtw_r12au_write(c,REG_MAR+4,4,UINT32_MAX));
 DO(rtw_r12au_update(c,REG_RRSR,4,0xfffff,0xffff1));DO(rtw_r12au_write(c,REG_RETRY_LIMIT,2,0x3030));
 const uint16_t sifs[]={REG_SPEC_SIFS,REG_MAC_SPEC_SIFS,REG_SIFS,REG_SIFS+2};for(unsigned i=0;i<ARRAY_SIZE(sifs);++i)DO(rtw_r12au_write(c,sifs[i],2,0x100a));
 const uint16_t edca[]={REG_EDCA_BE_PARAM,REG_EDCA_BK_PARAM,REG_EDCA_VI_PARAM,REG_EDCA_VO_PARAM};const uint32_t edca_values[]={0x005ea42b,0x0000a44f,0x005ea324,0x002fa226};for(unsigned i=0;i<4;++i)DO(rtw_r12au_write(c,edca[i],4,edca_values[i]));
 DO(rtw_r12au_update(c,REG_FWHW_TXQ_CTRL,1,BIT(7),BIT(7)));DO(rtw_r12au_write(c,REG_ACKTO,1,0x80));DO(rtw_r12au_update(c,REG_DWBCN0_CTRL,4,0xf0,(single?6U:1U)<<4));if(single)DO(rtw_r12au_write(c,REG_DWBCN1_CTRL,1,12));
 DO(rtw_r12au_write(c,REG_BCN_CTRL,2,0x1010|(c->board.phy.btcoex?BIT_EN_BCN_FUNCTION:0)));DO(rtw_r12au_update(c,REG_TBTT_PROHIBIT,4,0xfffff,0x6404));DO(rtw_r12au_write(c,REG_DRVERLYINT,1,5));DO(rtw_r12au_write(c,REG_BCNDMATIM,1,2));DO(rtw_r12au_write(c,REG_BCNTCFG,2,0x4413));DO(rtw_r12au_write(c,REG_BCN_MAX_ERR,1,255));
 unsigned burst=c->usb.speed==RTW_R12AU_USB_SUPER?0:c->usb.speed==RTW_R12AU_USB_HIGH?1:2;
 DO(rtw_r12au_write(c,REG_RXDMA_MODE,1,BIT_DMA_BURST_CNT|BIT_DMA_MODE|(burst<<4)));DO(rtw_r12au_update(c,REG_TXDMA_OFFSET_CHK,2,BIT_DROP_DATA_EN,BIT_DROP_DATA_EN));
 DO(rtw_r12au_write(c,REG_RXDMA_AGG_PG_TH,2,c->usb.speed==RTW_R12AU_USB_SUPER?0x1a06:0x2005));DO(rtw_r12au_update(c,REG_TXDMA_PQ_MAP,1,BIT_RXDMA_AGG_EN,BIT_RXDMA_AGG_EN));
 DO(rtw_r12au_write(c,REG_USB3_RXITV,1,1));DO(rtw_r12au_write(c,REG_RXDMA_STATUS,2,0x7400));DO(rtw_r12au_write(c,REG_RXDMA_STATUS+1,1,0xf5));
 DO(rtw_r12au_write(c,REG_AMPDU_MAX_TIME,1,single?0x5e:0x70));DO(rtw_r12au_write(c,REG_AMPDU_MAX_LENGTH,4,UINT32_MAX));DO(rtw_r12au_write(c,REG_USTIME_TSF,1,0x50));DO(rtw_r12au_write(c,REG_USTIME_EDCA,1,0x50));
 if(c->usb.speed==RTW_R12AU_USB_SUPER)DO(rtw_r12au_update(c,REG_USB_MOD,1,BIT(4)|BIT(3),0));
 DO(rtw_r12au_update(c,REG_SINGLE_AMPDU_CTRL,1,BIT_EN_SINGLE_APMDU,BIT_EN_SINGLE_APMDU));DO(rtw_r12au_write(c,REG_RX_PKT_LIMIT,1,0x18));DO(rtw_r12au_write(c,REG_PIFS,1,0));DO(rtw_r12au_write(c,REG_MAX_AGGR_NUM,2,0x1f1f));
 if(single){DO(rtw_r12au_write(c,REG_FWHW_TXQ_CTRL,1,0x80));DO(rtw_r12au_write(c,REG_FAST_EDCA_CTRL,4,0x03087777));}else DO(rtw_r12au_update(c,REG_FWHW_TXQ_CTRL,1,BIT(7),0));
 DO(rtw_r12au_update(c,REG_RSV_CTRL,1,BIT(5)|BIT(6),BIT(5)|BIT(6)));
 const uint16_t arfr[]={REG_ARFR0,REG_ARFRH0,REG_ARFR1_V1,REG_ARFRH1_V1,REG_ARFR2_V1,REG_ARFRH2_V1,REG_ARFR3_V1,REG_ARFRH3_V1};const uint32_t arfr_values[]={0x10,0xfffff000,0x10,0x003ff000,0x15,0x003ff000,0x15,0xffcff000};for(unsigned i=0;i<8;++i)DO(rtw_r12au_write(c,arfr[i],4,arfr_values[i]));
 DO(rtw_r12au_write(c,0x670,4,BIT(31)|BIT(30)));DO(poll_clear(c,0x670,BIT(31),400));DO(rtw_r12au_write(c,0x680,2,0));
 DO(rtw_r12au_write(c,REG_HWSEQ_CTRL,1,255));DO(rtw_r12au_write(c,REG_BAR_MODE_CTRL,4,0x0201ffff));DO(rtw_r12au_write(c,REG_NAV_CTRL+2,1,0));DO(rtw_r12au_update(c,REG_GPIO_MUXCFG,1,BIT(5),0));DO(rtw_r12au_update(c,REG_QUEUE_CTRL,1,BIT(3),0));
 DO(rtw_r12au_write(c,REG_FWHW_TXQ_CTRL+1,1,0xf));DO(rtw_r12au_write(c,REG_EARLY_MODE_CONTROL+3,1,1));DO(rtw_r12au_write(c,REG_TX_RPT_TIME,2,0x3df0));DO(rtw_r12au_write(c,REG_SYS_SDIO_CTRL,1,0));DO(rtw_r12au_write(c,REG_ACLK_MON,1,0));DO(rtw_r12au_write(c,REG_USB_HRPWM,1,0));DO(rtw_r12au_update(c,REG_FWHW_TXQ_CTRL,4,BIT(12),BIT(12)));
 /* Keep encrypted payload/MIC intact for host software crypto; omit FCS so
  * RX24 iterator payload length is directly usable by the native stack. */
 c->rx_config=BIT_APP_MIC|BIT_APP_ICV|BIT_APP_PHYSTS|BIT_PKTCTL_DLEN|BIT_HTC_LOC_CTRL|BIT_AB|BIT_AM|BIT_APM|BIT_VHT_DACK;
 DO(station(c,NULL,0,0));DO(rtw_r12au_update(c,REG_CR,1,BIT_MACTXEN|BIT_MACRXEN,BIT_MACTXEN|BIT_MACRXEN));
 #undef DO
 c->mac_initialized=1;return 0;
}
