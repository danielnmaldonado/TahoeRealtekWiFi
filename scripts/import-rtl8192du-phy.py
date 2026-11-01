#!/usr/bin/env python3
"""Extract the pinned DU PHY/ROM-power algorithm closure without Linux runtime.

Input is an rtlwifi reference directory, never arbitrary build instructions.
No network, hardware, installation or generated source execution occurs here.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re

COMMIT = '0e135910deec43ffe97eca0b6af1f8f847285b59'
ROOTS = [
    'rtl92du_phy_query_bb_reg', 'rtl92du_phy_set_bb_reg',
    'rtl92d_phy_query_rf_reg', 'rtl92d_phy_set_rf_reg',
    'rtl92du_phy_mac_config', 'rtl92du_phy_bb_config', 'rtl92du_phy_rf_config',
    'rtl92du_phy_set_bw_mode', 'rtl92du_phy_sw_chnl',
    'rtl92du_phy_iq_calibrate', 'rtl92du_phy_lc_calibrate',
    'rtl92du_phy_init_pa_bias', 'rtl92d_phy_get_hw_reg_originalvalue',
    '_rtl92du_phy_set_rfsleep',
    '_rtl92d_read_txpower_info',
    'rtl92d_dm_initialize_txpower_tracking',
    'rtl92d_dm_check_txpower_tracking_thermal_meter',
]
SOURCES = ['rtl8192du/phy.c', 'rtl8192du/rf.c', 'rtl8192d/phy_common.c',
           'rtl8192d/rf_common.c', 'rtl8192d/hw_common.c', 'rtl8192d/dm_common.c']
INPUTS = SOURCES+['rtl8192d/reg.h', 'rtl8192d/def.h', 'rtl8192du/table.c',
    'rtl8192du/table.h', 'rtl8192d/phy_common.h', 'rtl8192d/dm_common.h',
    'wifi.h', 'core.c']

def mask(text):
    return re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',
                  lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]),
                  text, flags=re.S)

def functions(text):
    masked = mask(text)
    pattern = r'(?m)^(?:(?:static|const|inline)\s+)*(?:void|bool|int|long|u8|u16|u32|enum\s+\w+)\s+(\w+)\s*\([^;{}]*\)\s*\{'
    for match in re.finditer(pattern, masked):
        depth, end = 1, match.end()
        while depth:
            depth += (masked[end] == '{') - (masked[end] == '}')
            end += 1
        yield match[1], text[match.start():end], masked[match.start():end], match.start()

def without_includes(text):
    return re.sub(r'(?m)^#include[^\n]*\n', '', text)

def generate(reference, output):
    provenance = Path(__file__).resolve().parents[1]/'src/RTL8192DUPHY-Upstream.json'
    expected = json.loads(provenance.read_text())
    if expected['commit'] != COMMIT:
        raise ValueError('Unexpected DU reference commit')
    hashes = {item['path']: item['sha256'] for item in expected['sources']}
    for path in INPUTS:
        if hashlib.sha256((reference/path).read_bytes()).hexdigest() != hashes[path]:
            raise ValueError('Pinned DU reference differs: '+path)
    output.mkdir(parents=True, exist_ok=True)
    catalog = {}
    texts = {path: (reference/path).read_text() for path in SOURCES}
    for path, text in texts.items():
        for name, body, masked, _ in functions(text):
            catalog[name] = (path, body, masked)
    closure, pending = set(), ROOTS[:]
    while pending:
        name = pending.pop()
        if name in closure:
            continue
        path, body, masked = catalog[name]
        closure.add(name)
        pending.extend(n for n in re.findall(r'\b(\w+)\s*\(', masked)
                       if n in catalog and n not in closure)
    banner = '/* SPDX-License-Identifier: GPL-2.0-only */\n/* Generated from Linux '+COMMIT+'; do not edit by hand. */\n'
    prototypes, definitions = [], []
    for name in sorted(closure):
        path, body, _ = catalog[name]
        if not body.startswith('static '):
            body = 'static '+body
        body = body.replace('= {};', '= {0};')  # C11 zero initialization
        # A delay/USB fault permanently fences the invocation. Timeout must
        # not be transformed into a nominal LC calibration success.
        if name == '_rtl92du_phy_lc_calibrate_sw':
            needle = '\t\tRTPRINT(rtlpriv, FINIT, INIT_IQK,\n\t\t\t"PHY_LCK finish delay for %d ms=2\\n", timecount);'
            assert needle in body
            body = body.replace(needle, '\t\tif (!(u4tmp & BIT(11))) du_algorithm_failure(hw, RTW_92DU_POLL_TIMEOUT);\n'+needle)
        if name == 'rtl92du_phy_iq_calibrate':
            needle = '\t\tpatha_ok = true;\n\t\tpathb_ok = true;'
            assert needle in body
            body = body.replace(needle, needle+'\n\t\tif (!rege94 || (IS_92D_SINGLEPHY(rtlhal->version) && !regeb4))\n\t\t\tdu_algorithm_failure(hw, RTW_92DU_POLL_TIMEOUT);')
        if name in ('_rtl92du_phy_patha_iqk_5g_normal', '_rtl92du_phy_pathb_iqk_5g_normal'):
            # Original code carries on after the ready/result deadline. A
            # stale nonzero result must not turn that timeout into success.
            body, count = re.subn(r'(while \(timecount < timeout &&.*?\n\t\t\})',
                r'\1\n\t\tif (timecount >= timeout) du_algorithm_failure(hw, RTW_92DU_POLL_TIMEOUT);',
                body, flags=re.S)
            assert count == 2
        if name == '_rtl92d_phy_rf_serial_read':
            body = body.replace('\treturn retvalue;', '\tdu_rf_sample(hw, rfpath, offset, retvalue);\n\treturn retvalue;')
        if name == '_rtl92d_get_txpower_index':
            body = body[:-1]+'\telse {\n\t\tofdmpowerlevel[0] = ofdmpowerlevel[1] = 0;\n\t\tdu_algorithm_failure(hw, RTW_92DU_BAD_ARGUMENT);\n\t}\n}'
        if name == '_rtl92du_phy_set_rfsleep':
            body = body.replace('if (retry == 0) {', 'if (u4btmp != 0) {\n\t\tdu_algorithm_failure(hw, RTW_92DU_POLL_TIMEOUT);\n\t\treturn;')
        if name == 'rtl92d_phy_rf6052_set_cck_txpower':
            body = body.replace('\tu8 *ptr;\n', '')
            old = '''\t\tptr = (u8 *)(&tx_agc[idx1]);
\t\tfor (idx2 = 0; idx2 < 4; idx2++) {
\t\t\tif (*ptr > RF6052_MAX_TX_PWR)
\t\t\t\t*ptr = RF6052_MAX_TX_PWR;
\t\t\tptr++;
\t\t}'''
            new = '''\t\tu32 clamped = 0;
\t\tfor (idx2 = 0; idx2 < 4; idx2++) {
\t\t\tu32 byte = (tx_agc[idx1] >> (idx2 * 8)) & 0xff;
\t\t\tif (byte > RF6052_MAX_TX_PWR) byte = RF6052_MAX_TX_PWR;
\t\t\tclamped |= byte << (idx2 * 8);
\t\t}
\t\ttx_agc[idx1] = clamped;'''
            assert old in body
            body = body.replace(old, new)
        if name == 'rtl92d_bandtype_2_4G':
            body = body.replace('le32_to_cpu(*((__le32 *)cckswing))', 'du_get_le32(cckswing)')
            body = body.replace('u8 *cck_index_old', 's8 *cck_index_old')
        if name == 'rtl92d_dm_rxgain_tracking_thermalmeter':
            body = body.replace('\tu4tmp = index_mapping[idx] << 12;',
                '\tif (idx < 0) idx = 0;\n\tif (idx >= RX_INDEX_MAPPING_NUM) idx = RX_INDEX_MAPPING_NUM - 1;\n\tu4tmp = index_mapping[idx] << 12;')
        if name == 'rtl92d_bandtype_5G':
            body = body.replace('u8 *ofdm_index', 'int *ofdm_index')
        if name == 'rtl92d_dm_txpower_tracking_callback_thermalmeter':
            body = body.replace('u8 ofdm_index[2];', 'int ofdm_index[2];')
        prototypes.append(body[:body.index('{')].strip()+';\n')
        definitions.append('\n/* '+path+': '+name+' */\n'+body+'\n')
    prefix = ''
    for path in ['rtl8192du/phy.c', 'rtl8192d/phy_common.c', 'rtl8192d/dm_common.c']:
        first = next(functions(texts[path]))[3]
        prefix += without_includes(texts[path][:first])+'\n'
    (output/'RTL8192DUAlgorithms.inc').write_text(banner+prefix+'\n'.join(prototypes+definitions))
    (output/'RTL8192DURegisters.inc').write_text(banner+(reference/'rtl8192d/reg.h').read_text())
    (output/'RTL8192DUDefinitions.inc').write_text(banner+(reference/'rtl8192d/def.h').read_text())
    table = without_includes((reference/'rtl8192du/table.c').read_text())
    table = re.sub(r'(?m)^const u32 ', 'static const u32 ', table)
    header = '\n'.join(line for line in (reference/'rtl8192du/table.h').read_text().splitlines()
                       if line.startswith('#define ') and '__RTL' not in line)+'\n'
    (output/'RTL8192DUTables.inc').write_text(banner+header+table)
    data = {'commit': COMMIT, 'license': 'GPL-2.0-only',
            'functions': sorted(closure), 'sources': []}
    for path in INPUTS:
        data['sources'].append({'path': path,
            'sha256': hashlib.sha256((reference/path).read_bytes()).hexdigest(),
            'url': 'https://raw.githubusercontent.com/torvalds/linux/'+COMMIT+
                   '/drivers/net/wireless/realtek/rtlwifi/'+path})
    (output/'RTL8192DUPHY-Upstream.json').write_text(json.dumps(data, indent=2)+'\n')
    print(f'Imported {len(closure)} DU PHY/ROM-power functions and ten source tables')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--out', type=Path, default=Path(__file__).resolve().parents[1]/'src')
    args = parser.parse_args()
    generate(args.reference, args.out)
