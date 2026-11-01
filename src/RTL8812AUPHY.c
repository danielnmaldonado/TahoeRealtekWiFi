/* SPDX-License-Identifier: BSD-3-Clause */
/* Portable Linuxrtw88 RTL8812A/RTL8821A PHY adapter. See
 * RTL8812AUPHY-Upstream.json and RTL8812AUPHY-LICENSE.txt. */
#include "rtw/StationChannels.h"
#include "rtw/RTL8812AUPHY.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef int8_t s8;
#define BIT(n) (UINT32_C(1)<<(n))
#define GENMASK(h,l) ((UINT32_MAX<<(l))&(UINT32_MAX>>(31U-(h))))
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define abs_diff(a,b) ((a)>(b)?(a)-(b):(b)-(a))
#define min_t(t,a,b) ((t)(a)<(t)(b)?(t)(a):(t)(b))
#define max_t(t,a,b) ((t)(a)>(t)(b)?(t)(a):(t)(b))
#define RTW_BAND_2G 0
#define RTW_BAND_5G 1
#define RTW_CHANNEL_WIDTH_20 0
#define RTW_CHANNEL_WIDTH_40 1
#define RTW_CHANNEL_WIDTH_80 2
#define RTW_CHANNEL_WIDTH_5 3
#define RTW_CHANNEL_WIDTH_10 4
#define RTW_CHIP_TYPE_8812A 1
#define RTW_CHIP_TYPE_8821A 2
#define RTW_CHIP_VER_CUT_C 2
#define RTW_HCI_TYPE_USB 2
#define RTW_HCI_TYPE_PCIE 1
#define RF_1T1R 1
#define RF_2T2R 2
#define RTW_RF_PATH_MAX 2
#define RFREG_MASK 0xfffffU
#define RF18_BW_MASK (BIT(11)|BIT(10))
#define MASKLWORD 0xffffU
#define RTW_SC_20_UPPER 1
#define RTW_SC_20_LOWER 2
#define RTW_SC_20_LOWEST 4
#define RTW_SC_20_UPMOST 3
#define RTW_SC_40_UPPER 9
#define RTW_SC_40_LOWER 10
#define MASKBYTE0 0xffU
#define MASKDWORD UINT32_MAX
#define INV_RF_DATA UINT32_MAX
#define MACBB_REG_NUM_8812A 9
#define AFE_REG_NUM_8812A 12
#define RF_REG_NUM_8812A 3
#define MACBB_REG_NUM_8821A 8
#define AFE_REG_NUM_8821A 4
#define RF_REG_NUM_8821A 3
#define CAL_NUM_8821A 3
#define DESC_RATE1M 0
#define DESC_RATE2M 1
#define DESC_RATE5_5M 2
#define DESC_RATE11M 3
#define DESC_RATE6M 4
#define DESC_RATE12M 6
#define DESC_RATE24M 8
#define RTW_HCI_TYPE(r) RTW_HCI_TYPE_USB
#define rtw_hci_type(r) RTW_HCI_TYPE_USB
enum rtw_rf_path{RF_PATH_A,RF_PATH_B};
#include "RTL8812AURegisters.inc"
struct rtw_hal{u8 current_band_type,current_band_width,current_channel,cut_version,rf_path_num,rf_phy_num,rf_type;};
struct rtw_chip_info{u8 id;};
struct rtw_efuse{u8 rfe_option,tx_bb_swing_setting_2g,tx_bb_swing_setting_5g,thermal_meter_k,ext_pa_2g,ext_pa_5g,ext_lna_2g,btcoex,ant_div_cfg;};
struct thermal_average{u32 value;};
struct rtw_dm_info{struct thermal_average avg_thermal[2];u8 default_ofdm_index,default_cck_index,thermal_meter_k;int8_t delta_power_index[2],delta_power_index_last[2];bool pwr_trk_triggered,pwr_trk_init_trigger;u8 thermal_avg[2],tx_rate;s8 txagc_remnant_ofdm[2],txagc_remnant_cck;};
struct rtw_r12au_phy{
 rtw_usb_rw_transport bus;rtw_waiter waiter;rtw_r12au_phy_board board;
 struct {u8 type;}hci;struct rtw_hal hal;struct rtw_chip_info chip_value;const struct rtw_chip_info *chip;
 struct rtw_efuse efuse;struct rtw_dm_info dm_info;
 int io_error;bool cleanup,loaded,tuned,calibrated,tx_enabled,cal_timeout;
 u8 tx_iqk_mask,rx_iqk_mask,power[2][64],power_base[2][64],thermal,power_reduction_db,power_ceiling[2][64];int32_t power_limit_mbm;u32 thermal_samples,thermal_recalibrations;
 rtw_r12au_operating operating;uint64_t power_valid;
 const void*coex_context;rtw_r12au_coex_guard coex_guard;
};
struct rtw_phy_pg_cfg_pair{u32 band,path,txnum,addr,mask,data;};
struct rtw_txpwr_lmt_cfg_pair{u8 regd,band,bw,rs,ch;int8_t value;};
#include "RTL8812AUTables.inc"
static const u32 r12_txscale_tbl[37]={0x081,0x088,0x090,0x099,0x0a2,0x0ac,0x0b6,0x0c0,0x0cc,0x0d8,0x0e5,0x0f2,0x101,0x110,0x120,0x131,0x143,0x156,0x16a,0x180,0x197,0x1af,0x1c8,0x1e3,0x200,0x21e,0x23e,0x261,0x285,0x2ab,0x2d3,0x2fe,0x32b,0x35c,0x38e,0x3c4,0x3fe};
static unsigned shift(u32 m){unsigned s=0;if(m)while(!(m&1U)){++s;m>>=1;}return s;}
static void fault(struct rtw_r12au_phy*p,int e){if(e&&!p->io_error)p->io_error=e;}
static u32 rd(struct rtw_r12au_phy*p,u32 a,u8 w){u32 v=0;if(p->io_error&&!p->cleanup)return 0;if(a>UINT16_MAX){fault(p,EINVAL);return 0;}if(rtw_usb_rw_read(&p->bus,(u16)a,w,&v)!=RTW_USB_OK)fault(p,EIO);return v;}
static void wr(struct rtw_r12au_phy*p,u32 a,u8 w,u32 v){if(p->io_error&&!p->cleanup)return;if(a>UINT16_MAX){fault(p,EINVAL);return;}if(rtw_usb_write_register(&p->bus,(u16)a,w,v)!=RTW_USB_OK)fault(p,EIO);}
static u8 rtw_read8(struct rtw_r12au_phy*p,u32 a){return(u8)rd(p,a,1);}
static u16 rtw_read16(struct rtw_r12au_phy*p,u32 a){return(u16)rd(p,a,2);}
static u32 rtw_read32(struct rtw_r12au_phy*p,u32 a){return rd(p,a,4);}
static u32 rtw_read32_mask(struct rtw_r12au_phy*p,u32 a,u32 m){return(rd(p,a,4)&m)>>shift(m);}
static void rtw_write8(struct rtw_r12au_phy*p,u32 a,u8 v){wr(p,a,1,v);}
static void rtw_write16(struct rtw_r12au_phy*p,u32 a,u16 v){wr(p,a,2,v);}
static void rtw_write32(struct rtw_r12au_phy*p,u32 a,u32 v){wr(p,a,4,v);}
static void rtw_write32_mask(struct rtw_r12au_phy*p,u32 a,u32 m,u32 v){u32 old=rd(p,a,4);wr(p,a,4,(old&~m)|((v<<shift(m))&m));}
static void rtw_write8_set(struct rtw_r12au_phy*p,u32 a,u8 v){wr(p,a,1,rd(p,a,1)|v);}
static void rtw_write8_clr(struct rtw_r12au_phy*p,u32 a,u8 v){wr(p,a,1,rd(p,a,1)&(u32)(u8)~v);}
static void rtw_write32_set(struct rtw_r12au_phy*p,u32 a,u32 v){wr(p,a,4,rd(p,a,4)|v);}
static void rtw_write32_clr(struct rtw_r12au_phy*p,u32 a,u32 v){wr(p,a,4,rd(p,a,4)&~v);}
static void r12_delay(struct rtw_r12au_phy*p,u32 us){if(p->io_error&&!p->cleanup)return;if(!p->waiter.wait_us(p->waiter.context,us))fault(p,EIO);}
#define udelay(us) r12_delay(rtwdev,(u32)(us))
#define mdelay(ms) r12_delay(rtwdev,(u32)(ms)*1000U)
#define msleep(ms) mdelay(ms)
#define rtw_err(r,...) ((void)(r))
#define ewma_thermal_init(a) ((a)->value=0)
static u32 r12_phy_read_rf(struct rtw_r12au_phy*,enum rtw_rf_path,u32,u32);
static u32 rtw_read_rf(struct rtw_r12au_phy*p,enum rtw_rf_path path,u32 a,u32 m){return r12_phy_read_rf(p,path,a,m);}
static void rtw_write_rf(struct rtw_r12au_phy*p,enum rtw_rf_path path,u32 a,u32 m,u32 v){
 if(!m||(m&~RFREG_MASK)||(unsigned)path>RF_PATH_B){fault(p,EINVAL);return;}
 /* The pinned SIPI writer returns false without I/O for absent paths.
  * Its8812 IQK callback also visits pathB on an OTP1T1R module. That
  * absent path cannot latch a bus failure or count towards measured RFK. */
 if((unsigned)path>=p->hal.rf_phy_num)return;
 if(m!=RFREG_MASK){u32 old=rtw_read_rf(p,path,a,RFREG_MASK);v=(old&~m)|((v<<shift(m))&m);}
 wr(p,path==RF_PATH_A?REG_LSSI_WRITE_A:REG_LSSI_WRITE_B,4,((a&0xffU)<<20)|(v&RFREG_MASK));r12_delay(p,13);
}
static void r12_wait_txempty(struct rtw_r12au_phy*p){for(unsigned i=0;i<50&&!p->io_error;++i){if((rd(p,REG_TXPKT_EMPTY,2)&0x30U)==0x30U)return;r12_delay(p,50);}fault(p,ETIMEDOUT);}
#include "RTL8812AUAlgorithms.inc"
#undef udelay
#undef mdelay
#undef msleep

