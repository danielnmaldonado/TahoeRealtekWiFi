#!/usr/bin/env python3
"""Generate the pinned B14 PHY/RFK closure, never another chip's tables."""
from pathlib import Path
import argparse,hashlib,json,re,subprocess
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--check',action='store_true');a=p.parse_args()
paths=['phydm/halrf/halrf.c']+['phydm/rtl8814b/'+n for n in ['phydm_hal_api8814b.c','phydm_regconfig8814b.c','halhwimg8814b_bb.c']]+['phydm/halrf/rtl8814b/'+n for n in ['halhwimg8814b_rf.c','halrf_8814b.c','halrf_iqk_8814b.c','halrf_dpk_8814b.c','halrf_txgapk_8814b.c','halrf_rfk_init_8814b.c']]
headers=['cmn_info_file/rtw_sta_info.h','WlanHAL/HalMac88XX/halmac_reg_8814b.h','WlanHAL/HalHeader/HalComReg.h','phydm/phydm_interface.h','phydm/phydm_types.h','phydm/phydm_regtable.h','phydm/halrf/halrf_psd.h','phydm/phydm_pre_define.h','phydm/phydm_regdefine11ac.h','phydm/phydm_regdefine11n.h','phydm/phydm_regdefine.h','phydm/phydm_api.h','phydm/phydm_dig.h','phydm/phydm.h','phydm/halrf/halrf.h','phydm/halrf/halrf_iqk.h','phydm/halrf/halrf_dpk.h','phydm/halrf/halrf_powertracking_ap.h','phydm/halrf/halrf_powertracking_ce.h','phydm/rtl8814b/phydm_hal_api8814b.h']+['phydm/halrf/rtl8814b/'+n for n in ['halrf_8814b.h','halrf_iqk_8814b.h','halrf_dpk_8814b.h','halrf_txgapk_8814b.h','halrf_rfk_init_8814b.h']]
headers=[n for n in headers if (a.source/n).exists()]
manifest=r/'src/RTW8814BPHY-Upstream.json';expected=json.loads(manifest.read_text())['sources'] if manifest.exists() else {}
sources={n:(a.source/n).read_text() for n in paths+headers};hashes={n:hashlib.sha256((a.source/n).read_bytes()).hexdigest() for n in sources}
if not expected or hashes!=expected:raise SystemExit('Pinned B14 source hashes differ or manifest is missing')
settings='''#define RTL8814B_SUPPORT 1
#define ODM_WIN 1
#define ODM_CE 2
#define ODM_AP 4
#define DM_ODM_SUPPORT_TYPE ODM_CE
#define PHYDM_FW_API_ENABLE_8814B 1
#define PHYDM_FW_API_FUNC_ENABLE_8814B 0
#define __odm_func__
#define __iram_odm_func__
#define RF_DBG(...)
#define PHYDM_DBG(...)
#define ODM_RT_TRACE(...)
#define pr_debug(...)
#define pr_info(...)
'''
def preprocess(text):
 text=re.sub(r'(?m)^\s*#\s*include[^\n]*(?:\\\n[^\n]*)*','',text)
 return subprocess.run(['clang','-E','-P','-x','c','-'],input=settings+text,text=True,capture_output=True,check=True).stdout
