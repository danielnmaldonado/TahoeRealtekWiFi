#!/usr/bin/env python3
"""Generate the RTL8723BU portable source subset from a pinned rtl8xxxu tree."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
from host_cpp import compiler

ROOT=Path(__file__).resolve().parents[1]
COMMIT='0e135910deec43ffe97eca0b6af1f8f847285b59'
EXPECTED={'8723b.c': '06efb8be1f587922f545ed88d6ac3bc62aa8492b255a07dc11d850e709e0216d', '8723a.c': '240634718dd0b52d116bd705e413ba1a5a22e2830be70ee5e7f22fc69510c3cc', 'core.c': 'eab41606f9e0a304012f082ff95979222d3e63780d649ed1d923938b7e97b1b1', 'rtl8xxxu.h': '1979e54d2b5002a903c794751e31094aa414c32281459d109a1afd65178ef7f4', 'regs.h': '5d400b8adeae0f0f86ad3e7f5e2ea046c7e302cbc9a6b654c32909b753b21c11'}
NOTICE='''/* SPDX-License-Identifier: GPL-2.0-only */
/* Linux rtl8xxxu '''+COMMIT+'''.
 * Copyright(c)2014-2017 Jes Sorensen <Jes.Sorensen@gmail.com>.
 * Copyright(c)Realtek Corporation. Retain the accompanying provenance/license.
 * Generated register/table/algorithm subset, checked I/O supplied by the port. */
'''

def block(s,marker):
 start=s.index(marker);opening=s.index('{',start);depth=0
 for token in re.finditer(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|[{}]',s[opening:]):
  if token.group()=='{':depth+=1
  elif token.group()=='}':
   depth-=1
   if not depth:return s[start:opening+token.end()]
 raise ValueError(marker)

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--source',type=Path,required=True);args=parser.parse_args()
 files={name:(args.source/name).read_text() for name in ['8723b.c','8723a.c','core.c','rtl8xxxu.h','regs.h']}
 for name,expected in EXPECTED.items():
  if hashlib.sha256((args.source/name).read_bytes()).hexdigest()!=expected:raise ValueError('Wrong pinned primary source: '+name)
 header=files['rtl8xxxu.h'];chip=files['8723b.c'];core=files['core.c']
 defines_text='\n'.join(files.values())
 names=set(re.findall(r'^#define\s+(\w+)',defines_text,re.M))
 preprocessed=subprocess.check_output([*compiler('c'),'-x','c','-E','-dM','-D__LITTLE_ENDIAN=1','-'],input=re.sub(r'^#include[^\n]*','',defines_text,flags=re.M),text=True)
 defs=[line for line in preprocessed.splitlines() if re.match(r'#define\s+(\w+)',line).group(1) in names]
 (ROOT/'src/RTL8723BURegisters.inc').write_text(NOTICE+'\n'.join(defs)+'\n')
 structs=['rtl8723au_idx','rtl8723bu_pwr_idx','rtl8723bu_efuse_tx_power','rtl8723bu_efuse',
          'h2c_cmd','rtl8xxxu_cfo_tracking','rtl8xxxu_firmware_header','rtl8xxxu_btcoex','rtl8xxxu_reg8val','rtl8xxxu_reg32val','rtl8xxxu_rfregval','rtl8xxxu_rfregs']
 out=NOTICE
 for name in structs:
  text=block(header,'struct '+name+' {')
  out+=text+(' __attribute__((packed));\n' if name in ['rtl8723au_idx','rtl8723bu_pwr_idx','rtl8723bu_efuse','h2c_cmd','rtl8xxxu_cfo_tracking','rtl8xxxu_firmware_header'] else ';\n')
 for name in ['rtl8xxxu_rfpath','rtl8xxxu_rtl_chip','rtl8723bu_h2c','rtl8723bu_c2h_id','rtl8xxxu_bt_info_src','rtl8723b_1ant_bt_status','c2h_evt_8723b']:
  match=re.search(r'enum\s+'+name+r'\s*\{',header)
  if match:out+=block(header[match.start():],'enum')+';\n'
 # Enumerators are required under their actual primary enum names.
 for marker in ['H2C_8723B_MACID_CFG','BT_8723B_1ANT_STATUS_NON_CONNECTED_IDLE','BT_INFO_SRC_8723B_WIFI_FW','BT_MP_OP_WRITE_REG_VALUE','C2H_8723B_RA_REPORT']:
  pos=header.find(marker)
  if pos>=0:
   start=header.rfind('enum ',0,pos)
   enum=block(header[start:],'enum')
   if enum not in out:out+=enum+';\n'
 (ROOT/'src/RTL8723BUDefinitions.inc').write_text(out)
 out=NOTICE
 for name in ['rtl8723b_mac_init_table','rtl8723b_phy_1t_init_table','rtl8xxx_agc_8723bu_table','rtl8723bu_radioa_1t_init_table']:
  match=re.search(r'static const struct \w+\s+'+name+r'\[',chip)
  out+=block(chip[match.start():],'static const')+';\n'
 out+=block(core,'static const struct rtl8xxxu_rfregs rtl8xxxu_rfregs[]')+';\n'
 out+='static '+block(core,'const u32 rtl8xxxu_iqk_phy_iq_bb_reg[')+';\n'
 (ROOT/'src/RTL8723BUTables.inc').write_text(out)
 # Resolve the actual chip calibration/coexistence closure. Scalar register,
 # mailbox and delay transports are the only hand supplied platform boundary.
 functions={}
 for filename,text in files.items():
  if not filename.endswith('.c'):continue
  pattern=r'(?m)^(?:static\s+(?:inline\s+)?)?(?:void|bool|int|s8|u8|u16|u32|s32)\s+((?:rtl8xxxu|rtl8723bu|rtl8723b|rtl8723a)_\w+)\s*\([^;{}]*\)\s*\{'
  for match in re.finditer(pattern,text):functions[match.group(1)]=block(text[match.start():],match.group())
 roots=['rtl8723bu_reset_8051','rtl8723bu_power_on','rtl8723bu_power_off',
        'rtl8723b_set_tx_power','rtl8723bu_init_phy_bb','rtl8723bu_init_phy_rf',
        'rtl8723bu_phy_init_antenna_selection','rtl8723bu_phy_iq_calibrate',
        'rtl8723b_enable_rf','rtl8723bu_init_aggregation','rtl8723bu_init_statistics',
        'rtl8723a_phy_lc_calibrate','rtl8723a_set_crystal_cap',
        'rtl8xxxu_download_firmware','rtl8xxxu_start_firmware',
        'rtl8xxxu_auto_llt_table','rtl8xxxu_init_queue_reserved_page',
        'rtl8xxxu_init_queue_priority','rtl8xxxu_init_burst','rtl8xxxu_gen2_usb_quirks',
        'rtl8xxxu_gen2_config_channel','rtl8723bu_update_bt_link_info',
        'rtl8723bu_handle_bt_info','rtl8723bu_handle_bt_inquiry']
 platform={*['rtl8xxxu_'+name for name in ['read8','read16','read32','write8','write16','write32','writeN','gen2_h2c_cmd']], 'rtl8xxxu_is_assoc'}
 seen=set();ordered=[];missing=set()
 def visit(name):
  if name in seen or name in platform:return
  seen.add(name)
  if name not in functions:missing.add(name);return
  text=re.sub(r'#if 1\n(.*?)#else\n.*?#endif',r'\1',functions[name],flags=re.S)
  functions[name]=text
  for dep in re.findall(r'\b((?:rtl8xxxu|rtl8723bu|rtl8723b|rtl8723a)_\w+)\s*\(',re.sub(r'/\*[\s\S]*?\*/|//[^\n]*','',text)):
   if dep!=name:visit(dep)
  ordered.append(name)
 for name in roots:visit(name)
 if missing:raise ValueError('Missing actual primary helpers: '+', '.join(sorted(missing)))
 out=NOTICE
 for name in ordered:
  text=functions[name]
  text=re.sub(r'^static\s+(?:inline\s+)?','',text)
  text='static __attribute__((unused)) '+text
  text=text.replace('struct rtl8xxxu_priv','struct rtw_8723bu_private')
  text=text.replace('struct ieee80211_hw','struct rtw_8723bu_hw')
  text=text.replace('&priv->udev->dev','priv')
  text=text.replace('u8 *fwptr;', 'const u8 *fwptr;').replace('struct rtl8xxxu_fileops *fops', 'const struct rtl8xxxu_fileops *fops')
  text=re.sub(r'struct device \*dev = priv;',r'void *dev=priv;',text)
  text=re.sub(r'\budelay\(([^;]+)\)',r'r23_delay(priv,\1)',text)
  text=re.sub(r'\b(?:mdelay|msleep)\(([^;]+)\)',r'r23_delay(priv,1000U*(\1))',text)
  text=re.sub(r'\busleep_range\(([^,;]+),[^;]+\)',r'r23_delay(priv,\1)',text)
  text=text.replace('i < retry;', 'i < (u32)retry;')
  if name=='rtl8723bu_phy_iq_calibrate':
   text=text.replace('\n}', '\n priv->iqk_ok=candidate>=0&&reg_e94!=0&&reg_ea4!=0;\n (void)reg_eac;(void)reg_ecc;\n}')
  if name=='rtl8723bu_update_bt_link_info':
   # Primary typo overwrites the real A2DP profile instead of deriving the
   # a2dp_only flag, making the HID+A2DP coexistence branch unreachable.
   at=text.index('\n\tif (!btcoex->has_sco')
   text=text[:at]+text[at:].replace('btcoex->has_a2dp =','btcoex->a2dp_only =')
  if name=='rtl8723bu_power_off':
   for callee in ['rtl8xxxu_flush_fifo','rtl8xxxu_active_to_lps','rtl8723bu_active_to_emu']:text=text.replace(callee+'(priv);','shutdown_result(priv,'+callee+'(priv));')
  if name=='rtl8xxxu_active_to_lps':text=text.replace('u8 val32;','u32 val32;')
  if name=='rtl8xxxu_download_firmware':
   text=text.replace('fw_abort:\n','fw_abort:\n priv->cleanup=true;\n')
   text=text.replace('\n\treturn ret;','\n priv->cleanup=false;\n\treturn ret;')
  if name=='rtl8xxxu_init_queue_priority':
   # The register VOQ uses voq; its endpoint must follow that same queue,
   # not viq. Otherwise3-pipe VO descriptors go to the normal endpoint.
   text=text.replace('vop = viq ^ 3;', 'vop = voq ^ 3;')
  if name=='rtl8723a_phy_lc_calibrate':text=text.replace('REG_TXPAUSE, 0x00','REG_TXPAUSE, 0xff')
  if name=='rtl8723bu_phy_iqcalibrate':
   text=text.replace('priv->bb_backup, RTL8XXXU_BB_REGS);','priv->bb_backup, RTL8XXXU_BB_REGS);\n        if(priv->owner->last_error)return;',1)
  if name=='rtl8723b_set_tx_power':
   text=re.sub(r'(ofdm|mcs)base \+= ([^;]+);',r'\1base=(u8)r23_power_clamp((int)\1base+(\2));',text)

  # The selected channel remains20MHz; parsing other primary cases is useful
  # for differential preservation but does not grant wider-mode readiness.
  out+=text+'\n\n'
 (ROOT/'src/RTL8723BUAlgorithms.inc').write_text(out)
 manifest={'upstream':'torvalds/linux','commit':COMMIT,'license':'GPL-2.0-only','symbols':ordered,'adaptations':['typed serialized checked I/O; bounded cleanup after errors','Imported register tables have translation-unit-private linkage','factory signed power clamping0..63','IQK candidate requires nonzero TX/RX coefficients and real channel readback','LCK/RFK keep TX paused until station response rates are programmed','HID+A2DP profile flag typo corrected in primary coexistence helper','three-pipe VO endpoint follows programmed VOQ instead of VIQ'],'files':[
 {'path':name,'url':'https://raw.githubusercontent.com/torvalds/linux/'+COMMIT+'/drivers/net/wireless/realtek/rtl8xxxu/'+name,'sha256':hashlib.sha256((args.source/name).read_bytes()).hexdigest()} for name in files]}
 (ROOT/'src/RTL8723BU-Upstream.json').write_text(json.dumps(manifest,indent=2)+'\n')
 print('RTL8723BU: '+str(len(ordered))+' actual primary algorithm functions')

if __name__=='__main__':main()
