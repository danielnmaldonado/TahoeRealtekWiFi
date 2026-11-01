#!/usr/bin/env python3
"""Compile the actual gen1 codec and independent wire fixtures with ASan/UBSan."""
from pathlib import Path
from host_cpp import compiler, sanitizer_flags
import argparse
import subprocess


def main():
    project = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=project / "build/gen1-codec")
    parser.add_argument("--gen2-baseline", action="store_true",
                        help="Demonstrate the gen1 fixture rejects the existing gen2 codec")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    source = project / "src/RTL8192CDataPath.c"
    sources = [source, project / "src/USBPackets.c"]
    if args.gen2_baseline:
        source = args.out / "gen2_baseline.c"
        source.write_text('''#include "rtw/RTL8192CDataPath.h"
#include <string.h>
int rtw_92c_make_tx_descriptor(size_t n,const rtw_92e_tx_options *o,uint8_t *d) {
 uint8_t gen2[40];int r=rtw_92e_make_tx_descriptor_ex(n,o,gen2);if(r)memcpy(d,gen2,32);return r;
}
int rtw_92c_tx_protection(unsigned m,uint8_t *d) {uint8_t g[40]={0};memcpy(g,d,32);int r=rtw_92e_tx_protection(m,g);if(r)memcpy(d,g,32);return r;}
int rtw_92c_tx_report_request(int chip,int req,uint8_t *d) {(void)chip;uint8_t g[40]={0};memcpy(g,d,32);int r=rtw_92e_tx_report_request(req,g);if(r)memcpy(d,g,32);return r;}
rtw_packet_error rtw_92c_rx_next(rtw_rx_iterator *it,int chip,rtw_92c_rx_frame *f) {(void)chip;memset(f,0,sizeof(*f));return rtw_rx_next(it,&f->packet);}
int rtw_88e_parse_tx_report(const uint8_t *b,size_t n,rtw_88e_tx_report *r) {memset(r,0,sizeof(*r));return rtw_92e_parse_tx_report(b,n,&r->report);}
''')
        sources = [source, project / "src/USBPackets.c", project / "src/RTL8192EUDataPath.c"]
    objects = []
    common = ["-g", "-O1", *sanitizer_flags(), "-fno-omit-frame-pointer",
              "-Wall", "-Wextra", "-Werror", "-I" + str(project / "include")]
    for index, src in enumerate(sources):
        obj = args.out / (str(index) + ".o")
        subprocess.run([*compiler("c"), "-std=c11", *common, "-c", str(src), "-o", str(obj)], check=True)
        objects.append(obj)
    binary = args.out / "gen1-datapath-tests"
    subprocess.run([*compiler(), "-std=c++17", *common,
                    str(project / "tests/rtl8192c_datapath_tests.cpp"),
                    *map(str, objects), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