# Extract complete function bodies after selecting the non-offloaded host path.
functions={};arrays={};notices={};processed={}
for path in paths:
 raw=sources[path];text=preprocess(raw);processed[path]=text
 notice=raw[:raw.find('#include')].strip();notices[path]=notice
 # The BB and RF image files contain their own static condition resolver.
 if 'halhwimg' in path:
  for name in ['check_positive','check_negative']:
   text=re.sub(r'\b'+name+r'\b',name+'_'+Path(path).stem,text)
 for m in re.finditer(r'(?m)^(?:static\s+)?(?:void|boolean|u8|u16|u32|s8|s16|s32|u64|s64)\s+([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{',text):
  end=m.end();depth=1
  while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
  name=m[1];body=text[m.start():end]
  if name=='_iq_calibrate_8814b_init':
   body=body.replace('static boolean firstrun = true;','').replace('firstrun','dm->port->iqk_first')
  if name=='phy_iq_calibrate_8814b':
   dead=re.search(r'if\s*\(0\)\s*\{',body)
   if dead:
    stop=dead.end();nest=1
    while nest:nest+=(body[stop]=='{')-(body[stop]=='}');stop+=1
    otherwise=re.match(r'\s*else\s*\{',body[stop:]);assert otherwise
    begin=stop+otherwise.end();stop2=begin;nest=1
    while nest:nest+=(body[stop2]=='{')-(body[stop2]=='}');stop2+=1
    body=body[:dead.start()]+body[begin:stop2-1]+body[stop2:]
  if name in ['halrf_dack1_8814b','halrf_dack2_8814b']:
   body=re.sub(r'if\s*\(c\s*==\s*30\)\s*;', 'if(c==30){phy_fail(dm,ETIMEDOUT);return;}',body)
   tail=body.rfind('}')
   body=body[:tail]+'if(i==10 && '+('!(abs_idc<5&&abs_qdc<5)' if name=='halrf_dack1_8814b' else '!(idc<5&&qdc<5)')+'){phy_fail(dm,EIO);return;}\n'+body[tail:]
  if name=='halrf_mode':
   body=body.replace('c = 0;\n do {','if(i!=SN){*i_value=*q_value=0;phy_fail(dm,EIO);return;}\n c = 0;\n do {',1)
   body=body.replace('} while (c < 100);','} while (c < 100);\n if(i_delta>5||q_delta>5){*i_value=*q_value=0;phy_fail(dm,EIO);return;}',1)
  if name in ['halrf_reload_bp','halrf_reload_bprf','_iqk_reload_macbb_8814b','_iqk_reload_rf_8814b','_dpk_reload_macbb_8814b','_dpk_reload_rf_8814b','_txgapk_phy_restore_8814b','_txgapk_rf_restore_8814b']:
   brace=body.index('{')+1
   if 'void *dm_void' in body[:brace]:
    decl=re.search(r'struct dm_struct \*dm\s*=.*?;',body);assert decl;br=decl.end()
   else:br=brace
   body=body[:br]+'\n ++dm->port->restore_depth;\n'+body[br:]
   assert not re.search(r'\breturn\s*;',body)
   end=body.rfind('}');body=body[:end]+'--dm->port->restore_depth;\n'+body[end:]
  if name=='_phy_txgapk_calibrate_8814b':body=body.replace('u32 rf3f[11],','u32 rf3f[12],')
  if name=='_txgapk_one_shot_8814b':
   body=body.replace('return rx_dsp_power;','if(!rx_dsp_power||rx_dsp_power==0xabcdefULL)phy_fail(dm,EIO);\n return rx_dsp_power;',1)
  if name=='_txgapk_afe_setting_8814b':
   brace=body.index('{')+1;body=body[:brace]+'\n if(!do_txgapk)++dm->port->restore_depth;\n'+body[brace:];end=body.rfind('}');body=body[:end]+'if(!do_txgapk)--dm->port->restore_depth;\n'+body[end:]
  if name=='_dpk_check_fail_8814b':
   old="""  dpk_info->pwsf_2g[path][group] = 0;
  dpk_info->pwsf_5g[path][group - G_2G_N] = 0;
  dpk_info->dpk_result[path][group] = 0;
  dpk_info->dpk_5g_result[path][group - G_2G_N] = 0;"""
   new="""  if(group<G_2G_N){
   dpk_info->pwsf_2g[path][group]=0;
   dpk_info->dpk_result[path][group]=0;
  }else{
   dpk_info->pwsf_5g[path][group-G_2G_N]=0;
   dpk_info->dpk_5g_result[path][group-G_2G_N]=0;
  }"""
   assert body.count(old)==1;body=body.replace(old,new)
  if name=='dpk_track_8814b':
   for path_index in range(1,4):
    old=f'thermal_value[{path_index}] < dpk_info->thermal_dpk[0]'
    assert body.count(old)==1
    body=body.replace(old,f'thermal_value[{path_index}] < dpk_info->thermal_dpk[{path_index}]')
  if name=='_do_dpk_8814b':body=body.replace('static boolean do_clear = true;','').replace('do_clear','dm->port->dpk_clear')
  if name=='halrf_dac_cal_8814b':
   body=body.replace('static u32 count = 0;','').replace('count++','dm->port->dack_runs++')
   body=body.replace('u32 temp = 0, v1, v2;','u32 temp = 0, v1 = 0, v2 = 0;').replace('u32 iadc[4], qadc[4], idac[4], qdac[4];','u32 iadc[4]={0}, qadc[4]={0}, idac[4]={0}, qdac[4]={0};')
  body=body.replace('u32 *array =','const u32 *array =')
  if name in functions:raise SystemExit('Duplicate B14 function:'+name)
  functions[name]=(path,body)
 for m in re.finditer(r'(?m)^(?:static\s+)?(?:const\s+)?(?:u8|u16|u32|s8|s16|s32|char|const char\s*\*)\s+([A-Za-z_]\w*)\s*\[[^;]*?=\s*\{.*?\n\s*\};',text,re.S):arrays[m[1]]=(path,m[0])
