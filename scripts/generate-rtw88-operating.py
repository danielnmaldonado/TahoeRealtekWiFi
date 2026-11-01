#!/usr/bin/env python3
"""Port the pinned rtw88 B/C operating PHY function dependency closure.
Generates private types/macros/data/functions only; never opens a device.
The explicit checked adapter and safety changes live in RTW88PHYOperating.c.
"""
from pathlib import Path
import argparse,re,json,hashlib
p=argparse.ArgumentParser(description=__doc__);p.add_argument('reference',type=Path);args=p.parse_args();root=Path(__file__).resolve().parents[1]
meta=json.loads((root/'src/RTW88USBDataPath-Upstream.json').read_text());expected={x['path']:x['sha256'] for x in meta['files']}
files=['main.h','phy.h','reg.h','fw.h','mac.c','phy.c','rtw8821c.h','rtw8822b.h','rtw8822c.h','rtw8821c.c','rtw8822b.c','rtw8822c.c','rtw8821c_table.c','rtw8822b_table.c','rtw8822c_table.c','regd.c','efuse.h']
sources={}
for name in files:
 b=(args.reference/name).read_bytes()
 if hashlib.sha256(b).hexdigest()!=expected[name]:raise SystemExit('Upstream source hash mismatch: '+name)
 sources[name]=b.decode()
funcs={};ranges={}
pattern=r'(?m)^(?:(?:static|inline|const)\s+)*(?:[a-zA-Z_][\w]*\s+)+\**\s*([a-zA-Z_]\w*)\s*\([^;{}]*\)\s*\{'
for file in ['phy.c','mac.c','rtw8821c.c','rtw8822b.c','rtw8822c.c']:
 s=sources[file];ranges[file]=[]
 for m in re.finditer(pattern,s):
  begin=s.index('{',m.start());level=1;end=begin+1
  while level:level+=(s[end]=='{')-(s[end]=='}');end+=1
  funcs[m.group(1)]=(file,s[m.start():end]);ranges[file].append((m.start(),end))
roots=[]
for chip in ['8821c','8822b','8822c']:
 roots+=['rtw'+chip+'_'+n for n in ['set_channel','phy_calibration','pwr_track','set_tx_power_index','pwrtrack_init']]
roots+=['check_rf_status','rtw8822c_dpk_track','rtw8822c_gain_check_state','rtw8822c_gain_large_state','rtw8822c_gain_less_state','rtw8822c_gl_large_state','rtw8822c_gl_less_state','rtw8822c_loss_check_state','rtw8822b_set_channel_rfe_efem','rtw8822b_set_channel_rfe_ifem','rtw_set_channel_mac','rtw8822b_config_trx_mode','rtw8822b_phy_rfe_init','rtw8822c_config_trx_mode','rtw8822c_rf_init','rtw8822c_parse_tbl_dpk','rtw_parse_tbl_bb_pg','rtw_parse_tbl_txpwr_lmt','rtw_phy_init_tx_power','rtw_phy_tx_power_by_rate_config','rtw_phy_tx_power_limit_config']
selected=set()
def visit(name):
 if name in selected or name not in funcs:return
 selected.add(name)
 for dep in re.findall(r'\b(\w+)\s*\(',funcs[name][1]):
  if dep!=name:visit(dep)
for name in roots:visit(name)
# Global objects referenced directly by selected functions, plus each real
# chip's RFE/thermal-track definitions (token-pasted names require explicit roots).
objects={}
for file in ['phy.c','mac.c','rtw8821c.c','rtw8822b.c','rtw8822c.c']:
 s=sources[file]
 for m in re.finditer(r'(?m)^(?:static\s+)?const\s+[\w\s*]+?\b(\w+)\s*(?:\[[^\]]*\]\s*)*=\s*\{',s):
  if any(a<=m.start()<b for a,b in ranges[file]):continue
  begin=s.index('{',m.start());level=1;end=begin+1
  while level:level+=(s[end]=='{')-(s[end]=='}');end+=1
  end=s.index(';',end)+1;objects[m.group(1)]=(file,s[m.start():end])
text='\n'.join(funcs[n][1] for n in selected)
needed={name for name in objects if name in text or re.match(r'rtw882[12][bc]_(pwrtrk_|pwr_track_type|rfe_defs)',name)}
while True:
 content='\n'.join(objects[n][1] for n in needed);more={n for n in objects if n in content};new=needed|more
 if new==needed:break
 needed=new
