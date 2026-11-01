#!/usr/bin/env python3
"""Run actual imported eFuse routines in a host register simulator, without USB."""
from pathlib import Path
import subprocess, tempfile
project = Path(__file__).resolve().parents[3]
import runpy
_host = runpy.run_path(str(project / "scripts/host_cpp.py"))
_host_cxx = _host["compiler"]()
_host_sanitizers = _host["sanitizer_flags"]()
source = (project / 'vendor/rtwn/rtwn.cpp').read_text()
def body(name):
    start = source.index('\n' + name + '(') + 1
    brace = source.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]
pre = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <vector>
#define __packed __attribute__((__packed__))
#define IEEE80211_ADDR_LEN 6
#define nitems(a) (sizeof(a)/sizeof(a[0]))
#define explicit_bzero(p,n) memset(p,0,n)
#define DPRINTF(x) do {} while(0)
#define IEEE80211_ADDR_COPY(a,b) memcpy(a,b,6)
#include "rtw/Efuse.h"
#include "vendor/rtwn/r92creg.h"
#define RTWN_CHIP_92C 1
#define RTWN_CHIP_88C 0x10
#define RTWN_CHIP_88E 0x20
#define RTWN_CHIP_92E 0x40
#define RTWN_CHIP_88F 0x200
struct device { const char *dv_xname; };
struct ieee80211com { uint8_t ic_myaddr[6]={}; };
struct rtwn_softc {
    uint32_t chip=RTWN_CHIP_92E;
    int core_error=0,timeout=-1,current=0,largest=-1;
    bool access=false;
    device dev={"sim"};device *sc_pdev=&dev;
    uint8_t physical[512];
    r92e_rom sc_r92e_rom;
    ieee80211com sc_ic;
    uint8_t crystal_cap=0,regulatory=0;
};
int rtwn_io_error(rtwn_softc*s){return s->core_error;}
void rtwn_delay(rtwn_softc*,uint32_t){}
uint32_t rtwn_read_4(rtwn_softc*s,uint16_t a){
 if(a==R92C_EFUSE_CTRL){
  if(s->current==s->timeout)return 0;
  return R92C_EFUSE_CTRL_VALID|s->physical[s->current];
 }return 0;
}
uint16_t rtwn_read_2(rtwn_softc*,uint16_t){return 0;}
void rtwn_write_4(rtwn_softc*s,uint16_t a,uint32_t v){
 if(a==R92C_EFUSE_CTRL){s->current=MS(v,R92C_EFUSE_CTRL_ADDR);assert(s->current<512);if(s->current>s->largest)s->largest=s->current;}
}
void rtwn_write_2(rtwn_softc*,uint16_t,uint16_t){}
void rtwn_write_1(rtwn_softc*s,uint16_t a,uint8_t v){if(a==R92C_EFUSE_ACCESS)s->access=v==R92C_EFUSE_ACCESS_ON;}
void rtwn_efuse_switch_power(rtwn_softc*);
'''
main = r'''
static std::vector<uint8_t> pack(const r92e_rom&rom){
 const uint8_t*p=(const uint8_t*)&rom;std::vector<uint8_t> out;
 for(unsigned block=0;block<64;block++){
  unsigned mask=15;for(unsigned w=0;w<4;w++)if(p[block*8+w*2]!=255||p[block*8+w*2+1]!=255)mask&=~(1U<<w);
  if(mask==15)continue;
  if(block<16)out.push_back((uint8_t)((block<<4)|mask));
  else {out.push_back((uint8_t)(((block&7)<<5)|15));out.push_back((uint8_t)(((block&0x78)<<1)|mask));}
  for(unsigned w=0;w<4;w++)if(!(mask&(1U<<w))){out.push_back(p[block*8+w*2]);out.push_back(p[block*8+w*2+1]);}
 }out.push_back(255);return out;
}
static void valid(rtwn_softc&s){
 r92e_rom r;memset(&r,255,sizeof(r));r.id=0x8129;r.macaddr[0]=2;for(int i=1;i<6;i++)r.macaddr[i]=i;
 for(auto*p:{&r.txpwr_a,&r.txpwr_b}){for(int i=0;i<4;i++){p->cck_tx_pwr[i]=0x20;p->ht40_tx_pwr[i]=0x20;}p->ht20_ofdm_tx_pwr_diff=2;}
 r.xtal_k=0x20;r.rf_board_opt=0;auto data=pack(r);assert(data.size()<512);memset(s.physical,255,512);memcpy(s.physical,data.data(),data.size());
}
int main(){
 {rtwn_softc s;valid(s);rtwn_r92e_read_rom(&s);assert(s.core_error==0);assert(!s.access);assert(s.largest==511);assert(s.sc_ic.ic_myaddr[0]==2);}
 {rtwn_softc s;valid(s);s.timeout=511;rtwn_r92e_read_rom(&s);assert(s.core_error==ETIMEDOUT);assert(!s.access);assert(s.sc_r92e_rom.id==0xffff);assert(s.sc_ic.ic_myaddr[0]==0);}
 {rtwn_softc s;memset(s.physical,255,512);s.physical[0]=0xef;s.physical[1]=0xfe;s.physical[2]=1;s.physical[3]=2;rtwn_r92e_read_rom(&s);assert(s.core_error==EINVAL);assert(!s.access);}
 {rtwn_softc s;valid(s);uint8_t logical[512];assert(rtw_efuse_decode(s.physical,512,logical,512,nullptr)==RTW_EFUSE_OK);r92e_rom r;memcpy(&r,logical,512);r.txpwr_a.cck_tx_pwr[0]=255;auto data=pack(r);memset(s.physical,255,512);memcpy(s.physical,data.data(),data.size());rtwn_r92e_read_rom(&s);assert(s.core_error==EINVAL);assert(!s.access);}
 puts("eFuse core: valid map, extended-header bounds, late timeout, gate cleanup and erased calibration regressions passed");
}
'''
code = pre + '\nuint8_t\n' + body('rtwn_efuse_read_1') + '\nvoid\n' + body('rtwn_efuse_switch_power') + '\nvoid\n' + body('rtwn_efuse_read') + '\nvoid\n' + body('rtwn_r92e_read_rom') + main
with tempfile.TemporaryDirectory(prefix='rtwn-efuse-') as temp:
    path=Path(temp);(path/'test.cpp').write_text(code)
    subprocess.run(_host_cxx + ['-x','c++','-std=c++17','-Wno-deprecated-declarations','-I'+str(project),'-I'+str(project/'include'),str(path/'test.cpp'),str(project/'src/Efuse.c'),str(project/'src/USBRegisterIO.c'),str(project/'src/USBControl.c'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
