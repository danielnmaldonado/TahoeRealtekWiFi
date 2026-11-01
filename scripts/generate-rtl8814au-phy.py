#!/usr/bin/env python3
"""Reproduce private 8814AU dual-band PHY tables/algorithms from pinned Linux rtw88.

Source: torvalds/linux 0e135910deec43ffe97eca0b6af1f8f847285b59,
GPL-2.0 OR BSD-3-Clause, Copyright(c)2018-2019, 2025 Realtek Corporation.
The portable port selects BSD-3-Clause and retains the original algorithms.
"""
from pathlib import Path
import argparse,re,hashlib,json
ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--source',type=Path,required=True);args=parser.parse_args()
# Validate every pinned source before writing any generated artifact.
expected=json.loads((ROOT/'src/RTL8814AUPHY-Upstream.json').read_text())
if expected['commit']!='0e135910deec43ffe97eca0b6af1f8f847285b59':raise ValueError('Unexpected pinned Linux commit')
for item in expected['files']:
 if hashlib.sha256((args.source/item['path']).read_bytes()).hexdigest()!=item['sha256']:raise ValueError('Pinned-source mismatch: '+item['path'])
chip=(args.source/'rtw8814a.c').read_text();tables=(args.source/'rtw8814a_table.c').read_text();reg=(args.source/'reg.h').read_text()
notice='/* SPDX-License-Identifier: BSD-3-Clause */\n/* Adapted from Linux rtw88 0e135910deec43ffe97eca0b6af1f8f847285b59,\n * GPL-2.0 OR BSD-3-Clause. Copyright(c)2018-2019, 2025 Realtek Corporation.\n * Redistribution and use in source and binary forms, with or without\n * modification, are permitted provided that the copyright, conditions and\n * disclaimer in RTL8814AUPHY-LICENSE.txt are retained. */\n'
def array(name):
 m=re.search(r'static const (?:struct \w+|u32|u8)\s+'+re.escape(name)+r'\[.*?\]\s*=\s*\{',tables,re.S)
 if not m:raise ValueError(name)
 start=m.start();opening=tables.index('{',m.start());depth=0
 for i in range(opening,len(tables)):
  if tables[i]=='{':depth+=1
  elif tables[i]=='}':
   depth-=1
   if not depth:return tables[start:tables.index(';',i)+1]+'\n'
 raise ValueError(name)
names=['rtw8814a_agc','rtw8814a_bb','rtw8814a_rf_a','rtw8814a_rf_b','rtw8814a_rf_c','rtw8814a_rf_d','rtw8814a_bb_pg','rtw8814a_bb_pg_type0','rtw8814a_txpwr_lmt_type0','rtw8814a_txpwr_lmt_type1']
for typ in ['', 'type0_']:
 for path in 'abcd':
  for sign in 'np':names.append('rtw8814a_pwrtrk_'+typ+'2g'+path+'_'+sign)
  for sign in 'np':names.append('rtw8814a_pwrtrk_'+typ+'2g_cck_'+path+'_'+sign)
  for sign in 'np':names.append('rtw8814a_pwrtrk_'+typ+'5g'+path+'_'+sign)
out=notice+'/* Generated selected tables; 5GHz tables retained for band-specific power and thermal tracking. */\n'
for name in names:out+=array(name)+'\n'
out=out.replace('struct rtw_phy_pg_cfg_pair','struct phy_pg_row').replace('struct rtw_txpwr_lmt_cfg_pair','struct phy_limit_row').replace('rtw8814a_','phy14_')
(ROOT/'src/RTL8814AUTables.inc').write_text(out)
# Pure register definitions remain private to this translation unit.
defs=[];lines=reg.splitlines();i=0
while i<len(lines):
 line=lines[i]
 if line.startswith('#define ') or line.startswith('#define\t'):
  block=[line]
  while block[-1].endswith('\\'):
   i+=1;block.append(lines[i])
  defs.append('\n'.join(block))
 i+=1