# All public scalar constants used by the selected primary code.
def defines(s):
 return '\n'.join(line for line in s.replace('\\\n',' ').splitlines() if re.match(r'^\s*#define\s+',line) and not re.match(r'^\s*#define\s+(?:__|rtw_write)',line))
main=sources['main.h'];typeout=['/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */','/* Copyright(c)2018-2019 Realtek Corporation. Generated private pinned rtw88 types. */',defines(main),defines(sources['reg.h']),defines(sources['phy.h'])]
enums=['rtw_hci_type','rtw_bandwidth','rtw_sc_offset','rtw_rf_type','rtw_rf_path','rtw_bb_path','rtw_rate_section','rtw_chip_type','rtw_trx_desc_rate','rtw_regulatory_domains','rtw_chip_ver','rtw_rfe_fem','rtw_rf_band','rtw_dm_cap','rtw_sar_bands']
for name in enums:
 m=re.search(r'enum '+name+r'\s*\{.*?\n\};',main,re.S);assert m,name;typeout.append(m.group(0))
# Original packed power-index structures. No Linux object or lock is imported.
start=main.index('struct rtw_2g_1s_pwr_idx_diff {');end=main.index('struct rtw_channel_params {');typeout.append(main[start:end])
for name in ['rtw_backup_info','rtw_iqk_para','rtw_pwr_track_tbl','rtw_swing_table','rtw_dpk_info','rtw_gapk_info']:
 m=re.search(r'struct '+name+r'\s*\{.*?\n\}(?: __packed)?;',main,re.S)
 if not m:
  m=re.search(r'struct '+name+r'\s*\{.*?\n\}(?: __packed)?;',sources['phy.h']+'\n'+sources['fw.h'],re.S)
 assert m,name;typeout.append(m.group(0))
for name in ['rtw_phy_pg_cfg_pair','rtw_txpwr_lmt_cfg_pair','rtw_power_params']:
 m=re.search(r'struct '+name+r'\s*\{.*?\n\}(?: __packed)?;',sources['phy.h'],re.S);assert m,name
 typeout.append(m.group(0).replace('u8 pwr_base;', 's16 pwr_base;').replace('s8 pwr_sar;', 's32 pwr_sar;') if name=='rtw_power_params' else m.group(0))
(root/'src/RTW88PHYOperatingTypes.inc').write_text('\n\n'.join(typeout)+'\n')
# Function declarations precede data containing function pointers.
protos=[]
for name in sorted(selected):
 signature=funcs[name][1].split('{',1)[0].strip()
 if not signature.startswith('static '):signature='static '+signature
 if name in ['rtw_phy_get_2g_tx_power_index','rtw_phy_get_5g_tx_power_index']:signature=re.sub(r'^static\s+u8\b','static s16',signature)
 if name=='rtw_phy_get_tx_power_sar':signature=re.sub(r'^static\s+s8\b','static s32',signature)
 protos.append(signature+';')
