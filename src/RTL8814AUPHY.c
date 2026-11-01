/* SPDX-License-Identifier: BSD-3-Clause */
/* Portable RTL8814AU PHY, derived from Linux rtw88
 * 0e135910deec43ffe97eca0b6af1f8f847285b59, Copyright(c)2018-2019, 2025 Realtek.
 * See RTL8814AUPHY-LICENSE.txt. No allocation or platform I/O lives here. */
#include "rtw/RTL8814AUPHY.h"
#include <stdint.h>
#include <string.h>
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
#include <stdbool.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef int8_t s8;
#define BIT(n) (UINT32_C(1) << (n))
#define GENMASK(h,l) ((UINT32_MAX << (l)) & (UINT32_MAX >> (31U-(h))))
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define abs_diff(a,b) ((a)>(b)?(a)-(b):(b)-(a))
#define min_t(t,a,b) ((t)(a)<(t)(b)?(t)(a):(t)(b))
#define max_t(t,a,b) ((t)(a)>(t)(b)?(t)(a):(t)(b))
#define RTW_RF_PATH_MAX 4
#define RTW_BAND_2G 0
#define RTW_BAND_5G 1
#define RTW_CHANNEL_WIDTH_20 0
#define RTW_CHANNEL_WIDTH_40 1
#define RTW_CHANNEL_WIDTH_80 2
#define RTW_CHANNEL_WIDTH_5 3
#define RTW_CHANNEL_WIDTH_10 4
#define RTW_CHIP_VER_CUT_A 0
#define RTW_PWR_TRK_TBL_SZ 30
#define RTW_PWR_TRK_5G_NUM 3
#define PHY_5G_CHANNEL_COUNT 24
#define RF_PATH_A 0
#define RF_PATH_B 1
#define RF_PATH_C 2
#define RF_PATH_D 3
#define RTW_TXSCALE_SIZE 37
#define RFREG_MASK 0xfffffU
#define RF18_BW_MASK (BIT(11)|BIT(10))
#define MASKBYTE0 0xffU
#define MAC_REG_NUM_8814 2
#define BB_REG_NUM_8814 14
#define RF_REG_NUM_8814 1
#include "RTL8814AURegisters.inc"
struct rtw_hal {u8 current_band_type,current_band_width,current_channel,cut_version,rf_path_num;u32 ch_param[2];};
struct rtw_efuse {u8 rfe_option,tx_bb_swing_setting_2g,tx_bb_swing_setting_5g,thermal_meter_k;};
struct thermal_average {uint32_t value;};
struct rtw_dm_info {struct thermal_average avg_thermal[4];u8 default_ofdm_index,thermal_meter_k;int8_t delta_power_index[4],delta_power_index_last[4];bool pwr_trk_triggered,pwr_trk_init_trigger;};
struct rtw_8814au_phy {
    struct rtw_8814au *ctx;struct rtw_hal hal;struct rtw_efuse efuse;struct rtw_dm_info dm_info;
    bool cleanup,loaded,channel_set,lok_ok[4],tx_ok[4],rx_ok[4],resume_pending;
    u8 resume_txpause,tx_rate,power_constraint_db,applied_power_constraint_db;
    int8_t power_limit_dbm,applied_power_limit_dbm;
    int8_t by_rate[4][12],base_by_rate[4][2],limit[4][2][11];
    u8 power[4][28];
    int8_t by_rate_ht[2][4][16],base_ht[2][4][2],limit_ht[2][4][2][PHY_5G_CHANNEL_COUNT];
    int8_t by_rate_5g[4][12],base_5g[4],limit_5g[4][PHY_5G_CHANNEL_COUNT];
};
struct phy_pg_row {u32 band,path,txnum,addr,mask,data;};
struct phy_limit_row {u8 regd,band,bw,rs,ch;int8_t value;};
#include "RTL8814AUTables.inc"
static const u8*const swing_tables[2][2][2][4]={{{{phy14_pwrtrk_type0_2ga_n,phy14_pwrtrk_type0_2gb_n,phy14_pwrtrk_type0_2gc_n,phy14_pwrtrk_type0_2gd_n},{phy14_pwrtrk_type0_2ga_p,phy14_pwrtrk_type0_2gb_p,phy14_pwrtrk_type0_2gc_p,phy14_pwrtrk_type0_2gd_p}},{{phy14_pwrtrk_type0_2g_cck_a_n,phy14_pwrtrk_type0_2g_cck_b_n,phy14_pwrtrk_type0_2g_cck_c_n,phy14_pwrtrk_type0_2g_cck_d_n},{phy14_pwrtrk_type0_2g_cck_a_p,phy14_pwrtrk_type0_2g_cck_b_p,phy14_pwrtrk_type0_2g_cck_c_p,phy14_pwrtrk_type0_2g_cck_d_p}}},{{{phy14_pwrtrk_2ga_n,phy14_pwrtrk_2gb_n,phy14_pwrtrk_2gc_n,phy14_pwrtrk_2gd_n},{phy14_pwrtrk_2ga_p,phy14_pwrtrk_2gb_p,phy14_pwrtrk_2gc_p,phy14_pwrtrk_2gd_p}},{{phy14_pwrtrk_2g_cck_a_n,phy14_pwrtrk_2g_cck_b_n,phy14_pwrtrk_2g_cck_c_n,phy14_pwrtrk_2g_cck_d_n},{phy14_pwrtrk_2g_cck_a_p,phy14_pwrtrk_2g_cck_b_p,phy14_pwrtrk_2g_cck_c_p,phy14_pwrtrk_2g_cck_d_p}}}};
static const u8*const swing_5g[2][3][2][4]={
 {
  {
   {phy14_pwrtrk_type0_5ga_n[0],phy14_pwrtrk_type0_5gb_n[0],phy14_pwrtrk_type0_5gc_n[0],phy14_pwrtrk_type0_5gd_n[0]},
   {phy14_pwrtrk_type0_5ga_p[0],phy14_pwrtrk_type0_5gb_p[0],phy14_pwrtrk_type0_5gc_p[0],phy14_pwrtrk_type0_5gd_p[0]},
  },
  {
   {phy14_pwrtrk_type0_5ga_n[1],phy14_pwrtrk_type0_5gb_n[1],phy14_pwrtrk_type0_5gc_n[1],phy14_pwrtrk_type0_5gd_n[1]},
   {phy14_pwrtrk_type0_5ga_p[1],phy14_pwrtrk_type0_5gb_p[1],phy14_pwrtrk_type0_5gc_p[1],phy14_pwrtrk_type0_5gd_p[1]},
  },
  {
   {phy14_pwrtrk_type0_5ga_n[2],phy14_pwrtrk_type0_5gb_n[2],phy14_pwrtrk_type0_5gc_n[2],phy14_pwrtrk_type0_5gd_n[2]},
   {phy14_pwrtrk_type0_5ga_p[2],phy14_pwrtrk_type0_5gb_p[2],phy14_pwrtrk_type0_5gc_p[2],phy14_pwrtrk_type0_5gd_p[2]},
  },
 },
 {
  {
   {phy14_pwrtrk_5ga_n[0],phy14_pwrtrk_5gb_n[0],phy14_pwrtrk_5gc_n[0],phy14_pwrtrk_5gd_n[0]},
   {phy14_pwrtrk_5ga_p[0],phy14_pwrtrk_5gb_p[0],phy14_pwrtrk_5gc_p[0],phy14_pwrtrk_5gd_p[0]},
  },
  {
   {phy14_pwrtrk_5ga_n[1],phy14_pwrtrk_5gb_n[1],phy14_pwrtrk_5gc_n[1],phy14_pwrtrk_5gd_n[1]},
   {phy14_pwrtrk_5ga_p[1],phy14_pwrtrk_5gb_p[1],phy14_pwrtrk_5gc_p[1],phy14_pwrtrk_5gd_p[1]},
  },
  {
   {phy14_pwrtrk_5ga_n[2],phy14_pwrtrk_5gb_n[2],phy14_pwrtrk_5gc_n[2],phy14_pwrtrk_5gd_n[2]},
   {phy14_pwrtrk_5ga_p[2],phy14_pwrtrk_5gb_p[2],phy14_pwrtrk_5gc_p[2],phy14_pwrtrk_5gd_p[2]},
  },
 },
};
static const u32 phy14_txscale_tbl[37]={0x081,0x088,0x090,0x099,0x0a2,0x0ac,0x0b6,0x0c0,0x0cc,0x0d8,0x0e5,0x0f2,0x101,0x110,0x120,0x131,0x143,0x156,0x16a,0x180,0x197,0x1af,0x1c8,0x1e3,0x200,0x21e,0x23e,0x261,0x285,0x2ab,0x2d3,0x2fe,0x32b,0x35c,0x38e,0x3c4,0x3fe};
static int phy_abs(int value){return value<0?-value:value;}
static unsigned shift_mask(u32 mask){unsigned n=0;if(!mask)return 0;while(!(mask&1U)){mask>>=1;++n;}return n;}
static u32 rd(struct rtw_8814au_phy*p,u32 a,unsigned w){u32 v=0;if(a>UINT16_MAX){if(!p->ctx->io_error)p->ctx->io_error=EINVAL;return 0;}if(p->cleanup){int e=p->ctx->bus.read(p->ctx->bus.cookie,(u16)a,w,&v);if(e&&!p->ctx->io_error)p->ctx->io_error=e;}else(void)rtw_8814au_read(p->ctx,(u16)a,w,&v);return v;}
static void wr(struct rtw_8814au_phy*p,u32 a,unsigned w,u32 v){if(a>UINT16_MAX){if(!p->ctx->io_error)p->ctx->io_error=EINVAL;return;}if(p->cleanup){int e=p->ctx->bus.write(p->ctx->bus.cookie,(u16)a,w,v);if(e&&!p->ctx->io_error)p->ctx->io_error=e;}else(void)rtw_8814au_write(p->ctx,(u16)a,w,v);}
static u8 rtw_read8(struct rtw_8814au_phy*p,u32 a){return (u8)rd(p,a,1);}
static u16 rtw_read16(struct rtw_8814au_phy*p,u32 a){return (u16)rd(p,a,2);}
static u32 rtw_read32(struct rtw_8814au_phy*p,u32 a){return rd(p,a,4);}
static u32 rtw_read32_mask(struct rtw_8814au_phy*p,u32 a,u32 m){return (rd(p,a,4)&m)>>shift_mask(m);}
static void rtw_write8(struct rtw_8814au_phy*p,u32 a,u8 v){wr(p,a,1,v);}
static void rtw_write16(struct rtw_8814au_phy*p,u32 a,u16 v){wr(p,a,2,v);}
static void rtw_write32(struct rtw_8814au_phy*p,u32 a,u32 v){wr(p,a,4,v);}
static void rtw_write8_mask(struct rtw_8814au_phy*p,u32 a,u32 m,u32 v){u32 old=rd(p,a,1);wr(p,a,1,(old&~m)|((v<<shift_mask(m))&m));}
static void rtw_write32_mask(struct rtw_8814au_phy*p,u32 a,u32 m,u32 v){u32 old=rd(p,a,4);wr(p,a,4,(old&~m)|((v<<shift_mask(m))&m));}
static void rtw_write8_set(struct rtw_8814au_phy*p,u32 a,u32 b){wr(p,a,1,rd(p,a,1)|b);}
static void rtw_write8_clr(struct rtw_8814au_phy*p,u32 a,u32 b){wr(p,a,1,rd(p,a,1)&~b);}
static void rtw_write32_set(struct rtw_8814au_phy*p,u32 a,u32 b){wr(p,a,4,rd(p,a,4)|b);}
static void rtw_write32_clr(struct rtw_8814au_phy*p,u32 a,u32 b){wr(p,a,4,rd(p,a,4)&~b);}
static void delay(struct rtw_8814au_phy*p,u32 us){rtw_8814au_delay(p->ctx,us);}
#define usleep_range(a,b) delay(rtwdev,(a))
#define udelay(a) delay(rtwdev,(a))
#define rtw_warn(...) ((void)0)
static void ewma_thermal_init(struct thermal_average*a){a->value=0;}
static u32 rtw_read_rf(struct rtw_8814au_phy*p,u8 path,u32 reg,u32 mask){static const u16 base[4]={0x2800,0x2c00,0x3800,0x3c00};if(path>3||reg>255||!mask){if(!p->ctx->io_error)p->ctx->io_error=EINVAL;return 0;}return rtw_read32_mask(p,(u16)(base[path]+(reg<<2)),mask&0xfffffU);}
static void rtw_write_rf(struct rtw_8814au_phy*p,u8 path,u32 reg,u32 mask,u32 value){static const u16 sipi[4]={0xc90,0xe90,0x1890,0x1a90};u32 old,v;if(path>3||reg>255||!mask){if(!p->ctx->io_error)p->ctx->io_error=EINVAL;return;}mask&=0xfffffU;old=mask==0xfffffU?0:rtw_read_rf(p,path,reg,0xfffffU);v=(old&~mask)|((value<<shift_mask(mask))&mask);rtw_write32(p,sipi[path],((reg<<20)|(v&0xfffffU))&0x0fffffffU);delay(p,13);}
static int phy_poll_ready(struct rtw_8814au_phy*p,bool*ready,u32 timeout){u32 elapsed;*ready=false;for(elapsed=0;elapsed<=timeout;elapsed+=1000){*ready=!rtw_read32_mask(p,0x1b00,BIT(0));if(p->ctx->io_error)return p->ctx->io_error;if(*ready)return 0;delay(p,1000);}return ETIMEDOUT;}
#include "RTL8814AUAlgorithms.inc"

