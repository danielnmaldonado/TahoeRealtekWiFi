/* SPDX-License-Identifier: BSD-3-Clause */
/* Typed B/C station adapter. Generic source is shared; hardware never is.
 * See RTW88Coex-Upstream.json for exact primary provenance. */
#include "rtw/RTW88Coex.h"
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
#define RTW_CHIP_TYPE_8822B 0
#define RTW_CHIP_TYPE_8822C 1
#define RTW_CHIP_TYPE_8821C 3
#define RTW_CHIP_TYPE_8821A 5
#define RF_PATH_A 0
#define RF_PATH_B 1
#define RFREG_MASK 0xfffffU
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
#define rtw_dev rtw88_coex
#include "RTW88CoexRegisters.inc"
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
struct rtw_dm_info{u32 cck_ok_cnt,cck_err_cnt,cck_fa_cnt,ofdm_fa_cnt,total_fa_cnt;
 u32 ofdm_ok_cnt,ofdm_err_cnt,ht_ok_cnt,ht_err_cnt,vht_ok_cnt,vht_err_cnt;
 u32 cck_cca_cnt,ofdm_cca_cnt,total_cca_cnt;u8 min_rssi;};
struct rtw_traffic_stats{u32 tx_throughput,rx_throughput;};
struct rtw_coex_info_req{u8 seq,op_code,para1,para2,para3;};
struct rtw_c2h_cmd{u8 id,seq,payload[];};
struct sk_buff{u8*data;u32 cb[12];};
struct rtw_dev{
 rtw88_boot_context*boot;rtw88_phy_context*phy;rtw88_coex_mimo_callback mimo;void*mimo_cookie;const struct rtw_chip_info*chip;struct rtw_coex coex;
 struct rtw_efuse efuse;struct rtw_dm_info dm_info;struct rtw_traffic_stats stats;
 struct{u8 current_band_type,current_band_width,current_channel;}hal;
 struct{u8 mode;}lps_conf;unsigned flags[1],sta_cnt;struct rtw_dev*hw;
 u64 now_ms;int error;bool initialized,active;
 struct{bool pending,ready,lna_pending;u8 lna_desired;u64 deadline;struct rtw_coex_info_req req;u8 bytes[18];struct sk_buff skb;}mp;
 u32 mp_timeouts,bt_info_events,epoch,operations;u64 ewma_tx,ewma_rx;bool rfk_active,mimo_enabled;
 rtw88_board board_snapshot;rtw88_fw_bus bus_snapshot,raw_bus_snapshot;rtw_wifi_family family_snapshot;
};
_Static_assert(sizeof(struct rtw_dev)<=RTW88_COEX_SCRATCH_BYTES,"B/C coex scratch");
static bool test_bit(unsigned bit,const unsigned*flags){return(flags[0]&BIT(bit))!=0;}
static u64 cx_le64(const void*data){const u8*b=data;u64 v=0;for(unsigned i=0;i<8;++i)v|=(u64)b[i]<<(8*i);return v;}
static u64 le64_get_bits(u64 v,u64 m){return(v&m)>>__builtin_ctzll(m);}
static u8 u8_get_bits(u8 v,u32 m){return(u8)FIELD_GET(m,v);}
static u32 u32_get_bits(u32 v,u32 m){return FIELD_GET(m,v);}
static void cx_fail(struct rtw_dev*c,int e){if(e<0)e=e==INT32_MIN?EIO:-e;if(e){if(!c->error)c->error=e;c->active=false;c->mp.pending=c->mp.ready=c->mp.lna_pending=false;if(!c->boot->firmware.last_error)c->boot->firmware.last_error=e;}}
static int cx_budget(struct rtw_dev*c){if(++c->operations>40000U)cx_fail(c,EOVERFLOW);return c->error;}
static u32 cx_read(struct rtw_dev*c,u32 a,unsigned w){u32 v=0;if(cx_budget(c))return 0;if((w!=1&&w!=2&&w!=4)||a>65536U-w)cx_fail(c,EINVAL);else cx_fail(c,c->boot->firmware.bus.read(c->boot->firmware.bus.cookie,(u16)a,w,&v));return c->error?0:v;}
static void cx_write(struct rtw_dev*c,u32 a,unsigned w,u32 v){if(cx_budget(c))return;if((w!=1&&w!=2&&w!=4)||a>65536U-w)cx_fail(c,EINVAL);else cx_fail(c,c->boot->firmware.bus.write(c->boot->firmware.bus.cookie,(u16)a,w,v));}
static u8 rtw_read8(struct rtw_dev*c,u32 a){return(u8)cx_read(c,a,1);}static u16 rtw_read16(struct rtw_dev*c,u32 a){return(u16)cx_read(c,a,2);}static u32 rtw_read32(struct rtw_dev*c,u32 a){return cx_read(c,a,4);}
static void rtw_write8(struct rtw_dev*c,u32 a,u8 v){cx_write(c,a,1,v);}static void rtw_write16(struct rtw_dev*c,u32 a,u16 v){cx_write(c,a,2,v);}static void rtw_write32(struct rtw_dev*c,u32 a,u32 v){cx_write(c,a,4,v);}
static void cx_update(struct rtw_dev*c,u32 a,unsigned w,u32 m,u32 v){if(c->error)return;u32 old=cx_read(c,a,w);cx_write(c,a,w,(old&~m)|(v&m));}
static void rtw_write8_mask(struct rtw_dev*c,u32 a,u32 m,u8 v){cx_update(c,a,1,m,FIELD_PREP(m,v));}static void rtw_write32_mask(struct rtw_dev*c,u32 a,u32 m,u32 v){cx_update(c,a,4,m,FIELD_PREP(m,v));}
static void rtw_write8_set(struct rtw_dev*c,u32 a,u8 v){cx_update(c,a,1,v,v);}static void rtw_write8_clr(struct rtw_dev*c,u32 a,u8 v){cx_update(c,a,1,v,0);}static void rtw_write16_set(struct rtw_dev*c,u32 a,u16 v){cx_update(c,a,2,v,v);}static void rtw_write32_set(struct rtw_dev*c,u32 a,u32 v){cx_update(c,a,4,v,v);}static void rtw_write32_clr(struct rtw_dev*c,u32 a,u32 v){cx_update(c,a,4,v,0);}
static void rtw_write_rf(struct rtw_dev*c,u8 path,u32 a,u32 mask,u32 v){if(cx_budget(c))return;if(a>255)cx_fail(c,EINVAL);else cx_fail(c,rtw88_phy_write_rf(c->phy,path,(u8)a,mask,v));}
static void cx_delay(struct rtw_dev*c,u32 us){if(!c->error)c->boot->firmware.bus.delay_us(c->boot->firmware.bus.cookie,us);}
#define mdelay(ms) cx_delay(rtwdev,(u32)(ms)*1000U)
#define rtw_err(...) ((void)0)
#define rtw_warn(...) ((void)0)
static void ieee80211_queue_delayed_work(struct rtw_dev*c,struct delayed_work*w,u32 delay){if(!w->queued){w->queued=true;w->deadline=c->now_ms+delay;}}
static void cancel_delayed_work_sync(struct delayed_work*w){w->queued=false;}
static void dev_kfree_skb_any(struct sk_buff*s){(void)s;}
static void rtw_leave_lps(struct rtw_dev*c){if(c->lps_conf.mode!=RTW_MODE_LPS_ACTIVE)cx_fail(c,ENOTSUP);}
static void rtw_set_txrx_1ss(struct rtw_dev*c,bool enabled){if(c->mimo_enabled==enabled)return;if(!c->mimo){cx_fail(c,ENOTSUP);return;}cx_fail(c,c->mimo(c->mimo_cookie,enabled));if(!c->error)c->mimo_enabled=enabled;}
static bool ltecoex_read_reg(struct rtw_dev*c,u16 a,u32*v){*v=0;if(cx_budget(c))return false;int e=c->boot->firmware.bus.coex_read(c->boot->firmware.bus.cookie,a,v);cx_fail(c,e);return !e;}
static bool ltecoex_reg_write(struct rtw_dev*c,u16 a,u32 v){if(cx_budget(c))return false;int e=c->boot->firmware.bus.coex_write(c->boot->firmware.bus.cookie,a,v);cx_fail(c,e);return !e;}
static void cx_command(struct rtw_dev*c,const u8 b[8]){if(!cx_budget(c))cx_fail(c,rtw88_fw_h2c(&c->boot->firmware,b));}
static void rtw_fw_query_bt_info(struct rtw_dev*c){const u8 b[8]={0x61,1};cx_command(c,b);}
static void rtw_fw_wl_ch_info(struct rtw_dev*c,u8 link,u8 ch,u8 bw){const u8 b[8]={0x66,link,ch,bw};cx_command(c,b);}
static void rtw_fw_query_bt_mp_info(struct rtw_dev*c,struct rtw_coex_info_req*r){const u8 b[8]={0x67,(u8)(r->seq<<4),r->op_code,r->para1,r->para2,r->para3};cx_command(c,b);}
static void rtw_fw_force_bt_tx_power(struct rtw_dev*c,u8 level){const u8 b[8]={0x62,(u8)(0-level)};cx_command(c,b);}
static void rtw_fw_bt_ignore_wlan_action(struct rtw_dev*c,bool enabled){const u8 b[8]={0x63,(u8)enabled};cx_command(c,b);}
static void rtw_fw_coex_tdma_type(struct rtw_dev*c,u8 a,u8 b,u8 d,u8 e,u8 f){const u8 p[8]={0x60,a,b,d,e,f};cx_command(c,p);}
static void rtw_fw_coex_query_hid_info(struct rtw_dev*c,u8 sub,u8 data){const u8 b[8]={0x73,sub,data};cx_command(c,b);}
static void rtw_fw_bt_wifi_control(struct rtw_dev*c,u8 op,u8*data){const u8 b[8]={0x69,op,data[0],data[1],data[2],data[3],data[4]};cx_command(c,b);}
/* A controller-gate blocking wait prevents its own USB C2H completion.
 * Preserve the primary single-flight10ms exchange with a real timer. */
