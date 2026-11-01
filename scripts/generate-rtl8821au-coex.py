#!/usr/bin/env python3
"""Extract the pinned8821A coex policy, callbacks and tables. No hardware I/O."""
from pathlib import Path
import argparse,hashlib,json,re
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);a=p.parse_args()
expected=json.loads((root/'src/RTL8812AUPHY-Upstream.json').read_text())['sources']
expected={k:expected[k] for k in ['main.h','reg.h','rtw8821a.c']}
expected.update({'coex.c':'f0ecf9a2d2f121fc7dab32dcf3ced250f388175d8251dd398df6298e3e412d41','coex.h':'047aa0d252aee946869be49960129a52b5132528563ed52d9355e2b9870b3800','fw.c':'0da28129dfcdf3e9b5661523b54ede4632c6c9e5af7008a6f7ce2a1f8b6a19f0','fw.h':'45a1bf5213eec95cd450a54bc50caa5652c959dd2b644684ec7447b278f6e701'})
files={k:(a.source/k).read_text() for k in expected}
for name,sha in expected.items():
 if hashlib.sha256(files[name].encode()).hexdigest()!=sha:raise SystemExit('Unpinned source: '+name)
notice='/* SPDX-License-Identifier: BSD-3-Clause */\n/* Pinned Linuxrtw88 Copyright(c)2018-2019,2024 Realtek Corporation.\n * Dual GPL-2.0 OR BSD-3-Clause; this port selects BSD-3-Clause.\n * See RTL8821AUCoex-Upstream.json and RTL8812AUPHY-LICENSE.txt. */\n'
def span(text,name):
 m=re.search(r'(?m)^(?:static )?(?:inline )?(?:const )?[\w\s*]+\b'+name+r'\([^;]*?\)\s*\{',text)
 if not m:raise ValueError(name)
 opening=text.index('{',m.start());depth=1;i=opening+1
 # The selected primary bodies contain no brace characters in comments/strings.
 for token in re.finditer(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|[{}]',text[i:],re.S):
  if token[0]=='{':depth+=1
  elif token[0]=='}':
   depth-=1
   if not depth:return m.start(),i+token.end()
 raise ValueError(name)
def function(text,name):
 begin,end=span(text,name);return text[begin:end]+'\n'
def portable(s):
 s=re.sub(r'rtw_dbg\(.*?\);','(void)0;',s,flags=re.S)
 s=re.sub(r'EXPORT_SYMBOL\([^;]+;','',s)
 s=s.replace('= {};','= {0};')
 # The single station caller serializes this entire module with its gate.
 s=re.sub(r'(?:lockdep_assert_held|mutex_lock|mutex_unlock)\([^;]+;','',s)
 # Preserve the upstream parameter ABI without warning suppression.
 def ignore(m):
  names=[re.search(r'(\w+)\s*$',v).group(1) for v in m[2].split(',') if v.strip() and v.strip()!='void']
  return m[0]+''.join('\n\t(void)'+n+';' for n in names)
 s=re.sub(r'(?m)^([A-Za-z][\w \t*]*\b\w+)\(([^;{}]*?)\)\s*\{',ignore,s)
 return s
header=files['coex.h'];at=header.index('static inline void rtw_coex_set_init')
definitions=header[:at];definitions=re.sub(r'#ifndef __RTW_COEX_H__\n#define __RTW_COEX_H__\n','',definitions)
for word in ['HIGH','MEDIUM','LOW']:
 definitions=re.sub(r'#define COEX_RSSI_'+word+r'\(rssi\).*?\}\)', '#define COEX_RSSI_'+word+'(rssi) ((rssi)==COEX_RSSI_STATE_'+word+' || (rssi)==COEX_RSSI_STATE_STAY_'+word+')',definitions,flags=re.S,count=1)
definitions=re.sub(r'le64_get_bits\(\*\(\(__le64 \*\)\(payload\)\),', 'le64_get_bits(cx_le64(payload),',definitions)
main=files['main.h'];begin=main.index('enum rtw_coex_bt_state_cnt');end=main.index('#define DPK_RF_REG_NUM',begin)
definitions+=main[begin:end].replace('\tstruct sk_buff_head queue;','').replace('\twait_queue_head_t wait;','')
(root/'src/RTL8821AUCoexTypes.inc').write_text(notice+definitions)
wrappers=header[at:header.rindex('#endif')]
wrappers=re.sub(r'void rtw_coex_display_coex_info\([^;]+;','',wrappers)
wrappers=re.sub(r'(?m)^(void|u32) (rtw_coex_\w+)\(([^;{}]*)\);',r'static \1 \2(\3);',wrappers)
(root/'src/RTL8821AUCoexWrappers.inc').write_text(notice+wrappers)
policy=files['coex.c'];policy=policy[policy.index('static u8 rtw_coex_next_rssi_state'):policy.index('#ifdef CONFIG_RTW88_DEBUGFS')]
for name in ['rtw_coex_get_bt_status_string','rtw_coex_get_bt_profile_string','rtw_coex_get_algo_string']:
 if re.search(r'\b'+name+r'\(',policy):
  begin,end=span(policy,name);policy=policy[:begin]+policy[end:]
begin,end=span(policy,'rtw_coex_info_request')
policy=policy[:begin]+'''static struct sk_buff *rtw_coex_info_request(struct rtw_dev *rtwdev,struct rtw_coex_info_req *req){return cx_info_request(rtwdev,req);}\n'''+policy[end:]
begin,end=span(policy,'rtw_coex_info_response')
policy=policy[:begin]+'''void rtw_coex_info_response(struct rtw_dev *rtwdev,struct sk_buff *skb){cx_info_response(rtwdev,skb);}\n'''+policy[end:]
begin,end=span(policy,'rtw_coex_set_bt_rx_gain')
policy=policy[:begin]+'''static void rtw_coex_set_bt_rx_gain(struct rtw_dev *rtwdev, u8 bt_lna_lvl)
{
 struct rtw_coex_dm *coex_dm = &rtwdev->coex.dm;

 if (rtwdev->error || !rtwdev->active)
  return;

 /* The primary blocking MP wait admitted every LNA request before caching
  * this level. A busy asynchronous slot retains only the latest decision. */
 rtwdev->mp.lna_desired = bt_lna_lvl;
 rtwdev->mp.lna_pending = false;
 if (bt_lna_lvl == coex_dm->cur_bt_lna_lvl)
  return;

 if (bt_lna_lvl < 7) {
  if (rtwdev->mp.pending && rtwdev->now_ms < rtwdev->mp.deadline) {
   rtwdev->mp.lna_pending = true;
   return;
  }
  rtw_coex_set_lna_constrain_level(rtwdev, bt_lna_lvl);
  if (rtwdev->error)
   return;
  rtw_coex_write_scbd(rtwdev, COEX_SCBD_RXGAIN, true);
 } else {
  rtw_coex_write_scbd(rtwdev, COEX_SCBD_RXGAIN, false);
 }
 /* Like the primary API, successful H2C admission is sufficient: the ACK's
  * return value was ignored there. Failed or deferred writes never apply. */
 if (!rtwdev->error)
  coex_dm->cur_bt_lna_lvl = bt_lna_lvl;
}
'''+policy[end:]
policy=portable(policy)
# The primaryu8 counter otherwise wraps and falsely re-enables inactive BT.
policy=policy.replace('coex_stat->bt_disable_cnt++;', 'if (coex_stat->bt_disable_cnt < 10) coex_stat->bt_disable_cnt++;')
policy=policy.replace('coex_stat->cnt_wl[COEX_CNT_WL_SCANAP] = 30; /* To do */',
 '/* Preserve the caller\'s actual scan-cache AP count. */')
policy=re.sub(r'(?m)^(void|u32) (rtw_coex_\w+)\(',r'static \1 \2(',policy)
# Retain primary entry points unused by this always-active8821 station.
policy=policy.replace('static void rtw_coex_lps_notify(', 'static __attribute__((unused)) void rtw_coex_lps_notify(').replace('static void rtw_coex_query_bt_hid_list(', 'static __attribute__((unused)) void rtw_coex_query_bt_hid_list(')
(root/'src/RTL8821AUCoexPolicy.inc').write_text(notice+policy)
names=['rtw8821a_coex_cfg_'+v for v in ['init','ant_switch','gnt_fix','gnt_debug','rfe_type','wl_tx_power','wl_rx_gain']]
callbacks='\n'.join(function(files['rtw8821a.c'],n) for n in names)
(root/'src/RTL8821AUCoexCallbacks.inc').write_text(notice+portable(callbacks))
chip=files['rtw8821a.c'];tables=chip[chip.index('static const u8 wl_rssi_step_8821a'):chip.index('static const struct rtw_reg_domain coex_info_hw_regs_8821a')]
tables=tables.replace('static_assert(', '_Static_assert(').replace('ARRAY_SIZE(rf_para_rx_8821a));','ARRAY_SIZE(rf_para_rx_8821a),"matching coex RF table sizes");')
params=chip[chip.index('\t.coex_para_ver ='):chip.index('\n\t.coex_info_hw_regs_num =')]
tables+='\nstatic const struct rtw_chip_info cx_chip={.ops=&cx_ops,.id=RTW_CHIP_TYPE_8821A,\n'+params+'\n};\n'
(root/'src/RTL8821AUCoexTables.inc').write_text(notice+tables)
manifest={'commit':'f47d223627f80cb7a1eb14c585fe6cb35de5e984','selected_license':'BSD-3-Clause','sources':expected,'callbacks':names,'adaptations':['Debugfs display tail and logging omitted; full station policy and chip tables retained.','One serialized station; delayed Linux work uses caller-driven monotonic deadlines.','BT MP requests are asynchronous to avoid blocking USB completion on the same controller gate.','Firmware commands share the typed AU bootstrap H2C cursor.','Inactive BT threshold counter saturates at10 instead of wrapping after256 checks.','SCAN_FINISH preserves the actual caller scan-cache count instead of the upstream TODO30.','Busy MP slots retain one latest desired LNA level; real C2H completion or a bounded periodic timeout retries it after live authority checks. The applied level is cached only after successful H2C/scoreboard admission; stop/error/reset discards deferred work.']}
(root/'src/RTL8821AUCoex-Upstream.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Generated pinned8821A station coex policy,callbacks,tables and types.')
