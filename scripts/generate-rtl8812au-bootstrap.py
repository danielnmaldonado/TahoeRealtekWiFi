#!/usr/bin/env python3
"""Extract pinned AU MAC tables and USB-only cold enable/disable sequences."""
from pathlib import Path
import argparse,re,ast,hashlib,json
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);a=p.parse_args()
names=['rtw88xxa.c','rtw8812a.c','rtw8821a.c','rtw8812a_table.c','rtw8821a_table.c','mac.c','mac.h','usb.c','main.c','main.h','fw.c','fw.h','mac80211.c','sec.h','sec.c']
EXPECTED = {
  "rtw88xxa.c": "5aea37c1db5310a2a86c2a6d0bdc390fa430faf55125dfcc1ffcb552b085e514",
  "rtw8812a.c": "31ec275f8da9026278b97e572367a231a5b6b847cae0cfac6b2090b9b541594b",
  "rtw8821a.c": "75906260b2fd5b21b4d5f0c5c81be6b56272a5b011951336afbc17321992b8c7",
  "rtw8812a_table.c": "e400b2daf6824f3c23d8a4f3b02fd396cb19f55f7c59ada570b62e2b442b235d",
  "rtw8821a_table.c": "08fb956aaa71ae0df8651f73c847922bedcfe44c695cdc6867bd3a3900ef2393",
  "mac.c": "ec8a96ef305231f15af22bc94ad5708e0daf3bfb3ff470ec96757913e2be59f6",
  "mac.h": "006c62b0d7b7a6dee95cc27172c0452bcfa6b05f80a1a91610c4750f42f4c91a",
  "usb.c": "64d44452d287386bdb0de219c6a53ab288aec7762983df6929e798b2bc610a3b",
  "main.c": "cd52ce31a39022269a9cf48a35c2e1b89cc8a25ccb0f2ed1cec8425e910dcfa4",
  "main.h": "6f2eaeb830844efface8a676815e2a0b15f59aa5408387fa85dc91613d351a24",
  "fw.c": "0da28129dfcdf3e9b5661523b54ede4632c6c9e5af7008a6f7ce2a1f8b6a19f0",
  "fw.h": "45a1bf5213eec95cd450a54bc50caa5652c959dd2b644684ec7447b278f6e701",
  "mac80211.c": "5de984df903ec1d96199ecd05b37f7ce3459de575a9439a59ac6f06d47a0853a",
  "sec.h": "78d112a2f089487d4e2221b3217c36df0694b3db8e1147acaa53204ce10004a8",
  "sec.c": "cad22a56ede19a42b3a29a260cdb8400ae901a19b34c86ad5dd173d2b6eebebc"
}
files={n:(a.source/n).read_text() for n in names}
for n in names:
 if hashlib.sha256(files[n].encode()).hexdigest()!=EXPECTED[n]:raise SystemExit('Pinned SHA256 mismatch: '+n)
notice='/* SPDX-License-Identifier: BSD-3-Clause */\n/* Linuxrtw88,Copyright(c)2018-2019,2024 Realtek Corporation.\n * Retain RTL8812AUPHY-LICENSE.txt. Generated from pinned primary source. */\n'
def expression(s):
 node=ast.parse(s.strip(),mode='eval').body
 def value(n):
  if isinstance(n,ast.Constant) and isinstance(n.value,int):return n.value
  if isinstance(n,ast.BinOp) and isinstance(n.op,ast.BitOr):return value(n.left)|value(n.right)
  if isinstance(n,ast.Call) and isinstance(n.func,ast.Name) and n.func.id=='BIT' and len(n.args)==1:return 1<<value(n.args[0])
  raise ValueError(s)
 return value(node)
def table(src,name,kind):
 m=re.search(r'static const '+kind+r' '+name+r'\[\] = \{(.*?)\n\};',files[src],re.S)
 if not m:raise ValueError(name)
 return m[1]
power=notice;mac=notice
for chip in ['8812a','8821a']:
 for label,transitions in [('enable',['carddis_to_cardemu','cardemu_to_act']),('lps',['act_to_lps']),('disable',['act_to_cardemu','cardemu_to_carddis'])]:
  power+='static const rtw_power_step au_'+chip+'_'+label+'[] = {\n'
  for tr in transitions:
   body=table('rtw'+chip+'_table.c','trans_'+tr+'_'+chip,'struct rtw_pwr_seq_cmd')
   for row in re.findall(r'\{([^{}]+)\}',body,re.S):
    values=[v.strip() for v in row.split(',') if v.strip()]
    addr,cut,intf,base,cmd,mask,data=values
    assert cut=='RTW_PWR_CUT_ALL_MSK'
    if 'RTW_PWR_INTF_USB_MSK' not in intf and 'RTW_PWR_INTF_ALL_MSK' not in intf:continue
    if cmd=='RTW_PWR_CMD_END':continue
    assert base=='RTW_PWR_ADDR_MAC'
    if cmd=='RTW_PWR_CMD_DELAY':
     us=expression(addr)*(1000 if data=='RTW_PWR_DELAY_MS' else 1);assert 0<us<=65535
     power+='    {%d,RTW_PWR_DELAY_US,0,0},\n'%us
    else:
     op={'RTW_PWR_CMD_WRITE':'RTW_PWR_WRITE8','RTW_PWR_CMD_POLLING':'RTW_PWR_POLL8'}[cmd]
     power+='    {0x%04x,%s,0x%02x,0x%02x},\n'%(expression(addr),op,expression(mask),expression(data))
  power+='    {0,RTW_PWR_END,0,0}\n};\n'
 body=table('rtw'+chip+'_table.c','rtw'+chip+'_mac','u32')
 mac+='static const uint32_t au_'+chip+'_mac[] = {'+body+'\n};\n'
(root/'src/RTL8812AUPower.inc').write_text(power)
(root/'src/RTL8812AUMACTables.inc').write_text(mac)
meta={'repository':'torvalds/linux','commit':'f47d223627f80cb7a1eb14c585fe6cb35de5e984','selected_license':'BSD-3-Clause','source_sha256':EXPECTED,'base_url':'https://raw.githubusercontent.com/torvalds/linux/f47d223627f80cb7a1eb14c585fe6cb35de5e984/drivers/net/wireless/realtek/rtw88/','adaptations':['USB-only power branches; all upstream cut masks are ALL.','Distinct 8812/8821 MAC tables, FIFO/page geometry and endpoint maps.','Bounded allocation-free OTP, LLT, firmware and H2C operations.','TX remains paused; hardware crypto disabled for software net80211 CCMP.']}
(root/'src/RTL8812AUBootstrap-Upstream.json').write_text(json.dumps(meta,indent=2)+'\n')
print('Generated distinct AU cold power flows and MAC tables')