/* 8814A condition words: RFE8,interface4,package4,platform4,cut4. */
static bool condition_matches(struct rtw_8814au_phy*p,u32 word){unsigned cut=(word>>24)&15U,pkg=(word>>12)&15U,intf=(word>>8)&15U;return (!cut||cut==(p->hal.cut_version?p->hal.cut_version:15U))&&(!pkg||pkg==15U)&&(!intf||intf==2U)&&(word&255U)==p->efuse.rfe_option;}
static int table(struct rtw_8814au_phy*p,const u32*data,size_t count,int rf){size_t i;u32 positive=0;bool matched=true,skipped=false;if(count&1U)return EINVAL;for(i=0;i<count&&!p->ctx->io_error;i+=2){u32 a=data[i],v=data[i+1];if(a&BIT(31)){switch((a>>28)&3U){case 3:matched=true;skipped=false;break;case 2:matched=!skipped;break;default:positive=a;break;}}else if(a&BIT(30)){if(!skipped){matched=condition_matches(p,positive);skipped=matched;}else matched=false;}else if(matched){if(rf>=0){if(a==0xffe)delay(p,50000);else if(a==0xfe)delay(p,100);else{rtw_write_rf(p,(u8)rf,a,0xfffffU,v);delay(p,1);}}else if(a>=0xf9&&a<=0xfe){static const u32 waits[6]={1,5,50,1000,5000,50000};delay(p,waits[a-0xf9]);}else if(a>65535U)return EINVAL;else rtw_write32(p,(u16)a,v);}}return p->ctx->io_error;}
static int power_tables(struct rtw_8814au_phy*p){const struct phy_pg_row*pg=p->efuse.rfe_option?phy14_bb_pg:phy14_bb_pg_type0;size_t np=p->efuse.rfe_option?ARRAY_SIZE(phy14_bb_pg):ARRAY_SIZE(phy14_bb_pg_type0);const struct phy_limit_row*lmt=p->efuse.rfe_option?phy14_txpwr_lmt_type1:phy14_txpwr_lmt_type0;size_t nl=p->efuse.rfe_option?ARRAY_SIZE(phy14_txpwr_lmt_type1):ARRAY_SIZE(phy14_txpwr_lmt_type0);unsigned seen[4]={0};size_t i;memset(p->limit,63,sizeof(p->limit));
 for(i=0;i<np;i++){const struct phy_pg_row*r=&pg[i];unsigned low=r->addr&255U,start,j;if(r->band||r->path>3||low<0x20||low>0x28)continue;start=(low-0x20);for(j=0;j<4;j++){unsigned b=(r->data>>(j*8))&255U;if((b&15U)>9||(b>>4)>9)return EBADMSG;p->by_rate[r->path][start+j]=(int8_t)((b>>4)*10+(b&15U));seen[r->path]|=1U<<(start+j);}}
 for(i=0;i<4;i++){unsigned r;if(seen[i]!=0xfffU)return EBADMSG;p->base_by_rate[i][0]=p->by_rate[i][3];p->base_by_rate[i][1]=p->by_rate[i][11];for(r=0;r<12;r++)p->by_rate[i][r]-=p->base_by_rate[i][r<4?0:1];}
 /* World-wide is the minimum of all configured primary regulatory domains,
  * exactly as rtw_phy_set_tx_power_limit builds its WW fallback. */
 for(i=0;i<nl;i++){const struct phy_limit_row*r=&lmt[i];if(r->band||r->bw||r->rs>1||r->ch<1||r->ch>11)continue;if(r->regd<3&&r->value<p->limit[r->regd][r->rs][r->ch-1])p->limit[r->regd][r->rs][r->ch-1]=r->value;if(r->value<p->limit[3][r->rs][r->ch-1])p->limit[3][r->rs][r->ch-1]=r->value;}
 for(i=0;i<4;i++){unsigned rs,ch;for(rs=0;rs<2;rs++)for(ch=0;ch<11;ch++){if(p->limit[i][rs][ch]==63)p->limit[i][rs][ch]=p->limit[3][rs][ch];if(p->limit[i][rs][ch]==63)return ENOTSUP;p->limit[i][rs][ch]-=p->base_by_rate[0][rs];}}
 return 0;}