roots=['config_phydm_read_rf_reg_8814b','config_phydm_read_syn_reg_8814b','config_phydm_write_rf_reg_8814b','config_phydm_write_rf_syn_8814b','config_phydm_parameter_init_8814b','config_phydm_switch_channel_8814b','config_phydm_switch_bw_8814b','config_phydm_trx_mode_8814b','phydm_rfe_8814b_init','phydm_set_txagc_by_table_8814b','phy_iq_calibrate_8814b','do_dpk_8814b','dpk_track_8814b','phy_txgap_calibrate_8814b','phy_lc_calibrate_8814b','halrf_dac_cal_8814b','halrf_tssi_get_efuse_8814b','halrf_get_efuse_thermal_pwrtype_8814b','odm_read_and_config_mp_8814b_cal_init']
roots+=['odm_read_and_config_mp_8814b_'+n for n in ['agc_tab','phy_reg','radioa','radiob','radioc','radiod','radiosyn0','radiosyn1']]
selected=set();pending=list(roots)
while pending:
 n=pending.pop()
 if n in selected:continue
 if n not in functions:raise SystemExit('Missing B14 function:'+n)
 selected.add(n)
 pending += [v for v in re.findall(r'\b(\w+)\s*\(',functions[n][1]) if v in functions and v not in selected]
needed=set()
for n in selected:needed.update(v for v in re.findall(r'\b\w+\b',functions[n][1]) if v in arrays)
notice='/* SPDX-License-Identifier: GPL-2.0-only\n * Generated from tobyw121/Openwrt_RTL@85ef7eb7ccbd4336cd6e628a743ddc211333901c.\n * Vendor notices and primary algorithms preserved; checked host adapter only. */\n'
outputs={};prototypes=[]
# Keep every copied header's copyright and redistribution notice as well as
# the algorithm C-file notices, even when Linux/platform declarations are cut.
header_notices=[];seen_notices=set()
for path in headers:
 raw=sources[path]
 for block in re.findall(r'/\*.*?\*/|(?m:^\s*//[^\n]*SPDX[^\n]*)',raw,re.S):
  if not re.search(r'copyright|general public license|SPDX',block,re.I):continue
  if block in seen_notices:continue
  seen_notices.add(block);header_notices.append('/* Primary header: '+path+' */\n'+block)
outputs['src/RTW8814BPHY-Notices.inc']=notice+'\n\n'.join(header_notices)+'\n'

for path in paths:
 chunks=[notices[path]]
 for n in sorted(needed):
  if arrays[n][0]==path:chunks.append('static const '+re.sub(r'^(?:static\s+)?(?:const\s+)?','',arrays[n][1]))
 for n in sorted(selected):
  if functions[n][0]!=path:continue
  body=functions[n][1];signature=body[:body.index('{')].strip();signature=re.sub(r'^static\s+','',signature)
  prototypes.append('static '+signature+';');chunks.append('static '+re.sub(r'^static\s+','',body))
 outputs['src/RTW8814BPHY-'+Path(path).stem+'.inc']=notice+'\n\n'.join(chunks)+'\n'
outputs['src/RTW8814BPHY-Prototypes.inc']=notice+'\n'.join(prototypes)+'\n'
# Keep the exact primary constants and selected common type source available
# to the private boundary, without Linux driver/platform headers.
outputs['src/RTW8814BPHY-PrimaryHeaders.inc']=notice+preprocess('\n'.join(sources[n] for n in headers))
header_text=preprocess('\n'.join(sources[n] for n in headers))
def declarations(kind):
 found={}
 for m in re.finditer(r'(?m)^'+kind+r'\s+(\w+)\s*\{',header_text):
  end=m.end();depth=1
  while depth:depth+=(header_text[end]=='{')-(header_text[end]=='}');end+=1
  found[m[1]]=header_text[m.start():end]+';'
 return found
structs=declarations('struct');enums=declarations('enum')
used_dm=set()
for n in selected:used_dm.update(re.findall(r'\bdm->(\w+)',functions[n][1]))
dm=structs['dm_struct'];fields=[v.strip()+';' for v in dm[dm.index('{')+1:dm.rindex('}')].split(';') if any(re.search(r'\b'+k+r'\b',v) for k in used_dm)]
structs['dm_struct']='struct dm_struct {\n struct rtwb14_phy *port;\n'+'\n'.join(fields)+'\n};'
ordered=[];seen=set()
def visit(n):
 if n in seen:return
 if n not in structs:return
 seen.add(n);body=structs[n]
 for dep in re.findall(r'\bstruct\s+(\w+)\s+(?!\*)(\w+)',body[body.index('{')+1:]):visit(dep[0])
 ordered.append(body)
visit('dm_struct')
for n in selected:
 for t in re.findall(r'\bstruct\s+(\w+)',functions[n][1]):visit(t)