(ROOT/'src/RTL8814AURegisters.inc').write_text(notice+'\n'.join(defs)+'\n')
def function(name):
 marker=name+'(';pos=chip.index(marker);start=chip.rfind('static ',0,pos);opening=chip.index('{',pos);depth=0
 for m in re.finditer(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|[{}]',chip[opening:]):
  if m.group()=='{':depth+=1
  elif m.group()=='}':
   depth-=1
   if depth==0:return chip[start:opening+m.end()]+'\n'
 raise ValueError(name)
names=['rtw8814a_init_rfe_reg','rtw8814a_get_bb_swing','rtw8814a_get_swing_index','rtw8814a_pwrtrack_init','rtw8814a_config_trx_path','rtw8814a_config_cck_rx_antenna_init','rtw8814a_set_rfe_reg_24g','rtw8814a_set_channel_bb_swing','rtw8814a_switch_channel','rtw8814a_set_bw_reg_adc','rtw8814a_set_bw_reg_agc','rtw8814a_24g_cck_tx_dfir','rtw8814a_set_bw_reg_mac','rtw8814a_set_bw_rf','rtw8814a_adc_clk','rtw8814a_spur_calibration_ch140','rtw8814a_set_nbi_reg','rtw8814a_nbi_setting','rtw8814a_spur_nbi_setting','rtw8814a_spur_calibration','rtw8814a_iqk_backup_mac_bb','rtw8814a_iqk_backup_rf','rtw8814a_iqk_afe_setting','rtw8814a_iqk_restore_mac_bb','rtw8814a_iqk_restore_rf','rtw8814a_iqk_reset_nctl','rtw8814a_iqk_configure_mac','rtw8814a_lok_one_shot','rtw8814a_iqk_tx_one_shot','rtw8814a_iqk_rx_one_shot','rtw8814a_iqk','rtw8814a_do_iqk','rtw8814a_txagc_swing_offset','rtw8814a_pwrtrack_set_pwr']
txgi=re.search(r'\.txgi_factor\s*=\s*(\d+)',chip).group(1)
out=notice+'#define PHY14_TXGI_FACTOR '+txgi+'\n'+'\n'.join(function(n) for n in names)
out=out.replace('struct rtw_dev','struct rtw_8814au_phy').replace('rtw8814a_','phy14_').replace('hal.primary_channel','hal.current_channel')
# Read-only no-op debug statements are removed rather than evaluating I/O in logs.
out=re.sub(r'rtw_dbg\(.*?\);','',out,flags=re.S)
out=re.sub(r'if \(read_poll_timeout\(!rtw_read32_mask, (lok_ready|\*tx_ok|rx_ok),.*?rtwdev, 0x1b00, BIT\(0\)\)\)',lambda m:'if (phy_poll_ready(rtwdev, '+('&'+m.group(1) if m.group(1)!='*tx_ok' else 'tx_ok')+', '+('10000' if m.group(1)=='lok_ready' else '20000')+'))',out,flags=re.S)
out=out.replace('bool rx_ok;', 'bool rx_ok = false;')
out=out.replace('u32 tx_matrix[RTW_RF_PATH_MAX];','u32 tx_matrix[RTW_RF_PATH_MAX] = {0};').replace('bool tx_ok[RTW_RF_PATH_MAX];','bool tx_ok[RTW_RF_PATH_MAX] = {false};')
out=out.replace('rtw_write32(rtwdev, 0x1b00, 0xf8000000 | (path << 1));\n\trtw_write32(rtwdev, 0x1bd4','rtwdev->lok_ok[path] = true;\n\trtw_write32(rtwdev, 0x1b00, 0xf8000000 | (path << 1));\n\trtw_write32(rtwdev, 0x1bd4',1)
out=out.replace('if (*tx_ok) {\n\t\t*tx_matrix', 'rtwdev->tx_ok[path] = *tx_ok;\n\tif (*tx_ok) {\n\t\t*tx_matrix',1)
out=out.replace('if (rx_ok) {\n\t\trtw_write32', 'rtwdev->rx_ok[path] = rx_ok;\n\tif (rx_ok) {\n\t\trtw_write32',1)
# All backups must be valid before any calibration register is changed.
out=out.replace('phy14_iqk_afe_setting(rtwdev, true);\n\tphy14_iqk_backup_rf', 'phy14_iqk_backup_rf')
out=out.replace('backup_rf_reg);\n\tphy14_iqk_configure_mac', 'backup_rf_reg);\n\tif (rtwdev->ctx->io_error) return;\n\tphy14_iqk_afe_setting(rtwdev, true);\n\tphy14_iqk_configure_mac')
out=out.replace('phy14_iqk(rtwdev);\n\tphy14_iqk_reset_nctl', 'phy14_iqk(rtwdev);\n\trtwdev->cleanup = true;\n\tphy14_iqk_reset_nctl')
out=out.replace('backup_rf_reg);\n}', 'backup_rf_reg);\n\trtwdev->cleanup = false;\n}')
# ADC workaround backs up all IQC words before its first destructive write.
out=out.replace('rtw_write8(rtwdev, REG_TXPAUSE, 0x3f);\n\n\t/* 1 Step 2.', '/* 1 Step 2.')
out=out.replace('rxiqc[i] = rtw_read32(rtwdev, rxiqc_reg[0][i]);\n\t\trtw_write32', 'rxiqc[i] = rtw_read32(rtwdev, rxiqc_reg[0][i]);\n\t}\n\tif (rtwdev->ctx->io_error) return;\n\trtw_write8(rtwdev, REG_TXPAUSE, 0x3f);\n\tfor (i = 0; i < 4; i++) {\n\t\trtw_write32',1)
out=out.replace('/* 1 Step 5. Recover MAC TX & IQC */','/* 1 Step 5. Recover MAC TX & IQC */\n\trtwdev->cleanup = true;')
out=out.replace('rtw_write32_mask(rtwdev, REG_PRECTRL, BIT_IQ_WGT, 0x0);\n}', 'rtw_write32_mask(rtwdev, REG_PRECTRL, BIT_IQ_WGT, 0x0);\n\trtwdev->cleanup = false;\n}',1)
out=out.replace('u32 rx_matrix;','').replace('rx_matrix = rtw_read32(rtwdev, 0x1b3c);','(void)rtw_read32(rtwdev, 0x1b3c);')
out=out.replace('static void phy14_iqk(struct rtw_8814au_phy *rtwdev)\n{\n\tu8 band = rtwdev->hal.current_band_type;\n\tu8 bw = rtwdev->hal.current_band_width;', 'static void phy14_iqk(struct rtw_8814au_phy *rtwdev)\n{\n\tu8 band = rtwdev->hal.current_band_type;')
# Explicit narrowing retains upstream bounded values while allowing the portable
# unit to build with the host's conversion diagnostics enabled.
out=out.replace('(path << 1)', '((u32)path << 1)')
out=out.replace('((bw + 3) << 8)', '(((u32)bw + 3U) << 8)').replace('((9 - bw) << 8)', '((9U - (u32)bw) << 8)')
out=out.replace('abs(', 'phy_abs(')
out=out.replace('agc_index = tx_pwr_idx_offset;', 'agc_index = (s8)tx_pwr_idx_offset;')
out=out.replace('swing_index = dm_info->default_ofdm_index +\n\t\t\t\t\tdelta_pwr_idx - tx_pwr_idx_offset;', 'swing_index = (u8)(dm_info->default_ofdm_index +\n\t\t\t\t\tdelta_pwr_idx - tx_pwr_idx_offset);')
out=out.replace('dm_info->default_ofdm_index + delta_pwr_idx;', '(u8)(dm_info->default_ofdm_index + delta_pwr_idx);')
out=out.replace('\n\t\t\t txagc_idx);', '\n\t\t\t (u32)(u8)txagc_idx);')
(ROOT/'src/RTL8814AUAlgorithms.inc').write_text(out)
manifest={'upstream':'torvalds/linux','commit':'0e135910deec43ffe97eca0b6af1f8f847285b59','license':'BSD-3-Clause selected from GPL-2.0 OR BSD-3-Clause','files':[{ 'path':str(p.name),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in [args.source/'rtw8814a.c',args.source/'rtw8814a_table.c',args.source/'reg.h',args.source/'phy.c',args.source/'main.h']]}
(ROOT/'src/RTL8814AUPHY-Upstream.json').write_text(json.dumps(manifest,indent=2)+'\n')