/* Only centers with complete pinned RFE0/1 HT20 OFDM/HT1S/HT2S limits.
 * RF programming covers144, but the source supplies no144 power limits. */
static const u8 five_channels[PHY_5G_CHANNEL_COUNT]={36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165};
static int five_slot(u8 ch){for(unsigned i=0;i<ARRAY_SIZE(five_channels);i++)if(five_channels[i]==ch)return(int)i;return-1;}
static unsigned five_group(u8 ch){static const u8 groups[PHY_5G_CHANNEL_COUNT]={0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,10,10,11,11,12};int slot=five_slot(ch);return slot>=0?groups[slot]:0;}
/* Linux phy.c: packed2G18 bytes followed by5G14 base indexes and deltas. */
static int power_tables_5g(struct rtw_8814au_phy*p){
 const struct phy_pg_row*pg=p->efuse.rfe_option?phy14_bb_pg:phy14_bb_pg_type0;
 size_t np=p->efuse.rfe_option?ARRAY_SIZE(phy14_bb_pg):ARRAY_SIZE(phy14_bb_pg_type0);
 const struct phy_limit_row*lmt=p->efuse.rfe_option?phy14_txpwr_lmt_type1:phy14_txpwr_lmt_type0;
 size_t nl=p->efuse.rfe_option?ARRAY_SIZE(phy14_txpwr_lmt_type1):ARRAY_SIZE(phy14_txpwr_lmt_type0);
 unsigned seen[4]={0};size_t i;unsigned path,rate,domain,ch;
 memset(p->limit_5g,63,sizeof(p->limit_5g));
 for(i=0;i<np;i++){
  const struct phy_pg_row*r=&pg[i];unsigned low=r->addr&255U,j;
  if(r->band!=1||r->path>3||(low!=0x24&&low!=0x28))continue;
  for(j=0;j<4;j++){
   unsigned b=(r->data>>(j*8))&255U,ri=low-0x20+j;
   if((b&15U)>9||(b>>4)>9)return EBADMSG;
   p->by_rate_5g[r->path][ri]=(int8_t)((b>>4)*10+(b&15U));seen[r->path]|=1U<<ri;
  }
 }
 for(path=0;path<4;path++){
  if(seen[path]!=0xff0U)return EBADMSG;
  p->base_5g[path]=p->by_rate_5g[path][11];
  for(rate=4;rate<12;rate++)p->by_rate_5g[path][rate]-=p->base_5g[path];
 }
 for(i=0;i<nl;i++){
  const struct phy_limit_row*r=&lmt[i];unsigned slot;
  if(r->band!=1||r->bw||r->rs!=1||five_slot(r->ch)<0)continue;
  slot=(unsigned)five_slot(r->ch);
  if(r->regd<3&&r->value<p->limit_5g[r->regd][slot])p->limit_5g[r->regd][slot]=r->value;
  if(r->value<p->limit_5g[3][slot])p->limit_5g[3][slot]=r->value;
 }
 for(domain=0;domain<4;domain++)for(ch=0;ch<ARRAY_SIZE(five_channels);ch++){
  if(p->limit_5g[domain][ch]==63)p->limit_5g[domain][ch]=p->limit_5g[3][ch];
  if(p->limit_5g[domain][ch]==63)return ENOTSUP;
  p->limit_5g[domain][ch]-=p->base_5g[0];
 }
 return 0;
}
int rtw_8814au_phy_channel_supported(const struct rtw_8814au*ctx,u8 ch,unsigned bw){
 unsigned path,g,rs,domain;int slot;const struct phy_limit_row*rows;size_t count;
 if(!ctx||!ctx->board_valid||bw!=20||ctx->board.rfe_option>1||ctx->board.physical_rf_paths!=4||ctx->board.cut>6||ctx->board.tx_streams<1||ctx->board.tx_streams>2||ctx->board.rx_streams<1||ctx->board.rx_streams>2)return 0;
 /* Initialization retains its validated2G factory prerequisite. The selected
  *5G group is checked independently; an erased adjacent group cannot pass. */
 for(path=0;path<4;path++)for(g=0;g<4;g++)if(ctx->board.txpwr_idx_table[path][g]>63||ctx->board.txpwr_idx_table[path][6+g]>63)return 0;
 if(ch>=1&&ch<=11)return 1;slot=five_slot(ch);if(slot<0)return 0;
 g=five_group(ch);for(path=0;path<4;path++)if(ctx->board.txpwr_idx_table[path][18+g]>63)return 0;
 rows=ctx->board.rfe_option?phy14_txpwr_lmt_type1:phy14_txpwr_lmt_type0;count=ctx->board.rfe_option?ARRAY_SIZE(phy14_txpwr_lmt_type1):ARRAY_SIZE(phy14_txpwr_lmt_type0);domain=ctx->board.regd<3?ctx->board.regd:3;
 for(rs=1;rs<=3;rs++){int chosen=63,world=63;for(size_t i=0;i<count;i++){const struct phy_limit_row*r=&rows[i];if(r->band!=1||r->bw||r->rs!=rs||r->ch!=ch)continue;if(r->value<world)world=r->value;if(r->regd==domain&&r->value<chosen)chosen=r->value;}if(chosen==63)chosen=world;if(chosen==63)return 0;}
 return 1;
}
int rtw_8814au_phy_supports_5g(const struct rtw_8814au*ctx){for(unsigned i=0;i<ARRAY_SIZE(five_channels);i++)if(rtw_8814au_phy_channel_supported(ctx,five_channels[i],20))return 1;return 0;}
/* Linux phy.c: HT20 sections have independent MCS7/MCS15 PG bases and
 * signed 1S/2S ROM deltas. Never reuse OFDM limits for an HT descriptor. */