static bool positive(const struct rtw_r12au_phy*p,u32 c,u32 c2){
 const rtw_r12au_phy_board*b=&p->board;unsigned cut=(c>>24)&15,pkg=(c>>12)&15,intf=(c>>8)&15;
 unsigned target=(unsigned)b->ext_lna_2g|((unsigned)b->ext_pa_2g<<1)|((unsigned)b->ext_lna_5g<<2)|((unsigned)b->ext_pa_5g<<3)|((unsigned)b->btcoex<<4);
 if(cut&&cut!=(b->cut?b->cut:15))return false;
 if(pkg&&pkg!=(b->package_type?b->package_type:15))return false;
 if(intf&&intf!=2)return false;
 if(!(c&15U))return true;
 if(((c&0xffU)&target)!=(c&0xffU))return false;
 const u8 types[4]={b->glna_type,b->gpa_type,b->alna_type,b->apa_type};
 for(unsigned i=0;i<4;++i)if((c&BIT(i))&&((c2>>(8*i))&255U)!=types[i])return false;
 return true;
}
static void apply(struct rtw_r12au_phy*p,const u32*t,size_t n,int mode){
 bool matched=true,skipped=false;u32 c=0,c2=0;
 if(n%2){fault(p,EINVAL);return;}
 for(size_t i=0;i<n&&!p->io_error;i+=2){u32 a=t[i],v=t[i+1];
  if(a&BIT(31)){switch((a>>28)&3U){case 3:matched=true;skipped=false;break;case 2:matched=!skipped;break;default:c=a;c2=v;break;}continue;}
  if(a&BIT(30)){if(skipped)matched=false;else{matched=positive(p,c,c2);skipped=matched;}continue;}
  if(!matched)continue;
  if(mode<0){u32 delay=0;switch(a){case 0xfe:delay=50000;break;case 0xfd:delay=5000;break;case 0xfc:delay=1000;break;case 0xfb:delay=50;break;case 0xfa:delay=5;break;case 0xf9:delay=1;break;default:break;}if(delay)r12_delay(p,delay);else wr(p,a,4,v);}
  else if(a==0xffe)r12_delay(p,50000);else if(a==0xfe)r12_delay(p,100);else{rtw_write_rf(p,(enum rtw_rf_path)mode,a,RFREG_MASK,v);r12_delay(p,1);}
 }
}
static bool allowed(u8 ch){return(ch>=1&&ch<=11)||rtw_station_5ghz_channel(ch);}
/* Pinned Linux phy.c rtw_get_channel_group: factory 5G power groups are
 * distinct across DFS. In particular, middle-band inputs cannot use group10.
 * allowed()/operating_geometry() check the actual primary and width first. */
