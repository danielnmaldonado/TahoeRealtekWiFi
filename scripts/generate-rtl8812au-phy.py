#!/usr/bin/env python3
"""Extract pinned Linuxrtw88 8812A/8821A PHY tables and RFK algorithms.

No driver installation or hardware operations. The standalone adapter supplies
bounded I/O and failure cleanup; all structural adaptations are listed below.
"""
from pathlib import Path
import argparse,re,hashlib,json
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);a=p.parse_args()
EXPECTED_LINUX_SHA256 = {
  "phy.h": "55388bdbec3b743553776282b918d2f6ffe06d4ac1d8849cf0ea0e22f19867e4",
  "tx.h": "5f0c1dc2c8e9dbcdfece4e34c264bd24731ab6a3ff28011f9ddb09cb695aaa46",
  "tx.c": "dfc212ab3caf47437ed234dba1b7f7ef0cbd45749d4d34162699446a2a4adf7e",
  "rtw88xxa.c": "5aea37c1db5310a2a86c2a6d0bdc390fa430faf55125dfcc1ffcb552b085e514",
  "rtw8812a.c": "31ec275f8da9026278b97e572367a231a5b6b847cae0cfac6b2090b9b541594b",
  "rtw8821a.c": "75906260b2fd5b21b4d5f0c5c81be6b56272a5b011951336afbc17321992b8c7",
  "rtw8812a_table.c": "e400b2daf6824f3c23d8a4f3b02fd396cb19f55f7c59ada570b62e2b442b235d",
  "rtw8821a_table.c": "08fb956aaa71ae0df8651f73c847922bedcfe44c695cdc6867bd3a3900ef2393",
  "reg.h": "c8246f9305539347fdb62b0294fe12ea24769cb3ad56c2b0235c4269a0307324",
  "phy.c": "669d76c8acfc9f30418dbd119e777eae4404f6e3bb1198292aab536a3aa9e4ec",
  "main.h": "6f2eaeb830844efface8a676815e2a0b15f59aa5408387fa85dc91613d351a24",
  "rtw88xxa.h": "e217f23f9cc1918332a2005da5aca234f481fd4f291002d305b74ae9c8a5a2b6"
}
files={name:(a.source/name).read_text() for name in EXPECTED_LINUX_SHA256}
for name,expected in EXPECTED_LINUX_SHA256.items():
 if hashlib.sha256(files[name].encode()).hexdigest()!=expected:
  raise SystemExit("Pinned upstreamSHA256 mismatch: "+name)
