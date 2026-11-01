/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
 * Copyright(c)2018-2019 Realtek Corporation.
 * Native checked adapter for pinned rtw88 B/C operating PHY algorithms.
 * Generated dependency closure is kept private to this translation unit. */
#include "rtw/RTW88PHYOperating.h"
#include "rtw/RTW88USBDataPath.h"
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
#include <string.h>
#include <stdbool.h>
typedef uint8_t u8;typedef int8_t s8;typedef uint16_t u16;typedef int16_t s16;typedef uint32_t u32;typedef int32_t s32;typedef uint64_t u64;typedef int64_t s64;
#define BIT(n) (UINT32_C(1)<<(n))
#define GENMASK(h,l) ((UINT32_MAX>>(31U-(h)))&(UINT32_MAX<<(l)))
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define __packed __attribute__((packed))
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1
#endif
#define DECLARE_BITMAP(n,b) unsigned long n[((b)+63U)/64U]
struct ewma_thermal {unsigned long internal;};
static void ewma_thermal_init(struct ewma_thermal*a){a->internal=0;}
static void ewma_thermal_add(struct ewma_thermal*a,unsigned long v){a->internal=a->internal?((a->internal*3U)+(v<<10U))/4U:v<<10U;}
static unsigned long ewma_thermal_read(const struct ewma_thermal*a){return a->internal>>10U;}
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define min3(a,b,c) min(min(a,b),c)
#define max3(a,b,c) max(max(a,b),c)
#define min_t(t,a,b) min((t)(a),(t)(b))
#define max_t(t,a,b) max((t)(a),(t)(b))
#define clamp_t(t,v,l,h) min_t(t,max_t(t,v,l),h)
#define clamp(v,l,h) min(max(v,l),h)
#define abs(x) ((x)<0?-(x):(x))
#define swap(a,b) do{__typeof__(a) temp_=(a);(a)=(b);(b)=temp_;}while(0)
#define BUILD_BUG_ON(x) _Static_assert(!(x),"rtw88 layout")
#define fallthrough __attribute__((fallthrough))
static unsigned native_shift(uint32_t m){unsigned s=0;if(!m)return 0;while(!(m&1U)){m>>=1U;++s;}return s;}
#define FIELD_GET(m,v) (((v)&(m))>>native_shift(m))
#define FIELD_PREP(m,v) (((v)<<native_shift(m))&(m))
static void set_bit(unsigned n,unsigned long*p){p[n/64U]|=1UL<<(n%64U);}
static void clear_bit(unsigned n,unsigned long*p){p[n/64U]&=~(1UL<<(n%64U));}
static int test_bit(unsigned n,const unsigned long*p){return (p[n/64U]>>(n%64U))&1UL;}
static u8 bcd2bin(u8 x){return (u8)((x>>4U)*10U+(x&15U));}
static unsigned __fls(uint32_t v){return v?31U-(unsigned)__builtin_clz(v):0;}
static uint64_t div_u64(uint64_t a,uint32_t b){return b?a/b:0;}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#pragma clang diagnostic ignored "-Wpointer-sign"
#include "RTW88PHYOperatingTypes.inc"
#define PHY_BAND_2G 0
#define PHY_BAND_5G 1
#define RTW_BAND_2G 1
#define RTW_BAND_5G 2
#define EFUSE_READ_FAIL 0xff
#define DIV_ROUND_CLOSEST(n,d) (((n)+(d)/2)/(d))
#define NATIVE_WARN(c,...) ((c)!=0)
#define NATIVE_WARN_ON(c) ((c)!=0)
struct rtw_dev;
struct rtw_table {const void*data;u32 size;void(*parse)(struct rtw_dev*,const struct rtw_table*);};
struct rtw_rfe_def {const struct rtw_table*phy_pg_tbl,*txpwr_lmt_tbl;const struct rtw_pwr_track_tbl*pwr_track_tbl;};
#undef RTW_DEF_RFE
#undef RTW_DEF_RFE_EXT
#define RTW_DEF_RFE(chip,pg,lmt,track) {&rtw##chip##_bb_pg_type##pg##_tbl,&rtw##chip##_txpwr_lmt_type##lmt##_tbl,&rtw##chip##_pwr_track_type##track##_tbl}
#define RTW_DEF_RFE_EXT(chip,pg,lmt,track,btg) RTW_DEF_RFE(chip,pg,lmt,track)
struct rtw_chip_info {u8 id,max_power_index,txgi_factor,iqk_threshold,lck_threshold;u32 dpd_ratemask;bool en_dis_dpd,is_pwr_by_rate_dec;};
struct rtw_hal {
 bool txrx_1ss;
 u8 current_channel,current_band_width,primary_channel,rf_path_num,rf_type,cut_version,rfe_btg,sar_band,cch_by_bw[3];u32 antenna_rx,antenna_tx,ch_param[3];
 s8 tx_pwr_by_rate_offset_2g[4][DESC_RATE_MAX],tx_pwr_by_rate_offset_5g[4][DESC_RATE_MAX];
 s8 tx_pwr_by_rate_base_2g[4][RTW_RATE_SECTION_NUM],tx_pwr_by_rate_base_5g[4][RTW_RATE_SECTION_NUM];
 s8 tx_pwr_limit_2g[RTW_REGD_MAX][3][RTW_RATE_SECTION_NUM][14],tx_pwr_limit_5g[RTW_REGD_MAX][3][RTW_RATE_SECTION_NUM][49];
 s8 tx_pwr_tbl[4][DESC_RATE_MAX];
};
struct rtw_dm_info {
 u8 thermal_avg[4],thermal_meter_k,thermal_meter_lck,default_ofdm_index,tx_rate;bool pwr_trk_init_trigger,pwr_trk_triggered,is_bt_iqk_timeout;
 s8 delta_power_index[4],delta_power_index_last[4],txagc_remnant_cck,txagc_remnant_ofdm[4];struct ewma_thermal avg_thermal[4];
 u32 dack_adck[4];u16 dack_msbk[4][2][DACK_MSBK_BACKUP_NUM];u8 dack_dck[4][2][DACK_DCK_BACKUP_NUM];
 struct rtw_dpk_info dpk_info;struct rtw_gapk_info gapk;u32 dm_flags;
};
struct rtw_efuse {u8 rfe_option,power_track_type,thermal_meter_k,thermal_meter[4],tx_bb_swing_setting_2g,tx_bb_swing_setting_5g;struct rtw_txpwr_idx txpwr_idx_table[4];};
struct rtw_sar_arg {u8 sar_band,path,rs;};
struct rtw_dev {
 uint32_t magic,power_epoch,thermal_swing[2];rtw88_phy_context*native;struct rtw_chip_info*chip;struct rtw_chip_info chip_value;
 struct rtw_hal hal;struct rtw_dm_info dm_info;struct rtw_efuse efuse;bool mp_mode,associated;
 int32_t power_limit_mbm;uint8_t regd,power_reduction_db,power_needs_tune,restore_depth,iqk_complete,dac_complete,dac_a_complete,gnt_saved;uint16_t packet_sequence,*packet_counter;
 uint32_t operations;uint64_t delay_total;rtw88_phy_radio_status status;
};
struct cca_ccut;struct rtw8822b_rfe_info;struct rtw8822c_dpk_data;struct dpk_cfg_pair;
#include "RTW88PHYOperatingPrototypes.inc"
static int native_fail(struct rtw_dev*d,int e){if(e<0)e=e==INT32_MIN?EIO:-e;if(e){if(!d->status.error)d->status.error=e;if(!d->native->boot->firmware.last_error)d->native->boot->firmware.last_error=e;}return e;}
static int native_error(struct rtw_dev*d){return d->status.error?d->status.error:d->native->boot->firmware.last_error;}
static int native_live_health(struct rtw_dev*d);
static int native_budget(struct rtw_dev*d){if(++d->operations>500000U)return native_fail(d,EOVERFLOW);return d->restore_depth?0:native_error(d);}
static u32 native_read(struct rtw_dev*d,u16 a,unsigned w){u32 v=0;rtw88_fw_context*f=&d->native->boot->firmware;if(native_budget(d))return 0;int e=f->bus.read(f->bus.cookie,a,w,&v);if(e){native_fail(d,e);return 0;}return v;}
static void native_write(struct rtw_dev*d,u16 a,unsigned w,u32 v){
 /* A restored pause mask can resume TX even inside imported RFK cleanup. */
 if(a==0x522&&(v&255U)!=255U&&(native_error(d)||native_live_health(d)))return;
rtw88_fw_context*f=&d->native->boot->firmware;if(native_budget(d))return;native_fail(d,f->bus.write(f->bus.cookie,a,w,v));}
static u8 rtw_read8(struct rtw_dev*d,u32 a){return (u8)native_read(d,(u16)a,1);}
static u32 rtw_read32(struct rtw_dev*d,u32 a){return native_read(d,(u16)a,4);}
static u32 rtw_read32_mask(struct rtw_dev*d,u32 a,u32 m){return (rtw_read32(d,a)&m)>>native_shift(m);}
static u8 rtw_read8_mask(struct rtw_dev*d,u32 a,u8 m){return (u8)((rtw_read8(d,a)&m)>>native_shift(m));}
static void rtw_write8(struct rtw_dev*d,u32 a,u8 v){native_write(d,(u16)a,1,v);}
static void rtw_write32(struct rtw_dev*d,u32 a,u32 v){native_write(d,(u16)a,4,v);}
static void rtw_write32_mask(struct rtw_dev*d,u32 a,u32 m,u32 v){u32 old=rtw_read32(d,a);rtw_write32(d,a,(old&~m)|((v<<native_shift(m))&m));}
static void rtw_write32_set(struct rtw_dev*d,u32 a,u32 m){rtw_write32_mask(d,a,m,UINT32_MAX);}
static void rtw_write32_clr(struct rtw_dev*d,u32 a,u32 m){rtw_write32_mask(d,a,m,0);}
static void rtw_write16_set(struct rtw_dev*d,u32 a,u16 m){u32 v=native_read(d,(u16)a,2);native_write(d,(u16)a,2,v|m);}
static void rtw_write16_clr(struct rtw_dev*d,u32 a,u16 m){u32 v=native_read(d,(u16)a,2);native_write(d,(u16)a,2,v&~(u32)m);}
static void rtw_write8_set(struct rtw_dev*d,u32 a,u8 m){rtw_write8(d,a,(u8)(rtw_read8(d,a)|m));}
static void rtw_write8_clr(struct rtw_dev*d,u32 a,u8 m){rtw_write8(d,a,(u8)(rtw_read8(d,a)&~m));}
static void rtw_write32s_mask(struct rtw_dev*d,u32 a,u32 m,u32 v){rtw_write32_mask(d,a,m,v);rtw_write32_mask(d,a+0x200,m,v);}
static u32 rtw_read_rf(struct rtw_dev*d,u8 p,u32 a,u32 m){u32 v=0;if(native_budget(d))return 0;int e=rtw88_phy_read_rf(d->native,p,(u8)a,m,&v);if(e)native_fail(d,e);return v;}
static void rtw_write_rf(struct rtw_dev*d,u8 p,u32 a,u32 m,u32 v){if(native_budget(d))return;rtw88_fw_context*f=&d->native->boot->firmware;int previous=f->last_error;if(d->restore_depth)f->last_error=0;int e=rtw88_phy_write_rf(d->native,p,(u8)a,m,v);if(previous)f->last_error=previous;
 if(e)native_fail(d,e);}
