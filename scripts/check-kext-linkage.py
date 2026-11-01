#!/usr/bin/env python3
"""Reject unresolved private driver symbols; Apple KPI imports remain external."""
import argparse
import re
import subprocess
import sys


def unresolved_driver_symbols(output):
    symbols = [line.split()[-1] for line in output.splitlines() if line.split()]
    return sorted(set(symbol for symbol in symbols if
        re.match(r'^_(?:rtw_|rtwn_|ieee80211_)', symbol) or
        (symbol.startswith('__Z') and re.search(r'\dRTW[A-Za-z]', symbol))))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable')
    args = parser.parse_args()
    result = subprocess.run(['nm', '-u', args.executable], capture_output=True, text=True)
    if result.returncode:
        sys.stderr.write(result.stderr)
        return 1
    missing = unresolved_driver_symbols(result.stdout)
    if missing:
        print('ERROR: driver implementations missing from kext: ' + ', '.join(missing), file=sys.stderr)
        return 1
    print('Kext linkage: no unresolved private driver symbols')
    return 0


if __name__ == '__main__':
    sys.exit(main())
