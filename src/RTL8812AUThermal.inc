/* SPDX-License-Identifier: BSD-3-Clause
 * Measured AU thermal tracking, rtw88xxa.c/phy.c and chip pwr_track callbacks.
 * Exact chip/RFE tables are generated from the pinned primary sources.
 * Signed swing arithmetic is bounded before indexing, avoiding upstream u8
 * wrap on cold readings. ROM swing defaults are retained per physical path. */
#define RTW_PWR_TRK_TBL_SZ 30
#include "RTL8812AUThermalTables.inc"
struct au_thermal_tables{
 const u8(*hot5[2])[30],(*cold5[2])[30];
 const u8 *hot2[2],*cold2[2],*hotcck[2],*coldcck[2];
};
#define AU_THERMAL_TABLES(prefix) {{prefix##_5ga_p,prefix##_5gb_p},{prefix##_5ga_n,prefix##_5gb_n},{prefix##_2ga_p,prefix##_2gb_p},{prefix##_2ga_n,prefix##_2gb_n},{prefix##_2g_cck_a_p,prefix##_2g_cck_b_p},{prefix##_2g_cck_a_n,prefix##_2g_cck_b_n}}
static const struct au_thermal_tables au_thermal_8812=AU_THERMAL_TABLES(rtw8812a_pwrtrk);
static const struct au_thermal_tables au_thermal_8812rfe3=AU_THERMAL_TABLES(rtw8812a_pwrtrk_rfe3);
static const struct au_thermal_tables au_thermal_8821=AU_THERMAL_TABLES(rtw8821a_pwrtrk);
#undef AU_THERMAL_TABLES
static unsigned tracking_limit(u8 rate){
 if(rate<4)return 32;if(rate<11)return 30;if(rate==11)return 28;
 unsigned mcs=rate>=44?(rate-44U)%10U:(rate-12U)%8U;
 if(mcs<=2)return 34;if(mcs<=4)return 30;if(rate<44||mcs<=6)return 28;if(mcs==7)return 26;return mcs==8?24:22;
}
static const u8*thermal_table(struct rtw_r12au_phy*p,unsigned path,bool hot){
 const struct au_thermal_tables*t=p->chip->id==RTW_CHIP_TYPE_8821A?&au_thermal_8821:p->board.rfe==3?&au_thermal_8812rfe3:&au_thermal_8812;
 if(!p->hal.current_band_type)return p->dm_info.tx_rate<4?(hot?t->hotcck[path]:t->coldcck[path]):(hot?t->hot2[path]:t->cold2[path]);
 unsigned group5=p->hal.current_channel<=64?0:p->hal.current_channel<=144?1:2;return hot?t->hot5[path][group5]:t->cold5[path][group5];
}
static int thermal_fail(struct rtw_r12au_phy*p,int error){p->calibrated=false;(void)pause_tx(p);return error;}
int rtw_r12au_phy_set_tx_rate(rtw_r12au_phy*p,u8 rate){
 if(!p)return EINVAL;if(p->io_error)return p->io_error;if(rate>=64||!(p->power_valid&(UINT64_C(1)<<rate)))return EINVAL;p->dm_info.tx_rate=rate;return 0;
}
int rtw_r12au_phy_tick(rtw_r12au_phy*p){
 if(!p)return EINVAL;if(p->io_error)return thermal_fail(p,p->io_error);if(!p->calibrated||!p->tx_enabled)return EBUSY;
 struct rtw_dm_info*d=&p->dm_info;
 if(!d->pwr_trk_triggered){rtw_write_rf(p,RF_PATH_A,RF_T_METER,GENMASK(17,16),3);if(p->io_error)return thermal_fail(p,p->io_error);d->pwr_trk_triggered=true;return 0;}
 d->pwr_trk_triggered=false;u32 measured=rtw_read_rf(p,RF_PATH_A,RF_T_METER,0xfc00);if(p->io_error)return thermal_fail(p,p->io_error);
 if(!measured||measured>=63)return EAGAIN;
 p->thermal=(u8)measured;++p->thermal_samples;
 /* DECLARE_EWMA(thermal,10,4), using a bounded fixed-point accumulator. */
 u32 previous=d->avg_thermal[0].value;d->avg_thermal[0].value=previous?(((previous<<2)-previous)+(measured<<10))>>2:measured<<10;
 unsigned average=d->avg_thermal[0].value>>10;d->thermal_avg[0]=(u8)average;
 bool need_iqk=abs_diff(average,(unsigned)d->thermal_meter_k)>=8;
 unsigned delta=abs_diff(average,(unsigned)p->board.thermal);if(delta>=30)delta=29;
 bool hot=average>p->board.thermal,changed=false;bool was_tx=p->tx_enabled;
 for(unsigned path=0;path<p->hal.rf_phy_num;++path){const u8*table=thermal_table(p,path,hot);int correction=hot?(int)table[delta]:-(int)table[delta];
  if(correction==(int)d->delta_power_index_last[path])continue;
  if(!changed){if(pause_tx(p))return thermal_fail(p,p->io_error);changed=true;}
  d->delta_power_index[path]=d->delta_power_index_last[path]=(s8)correction;
  u32 rom_swing=r12_get_bb_swing(p,p->hal.current_band_type,(u8)path);unsigned default_index=0;
  while(default_index<ARRAY_SIZE(r12_txscale_tbl)&&r12_txscale_tbl[default_index]!=rom_swing)++default_index;
  if(default_index==ARRAY_SIZE(r12_txscale_tbl))return thermal_fail(p,ENOTSUP);
  int raw=(int)default_index+correction,cck=(p->chip->id==RTW_CHIP_TYPE_8821A?0:24)+correction;unsigned limit=tracking_limit(d->tx_rate);
  int swing=raw;if(swing>(int)limit){d->txagc_remnant_ofdm[path]=(s8)(swing-(int)limit);if(!path)d->txagc_remnant_cck=(s8)(cck-(int)limit);swing=(int)limit;}
  else if(swing<=0){d->txagc_remnant_ofdm[path]=(s8)swing;if(!path)d->txagc_remnant_cck=(s8)cck;swing=0;}
  else{d->txagc_remnant_ofdm[path]=0;if(!path)d->txagc_remnant_cck=0;}
  if((unsigned)swing>=ARRAY_SIZE(r12_txscale_tbl))return thermal_fail(p,ERANGE);
  rtw_write32_mask(p,path?REG_TXSCALE_B:REG_TXSCALE_A,GENMASK(31,21),r12_txscale_tbl[swing]);
 }
 d->pwr_trk_init_trigger=false;
 if(changed){
  /* Same ordering as Linux: normalized PG/regulatory index, followed by
   * measured thermal remnant; the final hardware index is bounded0..63. */
  for(unsigned path=0;path<p->hal.rf_phy_num;++path)for(unsigned rate=0;rate<64;++rate){int index=p->power_base[path][rate]+(rate<4?d->txagc_remnant_cck:d->txagc_remnant_ofdm[path]);if(index<0)index=0;if(index>63)index=63;p->power[path][rate]=(u8)index;}
  program_operating_power(p);if(p->io_error)return thermal_fail(p,p->io_error);
 }
 if(need_iqk){int e=rtw_r12au_phy_calibrate(p);if(e)return thermal_fail(p,e);d->thermal_meter_k=(u8)average;++p->thermal_recalibrations;}
 if(was_tx&&(changed||need_iqk)){int e=rtw_r12au_phy_resume_tx(p);if(e)return thermal_fail(p,e);}return 0;
}