static unsigned group(u8 ch){if(ch<=2)return 0;if(ch<=5)return 1;if(ch<=8)return 2;if(ch<=11)return 3;if(ch<=42)return 0;if(ch<=50)return 1;if(ch<=58)return 2;if(ch<=64)return 3;if(ch<=106)return 4;if(ch<=114)return 5;if(ch<=122)return 6;if(ch<=130)return 7;if(ch<=138)return 8;if(ch<=144)return 9;if(ch<=155)return 10;if(ch<=161)return 11;return 12;}
static int bcd(u8 v){if((v&15)>9||(v>>4)>9)return-1;return(v>>4)*10+(v&15);}
static int signed4(u8 v){int x=v&15;return(x&8)?x-16:x;}
static int prepare_power(struct rtw_r12au_phy*p,u8 ch){rtw_r12au_operating op={ch,ch,20,RTW_R12AU_MODE_LEGACY};rtw_r12au_power_plan plan;int e=rtw_r12au_phy_power_plan(&p->board,&op,&plan);if(e)return e;memcpy(p->power,plan.index,sizeof(p->power));memcpy(p->power_base,plan.index,sizeof(p->power_base));p->power_valid=plan.valid_rates;p->operating=op;return 0;}
static int pause_tx(struct rtw_r12au_phy*p){bool cleanup=p->cleanup;p->cleanup=true;wr(p,REG_TXPAUSE,1,0x3f);p->cleanup=cleanup;p->tx_enabled=false;return p->io_error;}
/* RTL8821 PA/LNA masks also cross-checked against FreeBSD r21a_rom.c and
 * r12a_rom_image.h at685dc743dc3b5645e34836464128e1c0558b404b.
 * See RTL8812AUROM-FreeBSD-LICENSE.txt for the originalBSD-2-Clause notice. */