static int power_tables_ht(struct rtw_8814au_phy *p)
{
 const struct phy_pg_row *pg=p->efuse.rfe_option?phy14_bb_pg:phy14_bb_pg_type0;
 size_t np=p->efuse.rfe_option?ARRAY_SIZE(phy14_bb_pg):ARRAY_SIZE(phy14_bb_pg_type0);
 const struct phy_limit_row *lmt=p->efuse.rfe_option?phy14_txpwr_lmt_type1:phy14_txpwr_lmt_type0;
 size_t nl=p->efuse.rfe_option?ARRAY_SIZE(phy14_txpwr_lmt_type1):ARRAY_SIZE(phy14_txpwr_lmt_type0);
 unsigned seen[2][4]={{0}},band,path,rs,slot;size_t i;
 memset(p->limit_ht,63,sizeof(p->limit_ht));
 for(i=0;i<np;i++){
  const struct phy_pg_row *r=&pg[i];unsigned low=r->addr&255U,j;
  if(r->band>1||r->path>3||low<0x2c||low>0x38||low%4)continue;
  for(j=0;j<4;j++){
   unsigned mcs=low-0x2c+j,b=(r->data>>(8*j))&255U;
   if((b&15)>9||(b>>4)>9||r->txnum!=(mcs>=8))return EBADMSG;
   p->by_rate_ht[r->band][r->path][mcs]=(int8_t)((b>>4)*10+(b&15));
   seen[r->band][r->path]|=1U<<mcs;
  }
 }
 for(band=0;band<2;band++)for(path=0;path<4;path++){
  unsigned mcs;if(seen[band][path]!=0xffffU)return EBADMSG;
  for(rs=0;rs<2;rs++)p->base_ht[band][path][rs]=p->by_rate_ht[band][path][rs?15:7];
  for(mcs=0;mcs<16;mcs++)p->by_rate_ht[band][path][mcs]-=p->base_ht[band][path][mcs>=8];
 }
 for(i=0;i<nl;i++){
  const struct phy_limit_row *r=&lmt[i];
  if(r->band>1||r->bw||r->rs<2||r->rs>3)continue;
  if(!r->band){if(r->ch<1||r->ch>11)continue;slot=r->ch-1;}
  else {if(five_slot(r->ch)<0)continue;slot=(unsigned)five_slot(r->ch);}
  rs=r->rs-2;
  if(r->regd<3&&r->value<p->limit_ht[r->band][r->regd][rs][slot])p->limit_ht[r->band][r->regd][rs][slot]=r->value;
  if(r->value<p->limit_ht[r->band][3][rs][slot])p->limit_ht[r->band][3][rs][slot]=r->value;
 }
 for(band=0;band<2;band++)for(path=0;path<4;path++)for(rs=0;rs<2;rs++)for(slot=0;slot<(band?ARRAY_SIZE(five_channels):11U);slot++){
  int8_t *limit=&p->limit_ht[band][path][rs][slot];
  if(*limit==63)*limit=p->limit_ht[band][3][rs][slot];
  if(*limit==63)return ENOTSUP;
  *limit-=p->base_ht[band][0][rs];
 }
 return 0;
}
static int signed_nibble(unsigned value)
{
 value&=15U;return (value&8U)?(int)value-16:(int)value;
}
/* Keep the primary unconstrained indexes for thermal headroom. Reducing the
 * indexes used to compute thermal headroom would let tracking undo the AP cap. */
