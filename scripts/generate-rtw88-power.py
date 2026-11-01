#!/usr/bin/env python3
"""Generate exact USB-only B/C power tables from hash-pinned Linux sources.
Preserves silicon cut masks and per-chip on/off order; never touches USB.
"""
from pathlib import Path
import argparse,hashlib,json,re,subprocess,tempfile
p=argparse.ArgumentParser(description=__doc__);p.add_argument('reference',type=Path);args=p.parse_args()
root=Path(__file__).resolve().parents[1];metadata=json.loads((root/'src/RTW88USBDataPath-Upstream.json').read_text());expected={f['path']:f['sha256'] for f in metadata['files']}
for name in ['main.h','rtw8821c.c','rtw8822b.c','rtw8822c.c']:
 if hashlib.sha256((args.reference/name).read_bytes()).hexdigest()!=expected[name]:raise SystemExit('Upstream source hash mismatch: '+name)
defines='\n'.join(line for line in (args.reference/'main.h').read_text().splitlines() if line.startswith('#define RTW_PWR_'))
result=['/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */','/* Copyright(c)2018-2019 Realtek Corporation. Generated from pinned rtw88 power flows. */']
for chip in ['8821c','8822b','8822c']:
 source=(args.reference/('rtw'+chip+'.c')).read_text()
 arrays=re.findall(r'static const struct rtw_pwr_seq_cmd \w+\[\] = \{.*?\n\};',source,re.S)
 assert len(arrays)==4
 for mode,flow in [('on','card_enable_flow_'),('off','card_disable_flow_')]:
  match=re.search(r'static const struct rtw_pwr_seq_cmd \* const '+flow+chip+r'\[\] = \{(.*?)\};',source,re.S);assert match
  names=re.findall(r'\btrans_\w+',match.group(1));assert len(names)==2
  body='#include <stdint.h>\n#include <stdio.h>\n#define BIT(n) (1U<<(n))\n'+defines+'\n#define RTW_PWR_DELAY_US 0\n#define RTW_PWR_DELAY_MS 1\nstruct rtw_pwr_seq_cmd {uint16_t offset;uint8_t cut_mask,intf_mask,base,cmd,mask,value;};\n'+'\n'.join(arrays)+'\nint main(void){\n'
  for name in names:body+='for(unsigned j=0;j<sizeof('+name+')/sizeof('+name+'[0]);j++){const struct rtw_pwr_seq_cmd*p='+name+'+j;if(p->cmd==RTW_PWR_CMD_END)break;if(!(p->intf_mask&RTW_PWR_INTF_USB_MSK))continue;if(p->base!=RTW_PWR_ADDR_MAC)return 1;printf("%u %u %u %u %u\\n",p->offset,p->cut_mask,p->cmd,p->mask,p->value);}\n'
  body+='return 0;}\n'
  with tempfile.TemporaryDirectory() as temp:
   c=Path(temp)/'extract.c';exe=Path(temp)/'extract';c.write_text(body);subprocess.run(['xcrun','clang','-std=c11',str(c),'-o',str(exe)],check=True);lines=subprocess.check_output([str(exe)],text=True).splitlines()
  result.append('static const struct rtw88_power_op power_'+mode+'_'+chip+'[] = {')
  for line in lines:
   address,cut,cmd,mask,value=map(int,line.split());result.append(' {0x%04x,0x%02x,%u,0x%02x,0x%02x},'%(address,cut,cmd,mask,value))
  result.append(' {0xffff,0xff,4,0,0}\n};');print(chip,mode,len(lines),'USB power operations')
(root/'src/RTW88BootPowerTables.inc').write_text('\n'.join(result)+'\n')
