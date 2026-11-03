#!/usr/bin/env python3
"""Run actual station RX filters for all13 families and shared rtwn HT traces."""
from pathlib import Path
import argparse
import re
import subprocess

def function(source,name):
    position=source.index("\n"+name+"(")+1
    start=source.rfind("\n",0,position-1)+1
    opening=source.index("{",position)
    depth=0
    for token in re.finditer(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]',source[opening:]):
        if token.group()=="{": depth+=1
        elif token.group()=="}":
            depth-=1
            if depth==0: return source[start:opening+token.end()]+"\n"
    raise ValueError(name)

def private_filters(project,out):
    """Reuse established hardware fixtures; compile the actual private MACs.

    Low RCR bits share primary definitions; bit12 deliberately is not shared.
    AU subtype writes are extracted from its real controller entry point.
    """
    from host_cpp import compiler,sanitizer_flags,dead_strip_flags
    out=out/"family-filters";out.mkdir(parents=True,exist_ok=True)
    common=["-g","-O1","-UNDEBUG",*sanitizer_flags(),"-fno-omit-frame-pointer",
            "-Wall","-Wextra","-Werror","-Wno-unused-function",
            "-I"+str(project/"include"),"-I"+str(project/"tests")]
    bus=["Efuse","USBControl","USBRegisterIO"]
    cases=[
        (1,"8192fu",None,None,["RTL8192FU",*bus]),
        (2,"8710bu",None,None,["RTL8710BU",*bus]),
        (3,"8723bu","rtl8723bu_core_tests.cpp","static void read_factory_from_otp",["RTL8723BU"]),
        (4,"8192du","rtl8192du_station_tests.cpp","static void checked_cleanup",
            ["RTL8192DUStation","RTL8192DUBoot","RTL8192DUPHY","RTL8192DUUSB",
             "RTL8192DUEfuse","USBWiFiProfiles","FamilyBackend","USBDescriptors",*bus]),
        (5,"8812au-8821au","rtl8812au_bootstrap_tests.cpp","static void happy()",
            ["RTL8812AUBootstrap","RTL8812AUMAC","RTL8812AUPHY",*bus]),
        (6,"8821cu-8822bu-8822cu","rtw88_station_tests.cpp","static constexpr rtw_wifi_family families",["RTW88Station","USBWiFiProfiles"]),
        (7,"8814au","rtl8814au_station_tests.cpp","int main()",["RTL8814AUBoot","RTL8814AUStation"])]
    objects={}
    for kind,name,fixture,stop,sources in cases:
        folder=out/name;folder.mkdir(exist_ok=True)
        prelude=(project/"tests"/fixture).read_text().split(stop,1)[0] if fixture else ""
        prelude=prelude.replace('#include "../kext/','#include "'+str(project/"kext")+'/')
        if kind==5:
            controller=(project/"kext/RTW8812Backend.inc").read_text()
            maps=re.search(r'e=rtw_r12au_write\(&auCore_,0x6a2,2,run&&auOperating_\.mode[^\n]+',controller).group()
            prelude+='\nstatic int actual_au_receive_maps(rtw_r12au_boot *core,bool run,bool ht) {'\
                'auto &auCore_=*core;struct {int mode;} auOperating_{ht?RTW_R12AU_MODE_HT:RTW_R12AU_MODE_LEGACY};int e;'+maps+'return e;}\n'
        header=folder/"fixture.hpp";header.write_text(prelude)
        selected=[]
        for source in sources:
            if source not in objects:
                obj=out/(source+".o");objects[source]=obj
                subprocess.run([*compiler("c"),"-std=c11",*common,"-ffunction-sections","-fdata-sections",
                    "-c",str(project/"src"/(source+".c")),"-o",str(obj)],check=True)
            selected.append(str(objects[source]))
        binary=folder/"filters"
        subprocess.run([*compiler(),"-std=c++17",*common,"-DRTW_PRIVATE_FILTER="+str(kind),
            '-DRTW_FILTER_FIXTURE="'+str(header)+'"',str(project/"tests/rtwn_ht_tests.cpp"),
            *selected,*dead_strip_flags(),"-o",str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

def main():
    from host_cpp import compiler, sanitizer_flags
    project=Path(__file__).resolve().parents[1]
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out",type=Path,default=project/"out-rtwn-ht")
    args=parser.parse_args();args.out=args.out.resolve()
    args.out.mkdir(parents=True,exist_ok=True)
    source=(project/"vendor/rtwn/rtwn.cpp").read_text()
    closure='#include "rtwn_ht_host.hpp"\n'
    # Exercise actual attach/init policy without replacing either assignment
    # with a test-owned station/MAC implementation.
    default_mode=re.search(r'ic->ic_opmode = IEEE80211_M_STA;[^\n]*',source).group()
    mac_loop=re.search(r'for \(i = 0; i < IEEE80211_ADDR_LEN; i\+\+\)\s*rtwn_write_1\(sc, R92C_MACID \+ i, ic->ic_myaddr\[i\]\);',source).group()
    closure+='void actual_station_defaults(rtwn_softc *sc) {auto *ic=&sc->sc_ic;int i;'+default_mode+'\n'+mac_loop+'}\n'
    extension=project/"vendor/rtwn/r92e_ht.inc"
    if extension.exists(): closure+=extension.read_text()+"\n"
    for name in ("rtwn_ht_capabilities","rtwn_chan2group","rtwn_r92e_get_txpower","rtwn_read_chipid","rtwn_r92c_ra_init","rtwn_r88e_ra_init","rtwn_ra_init","rtwn_set_chan","rtwn_r92e_run_extension","rtwn_rxfilter_init","rtwn_newstate"): closure+=function(source,name)
    generated=args.out/"actual_ht.cpp";generated.write_text(closure)
    command=[*compiler(),"-std=c++17","-g","-O1","-x","c++",*sanitizer_flags(),"-fno-omit-frame-pointer","-Wall","-Wextra","-Werror","-Wno-sign-compare","-Wno-unused-parameter","-I"+str(project),"-I"+str(project/"include"),"-I"+str(project/"tests"),str(generated),str(project/"tests/rtwn_ht_tests.cpp")]
    portable=project/"src/RTL8192EUHT.c"
    if portable.exists(): command.append(str(portable))
    command += [str(project/"src/RTL8192CDataPath.c"),str(project/"src/USBPackets.c")]
    command += ["-o",str(args.out/"rtwn-ht-tests")]
    subprocess.run(command,check=True)
    subprocess.run([str(args.out/"rtwn-ht-tests")],check=True)
    private_filters(project,args.out)

if __name__=="__main__": main()