static int sar_offset(struct rtw_8814au_phy *p,int pg_base)
{
 /* sar.c:rtw_sar_to_phy uses the path/section PG base, never raw ROM index.
  * Multiplication preserves its fixed-point conversion for negative dBm. */
 int ceiling=(int)p->power_limit_dbm-(int)p->power_constraint_db;
 return ceiling*(int)(1U<<PHY14_TXGI_FACTOR)-pg_base;
}
static int calculate_ht_power(struct rtw_8814au_phy *p,u8 channel,u8 power[4][28],bool sar)
{
 unsigned band=channel>14,path,mcs,group=band?five_group(channel):(channel<=2?0U:channel<=5?1U:channel<=8?2U:3U);
 unsigned slot=band?(unsigned)five_slot(channel):channel-1,domain=p->ctx->board.regd<3?p->ctx->board.regd:3;
 for(path=0;path<4;path++){
  const u8 *rom=p->ctx->board.txpwr_idx_table[path];
  for(mcs=0;mcs<16;mcs++){
   int base=rom[(band?18U:6U)+group]+signed_nibble(rom[band?32:11]>>4);
   int offset=p->by_rate_ht[band][path][mcs],limit=p->limit_ht[band][domain][mcs>=8][slot],value;
   if(mcs>=8)base+=signed_nibble(rom[band?33:12]);
   if(offset>limit)offset=limit;
   if(sar){int cap=sar_offset(p,p->base_ht[band][path][mcs>=8]);if(offset>cap)offset=cap;}
   value=base+offset;if(value<0){if(sar)return ERANGE;value=0;}if(value>63)value=63;
   power[path][12+mcs]=(u8)value;
  }
 }
 return 0;
}
static int calculate_power(struct rtw_8814au_phy *p,u8 channel,u8 power[4][28],bool sar)
{
 unsigned path,rate,band=channel>14,group=band?five_group(channel):(channel<=2?0U:channel<=5?1U:channel<=8?2U:3U);
 unsigned slot=band?(unsigned)five_slot(channel):channel-1,domain=p->ctx->board.regd<3?p->ctx->board.regd:3;
 for(path=0;path<4;path++){
  const u8 *rom=p->ctx->board.txpwr_idx_table[path];
  for(rate=band?4U:0U;rate<12;rate++){
   unsigned nibble=rom[band?32:11]&15U;
   int delta=(nibble&8U)?(int)nibble-16:(int)nibble;
   int base=band?rom[18+group]:rate<4?rom[group]:rom[6+group];
   int off=band?p->by_rate_5g[path][rate]:p->by_rate[path][rate];
   int limit=band?p->limit_5g[domain][slot]:p->limit[domain][rate<4?0:1][slot],value;
   if(rate>=4)base+=delta;if(off>limit)off=limit;
   if(sar){int cap=sar_offset(p,band?p->base_5g[path]:p->base_by_rate[path][rate<4?0:1]);if(off>cap)off=cap;}
   value=base+off;if(value<0){if(sar)return ERANGE;value=0;}if(value>63)value=63;
   power[path][rate]=(u8)value;
  }
 }
 return calculate_ht_power(p,channel,power,sar);
}
static unsigned encoded_power(unsigned index){return index>61U?63U:index+2U;}
static int validate_power_constraint(struct rtw_8814au_phy *p,u8 channel,u8 power[4][28])
{
 /* Pinned sar.c:rtw_sar_to_phy converts whole dBm by <<txgi_factor.
  * The factor comes from the pinned8814A chip definition, not a guessed unit.
  * Check the final +2/max63 wire encoding: clamping after subtraction can
  * otherwise turn a requested reduction into a smaller or zero reduction.
  * Retain the primary nonnegative logical-index floor (encoded index2). */
 unsigned reduction=p->power_limit_dbm==RTW_8814AU_POWER_LIMIT_DISABLED?
  (unsigned)p->power_constraint_db<<PHY14_TXGI_FACTOR:0;
 for(unsigned path=0;path<4;path++)for(unsigned rate=channel>14?4U:0U;rate<28;rate++)
  if(encoded_power(power[path][rate])<reduction+2U)return ERANGE;
 return 0;
}
static int program_power(struct rtw_8814au_phy *p,u8 channel,u8 power[4][28],u8 reference[4][28])
{
 unsigned reduction=p->power_limit_dbm==RTW_8814AU_POWER_LIMIT_DISABLED?
  (unsigned)p->power_constraint_db<<PHY14_TXGI_FACTOR:0;
 memcpy(p->power,reference,sizeof(p->power));
 /* Retain the original legacy-then-HT write order and the address0 double write. */
 for(unsigned section=0;section<2;section++)for(unsigned path=0;path<4;path++)
  for(unsigned rate=section?12U:channel>14?4U:0U;rate<(section?28U:12U);rate++){
   unsigned value=encoded_power(power[path][rate])-reduction;
   u32 word=0x00801000U|((u32)value<<24)|((u32)path<<8)|rate;
   rtw_write32(p,REG_AGC_TBL,word);if(!rate)rtw_write32(p,REG_AGC_TBL,word);
  }
 if(!p->ctx->io_error){p->applied_power_constraint_db=p->power_constraint_db;p->applied_power_limit_dbm=p->power_limit_dbm;}
 return p->ctx->io_error;
}
static void reset_constraint_tracking(struct rtw_8814au_phy *p,bool reset)
{
 static const u32 txagc_reg[4]={REG_TX_AGC_A,REG_TX_AGC_B,REG_TX_AGC_C,REG_TX_AGC_D};
 /* A new cap cannot inherit old positive AGC. The channel path already
  * restored each path's factory BB swing; preserve those independent values.
  * This is the zero-AGC field written by primary pwrtrack_set_pwr(). */
 if(reset)for(u8 path=0;path<4;path++)
  rtw_write32_mask(p,txagc_reg[path],GENMASK(29,25),0);
}
static int fail_paused(struct rtw_8814au_phy*p,int error){p->ctx->phy_initialized=0;p->cleanup=true;rtw_write8(p,REG_TXPAUSE,0x3f);p->cleanup=false;return error;}
static int validate_board(const struct rtw_8814au*ctx){unsigned path,g;if(!ctx->board_valid||!ctx->powered||!ctx->mac_initialized||!ctx->fw_running)return EINVAL;if(ctx->board.rfe_option>1||ctx->board.physical_rf_paths!=4||ctx->board.cut>6||ctx->board.tx_streams<1||ctx->board.tx_streams>2||ctx->board.rx_streams<1||ctx->board.rx_streams>2)return ENOTSUP;for(path=0;path<4;path++)for(g=0;g<4;g++)if(ctx->board.txpwr_idx_table[path][g]>63||ctx->board.txpwr_idx_table[path][6+g]>63)return ENOTSUP;return 0;}
int rtw_8814au_phy_set_tx_rate(struct rtw_8814au*ctx,u8 rate){if(!ctx||!ctx->phy||rate>27||(ctx->phy->hal.current_band_type==RTW_BAND_5G&&rate<4))return EINVAL;ctx->phy->tx_rate=rate;return 0;}
int rtw_8814au_phy_set_power_constraint(struct rtw_8814au *ctx,u8 db)
{
 struct rtw_8814au_phy *p;
 if(!ctx||!(p=ctx->phy)||p->ctx!=ctx||db>(63U>>PHY14_TXGI_FACTOR))return EINVAL;
 if(ctx->io_error)return ctx->io_error;
 if(p->power_constraint_db!=db){p->power_constraint_db=db;ctx->phy_initialized=0;p->channel_set=false;}
 return 0;
}
int rtw_8814au_phy_set_power_limit_dbm(struct rtw_8814au *ctx,int8_t dbm)
{
 struct rtw_8814au_phy *p;
 if(!ctx||!(p=ctx->phy)||p->ctx!=ctx||
    (dbm!=RTW_8814AU_POWER_LIMIT_DISABLED&&(dbm< -32||dbm>31)))return EINVAL;
 if(ctx->io_error)return ctx->io_error;
 if(p->power_limit_dbm!=dbm){p->power_limit_dbm=dbm;ctx->phy_initialized=0;p->channel_set=false;}
 return 0;
}
size_t rtw_8814au_phy_size(void){return sizeof(struct rtw_8814au_phy);}
int rtw_8814au_phy_init(struct rtw_8814au*ctx,void*storage,size_t size){int e;struct rtw_8814au_phy*p;if(!ctx||!storage||size<sizeof(*p)||((uintptr_t)storage%_Alignof(struct rtw_8814au_phy))||!ctx->bus.read||!ctx->bus.write||!ctx->bus.delay_us)return EINVAL;if(ctx->io_error)return ctx->io_error;e=validate_board(ctx);if(e)return e;p=(struct rtw_8814au_phy*)storage;memset(p,0,sizeof(*p));p->ctx=ctx;p->power_limit_dbm=p->applied_power_limit_dbm=RTW_8814AU_POWER_LIMIT_DISABLED;p->hal.cut_version=ctx->board.cut;p->hal.rf_path_num=4;p->efuse.rfe_option=ctx->board.rfe_option;p->efuse.tx_bb_swing_setting_2g=ctx->board.tx_bb_swing_setting_2g;p->efuse.tx_bb_swing_setting_5g=ctx->board.tx_bb_swing_setting_5g;p->efuse.thermal_meter_k=ctx->board.thermal_meter;ctx->phy=p;ctx->phy_initialized=0;e=power_tables(p);if(!e)e=power_tables_5g(p);return e?e:power_tables_ht(p);}
int rtw_8814au_phy_load_tables(struct rtw_8814au*ctx){struct rtw_8814au_phy*p;u8 path;u32 v;if(!ctx||!(p=ctx->phy)||p->ctx!=ctx)return EINVAL;if(ctx->io_error)return ctx->io_error;ctx->phy_initialized=0;if(!p->resume_pending){p->resume_txpause=rtw_read8(p,REG_TXPAUSE);p->resume_pending=true;}rtw_write8(p,REG_TXPAUSE,0x3f);rtw_write8_set(p,REG_SYS_FUNC_EN,BIT_FEN_USBA);rtw_write8_set(p,REG_SYS_CFG3_8814A+2,BIT_FEN_BB_GLB_RST|BIT_FEN_BB_RSTB);rtw_write8(p,REG_RF_CTRL,7);rtw_write8(p,REG_RF_CTRL1,7);rtw_write8(p,REG_RF_CTRL2,7);rtw_write8(p,REG_RF_CTRL3,7);
 table(p,phy14_bb,ARRAY_SIZE(phy14_bb),-1);table(p,phy14_agc,ARRAY_SIZE(phy14_agc),-1);v=ctx->board.crystal_cap&0x3fU;v|=v<<6;rtw_write32_mask(p,REG_AFE_CTRL3,0x07ff8000,v);phy14_config_trx_path(p);
 table(p,phy14_rf_a,ARRAY_SIZE(phy14_rf_a),0);table(p,phy14_rf_b,ARRAY_SIZE(phy14_rf_b),1);table(p,phy14_rf_c,ARRAY_SIZE(phy14_rf_c),2);table(p,phy14_rf_d,ARRAY_SIZE(phy14_rf_d),3);
 v=rtw_read_rf(p,0,RF_RCK1_V1,0xfffff);for(path=1;path<4;path++)rtw_write_rf(p,path,RF_RCK1_V1,0xfffff,v);rtw_write32_set(p,REG_RXPSEL,BIT_RX_PSEL_RST);rtw_write8(p,REG_HWSEQ_CTRL,0xff);rtw_write32(p,REG_BAR_MODE_CTRL,0x0201ffff);rtw_write8(p,REG_MISC_CTRL,BIT_DIS_SECOND_CCA);rtw_write8(p,REG_NAV_CTRL+2,0);rtw_write8_clr(p,REG_GPIO_MUXCFG,BIT(5));phy14_config_cck_rx_antenna_init(p);phy14_pwrtrack_init(p);phy14_init_rfe_reg(p);rtw_write8_clr(p,REG_QUEUE_CTRL,BIT(3));rtw_write8(p,REG_NAV_CTRL+2,235);rtw_write8(p,REG_FWHW_TXQ_CTRL+1,0x1f);rtw_write8(p,REG_SYS_SDIO_CTRL,0);rtw_write8(p,REG_ACLK_MON,0);p->loaded=ctx->io_error==0;return ctx->io_error?fail_paused(p,ctx->io_error):0;}
