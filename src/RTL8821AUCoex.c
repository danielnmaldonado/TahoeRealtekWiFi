/* SPDX-License-Identifier: BSD-3-Clause */
/* Typed8821AU station coex adapter; see RTL8821AUCoex-Upstream.json. */
#include "rtw/RTL8821AUCoex.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
typedef uint8_t u8;typedef int8_t s8;typedef uint16_t u16;typedef uint32_t u32;typedef uint64_t u64;
#define BIT(n) (UINT32_C(1)<<(n))
#define GENMASK(h,l) ((UINT32_MAX<<(l))&(UINT32_MAX>>(31U-(h))))
#define GENMASK_ULL(h,l) ((UINT64_MAX<<(l))&(UINT64_MAX>>(63U-(h))))
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define FIELD_GET(m,v) (((v)&(m))>>__builtin_ctz((u32)(m)))
#define FIELD_PREP(m,v) (((u32)(v)<<__builtin_ctz((u32)(m)))&(m))
#define __ffs(v) ((unsigned)__builtin_ctz((u32)(v)))
#define WARN(condition,...) ((bool)(condition))
#define MASKLWORD 0xffffU
#define MASKHWORD 0xffff0000U
#define MASKDWORD UINT32_MAX
#define MASKBYTE0 255U
#define HZ 1000U
#define msecs_to_jiffies(v) (v)
#define __packed __attribute__((packed))
#define container_of(p,t,m) ((t*)((u8*)(p)-offsetof(t,m)))
#define RTW_CHIP_TYPE_8821A 2
#define RTW_CHIP_TYPE_8822B 3
#define RTW_CHIP_TYPE_8822C 4
#define RTW_BAND_2G 0
#define RTW_BAND_5G 1
#define RTW_CHANNEL_WIDTH_20 0
#define RTW_CHANNEL_WIDTH_40 1
#define RTW_CHANNEL_WIDTH_80 2
#define DESC_RATE5_5M 2
#define RTW_FLAG_RUNNING 0
#define RTW_FLAG_SCANNING 1
#define RTW_FLAG_BUSY_TRAFFIC 2
#define RTW_ALL_LPS_OFF 0
#define RTW_MODE_LPS_ACTIVE 0
#define rtw_dev rtw_r21au_coex
#include "RTL8812AURegisters.inc"
struct work_struct{unsigned id;};
struct delayed_work{struct work_struct work;u64 deadline;bool queued;};
#include "RTL8821AUCoexTypes.inc"
struct rtw_chip_ops{
 void(*coex_set_init)(struct rtw_dev*);void(*coex_set_ant_switch)(struct rtw_dev*,u8,u8);
 void(*coex_set_gnt_fix)(struct rtw_dev*);void(*coex_set_gnt_debug)(struct rtw_dev*);
 void(*coex_set_rfe_type)(struct rtw_dev*);void(*coex_set_wl_tx_power)(struct rtw_dev*,u8);
 void(*coex_set_wl_rx_gain)(struct rtw_dev*,bool);
};
struct rtw_hw_reg{u32 addr,mask;};struct rtw_ltecoex_addr{u32 ctrl,wdata,rdata;};
struct rtw_chip_info{
 const struct rtw_chip_ops*ops;unsigned id;const struct rtw_ltecoex_addr*ltecoex_addr;const struct rtw_hw_reg*btg_reg;
 u32 coex_para_ver;u8 bt_desired_ver;bool scbd_support,new_scbd10_def,ble_hid_profile_support,wl_mimo_ps_support;
 u8 pstdma_type,bt_rssi_type,ant_isolation,rssi_tolerance,table_sant_num,table_nsant_num,tdma_sant_num,tdma_nsant_num;
 u8 bt_afh_span_bw20,bt_afh_span_bw40,afh_5g_num,wl_rf_para_num;
 const u8*bt_rssi_step,*wl_rssi_step;const struct coex_table_para*table_nsant,*table_sant;
 const struct coex_tdma_para*tdma_sant,*tdma_nsant;const struct coex_rf_para*wl_rf_para_tx,*wl_rf_para_rx;
 const struct coex_5g_afh_map*afh_5g;
};
struct rtw_efuse{u8 share_ant,btcoex,rfe_option;};
struct rtw_dm_info{u32 cck_ok_cnt,cck_err_cnt;u8 min_rssi;};
struct rtw_traffic_stats{u32 tx_throughput,rx_throughput;};
struct rtw_coex_info_req{u8 seq,op_code,para1,para2,para3;};
struct rtw_c2h_cmd{u8 id,seq,payload[];};
struct sk_buff{u8*data;u32 cb[12];};
struct rtw_dev{
 rtw_r12au_boot*boot;rtw_r12au_chip owner_chip;rtw_r12au_board owner_board;rtw_usb_rw_transport owner_bus;rtw_waiter owner_waiter;rtw_r12au_usb_config owner_usb;const struct rtw_chip_info*chip;struct rtw_coex coex;
 struct rtw_efuse efuse;struct rtw_dm_info dm_info;struct rtw_traffic_stats stats;
 struct{u8 current_band_type,current_band_width,current_channel;}hal;
 struct{u8 mode;}lps_conf;unsigned flags[1],sta_cnt;struct rtw_dev*hw;
 u64 now_ms,tx_ewma,rx_ewma;int error;bool initialized,active;
 struct{bool pending,ready,lna_pending;u8 lna_desired;u64 deadline;struct rtw_coex_info_req req;u8 bytes[18];struct sk_buff skb;}mp;
 u32 mp_timeouts,bt_info_events;
};
static int cx_same_board(const rtw_r12au_phy_board*a,const rtw_r12au_phy_board*b){return a->chip==b->chip&&a->cut==b->cut&&a->rfe==b->rfe&&a->crystal==b->crystal&&a->thermal==b->thermal&&a->rf_paths==b->rf_paths&&a->package_type==b->package_type&&a->swing_2g==b->swing_2g&&a->swing_5g==b->swing_5g&&a->ext_pa_2g==b->ext_pa_2g&&a->ext_pa_5g==b->ext_pa_5g&&a->ext_lna_2g==b->ext_lna_2g&&a->ext_lna_5g==b->ext_lna_5g&&a->gpa_type==b->gpa_type&&a->apa_type==b->apa_type&&a->glna_type==b->glna_type&&a->alna_type==b->alna_type&&a->btcoex==b->btcoex&&!memcmp(a->txpwr_idx,b->txpwr_idx,sizeof(a->txpwr_idx));}
/* Retain immutable authority at init. Mutable bootstrap readiness/cursor
 * can change normally; its physical route, identity and calibration cannot. */