static void native_delay(struct rtw_dev*d,u32 us){if(native_error(d)&&!d->restore_depth)return;d->delay_total+=us;if(d->delay_total>120000000U){native_fail(d,ETIMEDOUT);return;}rtw88_fw_context*f=&d->native->boot->firmware;f->bus.delay_us(f->bus.cookie,us);}
#define udelay(u) native_delay(rtwdev,(u))
#define fsleep(u) native_delay(rtwdev,(u))
#define mdelay(m) native_delay(rtwdev,(m)*1000U)
#define msleep(m) native_delay(rtwdev,(m)*1000U)
#define usleep_range(a,b) native_delay(rtwdev,(a))
#define read_poll_timeout(op,val,condition,sleep_us,timeout_us,sleep_first,args...) ({int poll_result_=-ETIMEDOUT;unsigned poll_count_=(unsigned)((timeout_us)/(sleep_us))+1U;if(sleep_first)native_delay(rtwdev,(sleep_us));for(unsigned poll_i_=0;poll_i_<poll_count_;poll_i_++){(val)=op(args);if(native_error(rtwdev)){poll_result_=-native_error(rtwdev);break;}if(condition){poll_result_=0;break;}if(poll_i_+1U<poll_count_)native_delay(rtwdev,(sleep_us));}if(poll_result_)native_fail(rtwdev,-poll_result_);poll_result_;})
#define read_poll_timeout_atomic read_poll_timeout
#define WARN(c,...) ((c)?(native_fail(rtwdev,EIO),1):0)
#define WARN_ON(c) WARN(c,"invalid condition")
#define rtw_dbg(...) ((void)0)
#define rtw_warn(...) ((void)0)
#define rtw_err(...) ((void)0)
static bool check_hw_ready(struct rtw_dev*d,u32 a,u32 m,u32 target){for(unsigned i=0;i<1000;i++){if(rtw_read32_mask(d,a,m)==target&&!native_error(d))return true;if(native_error(d))return false;if(i<999)native_delay(d,10);}native_fail(d,ETIMEDOUT);return false;}
static void rtw_restore_reg(struct rtw_dev*d,struct rtw_backup_info*b,u8 count){++d->restore_depth;for(unsigned i=0;i<count;i++)native_write(d,(u16)b[i].reg,b[i].len,b[i].val);--d->restore_depth;}
static int rtw_read8_physical_efuse(struct rtw_dev*d,u16 a,u8*out){rtw88_boot_context*b=d->native->boot;rtw88_efuse_geometry g;if(!out||!b->physical_valid||rtw88_boot_geometry(b->firmware.family,&g)||a>=g.physical_bytes){if(out)*out=255;return native_fail(d,EINVAL);}*out=b->physical[a];return 0;}
static u32 rtw_coex_read_indirect_reg(struct rtw_dev*d,u16 a){u32 v=0;if(native_budget(d))return 0;rtw88_fw_context*f=&d->native->boot->firmware;native_fail(d,f->bus.coex_read(f->bus.cookie,a,&v));return v;}
static void rtw_coex_write_indirect_reg(struct rtw_dev*d,u16 a,u32 m,u32 value){if(native_budget(d))return;u32 old=rtw_coex_read_indirect_reg(d,a);rtw88_fw_context*f=&d->native->boot->firmware;native_fail(d,f->bus.coex_write(f->bus.cookie,a,(old&~m)|((value<<native_shift(m))&m)));}
static bool rtw_is_assoc(struct rtw_dev*d){return d->associated;}
static bool rtw_chip_wcpu_8051(struct rtw_dev*d){(void)d;return false;}
static bool rtw_regd_srrc(struct rtw_dev*d){return d->regd==RTW_REGD_CN;}
static u8 rtw_regd_get(struct rtw_dev*d){return d->regd;}
static bool rtw_regd_has_alt(u8 regd,u8*alt){switch(regd){case RTW_REGD_IC:case RTW_REGD_CHILE:case RTW_REGD_MEXICO:*alt=RTW_REGD_FCC;return true;case RTW_REGD_KCC:case RTW_REGD_ACMA:case RTW_REGD_UKRAINE:case RTW_REGD_CN:case RTW_REGD_QATAR:case RTW_REGD_UK:*alt=RTW_REGD_ETSI;return true;default:return false;}}
static s32 rtw_query_sar(struct rtw_dev*d,const struct rtw_sar_arg*a){
 if(d->power_limit_mbm==INT32_MAX)return d->chip->max_power_index;
 /* Exact pinned sar.c rtw_sar_to_phy units and per-path PG reference. */
 int64_t mbm=d->power_limit_mbm;int unit=d->native->boot->firmware.family==RTW_WIFI_RTL8822CU?25:50;
 int absolute=(int)(mbm>=0?mbm/unit:-((-mbm+unit-1)/unit));
 if(absolute>(int)d->chip->max_power_index)absolute=d->chip->max_power_index;
 int cap=absolute-(d->hal.current_channel>14?d->hal.tx_pwr_by_rate_base_5g[a->path][a->rs]:d->hal.tx_pwr_by_rate_base_2g[a->path][a->rs]);
 /* Keep the exact signed offset through factory-base addition. Narrowing to
  * the primary s8 SAR type can raise an impossible cap into a valid index. */
 return cap;
}
static const struct rtw_rfe_def*rtw_get_rfe_def(struct rtw_dev*);
static void rtw_load_table(struct rtw_dev*d,const struct rtw_table*t){t->parse(d,t);}
static void native_put32(u8*b,u32 v){for(unsigned i=0;i<4;i++)b[i]=(u8)(v>>(i*8));}
static void rtw_fw_do_iqk(struct rtw_dev*d,struct rtw_iqk_para*p){if(native_budget(d))return;u8 packet[80]={0};rtw88_fw_context*f=&d->native->boot->firmware;if(rtw88_usb_make_control_descriptor(f->family,32,RTW88_USB_CONTROL_H2C,packet,48)){native_fail(d,EINVAL);return;}native_put32(packet+48,0x000eff01U);native_put32(packet+52,9U|((u32)*d->packet_counter<<16U));packet[56]=(u8)((p->clear?1U:0U)|(p->segment_iqk?2U:0U));native_fail(d,f->bus.bulk_write(f->bus.cookie,19,packet,sizeof(packet)));++*d->packet_counter;}
static void rtw_fw_inform_rfk_status(struct rtw_dev*d,bool start){if(native_budget(d))return;u8 command[8]={0x6d,0,0,0,0,0,0,0};command[1]=(u8)(start?1:0);rtw88_fw_context*f=&d->native->boot->firmware;int previous=f->last_error;if(!start&&d->restore_depth)f->last_error=0;int e=rtw88_fw_h2c(f,command);if(previous)f->last_error=previous;native_fail(d,e);}
/* Linux sar.c converts dBm via chip->txgi_factor exponent:2indices/dB
 * on21C/22B and4 on22C. Apply after ordinary saturation and every thermal
 * lookup; fail closed if a transmitting legacy/HT rate cannot take the drop. */
