#!/usr/bin/env python3
"""Generate dedicated RTL8710BU register tables and primary rtl8xxxu algorithms.
All source inputs are checksum pinned; this never communicates with hardware.
"""
from pathlib import Path
import re,json,hashlib,argparse
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--out',type=Path,default=r/'src');a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
EXPECTED={
 "8710b.c":"8ae2d50c8c10cea46de6c94653c6d2c452e7383e632ae3acc7919fe9998e90b0",
 "8188f.c":"75e74ded038786130ef5bf78e17e530f18a45b8a1c415b83588a8654c8b6583c",
 "core.c":"eab41606f9e0a304012f082ff95979222d3e63780d649ed1d923938b7e97b1b1",
 "regs.h":"5d400b8adeae0f0f86ad3e7f5e2ea046c7e302cbc9a6b654c32909b753b21c11",
 "rtl8xxxu.h":"1979e54d2b5002a903c794751e31094aa414c32281459d109a1afd65178ef7f4",
}
sources={name:(a.source/name).read_text() for name in EXPECTED}
for name,digest in EXPECTED.items():
 if hashlib.sha256(sources[name].encode()).hexdigest()!=digest:raise SystemExit('Pinned upstream SHA256 mismatch: '+name)
notice='''/* SPDX-License-Identifier: GPL-2.0-only */
/* Derived from Linux rtl8xxxu at 0e135910deec43ffe97eca0b6af1f8f847285b59.
 * Copyright(c)2023 Bitterblue Smith,2014-2017 Jes Sorensen,
 * 2007-2011 Realtek Corporation. Retain RTL8710BU-LICENSE.txt and manifest. */
'''
def balanced(s,start,open='{',close='}'):
 depth=0
 for m in re.finditer(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|[{}()]',s[start:]):
  token=m.group()
  if token==open:depth+=1
  elif token==close:
   depth-=1
   if not depth:return start+m.end()
 raise ValueError(s[start:start+50])
def function(file,name):
 s=sources[file];m=re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?(?:int|bool|void|u8|u16|u32|s8)\s+'+name+r'\(',s);assert m,(file,name);start=m.start();brace=s.index('{',m.end());end=balanced(s,brace);return s[start:end]+'\n'
def array(file,name):
 s=sources[file];m=re.search(r'(?m)^(?:static )?const [^;\n]+\b'+name+r'\[',s);assert m,name
 brace=s.index('{',m.end());end=balanced(s,brace);text=s[m.start():s.index(';',end)+1]
 if not text.startswith('static '):text='static '+text
 return text+'\n'
# The following compile-time specialization removes all other chip branches;
# it does not replace any8710BU register operation with a simulated operation.
fops={k:v.strip() for k,v in re.findall(r'\.(\w+)\s*=\s*([^,\n]+)',sources['8710b.c'].split('struct rtl8xxxu_fileops rtl8710bu_fops =',1)[1])}
chip_names=sorted(set(re.findall(r'\bRTL(?:81|82|87)\w*\b',sources['core.c']+sources['8710b.c'])))
constants={name:i+1 for i,name in enumerate(chip_names)};constants['true']=1;constants['false']=0
for name,value in fops.items():
 if re.fullmatch(r'[A-Za-z_]\w*',value) and not value.isupper():constants[value]=1
constants['macpower']=0
# Parse one C statement, retaining arbitrary original blocks/loops. Only
# if/else whose whole predicate is known compile-time data is pruned.
def skip(s,pos):
 while pos<len(s) and s[pos].isspace():pos+=1
 return pos
def end_stmt(s,pos):
 pos=skip(s,pos)
 if s[pos]=='{':return balanced(s,pos)
 if re.match(r'if\s*\(',s[pos:]):
  op=s.index('(',pos);cl=balanced(s,op,'(',')');end=end_stmt(s,cl);q=skip(s,end)
  if re.match(r'else\b',s[q:]):return end_stmt(s,q+4)
  return end
 # Semicolon ignoring parentheses/strings; labels are handled by their body.
 level=0
 for m in re.finditer(r'"(?:\\.|[^"\\])*"|[();{}]',s[pos:]):
  if m.group()=='(':level+=1
  elif m.group()==')':level-=1
  elif m.group()==';' and level==0:return pos+m.end()
  elif m.group()=='{' and level==0:return balanced(s,pos+m.start())
 raise ValueError(s[pos:pos+100])
def known(expr):
 expr=expr.strip()
 if re.search(r'[^A-Za-z0-9_xX\s!<>=&|()+-]',expr):return None
 tokens=re.findall(r'\b[A-Za-z_]\w*\b',expr)
 if any(t not in constants for t in tokens):return None
 for t in sorted(set(tokens),key=len,reverse=True):expr=re.sub(r'\b'+t+r'\b',str(constants[t]),expr)
 expr=' '.join(expr.split());expr=expr.replace('&&',' and ').replace('||',' or ');expr=re.sub(r'!(?!=)',' not ',expr)
 try:return bool(eval(expr,{'__builtins__':{}},{}))
 except (SyntaxError,ValueError):return None
def prune(s):
 out='';pos=0
 while True:
  m=re.search(r'\bif\s*\(',s[pos:])
  if not m:return out+s[pos:]
  st=pos+m.start();op=s.index('(',st);cl=balanced(s,op,'(',')');then_start=skip(s,cl);then_end=end_stmt(s,then_start);q=skip(s,then_end);has_else=bool(re.match(r'else\b',s[q:]));else_start=skip(s,q+4) if has_else else then_end;else_end=end_stmt(s,else_start) if has_else else then_end
  decision=known(s[op+1:cl-1]);out+=s[pos:st]
  if decision is not None:
   chosen=s[then_start:then_end] if decision else s[else_start:else_end] if has_else else '{}'
   out+=prune(chosen)
  else:
   out+=s[st:then_start]+prune(s[then_start:then_end])
   if has_else:out+=s[then_end:else_start]+prune(s[else_start:else_end])
  pos=else_end
common=['rtl8xxxu_flush_fifo','rtl8xxxu_read_rfreg','rtl8xxxu_write_rfreg','rtl8xxxu_init_phy_regs','rtl8xxxu_init_rf_regs','rtl8xxxu_init_phy_rf','rtl8xxxu_save_regs','rtl8xxxu_restore_regs','rtl8xxxu_save_mac_regs','rtl8xxxu_restore_mac_regs','rtl8xxxu_path_adda_on','rtl8xxxu_fill_iqk_matrix_a','rtl8xxxu_gen2_simularity_compare','rtl8xxxu_auto_llt_table','rtl8xxxu_init_queue_reserved_page','rtl8xxxu_init_queue_priority','rtl8xxxu_set_retry','rtl8xxxu_set_spec_sifs','rtl8xxxu_init_mac','rtl8xxxu_init_phy_bb','rtl8xxxu_gen2_usb_quirks','rtl8xxxu_init_burst','rtl8xxxu_request_hw_feature','rtl8xxxu_dump_hw_feature','rtl8xxxu_start_firmware','rtl8xxxu_download_firmware','rtl8xxxu_init_device']
chip=['rtl8710b_indirect_read32','rtl8710b_indirect_write32','rtl8710b_read_syson_reg','rtl8710b_write_syson_reg','rtl8710bu_power_off','rtl8710b_revise_cck_tx_psf','rtl8710bu_config_channel','rtl8710bu_init_aggregation','rtl8710bu_init_statistics','rtl8710bu_init_phy_bb','rtl8710bu_init_phy_rf','rtl8710bu_iqk_path_a','rtl8710bu_rx_iqk_path_a','rtl8710bu_phy_iqcalibrate','rtl8710bu_phy_iq_calibrate','rtl8710b_emu_to_active','rtl8710bu_active_to_emu','rtl8710bu_active_to_lps','rtl8710bu_power_on','rtl8710b_reset_8051','rtl8710b_enable_rf','rtl8710b_disable_rf','rtl8710b_usb_quirks','rtl8710b_set_crystal_cap']
functions=[('core.c',n) for n in common]+[('8710b.c',n) for n in chip]+[('8188f.c',n) for n in ['rtl8188f_set_tx_power','rtl8188f_phy_lc_calibrate','rtl8188f_channel_to_group']]
processed=[]
for file,name in functions:
 s=function(file,name);s=re.sub(r'/\*[\s\S]*?\*/|//[^\n]*','',s)
 s=re.sub(r'struct device \*dev = &priv->udev->dev;','',s)
 s=re.sub(r'dev_(?:dbg|info|warn|err)\([\s\S]*?\);','(void)0;',s)
 s=s.replace('struct rtl8xxxu_priv','struct rtw_r10bu').replace('priv->rtl_chip','RTL8710B')
 s=re.sub(r'usb_sndbulkpipe\(priv->udev, (priv->out_ep\[[^]]+\])\)',r'\1',s)
 s=re.sub(r'(?:priv->)?fops->(\w+)',lambda m:fops.get(m[1],'0'),s)
 s=re.sub(r'struct rtl8xxxu_fileops \*fops = priv->fops;','',s)
 s=re.sub(r'mutex_(?:lock|unlock)\([^;]+;','',s)
 if name=='rtl8xxxu_init_device':
  s=s.replace('bool macpower;','').replace('macpower = false;','(void)0;').replace('macpower = true;','(void)0;').replace('macpower','false')
  s=re.sub(r'set_bit\([^;]+;','',s)
 s=prune(s)
 s=re.sub(r'if \(rtl8xxxu_debug[^)]*\)\s*\(void\)0;','',s)
 if name=='rtl8710bu_power_off':
  for callee in ['rtl8xxxu_flush_fifo', 'rtl8710bu_active_to_lps', 'rtl8710bu_active_to_emu']:s=s.replace(callee+'(priv);','shutdown_result(priv,'+callee+'(priv));')
 if name=='rtl8710bu_active_to_lps':s=s.replace('if (!retry)','if (val32 != 0)')
 if name=='rtl8xxxu_download_firmware':s=s.replace('u8 *fwptr;','const u8 *fwptr;')
 if name in ['rtl8710b_indirect_read32','rtl8710b_indirect_write32']:
  s=s.replace('if (polling_count == 0)\n\t\t(void)0;', 'if (polling_count == 0 && !priv->error) priv->error=ETIMEDOUT;')
 if name=='rtl8188f_phy_lc_calibrate':
  s=s.replace('u32 val32;','u32 val32;\n u8 saved_pause=rtl8xxxu_read8(priv,REG_TXPAUSE);').replace('if (i == 100)\n\t\t(void)0;','if (i == 100) priv->cal_timeout=true;').replace('rtl8xxxu_write8(priv, REG_TXPAUSE, 0x00);','rtl8xxxu_write8(priv, REG_TXPAUSE, saved_pause);')
 if name=='rtl8710bu_phy_iqcalibrate':
  s=s.replace('int retry = 2;','u32 retry = 2;')
  s=s.replace('result[t][0] = (val32 >> 16) & 0x3ff;','result[t][0] = (val32 >> 16) & 0x3ff;if(result[t][0])priv->measured_tx[t]|=1;')
  s=s.replace('result[t][2] = (val32 >> 16) & 0x3ff;','result[t][2] = (val32 >> 16) & 0x3ff;if(result[t][2])priv->measured_rx[t]|=1;')
 if name=='rtl8710bu_phy_iq_calibrate':
  s=re.sub(r'reg_(?:e9c|eac) = result\[candidate\]\[\d\];','',s)
  s=s.replace('candidate = -1;','candidate = -1;\n memset(priv->measured_tx,0,sizeof(priv->measured_tx));memset(priv->measured_rx,0,sizeof(priv->measured_rx));',1)
  at=s.index('rtl8xxxu_save_regs(priv, rtl8xxxu_iqk_phy_iq_bb_reg')
  evidence='if(candidate>=0&&candidate<3){priv->tx_iqk_mask=priv->measured_tx[candidate];priv->rx_iqk_mask=priv->measured_rx[candidate];}else if(candidate==3){priv->tx_iqk_mask=priv->measured_tx[0]&priv->measured_tx[1]&priv->measured_tx[2];priv->rx_iqk_mask=priv->measured_rx[0]&priv->measured_rx[1]&priv->measured_rx[2];}\n'
  s=s[:at]+evidence+s[at:]
 if name=='rtl8710bu_power_on':
  at=s.rfind('return ret;');s=s[:at]+'rtl8xxxu_write8(priv,REG_TXPAUSE,0xff);'+s[at:]
 if name=='rtl8xxxu_dump_hw_feature':
  assert fops['hw_feature_report']=='0'
  branch=re.search(r'if \(!priv->fops->hw_feature_report\) \{([^}]+)\}',sources['core.c']).group(1)
  s=s[:s.index('{')+1]+'\n struct rtl8xxxu_hw_feature *hw_feature=&priv->hw_feature;'+branch+'\n}'
 for n in ['tx0_c','tx1_c']:s=s.replace('((('+n+' & 0x3c0) >> 6) << 28)','((u32)(('+n+' & 0x3c0) >> 6) << 28)')
 def unused(m):
  names=m[3].split(',')
  if any(not re.fullmatch(r'\s*\w+\s*',n) for n in names):return m[0]
  used=[n.strip() for n in names if len(re.findall(r'\b'+re.escape(n.strip())+r'\b',s))>1]
  return m[1]+m[2]+' '+', '.join(used)+';' if used else ''
 s=re.sub(r'(?m)^(\s*)(u8|u16|u32|s8|s32|int|bool) ([^;=\n]+);',unused,s)
 s=re.sub(r'^(?:static\s+)?','static ',s,count=1);processed.append((name,s))
prototypes='\n'.join(s[:s.index('{')].strip()+';' for _,s in processed)
tables=''.join(array('8710b.c',n) for n in ['rtl8710b_mac_init_table','rtl8710bu_qfn48m_u_phy_init_table','rtl8710bu_qfn48m_s_phy_init_table','rtl8710b_agc_table','rtl8710bu_qfn48m_u_radioa_init_table','rtl8710bu_qfn48m_s_radioa_init_table'])+array('core.c','rtl8xxxu_rfregs')+array('core.c','rtl8xxxu_iqk_phy_iq_bb_reg')
(a.out/'RTL8710BUTables.inc').write_text(notice+tables)
(a.out/'RTL8710BUAlgorithms.inc').write_text(notice+prototypes+'\n\n'+'\n'.join(s for _,s in processed))
(a.out/'RTL8710BURegisters.inc').write_text(notice+sources['regs.h'])
h=sources['rtl8xxxu.h'].replace('\\\n',' ');definitions=[]
for line in h.splitlines():
 if re.match(r'^#define\s+(?:TXDESC_QUEUE_.*|RTL8XXXU_(?:MAX_REG_POLL|FIRMWARE_POLL_MAX|ADDA_REGS|MAC_REGS|BB_REGS)|TX_(?:TOTAL_PAGE_NUM|PAGE_NUM).*|RTL_FW_PAGE_SIZE|EFUSE_UNDEFINED|C2H_HW_FEATURE_|NAV_UPPER_UNIT)\b',line):definitions.append(line)
definitions+=re.findall(r'(?m)^#define\s+(?:XTAL[01]|MAX_TOLERANCE|PACKAGE_QFN48M_[US]|EEPROM_PACKAGE_TYPE_8710B)\s+[^\n]+',sources['8710b.c']+sources['core.c'])
definitions+=['#define '+n+' '+str(v) for n,v in constants.items() if n not in ['true','false','macpower'] and n not in fops.values()]
(a.out/'RTL8710BUConstants.inc').write_text(notice+'\n'.join(definitions)+'\n')
manifest={'upstream':'torvalds/linux','commit':'0e135910deec43ffe97eca0b6af1f8f847285b59','selected_license':'GPL-2.0-only','sources':EXPECTED,'functions':[n for _,n in functions],'adaptations':['8710BU-only static specialization and dedicated opaque context','Imported register tables have translation-unit-private linkage','Source low-register8000 aperture and indirect SYSON/OTP preserved','Bounded synchronous USB/wait callbacks, sticky failure fencing','Independent SMIC/UMC firmware tag and package BB/RF table selection','Actual measured IQK masks exclude source fallback matrices; LCK timeout blocks TX','Host sequence, software CCMP and net80211 controlled port stay authoritative']}
(a.out/'RTL8710BU-Upstream.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Generated dedicated8710BU tables and',len(functions),'pinned primary algorithms')
