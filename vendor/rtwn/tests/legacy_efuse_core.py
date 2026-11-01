#!/usr/bin/env python3
"""Exercise actual CU/88E eFuse readers under ASan/UBSan, with no hardware."""
from pathlib import Path
import ast
import subprocess
import tempfile

project = Path(__file__).resolve().parents[3]
import runpy
_host = runpy.run_path(str(project / "scripts/host_cpp.py"))
_host_cxx = _host["compiler"]()
_host_sanitizers = _host["sanitizer_flags"]()
source = (project / 'vendor/rtwn/rtwn.cpp').read_text()
fixture = ast.parse((Path(__file__).with_name('efuse_core.py')).read_text())
pre = next(ast.literal_eval(n.value) for n in fixture.body
           if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'pre' for t in n.targets))
pre = pre.replace('bool access=false;', 'bool access=false;unsigned access_writes=0;')
pre = pre.replace('s->access=v==R92C_EFUSE_ACCESS_ON;', '++s->access_writes;s->access=v==R92C_EFUSE_ACCESS_ON;')

def body(name):
    start = source.index('\n' + name + '(') + 1
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

main = r'''
int main(){
 for(uint32_t chip:{uint32_t(RTWN_CHIP_92C),uint32_t(RTWN_CHIP_88C),uint32_t(RTWN_CHIP_88E)}){
  rtwn_softc s;s.chip=chip;memset(s.physical,255,sizeof(s.physical));
  // A legacy CU empty-word header must not consume the next record as EXT.
  const uint8_t raw[]={0x0f,0x2e,0x29,0x81,0xff};memcpy(s.physical,raw,sizeof(raw));
  uint8_t map[512];const size_t size=chip==RTWN_CHIP_88E?512:128;
  rtwn_efuse_read(&s,map,size);assert(s.core_error==0);assert(!s.access);assert(s.access_writes==(chip==RTWN_CHIP_88E?2U:0U));
  assert(s.largest==(chip==RTWN_CHIP_88E?255:511));
  assert(map[chip==RTWN_CHIP_88E?128:16]==0x29);
  assert(map[chip==RTWN_CHIP_88E?129:17]==0x81);
 }
 {rtwn_softc s;s.chip=RTWN_CHIP_88E;memset(s.physical,255,512);
  s.physical[255]=0x0e;uint8_t map[512];rtwn_efuse_read(&s,map,sizeof(map));assert(s.core_error==0);}
 {rtwn_softc s;s.chip=RTWN_CHIP_88E;memset(s.physical,0x0f,512);s.physical[255]=0x0e;
  uint8_t map[512];rtwn_efuse_read(&s,map,sizeof(map));assert(s.core_error==EINVAL);assert(!s.access);}
 {rtwn_softc s;s.chip=RTWN_CHIP_88E;memset(s.physical,255,512);s.timeout=255;
  uint8_t map[512];rtwn_efuse_read(&s,map,sizeof(map));assert(s.core_error==ETIMEDOUT);assert(!s.access);for(auto b:map)assert(b==255);}
 {rtwn_softc s;s.chip=RTWN_CHIP_92C;memset(s.physical,255,512);s.timeout=511;
  uint8_t map[128];rtwn_efuse_read(&s,map,sizeof(map));assert(s.core_error==ETIMEDOUT);for(auto b:map)assert(b==255);}
 {rtwn_softc s;s.chip=RTWN_CHIP_92C;memset(s.physical,255,512);s.physical[0]=0xfe;s.physical[1]=1;s.physical[2]=2;
  uint8_t map[127];rtwn_efuse_read(&s,map,sizeof(map));assert(s.core_error==EINVAL);}
 puts("Legacy eFuse: CU no-EXT/map bounds, 88E physical bound/EXT, late failures and cleanup passed");
}
'''
code = pre + '\nuint8_t\n' + body('rtwn_efuse_read_1') + '\nvoid\n' + body('rtwn_efuse_switch_power') + '\nvoid\n' + body('rtwn_efuse_read') + main
with tempfile.TemporaryDirectory(prefix='rtwn-legacy-efuse-') as temp:
    path=Path(temp);(path/'test.cpp').write_text(code)
    subprocess.run(_host_cxx + ['-x','c++','-std=c++17','-g','-O1',*_host_sanitizers,'-fno-omit-frame-pointer','-Wno-deprecated-declarations','-I'+str(project),'-I'+str(project/'include'),str(path/'test.cpp'),str(project/'src/Efuse.c'),str(project/'src/USBRegisterIO.c'),str(project/'src/USBControl.c'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
