#!/usr/bin/env python3
"""Pinned B/C coexistence callbacks/tables; generic policy remains shared.

No hardware operations. --check verifies every input and generated output.
"""
from pathlib import Path
import argparse,hashlib,json,re
r=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--check',action='store_true');a=p.parse_args()
expected={
 'coex.c':'f0ecf9a2d2f121fc7dab32dcf3ced250f388175d8251dd398df6298e3e412d41',
 'coex.h':'047aa0d252aee946869be49960129a52b5132528563ed52d9355e2b9870b3800',
 'fw.c':'0da28129dfcdf3e9b5661523b54ede4632c6c9e5af7008a6f7ce2a1f8b6a19f0',
 'fw.h':'45a1bf5213eec95cd450a54bc50caa5652c959dd2b644684ec7447b278f6e701',
 'main.h':'6f2eaeb830844efface8a676815e2a0b15f59aa5408387fa85dc91613d351a24',
 'reg.h':'c8246f9305539347fdb62b0294fe12ea24769cb3ad56c2b0235c4269a0307324',
 'rtw8821c.c':'cdfe1d8670c42dadb6aa12a10137aa83c17c6bc072a1ce2b78fccf42b9ed5658',
 'rtw8822b.c':'fb51b70730020fcc77d482ee061c7fba87f4676ca3fc38f1c6c5c45d39a1e113',
 'rtw8822c.c':'a12e2b2ddde31ce8e0242d3e5b29769c364950a72b161fdcbd3905ec145d4a83'}
# Header hashes belong to the existing checked B/C PHY manifest.
phy=json.loads((r/'src/RTW88USBDataPath-Upstream.json').read_text())
phy_sources=phy.get('sources',{})
if not phy_sources:phy_sources={v['path']:v['sha256'] for v in phy.get('files',[])}
for name in ['rtw8821c.h','rtw8822b.h','rtw8822c.h','main.c','tx.c','tx.h']:
 if name in phy_sources:expected[name]=phy_sources[name]
 else:raise SystemExit('Missing pinned PHY header: '+name)
files={n:(a.source/n).read_text() for n in expected}
for n,sha in expected.items():
 if hashlib.sha256(files[n].encode()).hexdigest()!=sha:raise SystemExit('Unpinned source: '+n)