static int cx_owned(const struct rtw_dev*c){
 if(!c||!c->initialized||!c->boot)return 0;const rtw_r12au_boot*b=c->boot;const rtw_r12au_board*a=&c->owner_board,*z=&b->board;
 return b->chip==c->owner_chip&&b->bus.context==c->owner_bus.context&&b->bus.control_in==c->owner_bus.control_in&&b->bus.control_out==c->owner_bus.control_out&&
 b->waiter.context==c->owner_waiter.context&&b->waiter.wait_us==c->owner_waiter.wait_us&&cx_same_board(&a->phy,&z->phy)&&!memcmp(a->mac,z->mac,6)&&!memcmp(a->country,z->country,2)&&
 a->channel_plan==z->channel_plan&&a->regd==z->regd&&a->bt_setting==z->bt_setting&&a->share_ant==z->share_ant&&a->vht==z->vht&&a->eeprom_vendor==z->eeprom_vendor&&a->eeprom_product==z->eeprom_product&&a->sys_cfg==z->sys_cfg&&
 b->usb.speed==c->owner_usb.speed&&b->usb.bulk_max_packet==c->owner_usb.bulk_max_packet&&b->usb.bulk_in==c->owner_usb.bulk_in&&b->usb.bulk_out_count==c->owner_usb.bulk_out_count&&!memcmp(b->usb.bulk_out,c->owner_usb.bulk_out,4);
}
static bool test_bit(unsigned bit,const unsigned*flags){return (flags[0]&BIT(bit))!=0;}
static u64 cx_le64(const void*data){const u8*b=data;u64 v=0;for(unsigned i=0;i<8;++i)v|=(u64)b[i]<<(i*8);return v;}
static u64 le64_get_bits(u64 value,u64 mask){return(value&mask)>>__builtin_ctzll(mask);}
static u8 u8_get_bits(u8 value,u32 mask){return(u8)FIELD_GET(mask,value);}
static u32 u32_get_bits(u32 value,u32 mask){return FIELD_GET(mask,value);}
static void cx_fail(struct rtw_dev*c,int e){if(e&&!c->error)c->error=e;if(e){c->active=false;c->mp.pending=c->mp.ready=c->mp.lna_pending=false;}}
static int cx_require_owner(struct rtw_dev*c){if(cx_owned(c))return 1;cx_fail(c,ENXIO);rtw_r21au_coex_forget(c);return 0;}
static u32 cx_read(struct rtw_dev*c,u32 addr,u8 width){u32 value=0;if(c->error||!cx_require_owner(c))return 0;if(addr>65535U)cx_fail(c,EINVAL);else cx_fail(c,rtw_r12au_read(c->boot,(u16)addr,width,&value));return value;}
static void cx_write(struct rtw_dev*c,u32 addr,u8 width,u32 value){if(c->error||!cx_require_owner(c))return;if(addr>65535U)cx_fail(c,EINVAL);else cx_fail(c,rtw_r12au_write(c->boot,(u16)addr,width,value));}
static u8 rtw_read8(struct rtw_dev*c,u32 a){return(u8)cx_read(c,a,1);}static u16 rtw_read16(struct rtw_dev*c,u32 a){return(u16)cx_read(c,a,2);}static u32 rtw_read32(struct rtw_dev*c,u32 a){return cx_read(c,a,4);}
static void rtw_write8(struct rtw_dev*c,u32 a,u8 v){cx_write(c,a,1,v);}static void rtw_write16(struct rtw_dev*c,u32 a,u16 v){cx_write(c,a,2,v);}static void rtw_write32(struct rtw_dev*c,u32 a,u32 v){cx_write(c,a,4,v);}
static void cx_update(struct rtw_dev*c,u32 a,u8 width,u32 mask,u32 value){if(c->error)return;u32 old=cx_read(c,a,width);cx_write(c,a,width,(old&~mask)|(value&mask));}
static void rtw_write8_mask(struct rtw_dev*c,u32 a,u32 m,u8 v){cx_update(c,a,1,m,FIELD_PREP(m,v));}static void rtw_write32_mask(struct rtw_dev*c,u32 a,u32 m,u32 v){cx_update(c,a,4,m,FIELD_PREP(m,v));}
static void rtw_write8_set(struct rtw_dev*c,u32 a,u8 v){cx_update(c,a,1,v,v);}static void rtw_write8_clr(struct rtw_dev*c,u32 a,u8 v){cx_update(c,a,1,v,0);}static void rtw_write32_set(struct rtw_dev*c,u32 a,u32 v){cx_update(c,a,4,v,v);}static void rtw_write32_clr(struct rtw_dev*c,u32 a,u32 v){cx_update(c,a,4,v,0);}
static void cx_delay(struct rtw_dev*c,u32 us){if(!c->error&&cx_require_owner(c))cx_fail(c,rtw_r12au_delay(c->boot,us));}
#define mdelay(ms) cx_delay(rtwdev,(u32)(ms)*1000U)
#define rtw_err(...) ((void)0)
#define rtw_warn(...) ((void)0)
static void ieee80211_queue_delayed_work(struct rtw_dev*c,struct delayed_work*w,u32 delay){if(!w->queued){w->queued=true;w->deadline=c->now_ms+delay;}}
static void cancel_delayed_work_sync(struct delayed_work*w){w->queued=false;}
static void dev_kfree_skb_any(struct sk_buff*s){(void)s;}
static void rtw_leave_lps(struct rtw_dev*c){if(c->lps_conf.mode!=RTW_MODE_LPS_ACTIVE)cx_fail(c,ENOTSUP);}
static void rtw_set_txrx_1ss(struct rtw_dev*c,bool enabled){(void)enabled;cx_fail(c,ENOTSUP);}
/* The primary8821A descriptor excludes newer-chip indirect LTE registers. */
static bool ltecoex_read_reg(struct rtw_dev*c,u16 a,u32*v){(void)a;(void)v;cx_fail(c,ENOTSUP);return false;}
static bool ltecoex_reg_write(struct rtw_dev*c,u16 a,u32 v){(void)a;(void)v;cx_fail(c,ENOTSUP);return false;}
static void cx_command(struct rtw_dev*c,const u8 bytes[8]){if(!c->error&&cx_require_owner(c))cx_fail(c,rtw_r12au_h2c(c->boot,bytes));}
/* Exact pinned fw.c/fw.h packing, with one canonical boot H2C cursor. */
static void rtw_fw_query_bt_info(struct rtw_dev*c){const u8 b[8]={0x61,1};cx_command(c,b);}
static void rtw_fw_wl_ch_info(struct rtw_dev*c,u8 link,u8 ch,u8 bw){const u8 b[8]={0x66,link,ch,bw};cx_command(c,b);}
static void rtw_fw_query_bt_mp_info(struct rtw_dev*c,struct rtw_coex_info_req*r){const u8 b[8]={0x67,(u8)(r->seq<<4),r->op_code,r->para1,r->para2,r->para3};cx_command(c,b);}
static void rtw_fw_force_bt_tx_power(struct rtw_dev*c,u8 level){const u8 b[8]={0x62,(u8)(0-level)};cx_command(c,b);}
static void rtw_fw_bt_ignore_wlan_action(struct rtw_dev*c,bool enabled){const u8 b[8]={0x63,(u8)enabled};cx_command(c,b);}
static void rtw_fw_coex_tdma_type(struct rtw_dev*c,u8 a,u8 b,u8 d,u8 e,u8 f){const u8 p[8]={0x60,a,b,d,e,f};cx_command(c,p);}
static void rtw_fw_coex_query_hid_info(struct rtw_dev*c,u8 sub,u8 data){const u8 b[8]={0x73,sub,data};cx_command(c,b);}
static void rtw_fw_bt_wifi_control(struct rtw_dev*c,u8 op,u8*data){const u8 b[8]={0x69,op,data[0],data[1],data[2],data[3],data[4]};cx_command(c,b);}
/* Single-flight asynchronous primary10ms request: a blocking wait on the
 * shared controller gate would prevent its USB C2H completion. */
