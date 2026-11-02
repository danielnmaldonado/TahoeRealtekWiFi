#!/usr/bin/env python3
"""Trace actual legacy rate negotiation and gen1 H2C on the host."""
from pathlib import Path
import subprocess
import tempfile

project=Path(__file__).resolve().parents[3]
import runpy
_host = runpy.run_path(str(project / "scripts/host_cpp.py"))
_host_cxx = _host["compiler"]()
_host_sanitizers = _host["sanitizer_flags"]()
source=(project/'vendor/rtwn/rtwn.cpp').read_text()
def body(name):
    start=source.index('\n'+name+'(')+1
    opening=source.index('{',start);depth,end=1,opening+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]

pre=r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <vector>
#define __packed __attribute__((__packed__))
#define IEEE80211_ADDR_LEN 6
#define nitems(a) (sizeof(a)/sizeof((a)[0]))
#define DPRINTF(x) do {} while(0)
#define RTWN_CHIP_92E 0x40U
#define RTWN_CHIP_92C 1U
#define RTWN_CHIP_92C_1T2R 2U
#define RTWN_CHIP_UMC 4U
#define RTWN_CHIP_UMC_A_CUT 8U
#define RTWN_CHIP_88C 0x10U
#define RTWN_CHIP_23A 0x80U
#define RTWN_CHIP_88E 0x20U
#define RTWN_CHIP_88F 0x200U
#define RTWN_CHIP_PCI 0x40000000U
#define RTWN_CHIP_USB 0x80000000U
#define RTWN_FLAG_FORCE_RAID_11B 4
#define RTWN_FLAG_EXT_HDR 8
#define IEEE80211_RATE_VAL 0x7f
#define IEEE80211_RATE_BASIC 0x80
#define IEEE80211_MODE_11B 1
#define IEEE80211_MODE_11G 2
#ifndef htole32
#define htole32(x) (x)
#endif
#include "vendor/rtwn/r92creg.h"
#include "rtw/RTL8192CDataPath.h"
#define IEEE80211_HTCAP_SGI20 0x20U
#define IEEE80211_NODE_HT 0x400U
#define IEEE80211_NODE_HT_SGI20 0x4000U
#define IEEE80211_HTCAP_SMPS_MASK 0x0cU
#define IEEE80211_HTCAP_SMPS_SHIFT 2U
#define IEEE80211_HTCAP_SMPS_DIS 3U
struct ieee80211_rateset { uint8_t rs_nrates=0,rs_rates[32]{}; };
struct ieee80211_node { ieee80211_rateset ni_rates;int ni_txrate=99;uint32_t ni_flags=0;uint16_t ni_htcaps=0;uint8_t ni_rxmcs[2]{}; };
struct ieee80211com { ieee80211_node *ic_bss=nullptr;int ic_curmode=IEEE80211_MODE_11G;uint16_t ic_htcaps=0; };
struct device { const char *dv_xname="legacy-ra"; };
struct Write { unsigned width;uint16_t address;uint32_t value; };
struct rtwn_softc {
 uint32_t chip=RTWN_CHIP_USB,sc_flags=0;int fwcur=0,error=0;
 unsigned ntxchains=1;rtw_92e_rate_config rate_config{};
 uint32_t sys_cfg=0,hpon_fsm=0;
 unsigned fail_write=0;std::vector<Write> writes;ieee80211com sc_ic;
 device dev;device*sc_pdev=&dev;
};
int rtwn_io_error(rtwn_softc*s){return s->error;}
uint8_t rtwn_read_1(rtwn_softc*,uint16_t){return 0;}
uint32_t rtwn_read_4(rtwn_softc*s,uint16_t a){return a==R92C_SYS_CFG?s->sys_cfg:a==R92C_HPON_FSM?s->hpon_fsm:0;}
void record(rtwn_softc*s,unsigned width,uint16_t address,uint32_t value){
 s->writes.push_back({width,address,value});if(s->writes.size()==s->fail_write)s->error=EIO;
}
void rtwn_write_1(rtwn_softc*s,uint16_t a,uint8_t v){record(s,1,a,v);}
void rtwn_write_2(rtwn_softc*s,uint16_t a,uint16_t v){record(s,2,a,v);}
void rtwn_write_4(rtwn_softc*s,uint16_t a,uint32_t v){record(s,4,a,v);}
void rtwn_delay(rtwn_softc*,uint32_t){}
int rtwn_r92e_ra_init(rtwn_softc*,uint32_t,int){std::abort();}
int rtwn_r92c_ra_init(rtwn_softc*,uint8_t,uint32_t,int,uint32_t,int);
int rtwn_r88e_ra_init(rtwn_softc*,uint8_t,uint32_t,int,uint32_t,int);
'''
main=r'''
static void expected(const Write&w,unsigned width,uint16_t a,uint32_t v){assert(w.width==width);assert(w.address==a);assert(w.value==v);}
int main(){
 // 8188EU I-cut has a different firmware/radio/rate-control contract.
 for(uint32_t cfg:{UINT32_MAX,uint32_t(8U<<12),uint32_t(R92C_SYS_CFG_TRP_VAUX_EN)}){
  rtwn_softc s;s.chip|=RTWN_CHIP_88E;s.sys_cfg=cfg;assert(rtwn_read_chipid(&s)!=0);assert(s.writes.empty());}
 {rtwn_softc s;s.chip|=RTWN_CHIP_88E;assert(rtwn_read_chipid(&s)==0);assert(s.sc_flags&RTWN_FLAG_EXT_HDR);}
 {rtwn_softc s;s.chip|=RTWN_CHIP_88E;s.error=EIO;assert(rtwn_read_chipid(&s)==EIO);}
 // Linux rtl8xxxu gen1 RAMASK wire layout: cmd,mask_hi16,MACID,mask_lo16.
 {rtwn_softc s;assert(rtwn_r92c_ra_init(&s,4,0xfff,11,1,0)==0);assert(s.writes.size()==6);
  expected(s.writes[0],2,0x88,0x0001);expected(s.writes[1],4,0x1d0,0x84400086);
  expected(s.writes[2],1,0x488,0);expected(s.writes[3],2,0x8a,0x0fff);
  expected(s.writes[4],4,0x1d4,0x80400086);expected(s.writes[5],1,0x484,11);}
 for(unsigned fail:{1U,2U,3U,4U,5U,6U}){rtwn_softc s;s.fail_write=fail;assert(rtwn_r92c_ra_init(&s,4,0xfff,11,1,0)==EIO);}
 {rtwn_softc s;ieee80211_node n;s.sc_ic.ic_bss=&n;assert(rtwn_ra_init(&s)==EINVAL);assert(s.writes.empty());assert(n.ni_txrate==99);}
 {rtwn_softc s;ieee80211_node n;s.sc_ic.ic_bss=&n;n.ni_rates.rs_nrates=255;assert(rtwn_ra_init(&s)==EINVAL);assert(s.writes.empty());}
 {rtwn_softc s;ieee80211_node n;s.sc_ic.ic_bss=&n;n.ni_rates.rs_nrates=1;n.ni_rates.rs_rates[0]=127;assert(rtwn_ra_init(&s)==EINVAL);assert(s.writes.empty());}
 {rtwn_softc s;ieee80211_node n;s.sc_ic.ic_bss=&n;n.ni_rates.rs_nrates=3;n.ni_rates.rs_rates[0]=0x80|12;n.ni_rates.rs_rates[1]=24;n.ni_rates.rs_rates[2]=48;
  assert(rtwn_ra_init(&s)==0);expected(s.writes[1],4,0x1d0,0x84500086);expected(s.writes[4],4,0x1d4,0x80500086);assert(n.ni_txrate==2);}
 {rtwn_softc s;ieee80211_node n;s.sc_ic.ic_bss=&n;s.chip|=RTWN_CHIP_88E;n.ni_rates.rs_nrates=2;n.ni_rates.rs_rates[0]=0x80|2;n.ni_rates.rs_rates[1]=4;
  assert(rtwn_ra_init(&s)==0);assert(n.ni_txrate==0);assert(s.sc_flags&RTWN_FLAG_FORCE_RAID_11B);assert(s.writes.size()==2);}
 {rtwn_softc s;s.chip|=RTWN_CHIP_88E;s.fail_write=1;assert(rtwn_r88e_ra_init(&s,4,0xfff,11,1,0)==EIO);}
 puts("Legacy rate control: literal gen1 RAMASK wire, BC/BSS failures, empty/invalid rates, pure-G and 88E AMRR passed");
}
'''
code=pre+'\nint\n'+body('rtwn_read_chipid')+'\nint\n'+body('rtwn_fw_cmd')+'\nint\n'+body('rtwn_r92c_ra_init')+'\nint\n'+body('rtwn_r88e_ra_init')+'\nint\n'+body('rtwn_ra_init')+main
with tempfile.TemporaryDirectory(prefix='rtwn-legacy-ra-') as temp:
    path=Path(temp);(path/'test.cpp').write_text(code)
    subprocess.run(_host_cxx + ['-std=c++17','-g','-O1',*_host_sanitizers,'-fno-omit-frame-pointer','-I'+str(project),'-I'+str(project/'include'),str(path/'test.cpp'),'-x','c++',str(project/'src/RTL8192CDataPath.c'),str(project/'src/USBPackets.c'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