int rtw_8814au_phy_set_channel(struct rtw_8814au*ctx,u8 ch,unsigned bw){struct rtw_8814au_phy*p;u8 path,power[4][28]={{0}},reference[4][28]={{0}};bool reset_tracking;int error;if(!ctx||!(p=ctx->phy)||!p->loaded||bw!=20||(!((ch>=1&&ch<=11)||(five_slot(ch)>=0))))return EINVAL;
 if(ctx->io_error)return ctx->io_error;
 if(!rtw_8814au_phy_channel_supported(ctx,ch,bw))return ENOTSUP;
 if(p->power_limit_dbm!=RTW_8814AU_POWER_LIMIT_DISABLED&&
    (int)p->power_limit_dbm-(int)p->power_constraint_db< -32)return ERANGE;
 error=calculate_power(p,ch,reference,false);if(error)return error;
 if(p->power_limit_dbm==RTW_8814AU_POWER_LIMIT_DISABLED)memcpy(power,reference,sizeof(power));
 else {error=calculate_power(p,ch,power,true);if(error)return error;}
 error=validate_power_constraint(p,ch,power);if(error)return error;
 reset_tracking=p->power_constraint_db||p->applied_power_constraint_db||
  p->power_limit_dbm!=RTW_8814AU_POWER_LIMIT_DISABLED||p->applied_power_limit_dbm!=RTW_8814AU_POWER_LIMIT_DISABLED;
 if(ch>14){int e;if(!rtw_8814au_phy_supports_5g(ctx))return ENOTSUP;
  e=rtw_8814au_phy_set_passive_5g_channel(ctx,ch,bw);if(e)return e;
  e=program_power(p,ch,power,reference);if(e)return fail_paused(p,e);
  p->tx_rate=4;phy14_pwrtrack_init(p);reset_constraint_tracking(p,reset_tracking);p->channel_set=ctx->io_error==0;return ctx->io_error?fail_paused(p,ctx->io_error):0;}
 ctx->phy_initialized=0;if(!p->resume_pending){p->resume_txpause=rtw_read8(p,REG_TXPAUSE);p->resume_pending=true;}rtw_write8(p,REG_TXPAUSE,0x3f);
 p->hal.current_band_type=RTW_BAND_2G;p->hal.current_band_width=RTW_CHANNEL_WIDTH_20;p->hal.current_channel=ch;
 rtw_write8_clr(p,REG_SYS_CFG3_8814A+2,BIT_FEN_BB_RSTB);rtw_write32_mask(p,REG_AGC_TABLE,0x1f,0);phy14_set_rfe_reg_24g(p);rtw_write32_mask(p,REG_TXPSEL,0xf0,2);rtw_write32_mask(p,REG_CCK_RX,0x0f000000,5);rtw_write32_mask(p,REG_RXPSEL,BIT_RX_PSEL_RST,3);rtw_write8(p,REG_CCK_CHECK,0);rtw_write32_mask(p,0xa80,BIT(18),0);phy14_set_channel_bb_swing(p,0);phy14_set_bw_reg_adc(p,0);phy14_set_bw_reg_agc(p,0,0);rtw_write8_set(p,REG_SYS_CFG3_8814A+2,BIT_FEN_BB_RSTB);
 rtw_write32_mask(p,REG_CLKTRK,0x1ffe0000,0x96a);for(path=0;path<4;path++)rtw_write_rf(p,path,RF_CFGCH,RF18_RFSI_MASK|RF18_BAND_MASK|RF18_CHANNEL_MASK,ch);phy14_24g_cck_tx_dfir(p,ch);phy14_set_bw_reg_mac(p,0);rtw_write8(p,REG_DATA_SC,0);phy14_set_bw_reg_adc(p,0);phy14_set_bw_reg_agc(p,0,0);phy14_set_bw_rf(p,0);phy14_adc_clk(p);
 phy14_spur_calibration(p,ch,RTW_CHANNEL_WIDTH_20);
 if(!ctx->io_error)program_power(p,ch,power,reference);phy14_pwrtrack_init(p);reset_constraint_tracking(p,reset_tracking);p->channel_set=ctx->io_error==0;return ctx->io_error?fail_paused(p,ctx->io_error):0;}
