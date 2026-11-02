#!/usr/bin/env python3
"""Generate dedicated FU register tables and primary rtl8xxxu algorithms.
All source inputs are checksum pinned; this never communicates with hardware.
"""
from pathlib import Path
import re,json,hashlib,argparse
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);a=p.parse_args()
EXPECTED={
 "8192f.c":"4bb14d679bd35228c9ab30e043881278c4a6aa7762a04eeffb896d32de68e710",
 "8188f.c":"75e74ded038786130ef5bf78e17e530f18a45b8a1c415b83588a8654c8b6583c",
 "core.c":"eab41606f9e0a304012f082ff95979222d3e63780d649ed1d923938b7e97b1b1",
 "regs.h":"5d400b8adeae0f0f86ad3e7f5e2ea046c7e302cbc9a6b654c32909b753b21c11",
 "rtl8xxxu.h":"1979e54d2b5002a903c794751e31094aa414c32281459d109a1afd65178ef7f4",
}
sources={name:(a.source/name).read_text() for name in EXPECTED}
for name,digest in EXPECTED.items():
 if hashlib.sha256(sources[name].encode()).hexdigest()!=digest:raise SystemExit('Pinned upstream SHA256 mismatch: '+name)
notice='''/* SPDX-License-Identifier: GPL-2.0-only */
/* Derived from Linux rtl8xxxu at f47d223627f80cb7a1eb14c585fe6cb35de5e984.
 * Copyright(c)2023 Bitterblue Smith,2014-2017 Jes Sorensen,
 * 2007-2011 Realtek Corporation. Retain RTL8192FU-LICENSE.txt and manifest. */
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
 s=sources[file];m=re.search(r'(?m)^(?:static )?const [^;\n]+\b'+name+r'\[',s);assert m,name;brace=s.index('{',m.end());end=balanced(s,brace);return s[m.start():s.index(';',end)+1]+'\n'
# The following compile-time specialization removes all other chip branches;
# it does not replace any FU register operation with a simulated operation.
fops={k:v.strip() for k,v in re.findall(r'\.(\w+)\s*=\s*([^,\n]+)',sources['8192f.c'].split('struct rtl8xxxu_fileops rtl8192fu_fops =',1)[1])}
chip_names=sorted(set(re.findall(r'\bRTL(?:81|82|87)\w*\b',sources['core.c']+sources['8192f.c'])))
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
common=['rtl8xxxu_flush_fifo','rtl8xxxu_firmware_self_reset','rtl8xxxu_read_rfreg','rtl8xxxu_write_rfreg','rtl8xxxu_write_rfreg_mask','rtl8xxxu_init_phy_regs','rtl8xxxu_init_rf_regs','rtl8xxxu_init_phy_rf','rtl8xxxu_save_regs','rtl8xxxu_restore_regs','rtl8xxxu_save_mac_regs','rtl8xxxu_restore_mac_regs','rtl8xxxu_fill_iqk_matrix_a','rtl8xxxu_fill_iqk_matrix_b','rtl8xxxu_gen2_simularity_compare','rtl8xxxu_auto_llt_table','rtl8xxxu_init_queue_reserved_page','rtl8xxxu_init_queue_priority','rtl8xxxu_set_retry','rtl8xxxu_set_spec_sifs','rtl8xxxu_init_mac','rtl8xxxu_init_phy_bb','rtl8xxxu_gen2_usb_quirks','rtl8xxxu_init_burst','rtl8xxxu_request_hw_feature','rtl8xxxu_dump_hw_feature','rtl8xxxu_start_firmware','rtl8xxxu_download_firmware','rtl8xxxu_init_device']
chip=['rtl8192f_set_tx_power','rtl8192fu_power_off','rtl8192f_revise_cck_tx_psf','rtl8192fu_config_kfree','rtl8192fu_config_channel','rtl8192fu_init_aggregation','rtl8192fu_init_phy_bb','rtl8192fu_init_phy_rf','rtl8192f_phy_lc_calibrate','rtl8192fu_iqk_path_a','rtl8192fu_rx_iqk_path_a','rtl8192fu_iqk_path_b','rtl8192fu_rx_iqk_path_b','rtl8192fu_phy_iqcalibrate','rtl8192fu_phy_iq_calibrate','rtl8192fu_disabled_to_emu','rtl8192fu_emu_to_active','rtl8192fu_active_to_emu','rtl8192fu_emu_to_disabled','rtl8192fu_active_to_lps','rtl8192fu_power_on','rtl8192f_reset_8051','rtl8192f_enable_rf','rtl8192f_disable_rf','rtl8192f_usb_quirks','rtl8192f_set_crystal_cap']
functions=[('core.c',n) for n in common]+[('8192f.c',n) for n in chip]+[('8188f.c','rtl8188f_phy_lc_calibrate'),('8188f.c','rtl8188f_channel_to_group')]
processed=[]
for file,name in functions:
 s=function(file,name);s=re.sub(r'/\*[\s\S]*?\*/|//[^\n]*','',s)
 s=re.sub(r'struct device \*dev = &priv->udev->dev;','',s)
 s=re.sub(r'dev_(?:dbg|info|warn|err)\([\s\S]*?\);','(void)0;',s)
 s=s.replace('struct rtl8xxxu_priv','struct rtw_r92fu');s=s.replace('priv->rtl_chip','RTL8192F');s=re.sub(r'usb_sndbulkpipe\(priv->udev, (priv->out_ep\[[^]]+\])\)',r'\1',s)
 s=re.sub(r'(?:priv->)?fops->(\w+)',lambda m:fops.get(m[1],'0'),s)
 s=re.sub(r'struct rtl8xxxu_fileops \*fops = priv->fops;','',s)
 if name=='rtl8xxxu_init_device':
  s=s.replace('bool macpower;','').replace('macpower = false;','(void)0;').replace('macpower = true;','(void)0;').replace('macpower','false')
  # Protocol bookkeeping remains owned by net80211, not a fake mac80211 map.
  s=re.sub(r'set_bit\([^;]+;','',s)
 s=prune(s)
 if name=='rtl8xxxu_read_rfreg' or name=='rtl8xxxu_write_rfreg':
  s=re.sub(r'if \(rtl8xxxu_debug[^)]*\)\s*\(void\)0;','',s)
 if name=='rtl8xxxu_dump_hw_feature':
  s=re.sub(r'ret = read_poll_timeout\([\s\S]*?REG_C2HEVT_MSG_NORMAL\);','ret = fu_poll_feature(priv);',s)
 if name=='rtl8192fu_power_off':
  for callee in ['rtl8xxxu_flush_fifo', 'rtl8192fu_active_to_lps', 'rtl8192fu_active_to_emu', 'rtl8192fu_emu_to_disabled']:s=s.replace(callee+'(priv);','shutdown_result(priv,'+callee+'(priv));')
 if name=='rtl8192fu_active_to_lps':s=s.replace('if (!retry)','if (val32 != 0)')
 if name=='rtl8xxxu_download_firmware':s=s.replace('u8 *fwptr;','const u8 *fwptr;')
 if name=='rtl8188f_phy_lc_calibrate':
  s=s.replace('u32 val32;','u32 val32;\n u8 saved_pause=rtl8xxxu_read8(priv,REG_TXPAUSE);').replace('if (i == 100)\n\t\t(void)0;','if (i == 100) priv->cal_timeout=true;').replace('rtl8xxxu_write8(priv, REG_TXPAUSE, 0x00);','rtl8xxxu_write8(priv, REG_TXPAUSE, saved_pause);')
 if name in ['rtl8192fu_iqk_path_a','rtl8192fu_rx_iqk_path_a','rtl8192fu_iqk_path_b','rtl8192fu_rx_iqk_path_b']:
  opening=s.index('{');s=s[:opening+1]+'\n bool measured_ready=true;'+s[opening+1:]
  pattern=r'while\s*\(rtl8xxxu_read32\(priv,\s*(REG_IQK_RPT_\w+)\) == 0 && ktime < 21\)\s*\{'
  offset=0
  while True:
   match=re.search(pattern,s[offset:])
   if not match:break
   brace=s.index('{',offset+match.start());end=balanced(s,brace);insert='\n measured_ready=measured_ready&&(rtl8xxxu_read32(priv,'+match[1]+')!=0);';s=s[:end]+insert+s[end:];offset=end+len(insert)
  s=s.replace('return result;','return measured_ready?result:0;')
 if name=='rtl8192fu_phy_iqcalibrate':
  for prefix,idx,mask in [('path_a',0,1),('path_b',4,2)]:
   tx=prefix+'_ok == 0x01';rx=prefix+'_ok == 0x03'
   tx_result='result[t]['+str(idx)+'] = (val32 >> 16) & 0x3ff;';rx_result='result[t]['+str(idx+2)+'] = (val32 >> 16) & 0x3ff;'
   s=s.replace(tx_result,tx_result+'if(result[t]['+str(idx)+'])priv->measured_tx[t]|='+str(mask)+';').replace(rx_result,rx_result+'if(result[t]['+str(idx+2)+'])priv->measured_rx[t]|='+str(mask)+';')
 if name=='rtl8192fu_phy_iq_calibrate':
  s=re.sub(r'reg_(?:e9c|eac|ebc|ecc) = result\[candidate\]\[\d\];','',s)
  s=s.replace('int result[4][8];','int result[4][8];').replace('candidate = -1;','candidate = -1;\n memset(priv->measured_tx,0,sizeof(priv->measured_tx));memset(priv->measured_rx,0,sizeof(priv->measured_rx));',1)
  insert='''if(candidate>=0&&candidate<3){priv->tx_iqk_mask=priv->measured_tx[candidate];priv->rx_iqk_mask=priv->measured_rx[candidate];}
 else if(candidate==3){priv->tx_iqk_mask=priv->measured_tx[0]&priv->measured_tx[1]&priv->measured_tx[2];priv->rx_iqk_mask=priv->measured_rx[0]&priv->measured_rx[1]&priv->measured_rx[2];}\n'''
  at=s.index('rtl8xxxu_write32_mask(priv, REG_TX_IQK_TONE_A');s=s[:at]+insert+s[at:]
 if name=='rtl8192fu_power_on':s=s.replace('return 0;','rtl8xxxu_write8(priv,REG_TXPAUSE,0xff);return 0;')
 for n in ['tx0_c','tx1_c']:s=s.replace('((('+n+' & 0x3c0) >> 6) << 28)','((u32)(('+n+' & 0x3c0) >> 6) << 28)')
 # Drop purely unreferenced scalar declarations after static specialization.
 def unused(m):
  names=m[3].split(',')
  if any(not re.fullmatch(r'\s*\w+\s*',n) for n in names):return m[0]
  used=[n.strip() for n in names if len(re.findall(r'\b'+re.escape(n.strip())+r'\b',s))>1]
  return m[1]+m[2]+' '+', '.join(used)+';' if used else ''
 s=re.sub(r'(?m)^(\s*)(u8|u16|u32|s8|s32|int|bool) ([^;=\n]+);',unused,s)
 if name=='rtl8192fu_phy_iqcalibrate':s=s.replace('int retry = 2;','u32 retry = 2;')

 # All native helpers are private and keep their actual argument/register ABI.
 s=re.sub(r'^(?:static\s+)?','static ',s,count=1);processed.append((name,s))
prototypes='\n'.join(s[:s.index('{')].strip()+';' for _,s in processed)
# Keep exact wire/PHY tables. Normal/Bluetooth/AP tables are never substituted.
tables=''.join(array('8192f.c',n) for n in ['rtl8192f_mac_init_table','rtl8192fu_phy_init_table','rtl8192f_agc_table','rtl8192fu_radioa_init_table','rtl8192fu_radiob_init_table'])+array('core.c','rtl8xxxu_rfregs')
(r/'src/RTL8192FUTables.inc').write_text(notice+tables)
(r/'src/RTL8192FUAlgorithms.inc').write_text(notice+prototypes+'\n\n'+'\n'.join(s for _,s in processed))
(r/'src/RTL8192FURegisters.inc').write_text(notice+sources['regs.h'])
# Public Linux constants whose values are outside regs.h.
h=sources['rtl8xxxu.h'].replace('\\\n',' ');definitions=[]
for line in h.splitlines():
 if re.match(r'^#define\s+(?:TXDESC_QUEUE_.*|RTL8XXXU_(?:MAX_REG_POLL|FIRMWARE_POLL_MAX|ADDA_REGS|MAC_REGS|BB_REGS)|TX_(?:TOTAL_PAGE_NUM|PAGE_NUM).*|RTL_FW_PAGE_SIZE|EFUSE_UNDEFINED|C2H_HW_FEATURE_|NAV_UPPER_UNIT)\b',line):definitions.append(line)
definitions+=re.findall(r'(?m)^#define\s+(?:XTAL[01]|MAX_TOLERANCE)\s+[^\n]+',sources['8192f.c']+sources['core.c'])
definitions+=['#define '+n+' '+str(v) for n,v in constants.items() if n not in ['true','false','macpower'] and n not in fops.values()]
(r/'src/RTL8192FUConstants.inc').write_text(notice+'\n'.join(definitions)+'\n')
manifest={'commit':'f47d223627f80cb7a1eb14c585fe6cb35de5e984','selected_license':'GPL-2.0-only','sources':EXPECTED,'functions':[n for _,n in functions],'adaptations':['FU-only static specialization, host protocol bookkeeping excluded','bounded transport and waiter replace Linux USB/sleeps; sticky errors retained','real measured IQK masks exclude upstream fallback matrices','LCK timeout fails readiness and preserves TX pause','TX remains paused through boot and calibration','signed-bit packing UB fixed without changing hardware fields','immutable caller firmware and FU-specific 0x4000 download window']}
manifest['secondary_primary_checks']={'hal/rtl8192f/rtl8192f_hal_init.c': {'source': 'https://raw.githubusercontent.com/kelebek333/rtl8192fu/master/hal/rtl8192f/rtl8192f_hal_init.c', 'source_ref': 'master', 'sha256': 'ee7f0e391794a5a9001cef81461c399936b0398e4aca650b9ecdf93744cb7e2d'}, 'hal/phydm/halrf/halrf.h': {'source': 'https://raw.githubusercontent.com/kelebek333/rtl8192fu/master/hal/phydm/halrf/halrf.h', 'source_ref': 'master', 'sha256': 'bb88243be7fbe85abf3fb8425931bae4a5650e23a9007590698895a83457228c'}, 'hal/phydm/halrf/rtl8192f/halrf_8192f.c': {'source': 'https://raw.githubusercontent.com/kelebek333/rtl8192fu/master/hal/phydm/halrf/rtl8192f/halrf_8192f.c', 'source_ref': 'master', 'sha256': '3199ebdd6c353e06b60966f5e18112550accc70563873459103ee2f54f5a5d1e'}, 'hal/phydm/halrf/rtl8192f/halrf_8192f.h': {'source': 'https://raw.githubusercontent.com/kelebek333/rtl8192fu/master/hal/phydm/halrf/rtl8192f/halrf_8192f.h', 'source_ref': 'master', 'sha256': 'a1c6f61b06272290d394d0d7a09e54b10d265897347ad98fd1af527b17a263c3'}}
manifest['secondary_primary_checks']['include/rtl8192f_cmd.h']={'source': 'https://raw.githubusercontent.com/kelebek333/rtl8192fu/master/include/rtl8192f_cmd.h', 'source_ref': 'master', 'sha256': 'e658b7083f7352fb47738232895640ba0321cf5e8baa329faad250ea6a664db4'}
manifest['adaptations']+=['FU txgi_max63 verified against primary Realtek hal_init; strict ROM arithmetic preflight','IQK report readiness and nonzero measured compensation coefficients required after every bounded poll','Native boot retunes actual table channel7 to declared channel1 and measures again','Native interface writes its own MACID and no-link state before receiving station traffic']
manifest['adaptations']+=['First crystal programming is forced, including valid ROM zero','FU H2C reserved RA bits remain clear; media reports remote AP role2','Station-port0 uses MACID0 for unicast and broadcast','FU v2 RTS/CTS sets hardware RTS and protects operational AMPDU']
(r/'src/RTL8192FU-Upstream.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Generated dedicated FU tables and',len(functions),'primary algorithms')