outputs['src/RTW8814BPHY-Types.inc']=notice+'\n'.join(enums.values())+'\n'+'\n'.join(ordered)+'\n'
# Only primary-defined macros are exported, excluding compiler internals.
macro_names=set(re.findall(r'(?m)^\s*#\s*define\s+(\w+)', '\n'.join(sources[n] for n in headers)))
macro_dump=subprocess.run(['clang','-E','-dM','-x','c','-'],input=settings+re.sub(r'(?m)^\s*#\s*include[^\n]*(?:\\\n[^\n]*)*','', '\n'.join(sources[n] for n in headers)),capture_output=True,text=True,check=True).stdout
outputs['src/RTW8814BPHY-Macros.inc']=notice+'\n'.join(line for line in macro_dump.splitlines() if re.match(r'#define (\w+)',line)[1] in macro_names and not re.match(r'#define _',line))+'\n'

meta={'repository':'tobyw121/Openwrt_RTL','commit':'85ef7eb7ccbd4336cd6e628a743ddc211333901c','license':'GPL-2.0-only','sources':hashes,'roots':roots,'functions':{n:{'file':functions[n][0],'sha256':hashlib.sha256(functions[n][1].encode()).hexdigest()} for n in sorted(selected)},'table_arrays':sorted(needed),'adaptations':[
 'Select non-offloaded CE host branches. The unreachable literal if(0) IQK firmware branch is removed; AP firmware H2Cs are not assumed compatible with ABI11 USB NIC firmware.',
 'Omit diagnostic logging, namespace the two image condition resolvers, and make read-only primary table arrays const. Preserve all original algorithm and copied-header copyright/license notices.',
 'Move the primary channel/width/RF56 globals, IQK firstrun, DPK do_clear and DACK diagnostic counter into the serialized per-device context.',
 'Increase only TXGAPK rf3f[11] to rf3f[12], because the primary gain snapshot writes indices1 through11. Fault-model UBSan reproduced the original stack overrun.',
 'On DPK failure, clear only the selected 2G or5G result group. The primary unconditionally writes both arrays, overflowing the3-entry2G row for5G and underflowing the5G row for2G. Actual algorithm fault injection reproduced this with UBSan.',
 'For per-channel DPK thermal compensation, compare pathsB/C/D against their own calibration temperature, matching the adjacent per-path absolute-delta calculation instead of the primary copied pathA sign reference.',
 'Latch DACK ready-poll timeout and nonconverging final ADC/DAC residuals. Reject incomplete or unstable common halrf_mode sample windows and zero/sentinel TXGAPK PSD instead of reporting RFK success.',
 'Initialize DACK output arrays and v1/v2 so a failed measurement cannot feed indeterminate scalar values into subsequent source cleanup.',
 'After a measurement error permit only named original saved-register/disable cleanup functions under a separate8192-operation budget. HAL retained ownership/live-firmware checks still guard every access. Abort the public RFK pipeline after failed DACK or TXGAPK before later algorithms could create fresh zero-valued backups.',
 'Reset public RFK path masks at each calibration attempt; a previous success cannot mask a failed recalibration. Preserve explicit primary external-PA DPK and per-band TXGAPK skips without inventing successful path bits.',
 'Decode signed factory TSSI25groups/path, thermalD0..D3 and power-track-typeC8 from the primary CE logical map. RFE/front-end/global thermal provenance and the actual regulatory/channel power plan remain explicit verified caller inputs; no AU ROM layout is substituted.',
 'Expose20MHz non-DFS geometry1..11,36/40/44/48,149/153/157/161/165 only; validate masks, channel/power inputs, aligned nonoverlapping storage and bounded work/delays before hardware changes.'
 ],'limits':[
 'No B14 station profile, catalog capability, association/WPA2 or traffic enablement is added by this component.',
 'The provided power plan is representability checked, not derived from a qualified USB factory/regulatory profile.',
 'TSSI factory decoding is implemented; TSSI DE programming/DCK and the generic thermal swing/TSSI runtime are not ported. Runtime track currently executes the primary DPK tracking routine only.',
 'RFE/front-end/crystal metadata production from the actual USB OTP is not proven here; crystal_cap is validated metadata, not a programmed CFO tracking implementation.',
 'DSP/register completion in tests is modeled; no physical RFK, RF output, USB station or throughput validation is claimed.'
 ],'physical_validation':False}
outputs['src/RTW8814BPHY-Upstream.json']=json.dumps(meta,indent=2)+'\n'
for name,text in outputs.items():
 dst=r/name
 if a.check:
  if not dst.exists() or dst.read_text()!=text:raise SystemExit('Generated B14 output differs:'+name)
 else:dst.write_text(text)
print('Verified' if a.check else 'Generated',len(outputs),'B14 artifacts;',len(selected),'actual functions;',len(needed),'primary arrays')