notice='/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause\n * Copyright(c)2018-2019 Realtek Corporation. Generated from pinned Linux.\n * This port selects BSD-3-Clause; see RTW88Coex-Upstream.json. */\n'
def end_brace(text,at):
 depth=0
 for m in re.finditer(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|[{}]',text[at:],re.S):
  if m[0]=='{':depth+=1
  elif m[0]=='}':
   depth-=1
   if not depth:return at+m.end()
 raise ValueError('Unbalanced source')
def function(text,name):
 m=re.search(r'(?m)^(?:static )?(?:inline )?(?:const )?[\w\s*]+\b'+name+r'\([^;]*?\)\s*\{',text);assert m,name
 return text[m.start():end_brace(text,text.index('{',m.start()))]+'\n'
def portable(text):
 text=re.sub(r'rtw_dbg\(.*?\);','(void)0;',text,flags=re.S)
 # u8 promotion is nonnegative0..65535; match the u32 saved switch field.
 text=text.replace('((ctrl_type << 8) + pos_type)','(((u32)ctrl_type << 8) + pos_type)')
 def unused(m):
  names=[re.search(r'(\w+)\s*$',v).group(1) for v in m[2].split(',') if v.strip() and v.strip()!='void']
  return m[0]+''.join('\n\t(void)'+n+';' for n in names)
 return re.sub(r'(?m)^([A-Za-z][\w \t*]*\b\w+)\(([^;{}]*?)\)\s*\{',unused,text)
def declaration(text,name):
 m=re.search(r'(?m)^static const [^;=]*\b'+name+r'(?:\[\])?\s*=\s*\{',text);assert m,name
 end=end_brace(text,text.index('{',m.start()));assert text[end]==';'
 return text[m.start():end+1]+'\n'
outputs={};callbacks=[];tables=[];names=[]
for chip in ['8821c','8822b','8822c']:
 text=files['rtw'+chip+'.c'];functions=['rtw'+chip+'_coex_cfg_'+v for v in ['init','gnt_fix','gnt_debug','rfe_type','wl_tx_power','wl_rx_gain']]
 if chip!='8822c':functions.insert(1,'rtw'+chip+'_coex_cfg_ant_switch')
 functions.append('rtw'+chip+'_false_alarm_statistics')
 names+=functions;callbacks.append('\n'.join(portable(function(text,n)) for n in functions))
 pointers=['rtw'+chip+'_coex_cfg_'+v for v in ['init','ant_switch','gnt_fix','gnt_debug','rfe_type','wl_tx_power','wl_rx_gain']]
 if chip=='8822c':pointers[1]='NULL'
 callbacks.append('static const struct rtw_chip_ops cx_ops_'+chip+'={'+','.join(pointers)+'};\n')
 for array in ['wl_rssi_step','bt_rssi_step','table_sant','table_nsant','tdma_sant','tdma_nsant','rf_para_tx','rf_para_rx','afh_5g']:
  tables.append(declaration(text,array+'_'+chip))
 tables.append(declaration(text,'rtw'+chip+'_ltecoex_addr'))
 at=text.index('\t.coex_para_ver =');end=text.index('\n\t.coex_info_hw_regs_num =',at)
 tables.append('static const struct rtw_chip_info cx_chip_'+chip+'={.ops=&cx_ops_'+chip+',.id=RTW_CHIP_TYPE_'+chip.upper()+',.ltecoex_addr=&rtw'+chip+'_ltecoex_addr,\n'+text[at:end]+'\n};\n')
outputs['src/RTW88CoexCallbacks.inc']=notice+'\n'.join(callbacks)
outputs['src/RTW88CoexTables.inc']=notice+'\n'.join(tables)
defines=[];seen=set()
for name in ['reg.h','rtw8821c.h','rtw8822b.h','rtw8822c.h']:
 for m in re.finditer(r'(?m)^#define\s+(\w+)(?:[^\n]*\\\n)*[^\n]*',files[name]):
  macro=m[1]
  if macro in seen:continue
  seen.add(macro);defines.append('#ifndef '+macro+'\n'+m[0]+'\n#endif\n')
outputs['src/RTW88CoexRegisters.inc']=notice+'\n'.join(defines)
manifest={'upstream':'torvalds/linux','commit':'0e135910deec43ffe97eca0b6af1f8f847285b59','selected_license':'BSD-3-Clause','sources':expected,'callbacks':names,'shared_policy':['RTL8821AUCoexTypes.inc','RTL8821AUCoexWrappers.inc','RTL8821AUCoexPolicy.inc'],'shared_policy_provenance':'RTL8821AUCoex-Upstream.json','adaptations':['Exact B/C callbacks and all shared/separate antenna TDMA/PTA/RF/RSSI/AFH tables.','Generic coex.c/coex.h/fw.c/fw.h bytes match the AU shared extraction at f47d223627f80cb7a1eb14c585fe6cb35de5e984.','Logging and unused-parameter warnings removed; no hardware decision or table changed.','Checked B/C PHY/FW context and canonical firmware.h2c_box; no AU hardware context.','Station timer retains primary deadlines; BT MP replies resume the completed request without resampling old BT-info.','Retained board/bus/PHY/epoch binding guards every I/O entry and the pure station TX gate; stop rechecks live MCU authority.','Kernel MLME/EDCA/rollback/stop preflight retained bootstrap authority and live MCU before hardware commands; invalid ownership permits host-only key/port/timer cleanup through the final power gate. The same pure ownership check gates fast TX for BT and no-BT boards without USB.','8822C uses the actual config_trx_mode API, firmware RA update and AP HT SMPS action; only matching CCX ACK commits local SMPS.','NACK or missing500ms CCX report fails closed through normal station quiesce, an explicit bounded port adaptation.',"Management frames retain this port's host sequence; CCX tokens are separate shifted6-bit tags with retired-tag protection."]}
manifest['adaptations'].append('Busy MP slots retain one latest desired LNA level; real C2H completion or bounded periodic timeout retries it after live authority checks. Cache the applied level only after successful H2C/scoreboard admission; stop/error/reset discards deferred work.')
outputs['src/RTW88Coex-Upstream.json']=json.dumps(manifest,indent=2)+'\n'
for name,text in outputs.items():
 path=r/name
 if a.check:
  if not path.exists() or path.read_text()!=text:raise SystemExit('Generated output differs: '+name)
 else:path.write_text(text)
print('Verified' if a.check else 'Generated',len(outputs),'B/C coex artifacts from',len(expected),'pinned primary files.')