/* RTL8814A 5 GHz receive-only RF path. The pinned rtw8814a.c
 * switch_band/switch_channel/set_bw_mode sequence covers the24 backed
 * HT20 centers. TX stays paused; operating tuning separately validates
 * factory power, sets TXAGC and requires successful calibration. */
int rtw_8814au_phy_set_passive_5g_channel(struct rtw_8814au *ctx,
                                          u8 channel, unsigned bandwidth_mhz)
{
    struct rtw_8814au_phy *p;
    u32 mux, mux_d, inv;

    if (!ctx || !(p = ctx->phy) || p->ctx != ctx || !p->loaded ||
        (five_slot(channel)<0) ||
        bandwidth_mhz != 20)
        return EINVAL;
    if (ctx->io_error) return ctx->io_error;

    /* Invalidate previously completed 2G calibration BEFORE touching the RF.
     * Do not alter the caller's saved pre-scan TXPAUSE on a second hop. */
    ctx->phy_initialized = 0;
    p->channel_set = false;
    if (!p->resume_pending) {
        p->resume_txpause = rtw_read8(p, REG_TXPAUSE);
        if (ctx->io_error) return fail_paused(p, ctx->io_error);
        p->resume_pending = true;
    }
    rtw_write8(p, REG_TXPAUSE, 0x3f);
    if (ctx->io_error) return fail_paused(p, ctx->io_error);

    /* Switch 2G CCK off; select per-RFE 5G receive path (rf switch/LNA). */
    rtw_write8_clr(p, REG_SYS_CFG3_8814A + 2, BIT_FEN_BB_RSTB);
    rtw_write8(p, REG_CCK_CHECK, BIT_CHECK_CCK_EN);
    rtw_write32_mask(p, 0xa80, BIT(18), 1);
    if (p->efuse.rfe_option == 1) {
        mux = 0x33173317U;
        mux_d = 0x77177717U;
        inv = 0x33U;
    } else {
        mux = 0x54775477U;
        mux_d = mux;
        inv = 0x54U;
    }
    rtw_write32(p, REG_RFE_PINMUX_A, mux);
    rtw_write32(p, REG_RFE_PINMUX_B, mux);
    rtw_write32(p, REG_RFE_PINMUX_C, mux);
    rtw_write32(p, REG_RFE_PINMUX_D, mux_d);
    rtw_write32_mask(p, REG_RFE_INVSEL_D, BIT_RFE_SELSW0_D, inv);
    rtw_write32_mask(p, REG_TXPSEL, 0xf0, 0);
    rtw_write32_mask(p, REG_CCK_RX, 0x0f000000, 0xf);
    rtw_write32_mask(p, REG_RXPSEL, BIT_RX_PSEL_RST, 2);
    p->hal.current_band_type = RTW_BAND_5G;
    p->hal.current_band_width = RTW_CHANNEL_WIDTH_20;
    p->hal.current_channel = channel;
    phy14_set_channel_bb_swing(p, RTW_BAND_5G);
    phy14_set_bw_reg_adc(p, RTW_CHANNEL_WIDTH_20);
    phy14_set_bw_reg_agc(p, RTW_BAND_5G, RTW_CHANNEL_WIDTH_20);
    rtw_write8_set(p, REG_SYS_CFG3_8814A + 2, BIT_FEN_BB_RSTB);

    phy14_switch_channel(p,channel);
    phy14_set_bw_reg_mac(p, RTW_CHANNEL_WIDTH_20);
    rtw_write8(p, REG_DATA_SC, 0);
    phy14_set_bw_reg_adc(p, RTW_CHANNEL_WIDTH_20);
    phy14_set_bw_reg_agc(p, RTW_BAND_5G, RTW_CHANNEL_WIDTH_20);
    phy14_set_bw_rf(p, RTW_CHANNEL_WIDTH_20);
    phy14_adc_clk(p);

    /* Complete primary HT20 spur chain, including RFE0 channel140
     * CCA/PDMFTH backup and subsequent channel cleanup. */
    phy14_spur_calibration(p, channel, RTW_CHANNEL_WIDTH_20);

    /* Deliberate: channel_set=false, phy_initialized=0, REG_TXPAUSE=0x3f.
     * Any attempt to use the station TX/association calibration is rejected. */
    return ctx->io_error ? fail_paused(p, ctx->io_error) : 0;
}

