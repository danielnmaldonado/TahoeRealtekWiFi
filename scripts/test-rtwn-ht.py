#!/usr/bin/env python3
"""Run the actual rtwn rate/HT functions against offline firmware register traces."""
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

def main():
    from host_cpp import compiler, sanitizer_flags
    project=Path(__file__).resolve().parents[1]
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out",type=Path,default=project/"out-rtwn-ht")
    args=parser.parse_args();args.out.mkdir(parents=True,exist_ok=True)
    source=(project/"vendor/rtwn/rtwn.cpp").read_text()
    closure='#include "rtwn_ht_host.hpp"\n'
    extension=project/"vendor/rtwn/r92e_ht.inc"
    if extension.exists(): closure+=extension.read_text()+"\n"
    for name in ("rtwn_ht_capabilities","rtwn_chan2group","rtwn_r92e_get_txpower","rtwn_read_chipid","rtwn_r92c_ra_init","rtwn_r88e_ra_init","rtwn_ra_init","rtwn_set_chan","rtwn_r92e_run_extension","rtwn_newstate"): closure+=function(source,name)
    generated=args.out/"actual_ht.cpp";generated.write_text(closure)
    command=[*compiler(),"-std=c++17","-g","-O1","-x","c++",*sanitizer_flags(),"-fno-omit-frame-pointer","-Wall","-Wextra","-Werror","-Wno-sign-compare","-Wno-unused-parameter","-I"+str(project),"-I"+str(project/"include"),"-I"+str(project/"tests"),str(generated),str(project/"tests/rtwn_ht_tests.cpp")]
    portable=project/"src/RTL8192EUHT.c"
    if portable.exists(): command.append(str(portable))
    command += [str(project/"src/RTL8192CDataPath.c"),str(project/"src/USBPackets.c")]
    command += ["-o",str(args.out/"rtwn-ht-tests")]
    subprocess.run(command,check=True)
    subprocess.run([str(args.out/"rtwn-ht-tests")],check=True)

if __name__=="__main__": main()