(root/'src/RTW88PHYOperatingPrototypes.inc').write_text('\n'.join(protos)+'\n')
for file,label in [('phy.c','Common'),('rtw8821c.c','8821C'),('rtw8822b.c','8822B'),('rtw8822c.c','8822C')]:
 out=['/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */','/* Copyright(c)2018-2019 Realtek Corporation. Pinned function bodies; checked adapter in RTW88PHYOperating.c. */']
 if label!='Common':
  chip=file[3:-2];out.append(defines(sources[file[:-2]+'.h']))
 # Global macros outside functions retain their source order.
 outside=sources[file]
 for a,b in reversed(ranges[file]):outside=outside[:a]+outside[b:]
 out.append(defines(outside))
 if label=='8821C':out.append(re.search(r'enum rtw8821ce_rf_set\s*\{.*?\n\};',sources[file],re.S).group(0))
 if label=='8822B':
  s=sources[file];a=s.index('enum {\n\tCCUT_IDX_');b=s.index('static void rtw8822b_set_channel_cca',a);out.append(s[a:b])
 if label=='8822C':
  for pattern in [r'enum rtw8822c_dpk_agc_phase\s*\{.*?\n\};',r'enum rtw8822c_dpk_one_shot_action\s*\{.*?\n\};']:
   out.append(re.search(pattern,sources['rtw8822c.h'],re.S).group(0))
  out.append(re.search(r'static u8 \(\*dpk_state\[\]\).*?= \{.*?\};',sources[file],re.S).group(0))
  for name in ['rtw8822c_dpk_data','dpk_cfg_pair']:
   m=re.search(r'struct '+name+r'\s*\{.*?\n\};',sources[file],re.S);assert m,name;out.append(m.group(0))
 for name in [n for n in objects if n in needed]:
  if objects[name][0]!=file:continue
  if label=='8822B' and (name.startswith('cca_') or name=='rtw8822b_rfe_info'):continue
  declaration=objects[name][1].split('=',1)[0].strip()
  if not declaration.startswith('static '):declaration='static '+declaration
  out.append(declaration+';')
 for name in [n for n in objects if n in needed]:
  if objects[name][0]!=file:continue
  if label=='8822B' and (name.startswith('cca_') or name=='rtw8822b_rfe_info'):continue
  obj=objects[name][1]
  if not obj.startswith('static '):obj='static '+obj
  out.append(obj)
 for name in sorted(selected):
  if funcs[name][0]!=file and not (label=='Common' and funcs[name][0]=='mac.c'):continue
  if label=='8822B' and name=='rtw8822b_get_cca_val':continue
  body=funcs[name][1]
  if not body.startswith('static '):body='static '+body
  # Full primary loops remain bounded; only checked-result integration changes
  # are applied below and individually documented/tested in the adapter.
  if name=='rtw8822c_dac_backup_reg':body=body.replace('backup_rf[path * i + i]','backup_rf[path * DACK_RF_8822C + i]')
  if 'struct rtw_dev *rtwdev' not in body.split('{',1)[0]:body=body.replace('WARN_ON(', 'NATIVE_WARN_ON(').replace('WARN(', 'NATIVE_WARN(')
  if name=='rtw8822c_dac_cal_iq_sample':
   body=body.replace('u32 temp;', 'u32 temp;\n\tmemset(iv,0,DACK_SN_8822C*sizeof(*iv));memset(qv,0,DACK_SN_8822C*sizeof(*qv));')
   body=body[:-1]+'\n if(i<DACK_SN_8822C)native_fail(rtwdev,EIO);\n}'
  if name=='rtw8822c_dac_cal_iq_search':
   body=body.replace('rtw8822c_dac_iq_offset(rtwdev, iv, i_value);','if(i_delta>5||q_delta>5)native_fail(rtwdev,EIO);\n\trtw8822c_dac_iq_offset(rtwdev, iv, i_value);')
  if name=='rtw8822c_dac_cal_adc':
   body=body.replace('/* ADCK step2 */', 'if(ic>=5||qc>=5)native_fail(rtwdev,EIO);\n\t/* ADCK step2 */')
  if name=='rtw8822c_dpk_gainloss':
   body=body.replace('\n\t\trtw8822c_dpk_dc_corr_check(rtwdev, path);','\n\t\tif(rtw8822c_dpk_dc_corr_check(rtwdev,path))native_fail(rtwdev,EIO);')
  if name=='rtw8822c_psd_log2base':
   body=body.replace('(val * 100) / (1 << val_integerd_b)', '((u64)val * 100U) / (UINT64_C(1) << val_integerd_b)')
  if name in ['rtw8821c_do_iqk','rtw8822b_do_iqk']:
   body=body.replace('rtw_fw_do_iqk(rtwdev, &para);','rtw_write_rf(rtwdev,RF_PATH_A,RF_DTXLOK,RFREG_MASK,0);\n\trtw_fw_do_iqk(rtwdev, &para);')
   body=body.replace('rtw_write_rf(rtwdev, RF_PATH_A, RF_DTXLOK, RFREG_MASK, 0x0);','if(counter>=300)native_fail(rtwdev,ETIMEDOUT);\n\trtw_write_rf(rtwdev, RF_PATH_A, RF_DTXLOK, RFREG_MASK, 0x0);')
   body=body.replace('iqk_fail_mask = rtw_read32_mask(rtwdev, REG_IQKFAILMSK, GENMASK(7, 0));','iqk_fail_mask = rtw_read32_mask(rtwdev, REG_IQKFAILMSK, GENMASK(7, 0));\n\tif(iqk_fail_mask)native_fail(rtwdev,EIO);\n\tif(!native_error(rtwdev))rtwdev->iqk_complete=1;')
  if name=='rtw8822c_do_iqk':
   body=body.replace('rtw_fw_do_iqk(rtwdev, &para);','rtw_write8(rtwdev,REG_IQKSTAT,0);\n\trtw_fw_do_iqk(rtwdev,&para);')
   body=body.replace('rtw_write8(rtwdev, REG_IQKSTAT, 0x0);','if(!ret&&!native_error(rtwdev))rtwdev->iqk_complete=1;\n\trtw_write8(rtwdev, REG_IQKSTAT, 0x0);')
  if name=='rtw8822c_rf_dac_cal':
   body=body.replace('if (rtw8822c_dac_cal_restore(rtwdev))\n\t\treturn;','if (rtw8822c_dac_cal_restore(rtwdev)){rtwdev->dac_complete=1;return;}')
   body=body.replace('rtw8822c_dac_cal_step4(rtwdev, RF_PATH_A);','rtwdev->dac_a_complete=(ic<5&&qc<5);\n\trtw8822c_dac_cal_step4(rtwdev, RF_PATH_A);')
   body=body.replace('rtw8822c_dac_cal_step4(rtwdev, RF_PATH_B);','rtwdev->dac_complete=(rtwdev->dac_a_complete&&ic<5&&qc<5&&!native_error(rtwdev));\n\trtw8822c_dac_cal_step4(rtwdev, RF_PATH_B);')
   body=body.replace('rtw8822c_dac_cal_backup(rtwdev);','if(rtwdev->dac_complete&&!native_error(rtwdev))rtw8822c_dac_cal_backup(rtwdev);else native_fail(rtwdev,EIO);')
  if name in ['rtw_phy_get_2g_tx_power_index','rtw_phy_get_5g_tx_power_index','rtw_phy_get_tx_power_index']:
   body=body.replace('u8 tx_power;', 'int tx_power;').replace('return tx_power;', 'return (u8)clamp_t(int,tx_power,0,rtwdev->chip->max_power_index);')
  if name in ['rtw_phy_get_2g_tx_power_index','rtw_phy_get_5g_tx_power_index']:
   body=re.sub(r'^static\s+u8\b','static s16',body)
   body=body.replace('return (u8)clamp_t(int,tx_power,0,rtwdev->chip->max_power_index);','return (s16)tx_power;')
  if name=='rtw_phy_get_tx_power_index':
   body=body.replace('s8 offset;', 's32 offset;')
   body=body.replace('return (u8)clamp_t(int,tx_power,0,rtwdev->chip->max_power_index);','return native_power_index(rtwdev,(u8)clamp_t(int,tx_power,0,rtwdev->chip->max_power_index),rate,&pwr_param);')
  if name=='rtw_get_tx_power_params':body=body.replace('u8 *base =', 's16 *base =').replace('s8 *sar =', 's32 *sar =')
  if name=='rtw_phy_get_tx_power_sar':body=re.sub(r'^static\s+s8\b','static s32',body)
  if name in ['rtw8821c_pwrtrack_set','rtw8822b_pwrtrack_set']:
   body=body.replace('rtw_phy_get_tx_power_index(', 'native_thermal_power_index(')
  if name=='rtw8822c_dpk_cal_coef1':
   body=body.replace('i_scaling = 0x16c00 / dpk_info->dpk_gs[path];','if(!dpk_info->dpk_gs[path]||native_error(rtwdev)){native_fail(rtwdev,EIO);return;}\n\t\ti_scaling = 0x16c00 / dpk_info->dpk_gs[path];')
  if name=='rtw8822c_dpk_set_gnt_wl':
   body=body.replace('dpk_info->gnt_value = rtw_coex_read_indirect_reg(rtwdev, 0x38);','dpk_info->gnt_value = rtw_coex_read_indirect_reg(rtwdev, 0x38);\n\t\tif(native_error(rtwdev))return;rtwdev->gnt_saved=1;')
   body=body.replace('} else {','} else {\n\t\tif(!rtwdev->gnt_saved)return;++rtwdev->restore_depth;')
   at=body.rfind('\n\t}');body=body[:at]+'\n\t\trtwdev->gnt_saved=0;--rtwdev->restore_depth;'+body[at:]
  if name=='rtw8822c_rfk_handshake':
   body=body.replace('} else {','} else {\n\t\t++rtwdev->restore_depth;')
   at=body.rfind('\n\t}');body=body[:at]+'\n\t\t--rtwdev->restore_depth;'+body[at:]
  if name=='rtw8822c_dpk_pas_agc':
   body=body.replace('for (;;) {','for (unsigned native_state_steps=0; ;native_state_steps++) {\n\t\tif(native_state_steps>=256||state>=ARRAY_SIZE(dpk_state)||native_error(rtwdev)){native_fail(rtwdev,ETIMEDOUT);break;}')
  if 'rtwdev' in body.split('{',1)[0] and ('restore' in name or 'reload' in name):
   signature=body.split('{',1)[0];args=signature[signature.index('(')+1:signature.rindex(')')];names=[]
   for arg in args.split(','):
    names.append(re.search(r'(\w+)\s*(?:\[[^\]]*\]\s*)*$',arg.strip()).group(1))
   call=name+'_restore_impl('+','.join(names)+')';impl=body.replace(name+'(',name+'_restore_impl(',1)
   wrapper=signature+'{++rtwdev->restore_depth;'
   if signature.split(name,1)[0].strip().endswith('void'):wrapper+=call+';--rtwdev->restore_depth;}'
   else:wrapper+='__typeof__('+call+') native_result='+call+';--rtwdev->restore_depth;return native_result;}'
   body=impl+'\n'+wrapper
  out.append(body)
 (root/('src/RTW88PHYOperating'+label+'.inc')).write_text('\n\n'.join(out)+'\n')
