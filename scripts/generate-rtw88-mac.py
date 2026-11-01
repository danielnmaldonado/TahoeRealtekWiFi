#!/usr/bin/env python3
"""Extract pinned Linux rtw88 chip MAC init register operations; never touches USB.
Usage: python3 scripts/generate-rtw88-mac.py /path/to/linux/rtw88
The complete upstream functions are executed against two deterministic register
models (all-zero/all-one reads). Their read-modify-write masks are preserved.
"""
from pathlib import Path
import argparse, hashlib, json, subprocess, tempfile
p=argparse.ArgumentParser(description=__doc__);p.add_argument('reference',type=Path);args=p.parse_args()
root=Path(__file__).resolve().parents[1]
provenance=json.loads((root/'src/RTW88USBDataPath-Upstream.json').read_text())
expected={f['path']:f['sha256'] for f in provenance['files']}
for name in ['reg.h','mac.h','bf.h','rtw8821c.c','rtw8821c.h','rtw8822b.c','rtw8822b.h','rtw8822c.c','rtw8822c.h']:
 if hashlib.sha256((args.reference/name).read_bytes()).hexdigest()!=expected[name]:
  raise SystemExit('Upstream source hash mismatch: '+name)

def defines(source):
 source=source.replace('\\\n',' ')
 return '\n'.join(line for line in source.splitlines() if line.startswith('#define ') and not line.split()[1].startswith('__'))

def function(source,name):
 start=source.index('static int '+name+'(');begin=source.index('{',start);level=1;end=begin+1
 while level:
  if source[end]=='{':level+=1
  if source[end]=='}':level-=1
  end+=1
 return source[start:end]

shim=r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;
#define BIT(n) (1U<<(n))
#define GENMASK(h,l) ((UINT32_MAX>>(31-(h)))&(UINT32_MAX<<(l)))
#define MASKDWORD 0xffffffffU
struct rtw_dev {struct {u16 rxfltmap1;}hal;};
static unsigned ones;
static u8 rtw_read8(struct rtw_dev*c,unsigned a){(void)c;(void)a;return ones?255:0;}
static u16 rtw_read16(struct rtw_dev*c,unsigned a){(void)c;(void)a;return ones?65535:0;}
static u32 rtw_read32(struct rtw_dev*c,unsigned a){(void)c;(void)a;return ones?UINT32_MAX:0;}
static void rec(unsigned w,unsigned a,u32 v,u32 m){printf("%u %u %u %u\n",w,a,v,m);}
#define OP(w) \
static void rtw_write##w(struct rtw_dev*c,unsigned a,u##w v){(void)c;rec(w/8,a,v,(u##w)~(u##w)0);} \
static void rtw_write##w##_set(struct rtw_dev*c,unsigned a,u##w v){(void)c;rec(w/8,a,v,v);} \
static void rtw_write##w##_clr(struct rtw_dev*c,unsigned a,u##w v){(void)c;rec(w/8,a,0,v);}
OP(8) OP(16) OP(32)
'''
result=['/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */','/* Copyright(c)2018-2019 Realtek Corporation. Generated from pinned rtw88 chip MAC init. */']
for chip in ['8821c','8822b','8822c']:
 source=(args.reference/('rtw'+chip+'.c')).read_text();name='rtw'+chip+'_mac_init'
 fn=function(source,name)
 body=shim+'\n'+defines((args.reference/'reg.h').read_text())+'\n'+defines((args.reference/'mac.h').read_text())+'\n'+defines((args.reference/'bf.h').read_text())+'\n'+defines((args.reference/('rtw'+chip+'.h')).read_text())+'\n'+defines(source[:source.index('static int '+name)])+'\n'+fn+'\nint main(int argc,char**argv){struct rtw_dev c={0};ones=argc>1?atoi(argv[1]):0;return '+name+'(&c);}\n'
 with tempfile.TemporaryDirectory() as temp:
  c=Path(temp)/'extract.c';exe=Path(temp)/'extract';c.write_text(body)
  subprocess.run(['xcrun','clang','-std=c11','-Wno-macro-redefined',str(c),'-o',str(exe)],check=True)
  zero=[tuple(map(int,line.split())) for line in subprocess.check_output([str(exe),'0'],text=True).splitlines()]
  one=[tuple(map(int,line.split())) for line in subprocess.check_output([str(exe),'1'],text=True).splitlines()]
 assert len(zero)==len(one)
 result.append('static const struct rtw88_mac_op mac_'+chip+'[] = {')
 for z,o in zip(zero,one):
  width,address,value,explicitmask=z
  assert (width,address,explicitmask)==(o[0],o[1],o[3])
  mask=explicitmask if explicitmask!=(1<<(width*8))-1 else explicitmask&~(value^o[2])
  result.append('    {0x%04x,%u,0x%08xU,0x%08xU},'%(address,width,mask,value&mask))
 result.append('};')
 print(chip,len(zero),'MAC operations',hashlib.sha256(fn.encode()).hexdigest())
(root/'src/RTW88MACChipTables.inc').write_text('\n'.join(result)+'\n')