int rtw_8814au_phy_set_scan_channel(struct rtw_8814au *ctx, u8 channel,
                                  unsigned bandwidth_mhz)
{
    struct rtw_8814au_phy *p;
    int error;

    if (!ctx || !(p = ctx->phy) || p->ctx != ctx)
        return EINVAL;
    if (channel < 1 || channel > 11 || bandwidth_mhz != 20)
        return EINVAL;
    if (ctx->io_error)
        return ctx->io_error;
    if (!ctx->phy_initialized || p->hal.current_band_type != RTW_BAND_2G)
        return EAGAIN;
    error = rtw_8814au_phy_set_channel(ctx, channel, bandwidth_mhz);
    if (error)
        return error;
    /* Linux defers RFK during scanning. Preserve the successful same-band
     * calibration; AUTH calls the operating-channel path and calibrates. */
    rtw_write8(p, REG_TXPAUSE, p->resume_txpause);
    if (ctx->io_error)
        return fail_paused(p, ctx->io_error);
    p->resume_pending = false;
    ctx->phy_initialized = 1;
    return 0;
}
int rtw_8814au_phy_calibrate(struct rtw_8814au*ctx){struct rtw_8814au_phy*p;unsigned path;int e=0;if(!ctx||!(p=ctx->phy)||!p->loaded||!p->channel_set)return EINVAL;ctx->phy_initialized=0;if(ctx->io_error)return ctx->io_error;memset(p->lok_ok,0,sizeof(p->lok_ok));memset(p->tx_ok,0,sizeof(p->tx_ok));memset(p->rx_ok,0,sizeof(p->rx_ok));phy14_do_iqk(p);if(ctx->io_error)return fail_paused(p,ctx->io_error);for(path=0;path<4;path++)if(!p->lok_ok[path]||!p->tx_ok[path]||!p->rx_ok[path])e=ETIMEDOUT;if(e){rtw_write8(p,REG_TXPAUSE,0x3f);return e;}if(p->resume_pending){rtw_write8(p,REG_TXPAUSE,p->resume_txpause);p->resume_pending=false;}ctx->phy_initialized=ctx->io_error==0;p->dm_info.thermal_meter_k=ctx->board.thermal_meter;return ctx->io_error;}
int rtw_8814au_phy_tick(struct rtw_8814au*ctx){struct rtw_8814au_phy*p;u8 therm,avg,delta,path;if(!ctx||!(p=ctx->phy)||!ctx->phy_initialized)return EINVAL;if(ctx->io_error)return ctx->io_error;if(ctx->board.thermal_meter==0xff)return 0;if(!p->dm_info.pwr_trk_triggered){rtw_write_rf(p,0,RF_T_METER,GENMASK(17,16),3);p->dm_info.pwr_trk_triggered=true;return ctx->io_error?fail_paused(p,ctx->io_error):0;}p->dm_info.pwr_trk_triggered=false;therm=(u8)rtw_read_rf(p,0,RF_T_METER,0xfc00);if(ctx->io_error)return fail_paused(p,ctx->io_error);
 {u32*value=&p->dm_info.avg_thermal[0].value;*value=*value?(((*value*3U)+(u32)therm*1024U)>>2):(u32)therm*1024U;avg=(u8)(*value>>10);}delta=(u8)phy_abs((int)avg-(int)ctx->board.thermal_meter);if(delta>29)delta=29;
 for(path=0;path<4;path++){bool positive=avg>ctx->board.thermal_meter;const u8*values=p->hal.current_band_type==RTW_BAND_5G?swing_5g[p->efuse.rfe_option][p->hal.current_channel>=149?2:p->hal.current_channel>=100?1:0][positive][path]:swing_tables[p->efuse.rfe_option][p->tx_rate<=3][positive][path];p->dm_info.delta_power_index[path]= (s8)(positive?(int)values[delta]:-(int)values[delta]);phy14_pwrtrack_set_pwr(p,path,(u8)(63-p->power[path][p->tx_rate]));}
 if(phy_abs((int)avg-(int)p->dm_info.thermal_meter_k)>=8){int e=rtw_8814au_phy_calibrate(ctx);if(e)return e;p->dm_info.thermal_meter_k=avg;}return ctx->io_error?fail_paused(p,ctx->io_error):0;}
int rtw_8814au_phy_stop(struct rtw_8814au*ctx){struct rtw_8814au_phy*p;if(!ctx||!(p=ctx->phy))return EINVAL;ctx->phy_initialized=0;p->cleanup=true;rtw_write8(p,REG_TXPAUSE,0x3f);rtw_write8(p,REG_RF_CTRL,0);rtw_write8(p,REG_RF_CTRL1,0);rtw_write8(p,REG_RF_CTRL2,0);rtw_write8(p,REG_RF_CTRL3,0);rtw_write8_clr(p,REG_SYS_CFG3_8814A+2,BIT_FEN_BB_RSTB|BIT_FEN_BB_GLB_RST);p->cleanup=false;p->loaded=false;p->channel_set=false;return ctx->io_error;}