static struct sk_buff*cx_info_request(struct rtw_dev*c,struct rtw_coex_info_req*req){
 if(c->error||!c->active)return NULL;
 if(c->mp.ready&&!memcmp(&c->mp.req,req,sizeof(*req))){c->mp.ready=false;return &c->mp.skb;}
 if(c->mp.pending&&c->now_ms<c->mp.deadline)return NULL;
 if(c->mp.pending){c->mp.pending=false;++c->mp_timeouts;}
 c->mp.req=*req;c->mp.pending=true;c->mp.ready=false;c->mp.deadline=c->now_ms+10;rtw_fw_query_bt_mp_info(c,req);return NULL;
}
static void cx_info_response(struct rtw_dev*c,struct sk_buff*s){if(c->mp.pending&&s->data[2]==1){c->mp.pending=false;c->mp.ready=true;}}
#include "RTL8821AUCoexWrappers.inc"
#include "RTL8821AUCoexCallbacks.inc"
static const struct rtw_chip_ops cx_ops={rtw8821a_coex_cfg_init,rtw8821a_coex_cfg_ant_switch,rtw8821a_coex_cfg_gnt_fix,rtw8821a_coex_cfg_gnt_debug,rtw8821a_coex_cfg_rfe_type,rtw8821a_coex_cfg_wl_tx_power,rtw8821a_coex_cfg_wl_rx_gain};
#include "RTL8821AUCoexTables.inc"
#include "RTL8821AUCoexPolicy.inc"
/* A single latest desired LNA value bridges the asynchronous MP slot. The
 * watchdog and real C2H completion retry it after their live owner/FW gate. */
