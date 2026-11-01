/* SPDX-License-Identifier: GPL-2.0-only
 * Portable checked RTL8814B PHY/RFK boundary. The included primary algorithms
 * retain the original Realtek copyright and GNU GPL version2 notices.
 */
#include "RTW8814BPHY-Notices.inc"
#include "rtw/RTW8814BPHY.h"
#include <stdbool.h>
#include <string.h>
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef uint64_t u64;
typedef int8_t s8;typedef int16_t s16;typedef int32_t s32;typedef int64_t s64;typedef bool boolean;
#define BIT(n) (UINT32_C(1)<<(n))
#define BIT_ULL(n) (UINT64_C(1)<<(n))
#define BIT0 BIT(0)
#define BIT1 BIT(1)
#define BIT2 BIT(2)
#define BIT3 BIT(3)
#define BIT4 BIT(4)
#define BIT5 BIT(5)
#define BIT6 BIT(6)
#define BIT7 BIT(7)
#define BIT8 BIT(8)
#define BIT9 BIT(9)
#define BIT10 BIT(10)
#define BIT11 BIT(11)
#define BIT12 BIT(12)
#define BIT13 BIT(13)
#define BIT14 BIT(14)
#define BIT15 BIT(15)
#define BIT16 BIT(16)
#define BIT17 BIT(17)
#define BIT18 BIT(18)
#define BIT19 BIT(19)
#define BIT20 BIT(20)
#define BIT21 BIT(21)
#define BIT22 BIT(22)
#define BIT23 BIT(23)
#define BIT24 BIT(24)
#define BIT25 BIT(25)
#define BIT26 BIT(26)
#define BIT27 BIT(27)
#define BIT28 BIT(28)
#define BIT29 BIT(29)
#define BIT30 BIT(30)
#define BIT31 BIT(31)
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#include "RTW8814BPHY-Macros.inc"
#include "RTW8814BPHY-Types.inc"
struct rtwb14_phy {
 uint32_t magic;rtwb14_context *hal;rtwb14_phy_board board;rtwb14_phy_status status;
 struct dm_struct dm;uint8_t logical[RTWB14_OTP_BYTES];
 uint8_t channel,second_channel,width,band,mp_mode;bool iqk_first,dpk_clear,rf56_txbb;uint32_t dack_runs;
 unsigned operations,restore_depth,restore_operations;uint64_t delayed;
};
_Static_assert(sizeof(struct rtwb14_phy)<=RTWB14_PHY_SCRATCH_BYTES,"B14 PHY scratch overflow");
#define PHY_MAGIC 0xb140cafeU
#define REG_TXPAUSE REG_TXPAUSE_8814B
#define rf_56_txbb_flag (dm->port->rf56_txbb)
#define central_ch_8814b (dm->port->channel)
#define central_ch2_8814b (dm->port->second_channel)
#define bw_8814b (dm->port->width)
static int phy_fail(struct dm_struct*dm,int e){rtwb14_phy*p=dm->port;if(e<0)e=e==INT32_MIN?EIO:-e;if(e&&!p->status.error)p->status.error=e;if(e)p->status.calibrated=p->status.tx_enabled=0;return p->status.error;}
static int phy_budget(struct dm_struct*dm){if(!rtwb14_phy_owned(dm->port->hal,dm->port))return phy_fail(dm,ENXIO);if(dm->port->restore_depth)return ++dm->port->restore_operations>8192U?phy_fail(dm,ETIMEDOUT):0;return dm->port->status.error?dm->port->status.error:++dm->port->operations>500000U?phy_fail(dm,ETIMEDOUT):0;}
static u32 phy_read(struct dm_struct*dm,u32 a,unsigned w){u32 v=0;if(phy_budget(dm))return 0;if(a>65536U-w){phy_fail(dm,EINVAL);return 0;}int e=rtwb14_register_read(dm->port->hal,(u16)a,w,&v);if(e){phy_fail(dm,e);return 0;}return v;}
static void phy_write(struct dm_struct*dm,u32 a,unsigned w,u32 v){if(!phy_budget(dm)){if(a>65536U-w)phy_fail(dm,EINVAL);else phy_fail(dm,rtwb14_register_write(dm->port->hal,(u16)a,w,v));}}
static unsigned shift(u32 m){unsigned n=0;if(m)while(!(m&1U)){m>>=1;++n;}return n;}
static u32 odm_get_bb_reg(struct dm_struct*dm,u32 a,u32 m){if(!m){phy_fail(dm,EINVAL);return 0;}return(phy_read(dm,a,4)&m)>>shift(m);}
static void odm_set_bb_reg(struct dm_struct*dm,u32 a,u32 m,u32 v){if(!m){phy_fail(dm,EINVAL);return;}u32 old=m==UINT32_MAX?0:phy_read(dm,a,4);phy_write(dm,a,4,(old&~m)|((v<<shift(m))&m));}
#define odm_get_mac_reg odm_get_bb_reg
#define odm_set_mac_reg odm_set_bb_reg
static u8 odm_read_1byte(struct dm_struct*dm,u32 a){return(u8)phy_read(dm,a,1);}
static u16 odm_read_2byte(struct dm_struct*dm,u32 a){return(u16)phy_read(dm,a,2);}
static u32 odm_read_4byte(struct dm_struct*dm,u32 a){return phy_read(dm,a,4);}
static void odm_write_1byte(struct dm_struct*dm,u32 a,u8 v){phy_write(dm,a,1,v);}
static void odm_write_2byte(struct dm_struct*dm,u32 a,u16 v){phy_write(dm,a,2,v);}
static void odm_write_4byte(struct dm_struct*dm,u32 a,u32 v){phy_write(dm,a,4,v);}
static void phy_delay(struct dm_struct*dm,uint64_t n){if(phy_budget(dm))return;if(n>120000000U-dm->port->delayed){phy_fail(dm,ETIMEDOUT);return;}dm->port->delayed+=n;while(n&&!dm->port->status.error){u32 step=n>50000U?50000U:(u32)n;phy_fail(dm,rtwb14_delay_us(dm->port->hal,step));n-=step;}}
#define ODM_delay_us(n) phy_delay(dm,(n))
#define ODM_delay_ms(n) phy_delay(dm,(uint64_t)(n)*1000U)
#define ODM_sleep_us(n) ODM_delay_us(n)
#define ODM_sleep_ms(n) ODM_delay_ms(n)
static enum hal_status phydm_set_reg_by_fw(struct dm_struct*dm,enum phydm_halmac_param t,u32 a,u32 v,u32 mask,enum rf_path path,u32 delay){(void)t;(void)a;(void)v;(void)mask;(void)path;(void)delay;phy_fail(dm,ENOTSUP);return HAL_STATUS_FAILURE;}
#include "RTW8814BPHY-Prototypes.inc"
static u32 odm_get_rf_reg(struct dm_struct*dm,enum rf_path p,u32 a,u32 m){return config_phydm_read_rf_reg_8814b(dm,p,a,m);}
static void odm_set_rf_reg(struct dm_struct*dm,enum rf_path p,u32 a,u32 m,u32 v){if(!config_phydm_write_rf_reg_8814b(dm,p,a,m,v))phy_fail(dm,EIO);}
static void odm_efuse_logical_map_read(struct dm_struct*dm,u8 n,u16 offset,u32*out){if(!out||n!=1||offset>=RTWB14_OTP_BYTES){phy_fail(dm,EINVAL);return;}*out=dm->port->logical[offset];}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"
#include "RTW8814BPHY-phydm_hal_api8814b.inc"
#include "RTW8814BPHY-phydm_regconfig8814b.inc"
#include "RTW8814BPHY-halhwimg8814b_bb.inc"
#include "RTW8814BPHY-halhwimg8814b_rf.inc"
#include "RTW8814BPHY-halrf_rfk_init_8814b.inc"
#include "RTW8814BPHY-halrf.inc"
#include "RTW8814BPHY-halrf_8814b.inc"
#include "RTW8814BPHY-halrf_iqk_8814b.inc"
#include "RTW8814BPHY-halrf_dpk_8814b.inc"
#include "RTW8814BPHY-halrf_txgapk_8814b.inc"
#pragma clang diagnostic pop
static int overlaps(const void*a,size_t an,const void*b,size_t bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return an&&bn&&(x<=y?y-x<an:x-y<bn);}
size_t rtwb14_phy_size(void){return sizeof(rtwb14_phy);}
int rtwb14_phy_board_validate(const rtwb14_phy_board*b){
 if(!b)return EINVAL;if(!b->logical||b->logical_bytes<0xd4U||b->logical_bytes>RTWB14_OTP_BYTES||!b->thermal_base||b->thermal_base>63)return ENODATA;
 if(b->rfe_type>9||b->package_type>15||b->crystal_cap>63)return ENOTSUP;
 /* Primary non-AP TSSI/thermal map: do not confuse this with AU power bases. */
 for(unsigned p=0;p<4;p++){unsigned base=0x10+42*p;bool programmed=false;for(unsigned i=0;i<11;i++)programmed|=b->logical[base+i]!=255;for(unsigned i=0;i<14;i++)programmed|=b->logical[base+18+i]!=255;if(!programmed||!b->logical[0xd0+p]||b->logical[0xd0+p]>63)return ENODATA;}
 if((b->logical[0xc8]>>4)==15)return ENODATA;return 0;
}
static int phy_valid(const rtwb14_phy*p){return p&&p->magic==PHY_MAGIC&&p->dm.port==p&&p->hal;}
static int phy_begin(rtwb14_phy*p){if(!phy_valid(p))return ENXIO;if(p->status.error)return p->status.error;if(!rtwb14_phy_owned(p->hal,p))return phy_fail(&p->dm,ENXIO);p->operations=p->restore_operations=0;p->delayed=0;int e=rtwb14_live_firmware(p->hal);if(!e&&!rtwb14_phy_owned(p->hal,p))e=ENXIO;return phy_fail(&p->dm,e);}
int rtwb14_phy_open(void*storage,size_t n,rtwb14_context*hal,const rtwb14_phy_board*b,rtwb14_phy**out){
 if(!out||!storage||!hal||n<RTWB14_PHY_SCRATCH_BYTES||((uintptr_t)storage&7U)||overlaps(storage,RTWB14_PHY_SCRATCH_BYTES,hal,sizeof(*hal))||overlaps(storage,RTWB14_PHY_SCRATCH_BYTES,out,sizeof(*out))||!b||overlaps(storage,RTWB14_PHY_SCRATCH_BYTES,b,sizeof(*b)))return EINVAL;
 int e=rtwb14_phy_board_validate(b);if(e)return e;if(overlaps(storage,RTWB14_PHY_SCRATCH_BYTES,b->logical,b->logical_bytes))return EINVAL;
 if((e=rtwb14_phy_claim(hal,storage)))return e;
 if((e=rtwb14_live_firmware(hal))||!rtwb14_phy_owned(hal,storage)){rtwb14_phy_release(hal,storage);return e?e:ENXIO;}
 u32 cut=0;if((e=rtwb14_register_read(hal,0xf1,1,&cut))||!rtwb14_phy_owned(hal,storage)){rtwb14_phy_release(hal,storage);return e?e:ENXIO;}
 rtwb14_phy*p=storage;memset(p,0,sizeof(*p));p->magic=PHY_MAGIC;p->hal=hal;p->board=*b;memset(p->logical,255,sizeof(p->logical));memcpy(p->logical,b->logical,b->logical_bytes);p->board.logical=p->logical;
 p->status.rf_paths=4;p->status.synthesizers=2;p->channel=1;p->iqk_first=p->dpk_clear=true;
 struct dm_struct*dm=&p->dm;dm->port=p;dm->cut_version=(u8)(cut>>4);dm->support_ic_type=ODM_RTL8814B;dm->rfe_type=b->rfe_type;dm->package_type=b->package_type;dm->support_platform=ODM_CE;dm->support_interface=ODM_ITRF_USB;
 dm->type_glna=b->type_glna;dm->type_gpa=b->type_gpa;dm->type_alna=b->type_alna;dm->type_apa=b->type_apa;
 dm->ext_pa=(b->board_type&ODM_BOARD_EXT_PA)!=0;dm->ext_pa_5g=(b->board_type&ODM_BOARD_EXT_PA_5G)!=0;
 dm->channel=&p->channel;dm->band_type=&p->band;dm->band_width=&p->width;dm->mp_mode=&p->mp_mode;dm->tx_ant_status=dm->rx_ant_status=BB_PATH_ABCD;
 dm->rf_table.eeprom_thermal=b->thermal_base;dm->rf_table.rf_supportability=HAL_RF_IQK|HAL_RF_LCK|HAL_RF_DPK|HAL_RF_TXGAPK|HAL_RF_DACK|HAL_RF_DPK_TRACK;
 dm->dpk_info.is_dpk_enable=dm->dpk_info.is_dpk_pwr_on=dm->dpk_info.is_dpk_by_channel=true;
 halrf_tssi_get_efuse_8814b(dm);halrf_get_efuse_thermal_pwrtype_8814b(dm);if(!rtwb14_phy_owned(hal,p))phy_fail(dm,ENXIO);if(p->status.error){rtwb14_phy_release(hal,p);return p->status.error;}*out=p;return 0;
}
int rtwb14_phy_load(rtwb14_phy*p){int e=phy_begin(p);if(e)return e;if(p->status.tables)return EBUSY;struct dm_struct*dm=&p->dm;
 phy_write(dm,REG_TXPAUSE,1,255);
 if(!config_phydm_parameter_init_8814b(dm,ODM_PRE_SETTING))phy_fail(dm,EIO);
 odm_read_and_config_mp_8814b_phy_reg(dm);odm_read_and_config_mp_8814b_agc_tab(dm);
 odm_read_and_config_mp_8814b_radioa(dm);odm_read_and_config_mp_8814b_radiob(dm);odm_read_and_config_mp_8814b_radioc(dm);odm_read_and_config_mp_8814b_radiod(dm);
 odm_read_and_config_mp_8814b_radiosyn0(dm);odm_read_and_config_mp_8814b_radiosyn1(dm);odm_read_and_config_mp_8814b_cal_init(dm);
 phydm_rfe_8814b_init(dm);if(!config_phydm_parameter_init_8814b(dm,ODM_POST_SETTING)||!config_phydm_trx_mode_8814b(dm,BB_PATH_ABCD,BB_PATH_ABCD))phy_fail(dm,EIO);
 if(!p->status.error)p->status.tables=1;return p->status.error;
}
static int legal_channel(unsigned ch){return(ch>=1&&ch<=11)||(ch>=36&&ch<=48&&ch%4==0)||(ch>=149&&ch<=165&&(ch-149)%4==0);}
static int valid_power(const rtwb14_phy_power*power){if(!power)return 0;for(unsigned p=0;p<4;p++){if(power->cck[p]>127||power->ofdm[p]>127)return 0;for(unsigned r=0;r<RTWB14_PHY_RATE_COUNT;r++){int value=(r<4?power->cck[p]:power->ofdm[p])+power->difference[r];if(power->difference[r]<-64||power->difference[r]>63||value<0||value>127)return 0;}}return 1;}
static void phy_reset_calibration(rtwb14_phy*p){
 p->status.calibrated=0;p->status.tx_enabled=0;
 p->status.dack_paths=0;p->status.iqk_paths=0;p->status.dpk_paths=0;
 p->status.lck_synths=0;p->status.txgapk_paths=0;
 p->status.dpk_skipped=0;p->status.txgapk_skipped=0;
}
int rtwb14_phy_tune(rtwb14_phy*p,u8 ch,const rtwb14_phy_power*power){if(!legal_channel(ch)||!valid_power(power))return EINVAL;int e=phy_begin(p);if(e)return e;if(!p->status.tables)return ENXIO;
 struct dm_struct*dm=&p->dm;p->status.channel=0;phy_reset_calibration(p);
 phy_write(dm,REG_TXPAUSE,1,255);if(!config_phydm_switch_channel_8814b(dm,ch)||!config_phydm_switch_bw_8814b(dm,0,CHANNEL_WIDTH_20))phy_fail(dm,EIO);p->band=ch<=14?ODM_BAND_2_4G:ODM_BAND_5G;
 struct txagc_table_8814b table;memcpy(table.ref_pow_cck,power->cck,4);memcpy(table.ref_pow_ofdm,power->ofdm,4);memcpy(table.diff_t,power->difference,sizeof(table.diff_t));phydm_set_txagc_by_table_8814b(dm,&table);
 if(!p->status.error)p->status.channel=ch;return p->status.error;
}
int rtwb14_phy_calibrate(rtwb14_phy*p){int e=phy_begin(p);if(e)return e;if(!p->status.tables||!p->status.channel)return ENXIO;struct dm_struct*dm=&p->dm;phy_reset_calibration(p);phy_write(dm,REG_TXPAUSE,1,255);
 halrf_dac_cal_8814b(dm);if(p->status.error)return p->status.error;p->status.dack_paths=15;phy_lc_calibrate_8814b(dm);if(!p->status.error)p->status.lck_synths=3;
 phy_txgap_calibrate_8814b(dm,true);if(p->status.error)return p->status.error;phy_iq_calibrate_8814b(dm,true,false);
 for(unsigned path=0;path<4;path++)if(!dm->IQK_info.lok_fail[path]&&!dm->IQK_info.iqk_fail_report[0][path][0]&&!dm->IQK_info.iqk_fail_report[0][path][1])p->status.iqk_paths|=(u8)(1U<<path);
 if(p->status.iqk_paths!=15&&!p->status.error)phy_fail(dm,EIO);
 if(!p->status.error){p->status.dpk_skipped=dm->ext_pa||dm->ext_pa_5g;do_dpk_8814b(dm);p->status.dpk_paths=(u8)(dm->dpk_info.dpk_path_ok&15);if(!p->status.dpk_skipped&&p->status.dpk_paths!=15)phy_fail(dm,EIO);}
 if(!p->status.error){p->status.txgapk_skipped=(dm->ext_pa&&p->band==ODM_BAND_2_4G)||(dm->ext_pa_5g&&p->band==ODM_BAND_5G);p->status.txgapk_paths=p->status.txgapk_skipped?0:15;p->status.calibrated=1;}
 return p->status.error;
}
int rtwb14_phy_track(rtwb14_phy*p){int e=phy_begin(p);if(e)return e;if(!p->status.calibrated)return ENXIO;if(!p->status.dpk_skipped)dpk_track_8814b(&p->dm);return p->status.error;}
int rtwb14_phy_resume(rtwb14_phy*p){int e=phy_begin(p);if(e)return e;if(!p->status.calibrated)return ENXIO;phy_write(&p->dm,REG_TXPAUSE,1,0);if(!p->status.error)p->status.tx_enabled=1;return p->status.error;}
int rtwb14_phy_stop(rtwb14_phy*p){int e=phy_begin(p);if(e)return e;phy_write(&p->dm,REG_TXPAUSE,1,255);p->status.tx_enabled=p->status.calibrated=0;return p->status.error;}
static int valid_mask(u32 mask,u32 value,int write){return mask&&!(mask&~0xfffffU)&&(!write||(value<=(mask>>shift(mask))&&!((value<<shift(mask))&~mask)));}
int rtwb14_phy_rf_read(rtwb14_phy*p,unsigned path,u8 reg,u32 mask,u32*out){if(!out||path>=4||!valid_mask(mask,0,0))return EINVAL;int e=phy_begin(p);if(e)return e;u32 v=config_phydm_read_rf_reg_8814b(&p->dm,(enum rf_path)path,reg,mask);if(!p->status.error)*out=v;return p->status.error;}
int rtwb14_phy_rf_write(rtwb14_phy*p,unsigned path,u8 reg,u32 mask,u32 value){if(path>=4||!valid_mask(mask,value,1))return EINVAL;int e=phy_begin(p);if(e)return e;if(!config_phydm_write_rf_reg_8814b(&p->dm,(enum rf_path)path,reg,mask,value))phy_fail(&p->dm,EIO);return p->status.error;}
int rtwb14_phy_synth_read(rtwb14_phy*p,unsigned path,u8 reg,u32 mask,u32*out){if(!out||path>=2||!valid_mask(mask,0,0))return EINVAL;int e=phy_begin(p);if(e)return e;u32 v=config_phydm_read_syn_reg_8814b(&p->dm,(enum rf_syn)path,reg,mask);if(!p->status.error)*out=v;return p->status.error;}
int rtwb14_phy_synth_write(rtwb14_phy*p,unsigned path,u8 reg,u32 mask,u32 value){if(path>=2||!valid_mask(mask,value,1))return EINVAL;int e=phy_begin(p);if(e)return e;if(!config_phydm_write_rf_syn_8814b(&p->dm,(enum rf_syn)path,reg,mask,value))phy_fail(&p->dm,EIO);return p->status.error;}
rtwb14_phy_status rtwb14_phy_get_status(const rtwb14_phy*p){rtwb14_phy_status s={0};if(!phy_valid(p)){s.error=ENXIO;return s;}if(!rtwb14_phy_owned(p->hal,p)){s.error=p->status.error?p->status.error:ENXIO;return s;}return p->status;}
void rtwb14_phy_forget(rtwb14_phy*p){if(p&&p->magic==PHY_MAGIC&&p->dm.port==p){rtwb14_phy_release(p->hal,p);memset(&p->status,0,sizeof(p->status));p->magic=0;p->hal=NULL;}}
int rtwb14_phy_get_factory(const rtwb14_phy*p,rtwb14_phy_factory*out){
 if(!out||overlaps(p,sizeof(*p),out,sizeof(*out)))return EINVAL;if(!phy_valid(p)||!rtwb14_phy_owned(p->hal,p))return ENXIO;
 memcpy(out->tssi,p->dm.rf_table.halrf_tssi_data.tssi_efuse,sizeof(out->tssi));
 memcpy(out->thermal,p->dm.rf_table.halrf_tssi_data.thermal,sizeof(out->thermal));out->power_track_type=p->dm.rf_table.power_track_type;return 0;
}
