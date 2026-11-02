/* SPDX-License-Identifier: BSD-3-Clause
 * AU OTP layouts: main.h; PG/limit/index formulas: phy.c; TXAGC addressing:
 * rtw88xxa.c. Exact source revisions are in the PHY upstream manifest. */
static int operating_geometry(const rtw_r12au_operating*o,u8*width,u8*sc,u8 centers[3]){
 if(!o||!width||!sc||!centers||!allowed(o->primary)||o->mode<RTW_R12AU_MODE_LEGACY||o->mode>RTW_R12AU_MODE_VHT)return ENOTSUP;
 if(rtw_station_is_dfs_channel(o->primary)&&o->width_mhz!=20)return ENOTSUP;
 unsigned p=o->primary,c=o->center;centers[0]=o->primary;centers[1]=centers[2]=0;*sc=0;
 if(o->width_mhz==20){if(p!=c)return ENOTSUP;*width=RTW_CHANNEL_WIDTH_20;}
 else if(o->width_mhz==40){
  if(o->mode==RTW_R12AU_MODE_LEGACY||abs_diff(p,c)!=2)return ENOTSUP;
  int secondary=(int)(2*c)-(int)p;if(secondary<1||secondary>255||!allowed((u8)secondary))return ENOTSUP;
  if(p<=14){if(c<3||c>9||secondary>11)return ENOTSUP;}
  else if(c!=38&&c!=46&&c!=151&&c!=159)return ENOTSUP;
  centers[1]=o->center;*sc=(u8)(p>c?RTW_SC_20_UPPER:RTW_SC_20_LOWER);*width=RTW_CHANNEL_WIDTH_40;
 }else if(o->width_mhz==80){
  if(o->mode!=RTW_R12AU_MODE_VHT||p<=14||(c!=42&&c!=155)||(abs_diff(p,c)!=2&&abs_diff(p,c)!=6))return ENOTSUP;
  centers[1]=(u8)(p>c?c+4:c-4);centers[2]=o->center;
  *sc=(u8)(p>c?(p-c==2?RTW_SC_20_UPPER:RTW_SC_20_UPMOST):(c-p==2?RTW_SC_20_LOWER:RTW_SC_20_LOWEST));*width=RTW_CHANNEL_WIDTH_80;
 }else return ENOTSUP;
 if(o->mode==RTW_R12AU_MODE_VHT&&p<=14)return ENOTSUP;
 return 0;
}
static unsigned power_section(unsigned rate){return rate<4?0:rate<12?1:rate<20?2:rate<28?3:rate<54?4:5;}
static unsigned power_reference(unsigned section){static const unsigned ref[6]={3,11,19,27,51,61};return ref[section];}
static int power_overlap(const void*a,size_t an,const void*b,size_t bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return an&&bn&&(x<=y?y-x<an:x-y<bn);}
static int limit_for(const struct rtw_txpwr_lmt_cfg_pair*table,size_t count,unsigned band,unsigned bw,unsigned rs,unsigned ch,int*out){
 bool any=false;int minimum=63;
 for(unsigned regd=0;regd<3;++regd){bool found=false;int value=63;
  for(size_t i=0;i<count;++i){const struct rtw_txpwr_lmt_cfg_pair*q=&table[i];if(q->regd==regd&&q->band==band&&q->bw==bw&&q->rs==rs&&q->ch==ch){if(q->value<value)value=q->value;found=true;}}
  /* Source cross-reference of missing HT/VHT limits: same NSS and20/40MHz. */
  if(!found&&band&&bw<=1&&rs>=2){unsigned paired=rs<4?rs+2:rs-2;
   for(size_t i=0;i<count;++i){const struct rtw_txpwr_lmt_cfg_pair*q=&table[i];if(q->regd==regd&&q->band==band&&q->bw==bw&&q->rs==paired&&q->ch==ch){if(q->value<value)value=q->value;found=true;}}
  }
  if(found){any=true;if(value<minimum)minimum=value;}
 }
 if(!any)return ENOTSUP;*out=minimum;return 0;
}
static int base_index(const u8 rom[42],unsigned band,unsigned group_index,unsigned width,unsigned rate,int*out){
 unsigned section=power_section(rate);int value;
 if(!band){
  unsigned base=section==0?group_index:6+group_index;if(rom[base]>63)return ENOTSUP;value=rom[base];
  if(section==1){if(rom[11]==255)return ENOTSUP;value+=signed4(rom[11]);}
  else if(section>=2){
   if(width==0){if(rom[11]==255)return ENOTSUP;value+=signed4(rom[11]>>4);}
   if(section==3){if(rom[12]==255)return ENOTSUP;value+=signed4((u8)(rom[12]>>(width?4:0)));}
  }
 }else{
  if(18+group_index>=32||rom[18+group_index]>63)return ENOTSUP;value=rom[18+group_index];
  if(section==1){if(rom[32]==255)return ENOTSUP;value+=signed4(rom[32]);}
  else if(width==2){
   /* main.h: ofdm_diff occupies bytes36..37; vht_1s/2s_diff
    * occupy38/39, with BW80 in the high signed nibble (BW160 low). */
   if(19+group_index>=32||rom[19+group_index]>63||rom[38]==255)return ENOTSUP;
   value=((int)rom[18+group_index]+rom[19+group_index])/2+signed4(rom[38]>>4);
   if(section==3||section==5){if(rom[39]==255)return ENOTSUP;value+=signed4(rom[39]>>4);}
  }else{
   if(width==0){if(rom[32]==255)return ENOTSUP;value+=signed4(rom[32]>>4);}
   if(section==3||section==5){if(rom[33]==255)return ENOTSUP;value+=signed4((u8)(rom[33]>>(width?4:0)));}
  }
 }
 *out=value;return 0;
}
static int power_plan_limit(const rtw_r12au_phy_board*b,const rtw_r12au_operating*o,rtw_r12au_power_plan*out,int32_t ceiling_mbm,u8 ceiling[2][64]){
 if(!out||!b||!o||power_overlap(out,sizeof(*out),b,sizeof(*b))||power_overlap(out,sizeof(*out),o,sizeof(*o)))return EINVAL;
 memset(out,0,sizeof(*out));if(ceiling)memset(ceiling,63,128);u8 width,sc,centers[3];if(!board_supported(b)||operating_geometry(o,&width,&sc,centers))return ENOTSUP;
 const struct rtw_phy_pg_cfg_pair*pg;size_t n;const struct rtw_txpwr_lmt_cfg_pair*limits;size_t nl;
 if(b->chip==RTW_R12AU_CHIP_8812AU){pg=b->rfe==3?rtw8812a_bb_pg_rfe3:rtw8812a_bb_pg;n=b->rfe==3?ARRAY_SIZE(rtw8812a_bb_pg_rfe3):ARRAY_SIZE(rtw8812a_bb_pg);limits=rtw8812a_txpwr_lmt;nl=ARRAY_SIZE(rtw8812a_txpwr_lmt);}
 else{pg=rtw8821a_bb_pg;n=ARRAY_SIZE(rtw8821a_bb_pg);limits=rtw8821a_txpwr_lmt;nl=ARRAY_SIZE(rtw8821a_txpwr_lmt);}
 const unsigned band=o->primary>14;int byrate[2][64];for(unsigned p=0;p<2;++p)for(unsigned r=0;r<64;++r)byrate[p][r]=-1;
 for(size_t i=0;i<n;++i){const struct rtw_phy_pg_cfg_pair*q=&pg[i];if(q->band!=band||q->path>=b->rf_paths)continue;unsigned base=q->path?0xe20:0xc20;if(q->addr<base||q->addr>base+0x2c||q->addr%4)continue;
  unsigned offset=q->addr-base;for(unsigned j=0;j<4;++j){if(((q->mask>>(j*8))&255U)!=255U)continue;unsigned rate=offset+j;if(rate>=28)rate+=16;byrate[q->path][rate]=bcd((u8)(q->data>>(j*8)));}
 }
 rtw_r12au_power_plan value;memset(&value,0,sizeof(value));
 for(unsigned rate=0;rate<64;++rate){
  unsigned rs=power_section(rate);if((rate>=28&&rate<44)||(band&&rs==0)||(rs==3&&b->rf_paths==1)||(rs==5&&b->rf_paths==1)||
    (o->mode==RTW_R12AU_MODE_LEGACY&&rs>=2)||(o->mode!=RTW_R12AU_MODE_VHT&&rs>=4))continue;
  unsigned reference=power_reference(rs),max_width=rs<2?0:rs<4?(width>1?1:width):width;int limit=63;
  for(unsigned w=0;w<=max_width;++w){int current=0;if(limit_for(limits,nl,band,w,rs,centers[w],&current))return ENOTSUP;if(current<limit)limit=current;}
  if(byrate[0][reference]<0)return ENOTSUP;int cap=limit-byrate[0][reference];
  for(unsigned path=0;path<b->rf_paths;++path){if(byrate[path][rate]<0||byrate[path][reference]<0)return ENOTSUP;
   int base=0;if(base_index(b->txpwr_idx[path],band,group(o->center),width,rate,&base))return ENOTSUP;
   int pathcap=cap;
   if(ceiling_mbm!=INT32_MAX){
    /* Pinned sar.c: dBm*2 minus the path/rate-section PG reference.
     * Negative/fractional mBm rounds down; factory OTP base remains intact. */
    int64_t mbm=ceiling_mbm;int absolute=(int)(mbm>=0?mbm/50:-((-mbm+49)/50));
    int sarcap=absolute-byrate[path][reference],maximum=base+sarcap;
    if(maximum<0)return ERANGE;if(maximum>63)maximum=63;
    if(ceiling)ceiling[path][rate]=(u8)maximum;if(sarcap<pathcap)pathcap=sarcap;
   }
   int delta=byrate[path][rate]-byrate[path][reference];if(delta>pathcap)delta=pathcap;
   int index=base+delta;if(index<0)index=0;if(index>63)index=63;value.index[path][rate]=(u8)index;
  }
  value.valid_rates|=UINT64_C(1)<<rate;
 }
 if(o->mode==RTW_R12AU_MODE_VHT&&width==0)value.valid_rates&=~((UINT64_C(1)<<53)|(UINT64_C(1)<<63));
 *out=value;return 0;
}
int rtw_r12au_phy_power_plan(const rtw_r12au_phy_board*b,const rtw_r12au_operating*o,rtw_r12au_power_plan*out){return power_plan_limit(b,o,out,INT32_MAX,NULL);}
static void program_operating_power(struct rtw_r12au_phy*p){
 /* Linux rtw88 SAR uses txgi exponent1 for both AU chips:2indices/dB.
  * Back off after normal clipping/thermal compensation. Never silently
  * clamp an insufficient requested reduction to the hardware floor. */
 unsigned reduction=(unsigned)p->power_reduction_db*2U;
 for(unsigned path=0;path<p->hal.rf_phy_num;++path)for(unsigned rate=0;rate<64;++rate)
  if((p->power_valid&(UINT64_C(1)<<rate))&&(p->power[path][rate]<p->power_ceiling[path][rate]?p->power[path][rate]:p->power_ceiling[path][rate])<reduction){fault(p,ERANGE);(void)pause_tx(p);return;}

 for(unsigned path=0;path<p->hal.rf_phy_num;++path)for(unsigned offset=p->hal.current_band_type?4:0;offset<=0x2c;offset+=4){
  if(p->operating.mode==RTW_R12AU_MODE_LEGACY&&offset>=12)break;
  if(p->operating.mode==RTW_R12AU_MODE_HT&&offset>=28)break;
  if(p->hal.rf_phy_num==1&&(offset==20||offset==24||offset>=40))continue;
  u32 word=0;unsigned bytes=p->hal.rf_phy_num==1&&offset==36?2:4;
  for(unsigned j=0;j<bytes;++j){unsigned rate=offset+j;if(rate>=28)rate+=16;word|=(u32)(((p->power_valid&(UINT64_C(1)<<rate))?((p->power[path][rate]<p->power_ceiling[path][rate]?p->power[path][rate]:p->power_ceiling[path][rate])-reduction):p->power[path][rate]))<<(8*j);}
  if(bytes==2)rtw_write32_mask(p,(path?0xe20U:0xc20U)+offset,0xffff,word);else wr(p,(path?0xe20U:0xc20U)+offset,4,word);
 }
}