int rtw_r12au_phy_parse_board(rtw_r12au_chip chip,u8 cut,u8 paths,unsigned bt,
 const u8 map[512],rtw_r12au_phy_board*out){
 rtw_r12au_phy_board b;unsigned nonzero=0;
 if(!map||!out||bt>1)return EINVAL;
 /* Reject overlap before clearing output so a failed decode cannot corruptOTP. */
 uintptr_t a=(uintptr_t)out,m=(uintptr_t)map;
 if((a>=m&&a-m<512)||(m>=a&&m-a<sizeof(*out)))return EINVAL;
 memset(out,0,sizeof(*out));
 if((chip!=RTW_R12AU_CHIP_8812AU&&chip!=RTW_R12AU_CHIP_8821AU)||cut>14||
    map[0]!=0x29||map[1]!=0x81||map[0xb9]>63||!map[0xba]||map[0xba]>=63||
    map[0xbc]==255||map[0xbd]==255||map[0xbf]==255||map[0xc1]==255)return ENOTSUP;
 if((chip==RTW_R12AU_CHIP_8812AU&&(paths<1||paths>2))||
    (chip==RTW_R12AU_CHIP_8821AU&&paths!=1))return ENOTSUP;
 unsigned mac=chip==RTW_R12AU_CHIP_8812AU?0xd7:0x107;
 if(map[mac]&1)return EINVAL;
 for(unsigned i=0;i<6;++i)nonzero|=map[mac+i];if(!nonzero)return EINVAL;
 memset(&b,0,sizeof(b));b.chip=chip;b.cut=cut;b.rf_paths=paths;
 b.crystal=map[0xb9];b.thermal=map[0xba];b.swing_2g=map[0xc6];b.swing_5g=map[0xc7];
 if(chip==RTW_R12AU_CHIP_8812AU){
  b.ext_pa_2g=(map[0xbc]&0x30)==0x30;b.ext_pa_5g=(map[0xbc]&3)==3;
  b.ext_lna_2g=(map[0xbd]&0x88)==0x88;b.ext_lna_5g=(map[0xbf]&0x88)==0x88;
  if(b.ext_pa_2g)b.gpa_type=(u8)(((map[0xbd]>>6)&1)*4+((map[0xbd]>>2)&1));
  if(b.ext_pa_5g)b.apa_type=(u8)(((map[0xbf]>>6)&1)*4+((map[0xbf]>>2)&1));
  if(b.ext_lna_2g)b.glna_type=(u8)(((map[0xbd]>>4)&3)*4+(map[0xbd]&3));
  if(b.ext_lna_5g)b.alna_type=(u8)(((map[0xbf]>>4)&3)*4+(map[0xbf]&3));
  b.rfe=map[0xca]&0x3f;
  /* rtw8812a_read_rfe_type: the explicit USB0xff fallback precedes the
   * bit7 amplifier inference. It does not authorize other unknown RFEs. */
  if(map[0xca]==255)b.rfe=0;
  else if(map[0xca]&0x80)b.rfe=(u8)(b.ext_lna_5g?(b.ext_pa_5g?(b.ext_lna_2g&&b.ext_pa_2g?3:0):2):4);
  else if(b.rfe==4&&(b.ext_pa_2g||b.ext_pa_5g||b.ext_lna_2g||b.ext_lna_5g))b.rfe=0;
  b.btcoex=(map[0xc1]&0xe0)==0x20;
 }else{
  /* 8821 has one chain. Do not apply the8812two-chain0x30/0x88 masks. */
  b.ext_pa_2g=(map[0xbc]&0x10)!=0;b.ext_pa_5g=(map[0xbc]&1)!=0;
  b.ext_lna_2g=(map[0xbd]&8)!=0;b.ext_lna_5g=(map[0xbf]&8)!=0;
  if(b.ext_pa_2g)b.gpa_type=(map[0xbd]>>2)&1;
  if(b.ext_pa_5g)b.apa_type=(map[0xbf]>>2)&1;
  if(b.ext_lna_2g)b.glna_type=map[0xbd]&3;
  if(b.ext_lna_5g)b.alna_type=map[0xbf]&3;
  b.rfe=0;b.btcoex=(u8)bt;
 }
 memcpy(b.txpwr_idx[0],map+0x10,42);memcpy(b.txpwr_idx[1],map+0x3a,42);
 *out=b;return 0;
}
static int board_supported(const rtw_r12au_phy_board*b){
 if(!b)return 0;
 if((b->chip!=RTW_R12AU_CHIP_8812AU&&b->chip!=RTW_R12AU_CHIP_8821AU)||b->cut>14||b->crystal>63||b->thermal==0||b->thermal>=63||b->package_type>15||b->btcoex>1||(b->btcoex&&b->chip!=RTW_R12AU_CHIP_8821AU))return 0;
 /* Active BT is implemented only for8821. PHY preparation stays paused;
  * resume additionally needs its matching live typed coex guard. No8812
  * BT policy/antenna tables are supplied by the pinned8812 descriptor. */
 if((b->chip==RTW_R12AU_CHIP_8812AU&&(b->rf_paths<1||b->rf_paths>2||b->rfe>3))||(b->chip==RTW_R12AU_CHIP_8821AU&&(b->rf_paths!=1||b->rfe!=0)))return 0;
 if(b->ext_pa_2g>1||b->ext_pa_5g>1||b->ext_lna_2g>1||b->ext_lna_5g>1||b->gpa_type>15||b->apa_type>15||b->glna_type>15||b->alna_type>15)return 0;
 return 1;
}
#include "RTL8812AUPowerModes.inc"
int rtw_r12au_phy_channel_supported(const rtw_r12au_phy_board*b,u8 channel,unsigned width){
 if(!board_supported(b)||!allowed(channel)||width!=20)return 0;
 struct rtw_r12au_phy p;memset(&p,0,sizeof(p));p.board=*b;p.chip_value.id=(u8)b->chip;p.chip=&p.chip_value;p.hal.rf_phy_num=b->rf_paths;return prepare_power(&p,channel)==0;
}
size_t rtw_r12au_phy_size(void){return sizeof(struct rtw_r12au_phy);}
int rtw_r12au_phy_init(void*storage,size_t n,const rtw_r12au_phy_board*b,const rtw_usb_rw_transport*bus,const rtw_waiter*waiter,rtw_r12au_phy**out){
 if(out)*out=NULL;if(!out||!storage||n<sizeof(struct rtw_r12au_phy)||((uintptr_t)storage%_Alignof(struct rtw_r12au_phy))||!b||!bus||!bus->control_in||!bus->control_out||!waiter||!waiter->wait_us)return EINVAL;
 if(!board_supported(b))return ENOTSUP;
 struct rtw_r12au_phy*p=storage;rtw_r12au_phy_board copy=*b;rtw_usb_rw_transport buscopy=*bus;rtw_waiter waitcopy=*waiter;memset(p,0,sizeof(*p));p->power_limit_mbm=INT32_MAX;p->board=copy;p->bus=buscopy;p->waiter=waitcopy;
 p->hci.type=RTW_HCI_TYPE_USB;p->chip_value.id=(u8)copy.chip;p->chip=&p->chip_value;p->hal.rf_phy_num=copy.rf_paths;p->hal.rf_path_num=copy.rf_paths;p->hal.rf_type=copy.rf_paths==1?RF_1T1R:RF_2T2R;p->hal.cut_version=copy.cut;
 p->efuse.rfe_option=copy.rfe;p->efuse.tx_bb_swing_setting_2g=copy.swing_2g;p->efuse.tx_bb_swing_setting_5g=copy.swing_5g;p->efuse.thermal_meter_k=copy.thermal;p->efuse.ext_pa_2g=copy.ext_pa_2g;p->efuse.ext_pa_5g=copy.ext_pa_5g;p->efuse.ext_lna_2g=copy.ext_lna_2g;p->efuse.btcoex=copy.btcoex;
 *out=p;return 0;
}
int rtw_r12au_phy_load_tables(rtw_r12au_phy*p){
 if(!p)return EINVAL;if(p->io_error)return p->io_error;if(p->loaded)return EBUSY;
 if(pause_tx(p))return p->io_error;
 rtw_write8_set(p,REG_SYS_FUNC_EN,BIT_FEN_USBA);rtw_write8_set(p,REG_SYS_FUNC_EN,BIT_FEN_BB_RSTB|BIT_FEN_BB_GLB_RST);
 wr(p,REG_RF_CTRL,1,7);wr(p,REG_RF_B_CTRL,1,7);
 if(p->chip->id==RTW_CHIP_TYPE_8812A){apply(p,rtw8812a_bb,ARRAY_SIZE(rtw8812a_bb),-1);apply(p,rtw8812a_agc,ARRAY_SIZE(rtw8812a_agc),-1);}
 else{apply(p,rtw8821a_bb,ARRAY_SIZE(rtw8821a_bb),-1);apply(p,rtw8821a_agc,ARRAY_SIZE(rtw8821a_agc),-1);}
 rtw_write32_mask(p,REG_AFE_CTRL3,p->chip->id==RTW_CHIP_TYPE_8812A?0x7ff80000U:0x00fff000U,((u32)p->board.crystal<<6)|p->board.crystal);
 if(p->chip->id==RTW_CHIP_TYPE_8812A){apply(p,rtw8812a_rf_a,ARRAY_SIZE(rtw8812a_rf_a),0);if(p->hal.rf_path_num>1)apply(p,rtw8812a_rf_b,ARRAY_SIZE(rtw8812a_rf_b),1);if(p->hal.rf_type==RF_1T1R)r12_chip12_config_1t(p);}
 else apply(p,rtw8821a_rf_a,ARRAY_SIZE(rtw8821a_rf_a),0);
 p->loaded=!p->io_error;return p->io_error;
}
int rtw_r12au_phy_set_power_limit_mbm(rtw_r12au_phy*p,int32_t mbm){
 if(!p)return EINVAL;if(p->io_error)return p->io_error;
 int e=pause_tx(p);p->tuned=p->calibrated=false;if(!e)p->power_limit_mbm=mbm;return e;
}
int rtw_r12au_phy_set_power_reduction(rtw_r12au_phy*p,u8 db){
 if(!p)return EINVAL;if(p->io_error)return p->io_error;
 int e=pause_tx(p);p->tuned=p->calibrated=false;if(!e)p->power_reduction_db=db;return e;
}
int rtw_r12au_phy_set_channel(rtw_r12au_phy*p,u8 ch,unsigned width){
 if(width!=20)return ENOTSUP;rtw_r12au_operating op={ch,ch,20,RTW_R12AU_MODE_LEGACY};return rtw_r12au_phy_set_operating(p,&op);
}
int rtw_r12au_phy_set_operating(rtw_r12au_phy*p,const rtw_r12au_operating*op){
 if(!p||!op)return EINVAL;if(p->io_error)return p->io_error;if(!p->loaded)return EBUSY;
 /* Planning can reject geometry or a later path/rate. Keep the live thermal
  * ceiling intact until the complete replacement plan has been validated. */
 rtw_r12au_power_plan plan;u8 ceiling[2][64];int e=power_plan_limit(&p->board,op,&plan,p->power_limit_mbm,ceiling);if(e)return e;u8 width,sc,centers[3];if(operating_geometry(op,&width,&sc,centers))return ENOTSUP;
 bool initial=!p->tuned;p->tuned=false;p->calibrated=false;p->tx_iqk_mask=p->rx_iqk_mask=0;
 if(pause_tx(p))return p->io_error;p->operating=*op;memcpy(p->power,plan.index,sizeof(p->power));memcpy(p->power_base,plan.index,sizeof(p->power_base));memcpy(p->power_ceiling,ceiling,sizeof(p->power_ceiling));p->power_valid=plan.valid_rates;
 p->hal.current_band_type=(u8)(op->primary>14);p->hal.current_band_width=width;p->hal.current_channel=op->center;
 if(initial)r12_switch_band(p,p->hal.current_band_type,width);
 r12_set_channel(p,op->center,width,sc);if(p->io_error){pause_tx(p);return p->io_error;}
 r12_set_channel_bb_swing(p,p->hal.current_band_type);p->dm_info.txagc_remnant_cck=0;p->dm_info.txagc_remnant_ofdm[0]=p->dm_info.txagc_remnant_ofdm[1]=0;program_operating_power(p);p->tuned=!p->io_error;return p->io_error;
}
int rtw_r12au_phy_calibrate(rtw_r12au_phy*p){
 if(!p)return EINVAL;if(p->io_error)return p->io_error;if(!p->tuned)return EBUSY;p->calibrated=false;p->tx_iqk_mask=0;p->rx_iqk_mask=0;p->cal_timeout=false;
 if(pause_tx(p))return p->io_error;u32 page=rd(p,REG_CCASEL,4),cca=rd(p,REG_CCA2ND,4);if(p->io_error)return p->io_error;
 if(p->chip->id==RTW_CHIP_TYPE_8812A){r12_chip12_do_lck(p);if(!p->io_error&&!p->cal_timeout)r12_chip12_do_iqk(p);}
 else r12_chip21_do_iqk(p);
 /* Only physically identified paths can contribute a measurement. Keep
  * the requirement for every present path: a failing A on1T or B on2T
  * must still leave TX paused. No fallback coefficient grants readiness. */
 u8 required=(u8)((1U<<p->hal.rf_phy_num)-1U);p->tx_iqk_mask&=required;p->rx_iqk_mask&=required;
 /* Restore page/CCA even on a failing backup or USBtransfer. */
 p->cleanup=true;wr(p,REG_CCASEL,4,page);wr(p,REG_CCA2ND,4,cca);p->cleanup=false;pause_tx(p);
 if(p->io_error)return p->io_error;
 if(p->cal_timeout||p->tx_iqk_mask!=required||p->rx_iqk_mask!=required)return ETIMEDOUT;
 if(p->chip->id==RTW_CHIP_TYPE_8812A&&p->hal.current_band_type){if(p->hal.current_channel<=64)apply(p,rtw8812a_agc_diff_lb,ARRAY_SIZE(rtw8812a_agc_diff_lb),-1);else apply(p,rtw8812a_agc_diff_hb,ARRAY_SIZE(rtw8812a_agc_diff_hb),-1);}
 p->calibrated=!p->io_error;return p->io_error;
}
int rtw_r12au_phy_bind_coex(rtw_r12au_phy*p,const void*context,rtw_r12au_coex_guard guard){if(!p||!context||!guard)return EINVAL;if(p->tx_enabled)return EBUSY;if(!p->board.btcoex||!guard(context,&p->board,&p->bus))return ENOTSUP;p->coex_context=context;p->coex_guard=guard;return 0;}
int rtw_r12au_phy_resume_tx(rtw_r12au_phy*p){if(!p)return EINVAL;if(p->io_error)return p->io_error;if(!p->calibrated)return EBUSY;if(p->board.btcoex&&(!p->coex_guard||!p->coex_guard(p->coex_context,&p->board,&p->bus)))return EBUSY;wr(p,REG_TXPAUSE,1,0);p->tx_enabled=!p->io_error;return p->io_error;}
#include "RTL8812AUThermal.inc"
int rtw_r12au_phy_stop(rtw_r12au_phy*p){if(!p)return EINVAL;p->calibrated=false;return pause_tx(p);}
void rtw_r12au_phy_forget(rtw_r12au_phy*p){if(!p)return;p->loaded=p->tuned=p->calibrated=p->tx_enabled=false;p->tx_iqk_mask=p->rx_iqk_mask=0;p->coex_context=NULL;p->coex_guard=NULL;}
void rtw_r12au_phy_get_status(const rtw_r12au_phy*p,rtw_r12au_phy_status*s){if(!s)return;memset(s,0,sizeof(*s));if(p){s->loaded=p->loaded;s->tuned=p->tuned;s->calibrated=p->calibrated;s->tx_enabled=p->tx_enabled;s->channel=p->operating.primary;s->primary=p->operating.primary;s->center=p->operating.center;s->width_mhz=p->operating.width_mhz;s->mode=(u8)p->operating.mode;s->thermal=p->thermal;s->thermal_samples=p->thermal_samples;s->thermal_recalibrations=p->thermal_recalibrations;s->rf_paths=p->hal.rf_phy_num;s->tx_iqk_mask=p->tx_iqk_mask;s->rx_iqk_mask=p->rx_iqk_mask;s->io_error=p->io_error;}}