static u8 native_power_index(struct rtw_dev*d,u8 index,u8 rate,const struct rtw_power_params*p){
 if(d->power_limit_mbm!=INT32_MAX){int cap=p->pwr_base+p->pwr_sar;if(cap<0){if(rate>=DESC_RATE6M&&rate<=DESC_RATEMCS15)native_fail(d,ERANGE);return 0;}if((int)index>cap)index=(u8)cap;}
 unsigned reduction=(unsigned)d->power_reduction_db*(d->native->boot->firmware.family==RTW_WIFI_RTL8822CU?4U:2U);
 if(index<reduction){if(rate>=DESC_RATE6M&&rate<=DESC_RATEMCS15)native_fail(d,ERANGE);return 0;}
 return (u8)(index-reduction);
}
#include "RTW88PHYOperatingPower.inc"
#include "RTW88PHYOperatingCommon.inc"
/* Thermal AGC headroom is a property of the original factory/regulatory
 * power plan. Computing it from a user-capped table creates extra positive
 * AGC and cancels the requested SAR/backoff at hardware saturation. Keep the
 * primary formula and thermal remnant, excluding only these user limits. */
static u8 native_thermal_power_index(struct rtw_dev*d,u8 path,u8 rate,enum rtw_bandwidth bw,u8 channel,u8 regd){
 struct rtw_power_params p={0};rtw_get_tx_power_params(d,path,rate,(u8)bw,channel,regd,&p);
 int offset=min(p.pwr_offset,p.pwr_limit);if(d->chip->en_dis_dpd)offset+=rtw_phy_get_dis_dpd_by_rate_diff(d,rate);
 return (u8)clamp_t(int,p.pwr_base+offset+p.pwr_remnant,0,d->chip->max_power_index);
}
#include "RTW88PHYOperating8821C.inc"
#include "RTW88PHYOperating8822B.inc"
#include "RTW88PHYOperating8822C.inc"
static const struct rtw_rfe_def*rtw_get_rfe_def(struct rtw_dev*d){return d->native->boot->firmware.family==RTW_WIFI_RTL8821CU?rtw8821c_rfe_defs+d->efuse.rfe_option:d->native->boot->firmware.family==RTW_WIFI_RTL8822BU?rtw8822b_rfe_defs+d->efuse.rfe_option:rtw8822c_rfe_defs+d->efuse.rfe_option;}
#pragma clang diagnostic pop
#define RADIO_MAGIC UINT32_C(0x38385250)
_Static_assert(sizeof(struct rtw_dev)<=RTW88_PHY_RADIO_SCRATCH_BYTES,"radio scratch too small");
_Static_assert(sizeof(struct rtw_txpwr_idx)==42,"pinned factory ROM layout");
static int native_overlap(const void*a,size_t an,const void*b,size_t bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return an&&bn&&(x<=y?y-x<an:x-y<bn);}
static int channel20(uint8_t p){return (p>=1&&p<=14)||(p>=36&&p<=64&&((p-36U)%4U)==0)||(p>=100&&p<=144&&((p-100U)%4U)==0)||(p>=149&&p<=177&&((p-149U)%4U)==0);}
int rtw88_phy_radio_validate_channel(uint8_t p,uint8_t c,uint8_t width,uint8_t index){
 if(!channel20(p))return EINVAL;
 if(width==20)return p==c&&index==0?0:EINVAL;
 int delta=(int)p-(int)c;
 if(width==40){if(!((delta==2&&index==1)||(delta==-2&&index==2)))return EINVAL;if(p<=14)return p<=13&&c>=3&&c<=11?0:EINVAL;
  if((c>=38&&c<=62&&((c-38U)%8U)==0)||(c>=102&&c<=142&&((c-102U)%8U)==0)||(c>=151&&c<=175&&((c-151U)%8U)==0))return 0;return EINVAL;}
 if(width!=80||p<=14)return EINVAL;
 if(c!=42&&c!=58&&c!=106&&c!=122&&c!=138&&c!=155&&c!=171)return EINVAL;
 return ((delta==2&&index==1)||(delta==-2&&index==2)||(delta==6&&index==3)||(delta==-6&&index==4))?0:EINVAL;
}
static int native_prerequisites(const rtw88_phy_context*c){return c&&c->boot&&c->mac&&c->mac->device==&c->boot->firmware&&c->mac->mac_ready&&c->tables_loaded&&c->boot->board_valid&&c->boot->physical_valid&&c->boot->firmware.identified&&c->boot->firmware.powered&&c->boot->firmware.firmware_running&&c->boot->firmware.bus.read&&c->boot->firmware.bus.write&&c->boot->firmware.bus.delay_us&&c->boot->board.rf_paths>=1&&c->boot->board.rf_paths<=2;}
/* Pinned reg.h/mac.c: 3081 firmware must match FW_READY under
 * FW_READY_MASK, excluding only the CPU-clock selector. Cached PHY readiness
 * and per-frame metadata remain pure; hardware changes use this live gate. */