static void cx_mp_expire(struct rtw_dev*c){if(c->mp.pending&&c->mp.deadline<=c->now_ms){c->mp.pending=c->mp.ready=false;++c->mp_timeouts;}}
static void cx_lna_retry(struct rtw_dev*c){if(c->active&&!c->error&&c->mp.lna_pending&&!c->mp.pending)rtw_coex_set_bt_rx_gain(c,c->mp.lna_desired);}
static struct delayed_work*cx_work(struct rtw_dev*c,unsigned n){switch(n){case 0:return &c->coex.bt_relink_work;case 1:return &c->coex.bt_reenable_work;case 2:return &c->coex.defreeze_work;case 3:return &c->coex.wl_remain_work;case 4:return &c->coex.bt_remain_work;case 5:return &c->coex.wl_connecting_work;case 6:return &c->coex.bt_multi_link_remain_work;default:return &c->coex.wl_ccklock_work;}}
static void cx_run_work(struct rtw_dev*c,unsigned n){struct work_struct*w=&cx_work(c,n)->work;switch(n){case 0:rtw_coex_bt_relink_work(w);break;case 1:rtw_coex_bt_reenable_work(w);break;case 2:rtw_coex_defreeze_work(w);break;case 3:rtw_coex_wl_remain_work(w);break;case 4:rtw_coex_bt_remain_work(w);break;case 5:rtw_coex_wl_connecting_work(w);break;case 6:rtw_coex_bt_multi_link_remain_work(w);break;default:rtw_coex_wl_ccklock_work(w);break;}}
static int cx_input(struct rtw_dev*c,const rtw_r21au_coex_input*i){
 if(!c||!i||!c->initialized)return EINVAL;if(!cx_require_owner(c))return c->error;
 if(i->now_ms<c->now_ms||i->now_ms>UINT64_MAX-60000U||i->rssi_percent>100||i->connected>1||i->scanning>1||!i->primary||!i->center||(i->width_mhz!=20&&i->width_mhz!=40&&i->width_mhz!=80))return EINVAL;
 c->now_ms=i->now_ms;c->sta_cnt=i->connected;c->flags[0]=(c->active?BIT(RTW_FLAG_RUNNING):0)|(i->scanning?BIT(RTW_FLAG_SCANNING):0)|(c->flags[0]&BIT(RTW_FLAG_BUSY_TRAFFIC));
 c->hal.current_band_type=i->primary>14?RTW_BAND_5G:RTW_BAND_2G;c->hal.current_band_width=i->width_mhz==80?2:i->width_mhz==40?1:0;c->hal.current_channel=i->center;
 c->dm_info.min_rssi=i->rssi_percent;
 c->coex.stat.wl_beacon_interval=i->beacon_interval;c->coex.stat.cnt_wl[COEX_CNT_WL_SCANAP]=i->scan_ap_count;c->coex.stat.wl_rx_rate=i->rx_rate;c->coex.stat.wl_tx_rate=i->tx_rate;
 return 0;
}
static int cx_health(struct rtw_dev*c){
 if(!c||!c->initialized)return EINVAL;if(!cx_require_owner(c))return c->error;if(c->error)return c->error;
 if(!c->boot->identified||!c->boot->powered||!c->boot->firmware_running||!c->boot->mac_initialized){c->boot->firmware_running=c->boot->associated=0;cx_fail(c,ENXIO);return c->error;}
 if((rtw_read8(c,0x80)&0xc7)!=0xc6){c->boot->firmware_running=c->boot->associated=0;cx_fail(c,ENXIO);}return c->error;
}
int rtw_r21au_coex_health(rtw_r21au_coex*c){return cx_health(c);}
size_t rtw_r21au_coex_size(void){return sizeof(struct rtw_dev);}
int rtw_r21au_coex_init(void*storage,size_t bytes,rtw_r12au_boot*boot,rtw_r21au_coex**out){
 if(out)*out=NULL;if(!storage||!boot||!out||bytes<sizeof(struct rtw_dev)||(uintptr_t)storage%_Alignof(struct rtw_dev))return EINVAL;
 if(!boot->identified||boot->chip!=RTW_R12AU_CHIP_8821AU||boot->board.phy.chip!=RTW_R12AU_CHIP_8821AU||boot->board.phy.rf_paths!=1||boot->board.phy.rfe||boot->board.phy.btcoex!=1||boot->board.share_ant>1||boot->board.share_ant!=(boot->board.bt_setting&1))return ENOTSUP;
 struct rtw_dev*c=storage;memset(c,0,sizeof(*c));c->boot=boot;c->owner_chip=boot->chip;c->owner_board=boot->board;c->owner_bus=boot->bus;c->owner_waiter=boot->waiter;c->owner_usb=boot->usb;c->chip=&cx_chip;c->hw=c;c->efuse.btcoex=1;c->efuse.share_ant=boot->board.share_ant;c->efuse.rfe_option=boot->board.phy.rfe;c->initialized=true;c->mp.skb.data=c->mp.bytes;for(unsigned n=0;n<8;++n)cx_work(c,n)->work.id=n;*out=c;return 0;
}
int rtw_r21au_coex_start(rtw_r21au_coex*c,const rtw_r21au_coex_input*i){
 if(!c)return EINVAL;if(!cx_require_owner(c))return c->error;if(c->active)return EBUSY;int e=cx_input(c,i);if(e)return e;if(cx_health(c))return c->error;
 c->active=true;c->flags[0]|=BIT(RTW_FLAG_RUNNING);rtw_coex_power_on_setting(c);rtw_coex_init_hw_config(c,false);if(c->error){c->active=false;return c->error;}return 0;
}
int rtw_r21au_coex_notify(rtw_r21au_coex*c,rtw_r21au_coex_event event,const rtw_r21au_coex_input*i){
 if(!c||event<RTW_R21CX_SCAN_START||event>RTW_R21CX_SWITCH_BAND)return EINVAL;if(!cx_require_owner(c))return c->error;if(!c->active)return c->error?c->error:ENXIO;int e=cx_input(c,i);if(e)return e;if(cx_health(c))return c->error;
 bool five=i->primary>14;
 switch(event){case RTW_R21CX_SCAN_START:rtw_coex_scan_notify(c,five?COEX_SCAN_START_5G:COEX_SCAN_START_2G);break;case RTW_R21CX_SCAN_FINISH:rtw_coex_scan_notify(c,COEX_SCAN_FINISH);break;case RTW_R21CX_ASSOC_START:rtw_coex_connect_notify(c,five?COEX_ASSOCIATE_5G_START:COEX_ASSOCIATE_START);break;case RTW_R21CX_ASSOC_FINISH:rtw_coex_connect_notify(c,five?COEX_ASSOCIATE_5G_FINISH:COEX_ASSOCIATE_FINISH);break;case RTW_R21CX_MEDIA_CONNECT:rtw_coex_media_status_notify(c,five?COEX_MEDIA_CONNECT_5G:COEX_MEDIA_CONNECT);break;case RTW_R21CX_MEDIA_DISCONNECT:rtw_coex_media_status_notify(c,COEX_MEDIA_DISCONNECT);break;case RTW_R21CX_SWITCH_BAND:rtw_coex_switchband_notify(c,five?COEX_SWITCH_TO_5G:COEX_SWITCH_TO_24G_NOFORSCAN);break;}
 return c->error;
}
int rtw_r21au_coex_watchdog(rtw_r21au_coex*c,const rtw_r21au_coex_input*i){
 if(!c)return EINVAL;if(!cx_require_owner(c))return c->error;if(!c->active)return c->error?c->error:ENXIO;bool busy=test_bit(RTW_FLAG_BUSY_TRAFFIC,c->flags);int e=cx_input(c,i);if(e)return e;if(cx_health(c))return c->error;
 c->flags[0]=(c->flags[0]&~BIT(RTW_FLAG_BUSY_TRAFFIC))|((i->tx_packets>100||i->rx_packets>100)?BIT(RTW_FLAG_BUSY_TRAFFIC):0);
 /* main.c's bytes/2s shift and DECLARE_EWMA(tp,10,2). */
 u64 tx=i->tx_bytes>>18,rx=i->rx_bytes>>18;if(tx>UINT32_MAX)tx=UINT32_MAX;if(rx>UINT32_MAX)rx=UINT32_MAX;
 c->tx_ewma=c->tx_ewma?(c->tx_ewma+(tx<<10))>>1:tx<<10;c->rx_ewma=c->rx_ewma?(c->rx_ewma+(rx<<10))>>1:rx<<10;
 c->stats.tx_throughput=(u32)(c->tx_ewma>>10);c->stats.rx_throughput=(u32)(c->rx_ewma>>10);
 for(unsigned n=0;n<8&&!c->error;++n){struct delayed_work*w=cx_work(c,n);if(w->queued&&w->deadline<=c->now_ms){w->queued=false;cx_run_work(c,n);}}
 cx_mp_expire(c);
 if(busy!=test_bit(RTW_FLAG_BUSY_TRAFFIC,c->flags))rtw_coex_wl_status_change_notify(c,0);
 if(i->scanning){cx_lna_retry(c);return c->error;}
 /* rtw88xxa_false_alarm_statistics: measured CCK CRC counts and the same
  * global CRC-counter reset, preserving unrelated register bits. */
 u32 crc=rtw_read32(c,REG_CRC_CCK);c->dm_info.cck_ok_cnt=crc&65535U;c->dm_info.cck_err_cnt=crc>>16;
 rtw_write32_set(c,REG_CNTRST,BIT(0));rtw_write32_clr(c,REG_CNTRST,BIT(0));
 rtw_coex_wl_status_check(c);rtw_coex_active_query_bt_info(c);cx_lna_retry(c);return c->error;
}
int rtw_r21au_coex_c2h(rtw_r21au_coex*c,const uint8_t*bytes,size_t length,uint64_t now_ms){
 if(!c||!bytes||length<2||length>257)return EINVAL;if(!cx_require_owner(c))return c->error;u8 id=bytes[0];if(id!=9&&id!=11&&id!=0x27&&id!=0x45)return ENOMSG;
 if(!c->active)return c->error?c->error:ENXIO;size_t n=length-2;const u8*p=bytes+2;u8 copy[10]={0};
 if(id==9){if(!n||n>10)return EINVAL;unsigned src=p[0]&15;if(src>=6||(src==3&&n<2)||(src==4&&n<3)||(src==5&&n<6)||((src==1||src==2)&&n!=7))return EINVAL;memcpy(copy,p,n);}
 else if(id==11){if(n<8||length>sizeof(c->mp.bytes)||!c->mp.pending||p[0]!=1)return EINVAL;}
 else if(id==0x27){if(n<8||n>10)return EINVAL;memcpy(copy,p,n);}
 else{if(n!=8)return EINVAL;memcpy(copy,p,n);}
 if(now_ms<c->now_ms||now_ms>UINT64_MAX-60000U)return EINVAL;c->now_ms=now_ms;
 if(id==11&&now_ms>=c->mp.deadline){c->mp.pending=false;++c->mp_timeouts;return EAGAIN;}
 if(cx_health(c))return c->error;
 if(id==9){++c->bt_info_events;rtw_coex_bt_info_notify(c,copy,(u8)n);}else if(id==11){memcpy(c->mp.bytes,bytes,length);rtw_coex_info_response(c,&c->mp.skb);if(c->mp.req.op_code==BT_MP_INFO_OP_SCAN_TYPE&&c->mp.ready){c->coex.stat.bt_ble_scan_type=(u8)GET_COEX_RESP_BT_SCAN_TYPE(c->mp.bytes+2);c->coex.stat.bt_init_scan=(c->coex.stat.bt_ble_scan_type&1)!=0;c->mp.ready=false;rtw_coex_run_coex(c,COEX_RSN_BTINFO);}else c->mp.ready=false;}else if(id==0x27)rtw_coex_wl_fwdbginfo_notify(c,copy,(u8)n);else rtw_coex_bt_hid_info_notify(c,copy,(u8)n);cx_lna_retry(c);return c->error;
}
void rtw_r21au_coex_forget(rtw_r21au_coex*c){if(!c)return;c->active=false;c->flags[0]=0;c->mp.pending=c->mp.ready=c->mp.lna_pending=false;c->coex.stop_dm=true;for(unsigned n=0;n<8;++n)cx_work(c,n)->queued=false;}
int rtw_r21au_coex_stop(rtw_r21au_coex*c){
 if(!c)return EINVAL;if(!cx_require_owner(c))return c->error;
 if(c->active&&!c->error)(void)cx_health(c);
 if(c->boot&&c->boot->identified&&c->boot->powered&&!c->boot->io_error&&(c->active||c->error)){
  int saved=c->error;c->error=0;
  if(!saved&&c->boot->firmware_running)rtw_coex_ips_notify(c,COEX_IPS_ENTER);
  else rtw_coex_set_ant_path(c,true,COEX_SET_ANT_WOFF);
  rtw_coex_power_off_setting(c);if(saved)c->error=saved;
 }
 rtw_r21au_coex_forget(c);return c->error;
}
static int cx_tx_guard(const void*context,const rtw_r12au_phy_board*board,const rtw_usb_rw_transport*bus){const struct rtw_dev*c=context;return c&&board&&bus&&cx_require_owner((struct rtw_dev*)c)&&c->active&&!c->error&&c->boot->identified&&c->boot->powered&&c->boot->firmware_running&&c->boot->mac_initialized&&!c->boot->io_error&&bus->context==c->boot->bus.context&&bus->control_in==c->boot->bus.control_in&&bus->control_out==c->boot->bus.control_out&&cx_same_board(board,&c->boot->board.phy)&&cx_health((struct rtw_dev*)c)==0;}
int rtw_r21au_coex_bind_phy(rtw_r21au_coex*c,rtw_r12au_phy*p){if(!c||!p)return EINVAL;int e=rtw_r12au_phy_bind_coex(p,c,cx_tx_guard);return e&&c->error?c->error:e;}
void rtw_r21au_coex_get_status(const rtw_r21au_coex*c,rtw_r21au_coex_status*s){if(!s)return;memset(s,0,sizeof(*s));if(!c)return;s->initialized=c->initialized;s->owner_valid=(u8)cx_owned(c);s->error=c->error?c->error:!cx_owned(c)?ENXIO:c->boot->io_error;s->active=c->active&&!s->error&&c->boot&&c->boot->identified&&c->boot->powered&&c->boot->firmware_running&&c->boot->mac_initialized;s->bt_disabled=c->coex.stat.bt_disabled;s->bt_profiles=c->coex.stat.bt_profile_num;s->table=c->coex.dm.cur_table;s->tdma=c->coex.dm.cur_ps_tdma;s->under_5g=c->coex.under_5g;s->frozen=c->coex.freeze;s->antenna_phase=(u8)c->coex.dm.cur_ant_pos_type;s->share_ant=c->efuse.share_ant;s->bt_rssi=(u8)c->coex.stat.bt_rssi;s->bt_ble_scan_type=c->coex.stat.bt_ble_scan_type;s->bt_init_scan=c->coex.stat.bt_init_scan;s->policy_runs=c->coex.stat.cnt_wl[COEX_CNT_WL_COEXRUN];s->bt_info_events=c->bt_info_events;s->mp_timeouts=c->mp_timeouts;for(unsigned n=0;n<8;++n)if(cx_work((struct rtw_dev*)c,n)->queued)++s->pending_work;}