static struct sk_buff*cx_info_request(struct rtw_dev*c,struct rtw_coex_info_req*r){
 if(c->error||!c->active)return NULL;
 if(c->mp.ready&&!memcmp(&c->mp.req,r,sizeof(*r))){c->mp.ready=false;return &c->mp.skb;}
 if(c->mp.pending&&c->now_ms<c->mp.deadline)return NULL;
 if(c->mp.pending){c->mp.pending=false;++c->mp_timeouts;}
 c->mp.req=*r;c->mp.pending=true;c->mp.ready=false;c->mp.deadline=c->now_ms+10;rtw_fw_query_bt_mp_info(c,r);return NULL;
}
static void cx_info_response(struct rtw_dev*c,struct sk_buff*s){if(c->mp.pending&&s->data[2]==COEX_RESP_ACK_BY_WL_FW){c->mp.pending=false;c->mp.ready=true;}}
#include "RTL8821AUCoexWrappers.inc"
#include "RTW88CoexCallbacks.inc"
#include "RTW88CoexTables.inc"
static void rtw_coex_ips_notify(struct rtw_dev*,u8) __attribute__((unused));
#include "RTL8821AUCoexPolicy.inc"
/* The primary synchronous MP wait serialized these two policy decisions.
 * Retry only the latest deferred LNA decision after the real slot is free;
 * admission keeps the primary one-command/10ms-timeout behavior. */