# RFE-specific factory offset/regulatory limit and DPK register triples.
out=['/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */','/* Copyright(c)2018-2019 Realtek Corporation. Exact pinned power/DPK tables. */']
for chip in ['8821c','8822b','8822c']:
 s=sources['rtw'+chip+'_table.c']
 for m in re.finditer(r'static const (struct rtw_phy_pg_cfg_pair|struct rtw_txpwr_lmt_cfg_pair|u32) (rtw'+chip+r'_(?:bb_pg_type\w+|txpwr_lmt_type\w+|dpk_afe_\w+|dpk_mac_bb))\[\]\s*=\s*\{.*?\n\};',s,re.S):
  out.append(m.group(0));kind,name=m.group(1),m.group(2);parser='rtw_parse_tbl_bb_pg' if 'phy_pg' in kind else 'rtw_parse_tbl_txpwr_lmt' if 'txpwr' in kind else 'rtw8822c_parse_tbl_dpk';out.append('static const struct rtw_table '+name+'_tbl={'+name+',ARRAY_SIZE('+name+'),'+parser+'};')
(root/'src/RTW88PHYOperatingPower.inc').write_text('\n\n'.join(out)+'\n')
generated={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted((root/'src').glob('RTW88PHYOperating*.inc'))}
meta['generated_operating_phy']={
 'generator':'scripts/generate-rtw88-operating.py',
 'functions':{n:{'source':funcs[n][0],'sha256':hashlib.sha256(funcs[n][1].encode()).hexdigest()} for n in sorted(selected)},
 'referenced_object_count':len(needed),'generated_sha256':generated,
 'checked_adaptations':[
  'Checked USB/RF/coexistence IO,500000-operation and120-second per-stage budgets,original bounded hardware polls',
  'Native saved-register restore attempts followed by primary per-chip power-off on failure',
  'Fresh firmware IQK completion required; A/B stale RF_DTXLOK is cleared before request',
  '8822C RF backup indexing uses path*DACK_RF_8822C+i',
  '8822C ADC/DAC samples initialized and minimum valid sample/stability/residual criteria enforced',
  '8822C DPK AGC transition count/state bounds,nonzero GS coefficient and second DC-correlation failure checked',
  '8822C saved LTE grant and RFK-end restoration attempted after failed calibration',
  '8822C PSD logarithm promotes arithmetic to64 bits',
  'Factory signed TX-power differences retain sign until final0..chipmax clamp',
  'SAR offset retains32-bit sign through final factory-base addition; impossible active-rate caps revoke readiness',
  '21C/22B thermal AGC headroom uses the uncapped factory/regulatory lookup, preserving reduction across table and separate gain',
  'Fresh channel/power plans reset stale thermal gain and restore22B factory swing independently per physical path',
  'Explicit actual association input selects segmented firmware IQK; readiness never implies association',
  'Canonical firmware-context mailbox cursor is shared; physical OTP tail cache supplies actual trim values',
  'Power epoch and PHY table reload invalidate previous calibrated readiness',
  'Actual hardware reconfiguration and every TXPAUSE restoration require live3081 FW_READY(CFFF/C078); loss latches and revokes PHY/MAC/FW authority while pure readiness/per-frame rate stay USB-free',
  'Actual firmware antenna cap controls TRX/power/rate and required DPK paths,physical RF transport remains distinct',
  'Caller-owned long-packet sequence can be shared with general/PHYDM startup; mailbox cursor remains canonical'
 ],
 'thermal_average_reference':'https://github.com/torvalds/linux/blob/'+meta['commit']+'/include/linux/average.h',
 'physical_hardware_validation':False}
(root/'src/RTW88USBDataPath-Upstream.json').write_text(json.dumps(meta,indent=2)+'\n')
print(len(selected),'pinned functions;',len(needed),'referenced global objects')