static int native_live_health(struct rtw_dev*d){
 rtw88_phy_context*c=d->native;rtw88_fw_context*f=&c->boot->firmware;uint32_t value=0;int e;
 if(native_error(d))return native_error(d);
 if(!native_prerequisites(c)||d->power_epoch!=c->boot->power_epoch)e=ENXIO;
 else{e=f->bus.read(f->bus.cookie,0x80,2,&value);if(!e&&(value&0xcfffU)!=0xc078U)e=ENXIO;}
 if(!e)return 0;native_fail(d,e);d->status.parameters_ready=d->status.channel_ready=d->status.calibrated_ready=0;
 f->firmware_running=0;c->tables_loaded=0;c->mac->mac_ready=0;return native_error(d);
}
static struct rtw_dev*native_state(const rtw88_phy_context*c){struct rtw_dev*d=c?c->radio_state:NULL;return d&&d->magic==RADIO_MAGIC&&d->native==c?d:NULL;}
static int native_complete(struct rtw_dev*d){int e=native_error(d);if(!e)return 0;d->status.parameters_ready=d->status.channel_ready=d->status.calibrated_ready=0;d->status.error=e;
 /* Failed calibration never rejoins a running radio. The primary chip power-off
  * sequence is the final recovery, even after an earlier register failure. */
 (void)rtw88_boot_stop(d->native->boot);d->native->tables_loaded=0;d->native->mac->mac_ready=0;return e;
}
static void native_stage_begin(struct rtw_dev*d){d->operations=0;d->delay_total=0;}
static void native_chip(struct rtw_dev*d,rtw_wifi_family f){memset(&d->chip_value,0,sizeof(d->chip_value));d->chip=&d->chip_value;d->chip->max_power_index=(u8)(f==RTW_WIFI_RTL8822CU?0x7f:0x3f);d->chip->txgi_factor=(u8)(f==RTW_WIFI_RTL8822CU?2:1);d->chip->is_pwr_by_rate_dec=f!=RTW_WIFI_RTL8822CU;d->chip->en_dis_dpd=f==RTW_WIFI_RTL8822CU;d->chip->dpd_ratemask=f==RTW_WIFI_RTL8822CU?0x3ff:0;d->chip->iqk_threshold=8;d->chip->lck_threshold=(u8)(f==RTW_WIFI_RTL8822CU?8:0);}
static void native_set_power(struct rtw_dev*d){
 unsigned paths=d->hal.rf_path_num;rtw_wifi_family f=d->native->boot->firmware.family;
 for(unsigned path=0;path<paths;path++)for(unsigned rate=0;rate<=DESC_RATEVHT2SS_MCS9;rate++){
  int legal=(rate<=DESC_RATE54M)||(rate>=DESC_RATEMCS0&&rate<=DESC_RATEMCS15)||(rate>=DESC_RATEVHT1SS_MCS0&&rate<=DESC_RATEVHT2SS_MCS9);
  if(!legal)continue;if(f==RTW_WIFI_RTL8821CU&&((rate>=DESC_RATEMCS8&&rate<=DESC_RATEMCS15)||rate>=DESC_RATEVHT2SS_MCS0))continue;
  if(d->hal.current_channel>14&&rate<=DESC_RATE11M){d->hal.tx_pwr_tbl[path][rate]=0;continue;}
  d->hal.tx_pwr_tbl[path][rate]=(s8)rtw_phy_get_tx_power_index(d,(u8)path,(u8)rate,(enum rtw_bandwidth)d->hal.current_band_width,d->hal.current_channel,d->regd);
 }
 if(f==RTW_WIFI_RTL8821CU)rtw8821c_set_tx_power_index(d);else if(f==RTW_WIFI_RTL8822BU)rtw8822b_set_tx_power_index(d);else rtw8822c_set_tx_power_index(d);
}
static void native_reset_thermal(struct rtw_dev*d,rtw_wifi_family f){
 /* A new table must not inherit positive gain from the previous table.
  * 21C set_channel already restores its ROM band swing and tracker. 22B
  * has no such channel callback: restore each saved factory path separately. */
 if(f==RTW_WIFI_RTL8821CU)rtw_write32_mask(d,0xc94,GENMASK(6,1),0);
 else if(f==RTW_WIFI_RTL8822BU){
  for(unsigned path=0;path<d->hal.rf_path_num;++path){rtw_write32_mask(d,0xc94U+path*0x200U,GENMASK(29,25),0);rtw_write32_mask(d,0xc1cU+path*0x200U,GENMASK(31,21),d->thermal_swing[path]);}
  rtw8822b_pwrtrack_init(d);
 }else{
  for(unsigned path=0;path<d->hal.rf_path_num;++path)rtw_write32_mask(d,path?0x41a0:0x18a0,PWR_TRACK_MASK,0);
  rtw8822c_pwrtrack_init(d);
 }
}
int rtw88_phy_radio_initialize(rtw88_phy_context*c,void*scratch,size_t bytes,uint8_t regd){
 if(!c)return EINVAL;c->radio_state=NULL;
 if(!scratch||bytes<RTW88_PHY_RADIO_SCRATCH_BYTES||((uintptr_t)scratch&7U)||regd>=RTW_REGD_MAX||native_overlap(c,sizeof(*c),scratch,RTW88_PHY_RADIO_SCRATCH_BYTES))return EINVAL;
 if(!native_prerequisites(c))return ENXIO;
 if(native_overlap(c->boot,sizeof(*c->boot),scratch,RTW88_PHY_RADIO_SCRATCH_BYTES)||native_overlap(c->mac,sizeof(*c->mac),scratch,RTW88_PHY_RADIO_SCRATCH_BYTES))return EINVAL;
 if(c->boot->firmware.last_error)return c->boot->firmware.last_error;
 rtw_wifi_family f=c->boot->firmware.family;if(f!=RTW_WIFI_RTL8821CU&&f!=RTW_WIFI_RTL8822BU&&f!=RTW_WIFI_RTL8822CU)return EINVAL;
 struct rtw_dev*d=scratch;memset(d,0,sizeof(*d));d->magic=RADIO_MAGIC;d->power_limit_mbm=INT32_MAX;d->power_epoch=c->boot->power_epoch;d->native=c;d->packet_counter=&d->packet_sequence;d->regd=regd;c->radio_state=d;native_chip(d,f);if(native_live_health(d))return native_error(d);
 rtw88_board*b=&c->boot->board;unsigned paths=b->features_valid?b->active_rf_paths:b->rf_paths;if(!paths||paths>b->rf_paths){native_fail(d,EINVAL);return native_complete(d);}d->hal.rf_path_num=(u8)paths;d->hal.rf_type=paths==1?RF_1T1R:RF_2T2R;d->hal.antenna_rx=d->hal.antenna_tx=paths==1?BB_PATH_A:BB_PATH_AB;d->hal.cut_version=b->cut;d->hal.rfe_btg=b->rfe_btg;memcpy(d->hal.ch_param,c->channel_filter,sizeof(d->hal.ch_param));d->hal.current_channel=1;d->hal.primary_channel=1;d->hal.cch_by_bw[0]=1;
 d->efuse.rfe_option=b->rfe_option;d->efuse.power_track_type=b->power_track_type;d->efuse.thermal_meter_k=b->thermal_meter_k;d->efuse.thermal_meter[0]=b->thermal_meter[0];d->efuse.thermal_meter[1]=b->thermal_meter[1];d->efuse.tx_bb_swing_setting_2g=b->tx_bb_swing_setting_2g;d->efuse.tx_bb_swing_setting_5g=b->tx_bb_swing_setting_5g;memcpy(d->efuse.txpwr_idx_table,b->txpwr_idx_table,sizeof(d->efuse.txpwr_idx_table));
 /* Unknown factory base indices cannot authorize transmit power. Signed
  * differential nibbles remain valid,including erased -1 encodings. */
 for(unsigned path=0;path<paths;path++){const struct rtw_txpwr_idx*p=&d->efuse.txpwr_idx_table[path];for(unsigned i=0;i<6;i++)if(p->pwr_idx_2g.cck_base[i]>d->chip->max_power_index){native_fail(d,EINVAL);return native_complete(d);}for(unsigned i=0;i<5;i++)if(p->pwr_idx_2g.bw40_base[i]>d->chip->max_power_index){native_fail(d,EINVAL);return native_complete(d);}for(unsigned i=0;i<14;i++)if(p->pwr_idx_5g.bw40_base[i]>d->chip->max_power_index){native_fail(d,EINVAL);return native_complete(d);}}
 native_stage_begin(d);rtw_phy_init_tx_power(d);const struct rtw_rfe_def*rfe=rtw_get_rfe_def(d);rtw_load_table(d,rfe->phy_pg_tbl);rtw_load_table(d,rfe->txpwr_lmt_tbl);rtw_phy_tx_power_by_rate_config(&d->hal);rtw_phy_tx_power_limit_config(&d->hal);
 u8 old_pause=rtw_read8(d,0x522);rtw_write8(d,0x522,255);
 if(f==RTW_WIFI_RTL8822BU){rtw8822b_config_trx_mode(d,(u8)d->hal.antenna_tx,(u8)d->hal.antenna_rx,false);rtw8822b_phy_rfe_init(d);for(unsigned path=0;path<paths;++path)d->thermal_swing[path]=rtw_read32_mask(d,0xc1cU+path*0x200U,GENMASK(31,21));rtw8822b_pwrtrack_init(d);}else if(f==RTW_WIFI_RTL8822CU){rtw8822c_config_trx_mode(d,(u8)d->hal.antenna_tx,(u8)d->hal.antenna_rx,false);d->dm_info.dpk_info.is_dpk_pwr_on=true;rtw8822c_rf_init(d);rtw8822c_pwrtrack_init(d);}else rtw8821c_pwrtrack_init(d);
 if(native_complete(d))return d->status.error;d->status.parameters_ready=1;rtw_write8(d,0x522,old_pause);return native_complete(d);
}
int rtw88_phy_radio_set_power_limit_mbm(rtw88_phy_context*c,int32_t mbm){
 struct rtw_dev*d=native_state(c);if(!d||!native_prerequisites(c)||!d->status.parameters_ready)return ENXIO;
 if(native_live_health(d))return native_error(d);rtw_write8(d,0x522,255);
 d->status.calibrated_ready=0;d->power_needs_tune=1;if(!native_error(d))d->power_limit_mbm=mbm;return native_complete(d);
}
int rtw88_phy_radio_set_power_reduction(rtw88_phy_context*c,uint8_t db){
 struct rtw_dev*d=native_state(c);if(!d||!native_prerequisites(c)||!d->status.parameters_ready)return ENXIO;
 if(native_live_health(d))return native_error(d);rtw_write8(d,0x522,255);
 d->status.calibrated_ready=0;d->power_needs_tune=1;if(!native_error(d))d->power_reduction_db=db;return native_complete(d);
}
int rtw88_phy_radio_tune(rtw88_phy_context*c,uint8_t primary,uint8_t center,uint8_t width,uint8_t index){struct rtw_dev*d=native_state(c);int e=rtw88_phy_radio_validate_channel(primary,center,width,index);if(e)return e;if(d&&(d->power_reduction_db||d->power_limit_mbm!=INT32_MAX)&&width!=20)return ENOTSUP;if(!d||!native_prerequisites(c)||!d->status.parameters_ready||d->power_epoch!=c->boot->power_epoch)return ENXIO;if(native_error(d))return native_error(d);if(native_live_health(d))return native_error(d);
 d->status.channel_ready=d->status.calibrated_ready=0;d->associated=false;d->iqk_complete=0;native_stage_begin(d);u8 old_pause=rtw_read8(d,0x522);rtw_write8(d,0x522,255);
 d->dm_info.tx_rate=(u8)(primary<=14?DESC_RATE1M:DESC_RATE6M);d->hal.current_channel=center;d->hal.primary_channel=primary;d->hal.current_band_width=(u8)(width==20?0:width==40?1:2);d->hal.cch_by_bw[0]=primary;d->hal.cch_by_bw[1]=(u8)(width==80?(primary>center?center+4U:center-4U):center);d->hal.cch_by_bw[2]=center;
 rtw_wifi_family f=c->boot->firmware.family;if(f==RTW_WIFI_RTL8821CU)rtw8821c_set_channel(d,center,d->hal.current_band_width,index);else if(f==RTW_WIFI_RTL8822BU)rtw8822b_set_channel(d,center,d->hal.current_band_width,index);else rtw8822c_set_channel(d,center,d->hal.current_band_width,index);
 if(!native_error(d))native_reset_thermal(d,f);if(!native_error(d))native_set_power(d);if(native_complete(d))return d->status.error;
 d->status.primary_channel=primary;d->status.center_channel=center;d->status.width_mhz=width;d->status.primary_index=index;d->status.channel_ready=1;d->power_needs_tune=0;rtw_write8(d,0x522,old_pause);return native_complete(d);
}
int rtw88_phy_radio_calibrate(rtw88_phy_context*c){struct rtw_dev*d=native_state(c);if(!d||!native_prerequisites(c)||!d->status.parameters_ready||!d->status.channel_ready||d->power_needs_tune||d->power_epoch!=c->boot->power_epoch)return ENXIO;if(native_error(d))return native_error(d);if(native_live_health(d))return native_error(d);d->status.calibrated_ready=0;d->iqk_complete=0;native_stage_begin(d);u8 pause=rtw_read8(d,0x522);rtw_write8(d,0x522,255);
 rtw_wifi_family f=c->boot->firmware.family;if(f==RTW_WIFI_RTL8821CU)rtw8821c_phy_calibration(d);else if(f==RTW_WIFI_RTL8822BU)rtw8822b_phy_calibration(d);else rtw8822c_phy_calibration(d);
 if(!d->iqk_complete&&!native_error(d))native_fail(d,EIO);
 if(f==RTW_WIFI_RTL8822CU&&(!d->dac_complete||!test_bit(0,d->dm_info.dpk_info.dpk_path_ok)||(d->hal.rf_path_num>1&&!test_bit(1,d->dm_info.dpk_info.dpk_path_ok))))native_fail(d,EIO);
 if(native_complete(d))return d->status.error;d->status.iqk_complete=d->iqk_complete;d->status.dac_complete=d->dac_complete;d->status.dpk_paths=(u8)(d->dm_info.dpk_info.dpk_path_ok[0]&3U);d->status.calibrated_ready=1;rtw_write8(d,0x522,pause);return native_complete(d);
}
int rtw88_phy_radio_set_associated(rtw88_phy_context*c,int associated){struct rtw_dev*d=native_state(c);if(associated!=0&&associated!=1)return EINVAL;if(!d||(associated&&!rtw88_phy_radio_ready(c)))return ENXIO;d->associated=associated!=0;return 0;}
int rtw88_phy_radio_set_h2c_sequence(rtw88_phy_context*c,uint16_t*sequence){struct rtw_dev*d=native_state(c);if(!d||!d->status.parameters_ready)return ENXIO;if(!sequence||((uintptr_t)sequence&1U))return EINVAL;d->packet_counter=sequence;return 0;}
int rtw88_phy_radio_set_tx_rate(rtw88_phy_context*c,uint8_t rate){struct rtw_dev*d=native_state(c);if(!d||!native_prerequisites(c)||!d->status.channel_ready||d->power_epoch!=c->boot->power_epoch)return ENXIO;int valid_rate=rate<=DESC_RATE54M||(rate>=DESC_RATEMCS0&&rate<=DESC_RATEMCS15)||(rate>=DESC_RATEVHT1SS_MCS0&&rate<=DESC_RATEVHT2SS_MCS9);if(!valid_rate||(d->hal.current_channel>14&&rate<=DESC_RATE11M)||((d->hal.rf_path_num==1||d->hal.txrx_1ss)&&((rate>=DESC_RATEMCS8&&rate<=DESC_RATEMCS15)||rate>=DESC_RATEVHT2SS_MCS0)))return EINVAL;d->dm_info.tx_rate=rate;return 0;}
int rtw88_phy_radio_set_txrx_1ss(rtw88_phy_context*c,int enabled){
 if(enabled!=0&&enabled!=1)return EINVAL;struct rtw_dev*d=native_state(c);
 if(!d||!native_prerequisites(c)||!d->status.parameters_ready||d->power_epoch!=c->boot->power_epoch)return ENXIO;
 if(c->boot->firmware.family!=RTW_WIFI_RTL8822CU)return ENOTSUP;if(native_error(d))return native_error(d);if(native_live_health(d))return native_error(d);
 native_stage_begin(d);u8 pause=rtw_read8(d,0x522);rtw_write8(d,0x522,255);
 /* Linux main.c rtw_set_txrx_1ss: keep factory antenna maps for restore.
  * Reapply even at the same logical setting after a completed RFK. */
 rtw8822c_config_trx_mode(d,enabled?BB_PATH_A:(u8)d->hal.antenna_tx,enabled?BB_PATH_A:(u8)d->hal.antenna_rx,false);
 if(native_complete(d))return d->status.error;d->hal.txrx_1ss=enabled!=0;d->status.txrx_1ss=(u8)enabled;rtw_write8(d,0x522,pause);return native_complete(d);
}
int rtw88_phy_radio_thermal_track(rtw88_phy_context*c){struct rtw_dev*d=native_state(c);if(!d||!rtw88_phy_radio_ready(c))return ENXIO;if(native_live_health(d))return native_error(d);native_stage_begin(d);rtw_wifi_family f=c->boot->firmware.family;if(f==RTW_WIFI_RTL8821CU)rtw8821c_pwr_track(d);else if(f==RTW_WIFI_RTL8822BU)rtw8822b_pwr_track(d);else{rtw8822c_pwr_track(d);if(!native_error(d))rtw8822c_dpk_track(d);}if(!native_error(d))(void)native_live_health(d);return native_complete(d);}
rtw88_phy_radio_status rtw88_phy_radio_get_status(const rtw88_phy_context*c){rtw88_phy_radio_status out={0};struct rtw_dev*d=native_state(c);if(!d){out.error=ENXIO;return out;}out=d->status;out.iqk_complete=d->iqk_complete;out.dac_complete=d->dac_complete;if(!native_prerequisites(c)||native_error(d)||d->power_epoch!=c->boot->power_epoch){out.parameters_ready=out.channel_ready=0;out.calibrated_ready=0;out.error=native_error(d)?native_error(d):ENXIO;}return out;}
int rtw88_phy_radio_ready(const rtw88_phy_context*c){struct rtw_dev*d=native_state(c);return d&&native_prerequisites(c)&&d->power_epoch==c->boot->power_epoch&&!native_error(d)&&d->status.parameters_ready&&d->status.channel_ready&&d->status.calibrated_ready;}
void rtw88_phy_radio_invalidate(rtw88_phy_context*c){struct rtw_dev*d=native_state(c);if(d){memset(&d->status,0,sizeof(d->status));d->magic=0;}if(c)c->radio_state=NULL;}