static void cx_mp_expire(struct rtw_dev*c){if(c->mp.pending&&c->mp.deadline<=c->now_ms){c->mp.pending=c->mp.ready=false;++c->mp_timeouts;}}
static void cx_lna_retry(struct rtw_dev*c){if(c->active&&!c->error&&c->mp.lna_pending&&!c->mp.pending)rtw_coex_set_bt_rx_gain(c,c->mp.lna_desired);}
static struct delayed_work*cx_work(struct rtw_dev*c,unsigned n){switch(n){case 0:return &c->coex.bt_relink_work;case 1:return &c->coex.bt_reenable_work;case 2:return &c->coex.defreeze_work;case 3:return &c->coex.wl_remain_work;case 4:return &c->coex.bt_remain_work;case 5:return &c->coex.wl_connecting_work;case 6:return &c->coex.bt_multi_link_remain_work;default:return &c->coex.wl_ccklock_work;}}
static void cx_run_work(struct rtw_dev*c,unsigned n){struct work_struct*w=&cx_work(c,n)->work;switch(n){case 0:rtw_coex_bt_relink_work(w);break;case 1:rtw_coex_bt_reenable_work(w);break;case 2:rtw_coex_defreeze_work(w);break;case 3:rtw_coex_wl_remain_work(w);break;case 4:rtw_coex_bt_remain_work(w);break;case 5:rtw_coex_wl_connecting_work(w);break;case 6:rtw_coex_bt_multi_link_remain_work(w);break;default:rtw_coex_wl_ccklock_work(w);break;}}
static int cx_input(struct rtw_dev*c,const rtw88_coex_input*i){
 if(!c||!i||!c->initialized)return EINVAL;
 if(i->now_ms<c->now_ms||i->now_ms>UINT64_MAX-60000U||i->rssi_percent>100||i->connected>1||i->scanning>1||!i->primary||!i->center||i->primary>177||i->center>177||(i->width_mhz!=20&&i->width_mhz!=40&&i->width_mhz!=80)||(i->width_mhz==20&&i->primary!=i->center))return EINVAL;
 c->now_ms=i->now_ms;c->sta_cnt=i->connected;c->flags[0]=(c->active?BIT(RTW_FLAG_RUNNING):0)|(i->scanning?BIT(RTW_FLAG_SCANNING):0)|(c->flags[0]&BIT(RTW_FLAG_BUSY_TRAFFIC));
 c->hal.current_band_type=i->primary>14?RTW_BAND_5G:RTW_BAND_2G;c->hal.current_band_width=i->width_mhz==80?2:i->width_mhz==40?1:0;c->hal.current_channel=i->center;
 c->dm_info.min_rssi=i->rssi_percent;c->coex.stat.wl_beacon_interval=i->beacon_interval;c->coex.stat.cnt_wl[COEX_CNT_WL_SCANAP]=i->scan_ap_count;c->coex.stat.wl_rx_rate=i->rx_rate;c->coex.stat.wl_tx_rate=i->tx_rate;return 0;
}
static int cx_owned(const rtw88_coex*c,const rtw88_phy_context*phy){
 if(!c||!c->initialized||!phy||c->phy!=phy||!c->boot||phy->boot!=c->boot||!phy->mac)return 0;
 const rtw88_fw_context*f=&c->boot->firmware;
 #define CX_SAME_BUS(a,b) ((a).cookie==(b).cookie&&(a).read==(b).read&&(a).write==(b).write&&(a).bulk_write==(b).bulk_write&&(a).delay_us==(b).delay_us&&(a).coex_read==(b).coex_read&&(a).coex_write==(b).coex_write)
 int owned=phy->mac->device==f&&f->family==c->family_snapshot&&CX_SAME_BUS(f->bus,c->bus_snapshot)&&CX_SAME_BUS(c->boot->raw_bus,c->raw_bus_snapshot)&&!memcmp(&c->boot->board,&c->board_snapshot,sizeof(c->board_snapshot))&&c->epoch==c->boot->power_epoch;
 #undef CX_SAME_BUS
 return owned;
}
int rtw88_coex_ready(const rtw88_coex*c,const rtw88_phy_context*phy){
 if(!cx_owned(c,phy)||!c->active||c->error)return 0;const rtw88_fw_context*f=&c->boot->firmware;
 return f->identified&&f->powered&&f->firmware_running&&!f->last_error&&c->boot->board_valid&&c->boot->physical_valid&&phy->mac->mac_ready&&phy->tables_loaded;
}
static int cx_health(struct rtw_dev*c){
 if(!c||!c->initialized)return EINVAL;c->operations=0;if(c->error)return c->error;
 if(!cx_owned(c,c->phy)){cx_fail(c,ENXIO);return c->error;}
 const rtw88_fw_context*f=&c->boot->firmware;
 if(!f->identified||!f->powered||!f->firmware_running||f->last_error||!c->boot->board_valid||!c->phy->mac->mac_ready||!c->phy->tables_loaded){cx_fail(c,f->last_error?f->last_error:ENXIO);return c->error;}
 if((rtw_read16(c,0x80)&0xcfffU)!=0xc078U&&!c->error)cx_fail(c,ENXIO);return c->error;
}
static bool cx_family(rtw_wifi_family f){return f==RTW_WIFI_RTL8821CU||f==RTW_WIFI_RTL8822BU||f==RTW_WIFI_RTL8822CU;}
static bool cx_overlap(const void*a,size_t an,const void*b,size_t bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return an&&bn&&(x<=y?y-x<an:x-y<bn);}
size_t rtw88_coex_size(void){return sizeof(struct rtw_dev);}
int rtw88_coex_board_supported(const rtw88_boot_context*b){
 if(!b)return EINVAL;const rtw88_board*board=&b->board;rtw_wifi_family f=b->firmware.family;
 if(!cx_family(f)||!b->firmware.identified||!b->physical_valid||!board->features_valid||board->btcoex!=1||board->share_ant>1||board->bt_setting==255||board->share_ant!=(board->bt_setting&1U)||(board->rf_board_option&0xe0U)!=0x20U||board->rf_paths<1||board->rf_paths>(f==RTW_WIFI_RTL8821CU?1:2)||!board->active_rf_paths||board->active_rf_paths>board->rf_paths||!board->tx_streams||board->tx_streams>board->active_rf_paths)return ENOTSUP;
 if((f==RTW_WIFI_RTL8821CU&&board->rfe_option!=0&&board->rfe_option!=2&&board->rfe_option!=4&&board->rfe_option!=6)||(f==RTW_WIFI_RTL8822BU&&board->rfe_option!=2&&board->rfe_option!=3&&board->rfe_option!=5)||(f==RTW_WIFI_RTL8822CU&&board->rfe_option>6))return ENOTSUP;return 0;
}
int rtw88_coex_init(void*storage,size_t bytes,rtw88_phy_context*phy,rtw88_coex_mimo_callback mimo,void*cookie,rtw88_coex**out){
 if(!out)return EINVAL;
 if((storage&&cx_overlap(storage,sizeof(struct rtw_dev),out,sizeof(*out)))||(phy&&cx_overlap(phy,sizeof(*phy),out,sizeof(*out)))||(phy&&phy->boot&&cx_overlap(phy->boot,sizeof(*phy->boot),out,sizeof(*out)))||(phy&&phy->mac&&cx_overlap(phy->mac,sizeof(*phy->mac),out,sizeof(*out))))return EINVAL;
 *out=NULL;if(!storage||!phy||!phy->boot||!phy->mac||bytes<sizeof(struct rtw_dev)||(uintptr_t)storage%_Alignof(struct rtw_dev))return EINVAL;
 rtw88_boot_context*b=phy->boot;rtw88_fw_context*f=&b->firmware;const rtw88_board*board=&b->board;
 if(cx_overlap(storage,sizeof(struct rtw_dev),phy,sizeof(*phy))||cx_overlap(storage,sizeof(struct rtw_dev),b,sizeof(*b))||cx_overlap(storage,sizeof(struct rtw_dev),phy->mac,sizeof(*phy->mac)))return EINVAL;
 if(!b->board_valid||rtw88_coex_board_supported(b))return ENOTSUP;
 if(phy->mac->device!=f||!f->bus.read||!f->bus.write||!f->bus.coex_read||!f->bus.coex_write||!f->bus.delay_us||(f->family==RTW_WIFI_RTL8822CU&&!mimo))return EINVAL;
 struct rtw_dev*c=storage;memset(c,0,sizeof(*c));c->boot=b;c->phy=phy;c->mimo=mimo;c->mimo_cookie=cookie;c->chip=f->family==RTW_WIFI_RTL8821CU?&cx_chip_8821c:f->family==RTW_WIFI_RTL8822BU?&cx_chip_8822b:&cx_chip_8822c;c->hw=c;c->efuse.btcoex=1;c->efuse.share_ant=board->share_ant;c->efuse.rfe_option=board->rfe_option;c->initialized=true;c->epoch=b->power_epoch;memcpy(&c->board_snapshot,board,sizeof(*board));c->bus_snapshot=f->bus;c->raw_bus_snapshot=b->raw_bus;c->family_snapshot=f->family;c->mp.skb.data=c->mp.bytes;for(unsigned n=0;n<8;++n)cx_work(c,n)->work.id=n;*out=c;return 0;
}
int rtw88_coex_start(rtw88_coex*c,const rtw88_coex_input*i){if(!c)return EINVAL;if(c->active)return EBUSY;int e=cx_input(c,i);if(e)return e;if(cx_health(c))return c->error;c->active=true;c->flags[0]|=BIT(RTW_FLAG_RUNNING);rtw_coex_power_on_setting(c);rtw_coex_init_hw_config(c,false);return c->error;}
int rtw88_coex_notify(rtw88_coex*c,rtw88_coex_event event,const rtw88_coex_input*i){
 if(!c||event<RTW88_CX_SCAN_START||event>RTW88_CX_LPS_LEAVE)return EINVAL;if(!c->active)return c->error?c->error:ENXIO;int e=cx_input(c,i);if(e)return e;if(c->rfk_active)return EBUSY;if(cx_health(c))return c->error;bool five=i->primary>14;
 switch(event){case RTW88_CX_SCAN_START:rtw_coex_scan_notify(c,five?COEX_SCAN_START_5G:COEX_SCAN_START_2G);break;case RTW88_CX_SCAN_FINISH:rtw_coex_scan_notify(c,COEX_SCAN_FINISH);break;case RTW88_CX_ASSOC_START:rtw_coex_connect_notify(c,five?COEX_ASSOCIATE_5G_START:COEX_ASSOCIATE_START);break;case RTW88_CX_ASSOC_FINISH:rtw_coex_connect_notify(c,five?COEX_ASSOCIATE_5G_FINISH:COEX_ASSOCIATE_FINISH);break;case RTW88_CX_MEDIA_CONNECT:rtw_coex_media_status_notify(c,five?COEX_MEDIA_CONNECT_5G:COEX_MEDIA_CONNECT);break;case RTW88_CX_MEDIA_DISCONNECT:rtw_coex_media_status_notify(c,COEX_MEDIA_DISCONNECT);break;case RTW88_CX_SWITCH_BAND:rtw_coex_switchband_notify(c,five?COEX_SWITCH_TO_5G:COEX_SWITCH_TO_24G_NOFORSCAN);break;case RTW88_CX_LPS_ENTER:rtw_coex_lps_notify(c,COEX_LPS_ENABLE);break;case RTW88_CX_LPS_LEAVE:rtw_coex_lps_notify(c,COEX_LPS_DISABLE);break;}return c->error;
}
uint64_t rtw88_coex_next_deadline(const rtw88_coex*c){if(!c||!c->active||c->error||c->rfk_active)return 0;u64 next=c->mp.pending?c->mp.deadline:0;for(unsigned n=0;n<8;++n){const struct delayed_work*w=cx_work((struct rtw_dev*)c,n);if(w->queued&&(!next||w->deadline<next))next=w->deadline;}return next;}
int rtw88_coex_poll(rtw88_coex*c,uint64_t now){if(!c||!c->active)return c&&c->error?c->error:ENXIO;if(now<c->now_ms||now>UINT64_MAX-60000U)return EINVAL;if(c->rfk_active)return EBUSY;c->now_ms=now;if(cx_health(c))return c->error;for(unsigned n=0;n<8&&!c->error;++n){struct delayed_work*w=cx_work(c,n);if(w->queued&&w->deadline<=now){w->queued=false;cx_run_work(c,n);}}cx_mp_expire(c);cx_lna_retry(c);return c->error;}
int rtw88_coex_watchdog(rtw88_coex*c,const rtw88_coex_input*i){
 if(!c||!c->active)return c&&c->error?c->error:ENXIO;int e=cx_input(c,i);if(e)return e;if(c->rfk_active)return EBUSY;if(cx_health(c))return c->error;
 c->flags[0]=(c->flags[0]&~BIT(RTW_FLAG_BUSY_TRAFFIC))|((i->tx_packets>100||i->rx_packets>100)?BIT(RTW_FLAG_BUSY_TRAFFIC):0);
 u64 tx=i->tx_bytes>>18,rx=i->rx_bytes>>18;tx=tx>UINT32_MAX?UINT32_MAX:tx;rx=rx>UINT32_MAX?UINT32_MAX:rx;
 c->ewma_tx=c->ewma_tx?((c->ewma_tx+(tx<<10))>>1):tx<<10;c->ewma_rx=c->ewma_rx?((c->ewma_rx+(rx<<10))>>1):rx<<10;c->stats.tx_throughput=(u32)(c->ewma_tx>>10);c->stats.rx_throughput=(u32)(c->ewma_rx>>10);
 if(c->boot->firmware.family==RTW_WIFI_RTL8821CU)rtw8821c_false_alarm_statistics(c);else if(c->boot->firmware.family==RTW_WIFI_RTL8822BU)rtw8822b_false_alarm_statistics(c);else rtw8822c_false_alarm_statistics(c);
 rtw_coex_wl_status_check(c);rtw_coex_query_bt_hid_list(c);rtw_coex_wl_status_change_notify(c,0);cx_mp_expire(c);cx_lna_retry(c);return c->error;
}
int rtw88_coex_c2h(rtw88_coex*c,const uint8_t*b,size_t length,const rtw88_coex_input*i){
 if(!c||!b||!i||length<2||length>257)return EINVAL;u8 id=b[0];if(id!=9&&id!=11&&id!=0x27&&id!=0x45)return ENOMSG;
 size_t n=length-2;const u8*p=b+2;u8 copy[10]={0};
 if(id==9){if(!n||n>10)return EINVAL;unsigned src=p[0]&15;if(src>=6||(src==3&&n<2)||(src==4&&n<3)||(src==5&&n<6)||((src==1||src==2)&&n!=7))return EINVAL;memcpy(copy,p,n);}
 else if(id==11){if(n<8||length>sizeof(c->mp.bytes)||!c->mp.pending||i->now_ms>=c->mp.deadline||p[0]!=COEX_RESP_ACK_BY_WL_FW)return EINVAL;}
 else if(id==0x27){if(n<8||n>10)return EINVAL;memcpy(copy,p,n);}
 else{if(n!=8)return EINVAL;memcpy(copy,p,n);}
 if(!c->active)return c->error?c->error:ENXIO;int e=cx_input(c,i);if(e)return e;if(c->rfk_active)return EBUSY;if(cx_health(c))return c->error;
 if(id==9){++c->bt_info_events;rtw_coex_bt_info_notify(c,copy,(u8)n);}else if(id==11){memcpy(c->mp.bytes,b,length);rtw_coex_info_response(c,&c->mp.skb);if(c->mp.req.op_code==BT_MP_INFO_OP_SCAN_TYPE&&c->mp.ready){c->coex.stat.bt_ble_scan_type=(u8)GET_COEX_RESP_BT_SCAN_TYPE(c->mp.bytes+2);c->coex.stat.bt_init_scan=(c->coex.stat.bt_ble_scan_type&1)!=0;c->mp.ready=false;rtw_coex_run_coex(c,COEX_RSN_BTINFO);}else c->mp.ready=false;}else if(id==0x27)rtw_coex_wl_fwdbginfo_notify(c,copy,(u8)n);else rtw_coex_bt_hid_info_notify(c,copy,(u8)n);cx_lna_retry(c);return c->error;
}
int rtw88_coex_rfk(rtw88_coex*c,int active){if(!c||(active!=0&&active!=1))return EINVAL;if(!c->active)return c->error?c->error:ENXIO;c->rfk_active=active;return 0;}
void rtw88_coex_forget(rtw88_coex*c){if(!c)return;c->active=c->rfk_active=false;c->flags[0]=0;c->mp.pending=c->mp.ready=c->mp.lna_pending=false;c->coex.stop_dm=true;for(unsigned n=0;n<8;++n)cx_work(c,n)->queued=false;}
int rtw88_coex_stop(rtw88_coex*c){
 if(!c)return EINVAL;
 if(c->active&&!c->error&&!cx_health(c)){
  /* Firmware is about to power off. Keep checked scoreboard/antenna MMIO,
   * while avoiding runtime IPS policy's dependency on a free TDMA mailbox. */
  c->coex.stat.wl_under_ips=true;rtw_coex_write_scbd(c,COEX_SCBD_ALL,false);
  rtw_coex_set_ant_path(c,true,COEX_SET_ANT_WOFF);rtw_coex_power_off_setting(c);
 }
 rtw88_coex_forget(c);return c->error;
}
void rtw88_coex_get_status(const rtw88_coex*c,rtw88_coex_status*s){if(!s)return;memset(s,0,sizeof(*s));if(!c)return;s->initialized=c->initialized;s->active=c->active;s->error=c->error;s->bt_disabled=c->coex.stat.bt_disabled;s->bt_profiles=c->coex.stat.bt_profile_num;s->table=c->coex.dm.cur_table;s->tdma=c->coex.dm.cur_ps_tdma;s->under_5g=c->coex.under_5g;s->frozen=c->coex.freeze;s->antenna_phase=(u8)c->coex.dm.cur_ant_pos_type;s->share_ant=c->efuse.share_ant;s->bt_rssi=(u8)c->coex.stat.bt_rssi;s->policy_runs=c->coex.stat.cnt_wl[COEX_CNT_WL_COEXRUN];s->bt_info_events=c->bt_info_events;s->mp_timeouts=c->mp_timeouts;s->mimo_ps=c->mimo_enabled;s->rfk_active=c->rfk_active;s->bt_ble_scan_type=c->coex.stat.bt_ble_scan_type;s->bt_init_scan=c->coex.stat.bt_init_scan;s->tx_mbps=c->stats.tx_throughput;s->rx_mbps=c->stats.rx_throughput;s->next_deadline_ms=rtw88_coex_next_deadline(c);for(unsigned n=0;n<8;++n)if(cx_work((struct rtw_dev*)c,n)->queued)++s->pending_work;}
int rtw88_coex_parse_tx_status(const uint8_t*b,size_t n,uint8_t*token,int*acked){if(token)*token=0;if(acked)*acked=0;if(!b||!token||!acked||n<2||n>257)return EINVAL;if(b[0]==3){if(n<9)return EINVAL;*token=b[8]&0xfcU;*acked=(b[2]&0xc0U)==0;return 0;}if(b[0]==255){if(n<3)return EINVAL;if(b[2]!=15)return ENOMSG;if(n<12)return EINVAL;*token=b[10]&0xfcU;*acked=(b[11]&0xc0U)==0;return 0;}return ENOMSG;}