notice='/* SPDX-License-Identifier: BSD-3-Clause */\n/* Linuxrtw88 Copyright(c)2018-2019,2024 Realtek Corporation. GPL-2.0 OR BSD-3-Clause.\n * This port selectsBSD-3-Clause; retain RTL8812AUPHY-LICENSE.txt. */\n'
def fun(src,name):
 s=files[src];m=re.search(r'(?m)^(?:static )?(?:void|bool|u32|u8)\s*\n?'+name+r'\(',s)
 if not m:raise ValueError((src,name))
 start=m.start();opening=s.index('{',m.end());depth=0
 for t in re.finditer(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|[{}]',s[opening:]):
  if t.group()=='{':depth+=1
  elif t.group()=='}':
   depth-=1
   if not depth:return s[start:opening+t.end()]+'\n'
 raise ValueError(name)
common=['rtw8812a_config_1t','rtw88xxa_get_bb_swing','rtw88xxa_get_swing_index','rtw88xxa_pwrtrack_init','rtw88xxa_set_channel_bb_swing','rtw8821a_set_ext_band_switch','rtw8821a_phy_set_rfe_reg_24g','rtw8821a_phy_set_rfe_reg_5g','rtw8812a_phy_set_rfe_reg_24g','rtw8812a_phy_set_rfe_reg_5g','rtw88xxa_switch_band','rtw88xxa_phy_read_rf','rtw8812a_phy_fix_spur','rtw88xxa_switch_channel','rtw88xxa_set_reg_bw','rtw88xxa_post_set_bw_mode','rtw88xxa_set_channel_rf','rtw88xxa_set_channel','rtw88xxa_iqk_backup_mac_bb','rtw88xxa_iqk_backup_afe','rtw88xxa_iqk_restore_mac_bb','rtw88xxa_iqk_configure_mac','rtw88xxa_iqk_finish']
chip12=['rtw8812a_do_lck','rtw8812a_iqk_backup_rf','rtw8812a_iqk_restore_rf','rtw8812a_iqk_restore_afe','rtw8812a_iqk_rx_fill','rtw8812a_iqk_tx_fill','rtw8812a_iqk','rtw8812a_do_iqk']
chip21=['rtw8821a_iqk_backup_rf','rtw8821a_iqk_restore_rf','rtw8821a_iqk_restore_afe','rtw8821a_iqk_rx_fill','rtw8821a_iqk_tx_fill','rtw8821a_iqk_tx_vdf_true','rtw8821a_iqk_tx_vdf_false','rtw8821a_iqk_rx','rtw8821a_iqk','rtw8821a_do_iqk']
s=notice+'\n'.join(fun('rtw88xxa.c',n) for n in common)+'\n'.join(fun('rtw8812a.c',n) for n in chip12)+'\n'.join(fun('rtw8821a.c',n) for n in chip21)
s=s.replace('struct rtw_dev','struct rtw_r12au_phy').replace('rtw88xxa_','r12_').replace('rtw8812a_','r12_chip12_').replace('rtw8821a_','r12_chip21_')
s=re.sub(r'rtw_dbg\(.*?\);','(void)0;',s,flags=re.S)
s=s.replace('read_poll_timeout_atomic(rtw_read16, reg_41a, (reg_41a & 0x30) == 0x30,\n\t\t\t\t\t 50, 2500, false, rtwdev, REG_TXPKT_EMPTY);','r12_wait_txempty(rtwdev);')
s=s.replace('u16 basic_rates, reg_41a;','u16 basic_rates;')
# Upstream8821 overrides its own hardwareflags afterreading them. A portable
# ready/failed resultmust preserve measured bits, never fabricate success.
s=s.replace('iqk_ready = true;', '/* Measured IQKready bit is required. */').replace('tx_fail = false;', '/* Measured TXfailure bit is authoritative. */')
# Hardware status is exposed explicitly; never call fallbackIQC a successfulcalibration.
s=s.replace('/* FillIQK Result */\n\t', 'rtwdev->tx_iqk_mask = (tx0_finish ? 1U : 0U) | (tx1_finish ? 2U : 0U);\n\trtwdev->rx_iqk_mask = (rx0_finish ? 1U : 0U) | (rx1_finish ? 2U : 0U);\n\t/* FillIQK Result */\n\t',1)
s=s.replace('if (rtw88xxa_iqk_finish', 'if (r12_iqk_finish')
s=s.replace('r12_chip21_iqk_tx_fill(rtwdev, tx_x, tx_y);','{rtwdev->tx_iqk_mask=1;r12_chip21_iqk_tx_fill(rtwdev, tx_x, tx_y);}')
s=s.replace('r12_chip21_iqk_rx_fill(rtwdev, rx_x, rx_y);','{rtwdev->rx_iqk_mask=1;r12_chip21_iqk_rx_fill(rtwdev, rx_x, rx_y);}')
# Prevent restoration from uninitialized backups and allow bounded best-effort
# restores after a failure while retaining the original latchederror.
s=s.replace('u32 macbb_backup[MACBB_REG_NUM_8821A];','u32 macbb_backup[MACBB_REG_NUM_8821A] = {0};').replace('u32 afe_backup[AFE_REG_NUM_8821A];','u32 afe_backup[AFE_REG_NUM_8821A] = {0};').replace('u32 rfa_backup[RF_REG_NUM_8821A];','u32 rfa_backup[RF_REG_NUM_8821A] = {0};')
s=s.replace('r12_iqk_configure_mac(rtwdev);','if (rtwdev->io_error) return;\n\tr12_iqk_configure_mac(rtwdev);')
s=s.replace('r12_chip12_iqk(rtwdev);','r12_chip12_iqk(rtwdev);\n\trtwdev->cleanup=true;').replace('r12_chip21_iqk(rtwdev);','r12_chip21_iqk(rtwdev);\n\trtwdev->cleanup=true;')
s=s.replace('backup_macbb_reg, MACBB_REG_NUM_8812A);\n}', 'backup_macbb_reg, MACBB_REG_NUM_8812A);\n\trtwdev->cleanup=false;\n}').replace('backup_macbb_reg, MACBB_REG_NUM_8821A);\n}', 'backup_macbb_reg, MACBB_REG_NUM_8821A);\n\trtwdev->cleanup=false;\n}')
# LCK must restore the pause state, not transiently unpause an unready radio.
s=s.replace('u32 cont_tx, lc_cal, i;', 'u32 cont_tx, lc_cal, i;\n\tu8 pause=rtw_read8(rtwdev,REG_TXPAUSE);')
s=s.replace('if (i == 5)\n\t\trtw_dbg(rtwdev, RTW_DBG_RFK, "LCK timed out\\n");','if (i == 5) rtwdev->cal_timeout=true;')
s=s.replace('if (i == 5)\n\t\t(void)0;', 'if (i == 5) rtwdev->cal_timeout=true;')
s=s.replace('rtw_write8(rtwdev, REG_TXPAUSE, 0);','rtw_write8(rtwdev, REG_TXPAUSE, pause);')
# C11 avoids compiler-extension emptyarrayinitializers used upstream.
s=s.replace('= {};','= {0};').replace('\nu32 r12_phy_read_rf(', '\nstatic u32 r12_phy_read_rf(')
s=s.replace('u8 txsc)\n{', 'u8 txsc)\n{\n\t(void)channel;').replace('u8 channel, u8 bw)\n{\n\tu32 rf_reg18', 'u8 channel, u8 bw)\n{\n\t(void)channel;\n\tu32 rf_reg18')
# Portable error cleanup and defined signed 11-bit IQKpacking.
s=re.sub(r'(static void r12_(?:post_set_bw_mode|set_channel_rf)\([^\{]+\{)',r'\1\n\t(void)channel;',s)
s=re.sub(r'([a-zA-Z0-9_]+\[[^;\n]+\]) <<= 21;',r'\1 = (int32_t)((uint32_t)\1 << 21);',s)
s=re.sub(r'case (\d+) \.\.\. (\d+):',lambda m:' '.join('case '+str(n)+':' for n in range(int(m[1]),int(m[2])+1)),s)
s=s.replace('bool set_cca, pi_mode;', 'bool set_cca, pi_mode;\n\tu32 original_cca=0;\n\tbool cleanup=rtwdev->cleanup;')
s=s.replace('if (set_cca)\n\t\trtw_write32_set(rtwdev, REG_CCA2ND, BIT(3));', 'if (set_cca) {\n\t\toriginal_cca=rtw_read32(rtwdev,REG_CCA2ND);\n\t\tif(rtwdev->io_error)return INV_RF_DATA;\n\t\trtw_write32(rtwdev,REG_CCA2ND,original_cca|BIT(3));\n\t}')
s=s.replace('if (set_cca)\n\t\trtw_write32_clr(rtwdev, REG_CCA2ND, BIT(3));','if (set_cca) {\n\t\trtwdev->cleanup=true;\n\t\trtw_write32(rtwdev,REG_CCA2ND,original_cca);\n\t\trtwdev->cleanup=cleanup;\n\t}')
s=s.replace('u8 pause=rtw_read8(rtwdev,REG_TXPAUSE);', 'u8 pause=rtw_read8(rtwdev,REG_TXPAUSE);\n\tu32 original_lck=rtw_read_rf(rtwdev,RF_PATH_A,RF_LCK,RFREG_MASK);\n\tbool cleanup=rtwdev->cleanup;')
s=s.replace('lc_cal = rtw_read_rf(rtwdev, RF_PATH_A, RF_CFGCH, RFREG_MASK);', 'lc_cal = rtw_read_rf(rtwdev, RF_PATH_A, RF_CFGCH, RFREG_MASK);\n\tif(rtwdev->io_error)return;')
s=s.replace('rtw_write_rf(rtwdev, RF_PATH_A, RF_CFGCH, RFREG_MASK, lc_cal);\n\n\trtw_write_rf(rtwdev, RF_PATH_A, RF_LCK, BIT(14), 0);', 'rtwdev->cleanup=true;\n\trtw_write_rf(rtwdev, RF_PATH_A, RF_CFGCH, RFREG_MASK, lc_cal);\n\n\trtw_write_rf(rtwdev, RF_PATH_A, RF_LCK, RFREG_MASK, original_lck);')
s=s.replace('rtw_write_rf(rtwdev, RF_PATH_A, RF_CFGCH, RFREG_MASK, lc_cal);\n}', 'rtw_write_rf(rtwdev, RF_PATH_A, RF_CFGCH, RFREG_MASK, lc_cal);\n\trtwdev->cleanup=cleanup;\n}')
# Make the upstream11-bit signed-coefficient conversions explicit without
# suppressing compiler diagnostics. Unsigned packing preserves signbit layout.
s=re.sub(r'((?:[tr]x_[xy][01](?:_temp)?|vdf_[xy])\[[^\n=]*?) = rtw_read32_mask',r'\1 = (int)rtw_read32_mask',s)
for suffix in ['x0','x1','x']:
 for prefix in ['tx','rx']:
  x,y=prefix+'_'+suffix,prefix+'_y'+suffix[1:]
  s=s.replace(', '+x+', '+y+');', ', (u32)'+x+', (u32)'+y+');')
s=s.replace('txsc = BIT_TXSC_20M(txsc20) | BIT_TXSC_40M(txsc40);','txsc = (u8)(BIT_TXSC_20M(txsc20) | BIT_TXSC_40M(txsc40));')
s=s.replace('(tx_dt[cal] & BIT(0))','(tx_dt[cal] & 1)')
# A converged but out-of-range8812RX matrix is replaced by fallbackIQC
# upstream. Keep that fallback out of the measured-success mask as well.
s=re.sub(r'(if \(rx_x >> 1 >= 0x112 \|\|\s*\(rx_y >> 1 >= 0x12 && rx_y >> 1 <= 0x3ee\)\) \{)',r'\1\n\t\trtwdev->rx_iqk_mask &= (u8)~BIT(path);',s)
# OTP1T1R is explicitly identified by rtw8812au_efuse_grant (both path
# counts become1). The primary IQK function nevertheless waits for both
# completion registers. Suppress only the absent path's polling/measurement;
# keep every present path's measured ready/fail/convergence requirements.
begin=s.index('static void r12_chip12_iqk(struct')
end=s.index('static void r12_chip12_do_iqk(',begin)
iqk=s[begin:end]
iqk=iqk.replace('bool iqk0_ready = false, tx0_finish = false, rx0_finish = false;',
 'const bool has_path_b = rtwdev->hal.rf_phy_num > 1;\n\tbool iqk0_ready = false, tx0_finish = false, rx0_finish = false;')
iqk=iqk.replace('if (!tx1_finish)', 'if (has_path_b && !tx1_finish)')
iqk=iqk.replace('if (!rx1_finish && tx1_finish)', 'if (has_path_b && !rx1_finish && tx1_finish)')
assert iqk.count('if (iqk0_ready && iqk1_ready)')==2
iqk=iqk.replace('if (iqk0_ready && iqk1_ready)', 'if (iqk0_ready && (!has_path_b || iqk1_ready))')
iqk=iqk.replace('tx1_fail = rtw_read32_mask', 'if (has_path_b) tx1_fail = rtw_read32_mask')
iqk=iqk.replace('rx1_fail = rtw_read32_mask', 'if (has_path_b) rx1_fail = rtw_read32_mask')
iqk=iqk.replace('if (!(tx1_fail || tx1_finish))', 'if (has_path_b && !(tx1_fail || tx1_finish))')
iqk=iqk.replace('if (!(rx1_fail || rx1_finish) && tx1_finish)', 'if (has_path_b && !(rx1_fail || rx1_finish) && tx1_finish)')
iqk=iqk.replace('if (tx0_finish && tx1_finish)', 'if (tx0_finish && (!has_path_b || tx1_finish))')
s=s[:begin]+iqk+s[end:]
# Internal-only symbols do not leak into the kext's global namespace.
s=re.sub(r'(?m)^(void|bool) (r12_\w+)\(',r'static \1 \2(',s)
(root/'src/RTL8812AUAlgorithms.inc').write_text(s)
def array(src,name):
 s=files[src];m=re.search(r'static const (?:u8|u32|struct \w+)\s+'+name+r'\[[^;]*?\]\s*=\s*\{',s,re.S)
 if not m:raise ValueError((src,name))
 end=s.index('\n};',m.end())+3
 return s[m.start():end]+'\n'
t=notice
for chip in ('8812a','8821a'):
 names=[f'rtw{chip}_{n}' for n in (['bb','agc','rf_a','rf_b','bb_pg','bb_pg_rfe3','txpwr_lmt','agc_diff_lb','agc_diff_hb'] if chip=='8812a' else ['bb','agc','rf_a','bb_pg','txpwr_lmt'])]
 for n in names:t+=array('rtw'+chip+'_table.c',n)+'\n'
(root/'src/RTL8812AUTables.inc').write_text(t)
thermal=notice
for chip in ('8812a','8821a'):
 source='rtw'+chip+'_table.c'
 for name in re.findall(r'static const u8 (rtw'+chip+r'_pwrtrk_\w+)\[',files[source]):thermal+=array(source,name)+'\n'
(root/'src/RTL8812AUThermalTables.inc').write_text(thermal)

reg=files['reg.h'].splitlines();defs=[];i=0
while i<len(reg):
 line=reg[i]
 if re.match(r'#define[ \t]',line):
  part=[line]
  while part[-1].endswith('\\'):i+=1;part.append(reg[i])
  defs.append('\n'.join(part))
 i+=1
(root/'src/RTL8812AURegisters.inc').write_text(notice+'\n'.join(defs)+'\n')
manifest={'commit':'f47d223627f80cb7a1eb14c585fe6cb35de5e984','selected_license':'BSD-3-Clause','sources':{k:hashlib.sha256(v.encode()).hexdigest() for k,v in files.items()},'functions':common+chip12+chip21,'adaptations':['Struct/context and internal symbols renamed; Linux logging removed.','USB callback wrappers latch errors; integer polls are bounded.','RFK success masks report actual convergence and exclude out-of-range fallback IQC.','Backup reads checked before destructive RFK; cleanup writes continue after errors.','LCK restores original TXPAUSE and RF_LCK; signed IQK packing uses unsigned shifts.','8821 IQK hardware-ready/TX-failure flags no longer overridden by constants.','8812 OTP1T1R IQK suppresses absent-path polling and measurement; every present path still requires measured ready/fail/convergence.']}
manifest['repository']='torvalds/linux'
manifest['base_url']='https://raw.githubusercontent.com/torvalds/linux/'+manifest['commit']+'/drivers/net/wireless/realtek/rtw88/'
manifest['freebsd_rom_crosscheck']={'r21a_rom.c': {'url': 'https://raw.githubusercontent.com/freebsd/freebsd-src/685dc743dc3b5645e34836464128e1c0558b404b/sys/dev/rtwn/rtl8821a/r21a_rom.c', 'sha256': '98e98a7d4799343c50a559e56c09d10a7cc156c6f71c3eba347f3a3172a68967'}, 'r12a_rom_image.h': {'url': 'https://raw.githubusercontent.com/freebsd/freebsd-src/685dc743dc3b5645e34836464128e1c0558b404b/sys/dev/rtwn/rtl8812a/r12a_rom_image.h', 'sha256': '423766a94d80b3edc6d52507dcca8776d3cc43167761f5241ee4df548c881b8d'}}
(root/'src/RTL8812AUPHY-Upstream.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Generated8812/8821 tables and',len(manifest['functions']),'primaryalgorithms')
