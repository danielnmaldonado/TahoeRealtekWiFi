#!/usr/bin/env python3
"""Compile pinned Linux channel/ROM/thermal algorithms to produce independent PHY goldens.

This never loads a driver or accesses hardware. The production port is not
imported: source functions, structs, constants and tables come from --source.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile
from host_cpp import compiler

ROOT = Path(__file__).resolve().parents[1]
# HT20 centers backed by both pinned RFE0/1 OFDM/HT1S/HT2S limit tables.
# Channel144 has RF programming but no primary limit rows, so it stays out.
FIVE_CHANNELS = (36,40,44,48,52,56,60,64,100,104,108,112,116,120,
                 124,128,132,136,140,149,153,157,161,165)
SPUR_TRANSITIONS = ((153,149),(153,157),(153,52),(153,100),
                    (140,36),(140,100),(140,153),(140,1))



def block(source, marker):
    start = source.index(marker)
    opening = source.index('{', start)
    depth = 0
    for token in re.finditer(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|[{}]', source[opening:]):
        if token.group() == '{': depth += 1
        elif token.group() == '}':
            depth -= 1
            if not depth: return source[start:opening + token.end()]
    raise ValueError(marker)


def macro_definitions(texts, code):
    """Copy the referenced upstream macros, including continued dependencies."""
    available = {}
    for text in texts:
        lines = iter(text.splitlines(keepends=True))
        for line in lines:
            match = re.match(r'#define\s+(\w+)', line)
            if not match:
                continue
            definition = line
            while line.rstrip().endswith('\\'):
                line = next(lines)
                definition += line
            available[match[1]] = definition
    pending = set(re.findall(r'\b[A-Za-z_]\w*\b', code))
    selected = {}
    while pending:
        name = pending.pop()
        if name in selected or name not in available:
            continue
        selected[name] = available[name]
        pending.update(re.findall(r'\b[A-Za-z_]\w*\b', available[name]))
    return ''.join(selected[name] for name in sorted(selected))


def spur_goldens(main_h, chip, register, common):
    """Run the unchanged complete upstream HT20 bandwidth/spur call chain."""
    functions = '\n'.join(block(chip, 'static void ' + name + '(') for name in [
        'rtw8814a_switch_channel', 'rtw8814a_set_bw_reg_adc',
        'rtw8814a_set_bw_reg_agc', 'rtw8814a_set_bw_reg_mac',
        'rtw8814a_set_bw_rf', 'rtw8814a_adc_clk',
        'rtw8814a_spur_calibration_ch140', 'rtw8814a_set_nbi_reg',
        'rtw8814a_nbi_setting', 'rtw8814a_spur_nbi_setting',
        'rtw8814a_spur_calibration', 'rtw8814a_set_bw_mode'])
    source = '''#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;
#define BIT(n) (UINT32_C(1)<<(n))
#define GENMASK(h,l) ((UINT32_MAX<<(l))&(UINT32_MAX>>(31U-(h))))
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define abs_diff(a,b) ((a)>(b)?(a)-(b):(b)-(a))
#define MASKBYTE0 0xffU
/* The Linux nl80211 band ABI values used by rtw_supported_band. */
enum {NL80211_BAND_2GHZ,NL80211_BAND_5GHZ,NL80211_BAND_60GHZ};
enum {RF_PATH_A,RF_PATH_B,RF_PATH_C,RF_PATH_D};
'''
    for marker in ['enum rtw_supported_band ', 'enum rtw_bandwidth ',
                   'enum rtw_sc_offset ', 'enum rtw_chip_ver ']:
        source += block(main_h, marker) + ';\n'
    source += '''/* Only fields accessed by the unchanged functions below. */
struct rtw_hal {u8 rf_path_num,cut_version,current_channel,primary_channel;u32 ch_param[2];};
struct rtw_efuse {u8 rfe_option;};
struct rtw_dev {struct rtw_hal hal;struct rtw_efuse efuse;};
static u8 registers[0x10000];
static u32 rf_registers[4][256];
static u32 read_register(u32 a,unsigned width){
 if(a>sizeof(registers)-width)abort();u32 value=0;
 for(unsigned i=0;i<width;i++)value|=(u32)registers[a+i]<<(8*i);
 return value;
}
static void write_register(u32 a,unsigned width,u32 value){
 if(a>sizeof(registers)-width)abort();
 for(unsigned i=0;i<width;i++)registers[a+i]=(u8)(value>>(8*i));
}
static unsigned mask_shift(u32 mask){
 if(!mask)abort();unsigned shift=0;while(!(mask&1)){mask>>=1;shift++;}return shift;
}
static u8 rtw_read8(struct rtw_dev*d,u32 a){(void)d;return (u8)read_register(a,1);}
static u16 rtw_read16(struct rtw_dev*d,u32 a){(void)d;return (u16)read_register(a,2);}
static u32 rtw_read32(struct rtw_dev*d,u32 a){(void)d;return read_register(a,4);}
static void rtw_write8(struct rtw_dev*d,u32 a,u8 v){(void)d;write_register(a,1,v);}
static void rtw_write16(struct rtw_dev*d,u32 a,u16 v){(void)d;write_register(a,2,v);}
static void rtw_write32(struct rtw_dev*d,u32 a,u32 v){(void)d;write_register(a,4,v);}
static void rtw_write32_set(struct rtw_dev*d,u32 a,u32 bits){
 rtw_write32(d,a,rtw_read32(d,a)|bits);
}
static void rtw_write32_clr(struct rtw_dev*d,u32 a,u32 bits){
 rtw_write32(d,a,rtw_read32(d,a)&~bits);
}
static void rtw_write8_mask(struct rtw_dev*d,u32 a,u32 m,u32 v){
 (void)d;write_register(a,1,(read_register(a,1)&~m)|((v<<mask_shift(m))&m));
}
static void rtw_write32_mask(struct rtw_dev*d,u32 a,u32 m,u32 v){
 (void)d;write_register(a,4,(read_register(a,4)&~m)|((v<<mask_shift(m))&m));
}
static void rtw_write_rf(struct rtw_dev*d,u8 path,u32 a,u32 m,u32 v){
 (void)d;if(path>=4||a>=256)abort();
 rf_registers[path][a]=(rf_registers[path][a]&~m)|((v<<mask_shift(m))&m);
}
'''
    source += macro_definitions([register, common], functions)
    source += functions
    source += '''
static void tune(struct rtw_dev *dev,u8 channel){
 dev->hal.current_channel=channel;dev->hal.primary_channel=channel;
 rtw8814a_switch_channel(dev,channel);
 rtw8814a_set_bw_mode(dev,channel<=14?RTW_BAND_2G:RTW_BAND_5G,channel,RTW_CHANNEL_WIDTH_20,RTW_SC_DONT_CARE);
}
static void row(unsigned rfe,unsigned previous,unsigned channel){
 struct rtw_dev dev={0};dev.hal.rf_path_num=4;dev.hal.cut_version=RTW_CHIP_VER_CUT_B;
 dev.efuse.rfe_option=(u8)rfe;
 memset(registers,0,sizeof(registers));memset(rf_registers,0,sizeof(rf_registers));
 write_register(REG_CCASEL,4,0xa1b2c3d4);write_register(REG_PDMFTH,4,0xaabbccdd);
 if(previous)tune(&dev,(u8)previous);tune(&dev,(u8)channel);
 printf("%u %u %u %u %u %u %u %u %u %u %u\\n",rfe,previous,channel,
  rtw_read32(&dev,REG_NBI_SETTING),rtw_read32(&dev,REG_CSI_MASK_SETTING1),
  rtw_read32(&dev,REG_CSI_FIX_MASK0),rtw_read32(&dev,REG_CSI_FIX_MASK1),
  rtw_read32(&dev,REG_CSI_FIX_MASK6),rtw_read32(&dev,REG_CSI_FIX_MASK7),
  rtw_read32(&dev,REG_CCASEL),rtw_read32(&dev,REG_PDMFTH));
}
int main(void){
 const u8 channels[]={CHANNEL_LIST};
 for(unsigned rfe=0;rfe<2;rfe++){
  for(unsigned i=0;i<ARRAY_SIZE(channels);i++)row(rfe,0,channels[i]);
  SPUR_TRANSITIONS
 }
 return 0;
}
'''
    source = source.replace('CHANNEL_LIST', ','.join(map(str, FIVE_CHANNELS)))
    source = source.replace('SPUR_TRANSITIONS', ''.join(
        f'row(rfe,{previous},{channel});' for previous, channel in SPUR_TRANSITIONS))
    with tempfile.TemporaryDirectory(prefix='8814-spur-reference-') as directory:
        src = Path(directory) / 'reference.c'
        binary = Path(directory) / 'reference'
        src.write_text(source)
        subprocess.run([*compiler('c'), '-std=c11', '-Wall', '-Wextra', '-Werror',
                        str(src), '-o', str(binary)], check=True)
        rows = [list(map(int, line.split())) for line in
                subprocess.check_output([str(binary)], text=True).splitlines()]
    assert len(rows) == 2 * (len(FIVE_CHANNELS) + len(SPUR_TRANSITIONS)) and all(len(row) == 11 for row in rows)
    return [' {' + ','.join(map(str, row)) + '},' for row in rows]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT / 'tests/RTL8814AUUpper5GGolden.inc')
    args = parser.parse_args()
    manifest = json.loads((ROOT / 'src/RTL8814AUPHY-Upstream.json').read_text())
    for item in manifest['files']:
        if hashlib.sha256((args.source / item['path']).read_bytes()).hexdigest() != item['sha256']:
            raise ValueError('Pinned-source mismatch: ' + item['path'])
    # set_bw_mode uses RF18_BW_MASK from this additional pinned Linux header.
    common_hash = 'e217f23f9cc1918332a2005da5aca234f481fd4f291002d305b74ae9c8a5a2b6'
    common_bytes = (args.source / 'rtw88xxa.h').read_bytes()
    if hashlib.sha256(common_bytes).hexdigest() != common_hash:
        raise ValueError('Pinned-source mismatch: rtw88xxa.h')
    common = common_bytes.decode()
    main_h = (args.source / 'main.h').read_text()
    phy = (args.source / 'phy.c').read_text()
    chip = (args.source / 'rtw8814a.c').read_text()
    tables = (args.source / 'rtw8814a_table.c').read_text()
    register = (args.source / 'reg.h').read_text()
    spurs = spur_goldens(main_h, chip, register, common)
    source = '''#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8;typedef int8_t s8;typedef uint32_t u32;
#define __packed __attribute__((packed))
#define __LITTLE_ENDIAN 1
#define WARN_ON(x) ((void)(x))
#define fallthrough __attribute__((fallthrough))
#define BIT(n) (1U<<(n))
#define MASKBYTE0 0xff
#define RTW_RF_PATH_MAX 4
enum {RF_PATH_A,RF_PATH_B,RF_PATH_C,RF_PATH_D};
'''
    for marker in ['enum rtw_bandwidth ', 'enum rtw_trx_desc_rate ',
                   'struct rtw_5g_ht_1s_pwr_idx_diff ', 'struct rtw_5g_ht_ns_pwr_idx_diff ',
                   'struct rtw_5g_ofdm_ns_pwr_idx_diff ', 'struct rtw_5g_vht_ns_pwr_idx_diff ',
                   'struct rtw_5g_txpwr_idx ', 'struct rtw_pwr_track_tbl ', 'struct rtw_swing_table ']:
        # The table pointer is first referenced earlier in main.h. Select its
        # actual declaration rather than the struct member using that type.
        match = re.search(re.escape(marker) + r'\{', main_h)
        if not match: raise ValueError(marker)
        declaration = block(main_h[match.start():], marker)
        source += declaration + (' __packed;\n' if 'pwr_idx_diff ' in marker or 'struct rtw_5g_txpwr_idx ' == marker else ';\n')
    # Definitions need to precede the power-track structs.
    constants = '\n'.join(line for line in main_h.splitlines() if
                          line.startswith('#define RTW_PWR_TRK_') or
                          line.startswith('#define IS_CH_2G_BAND') or
                          re.match(r'#define IS_CH_5G_BAND_[1-4]\(', line)) + '\n'
    source = source.replace('struct rtw_pwr_track_tbl {', constants + 'struct rtw_pwr_track_tbl {')
    source += '''struct rtw_chip_info {u8 txgi_factor;};
struct rtw_hal {u8 rf_path_num,current_channel;};
struct rtw_dm_info {u8 tx_rate;};
struct rtw_dev {struct rtw_hal hal;struct rtw_dm_info dm_info;const struct rtw_chip_info *chip;};
struct rtw_rfe_def {const struct rtw_pwr_track_tbl *pwr_track_tbl;};
static const struct rtw_rfe_def *current_rfe;
static const struct rtw_rfe_def *rtw_get_rfe_def(struct rtw_dev *d){(void)d;return current_rfe;}
static u32 fc,agc,rf[4];
static void rtw_write32_mask(struct rtw_dev*d,u32 a,u32 m,u32 v){(void)d;(void)m;if(a==0x860)fc=v;if(a==0x958)agc=v;}
static void rtw_write_rf(struct rtw_dev*d,u8 p,u32 a,u32 m,u32 v){(void)d;(void)a;(void)m;rf[p]=v;}
'''
    for name in ['REG_CLKTRK', 'REG_AGC_TABLE', 'RF_CFGCH', 'RF18_RFSI_MASK', 'RF18_BAND_MASK', 'RF18_CHANNEL_MASK']:
        source += re.search(r'^#define\s+' + name + r'\s+[^\n]+', register, re.M).group() + '\n'
    # Unchanged actual thermal arrays and their actual table initializers.
    for match in re.finditer(r'static const u8\s+(rtw8814a_pwrtrk_\w+)\[', tables):
        if not re.search(r'_type[1-9]_', match.group(1)):
            source += block(tables[match.start():], 'static const u8') + ';\n'
    for name in ['rtw8814a_rtw_pwrtrk_type0_tbl', 'rtw8814a_rtw_pwrtrk_tbl']:
        source += block(tables, 'const struct rtw_pwr_track_tbl ' + name + ' =') + ';\n'
    source += '''static const struct rtw_rfe_def defs[2]={{&rtw8814a_rtw_pwrtrk_type0_tbl},{&rtw8814a_rtw_pwrtrk_tbl}};
'''
    for text, marker in [(chip, 'static void rtw8814a_switch_channel('),
                         (phy, 'static u8 rtw_get_channel_group('),
                         (phy, 'static u8 rtw_phy_get_5g_tx_power_index('),
                         (phy, 'void rtw_phy_config_swing_table(')]:
        source += block(text, marker) + '\n'
    source += '''int main(void){
 const u8 channels[]={CHANNEL_LIST};
 struct rtw_chip_info chip={1};struct rtw_dev dev={{4,0},{4},&chip};
 for(unsigned rfe=0;rfe<2;rfe++)for(unsigned i=0;i<sizeof(channels);i++){
  u8 ch=channels[i],group=rtw_get_channel_group(ch,4);dev.hal.current_channel=ch;current_rfe=&defs[rfe];
  rtw8814a_switch_channel(&dev,ch);printf("%u %u %u %u %u",rfe,ch,group,fc,agc);
  for(unsigned path=0;path<4;path++)printf(" %u",rf[path]);
  for(unsigned path=0;path<4;path++){struct rtw_5g_txpwr_idx rom={0};
   for(unsigned g=0;g<14;g++)rom.bw40_base[g]=12+g+path;
   rom.ht_1s_diff.ofdm=-1;rom.ht_1s_diff.bw20=2;rom.ht_2s_diff.bw20=-1;
   for(unsigned rate=4;rate<28;rate++)printf(" %u",rtw_phy_get_5g_tx_power_index(&dev,&rom,RTW_CHANNEL_WIDTH_20,rate,group));
  }
  struct rtw_swing_table swing;rtw_phy_config_swing_table(&dev,&swing);
  for(unsigned sign=0;sign<2;sign++)for(unsigned path=0;path<4;path++)for(unsigned delta=0;delta<30;delta++)printf(" %u",(sign?swing.p[path]:swing.n[path])[delta]);
  puts("");
 }
 return 0;
}
'''
    source = source.replace('CHANNEL_LIST', ','.join(map(str, FIVE_CHANNELS)))
    with tempfile.TemporaryDirectory(prefix='8814-upper-reference-') as directory:
        src = Path(directory) / 'reference.c'
        binary = Path(directory) / 'reference'
        src.write_text(source)
        subprocess.run([*compiler('c'), '-std=c11', '-Wall', '-Wextra', '-Werror', str(src), '-o', str(binary)], check=True)
        lines = subprocess.check_output([str(binary)], text=True).splitlines()
    powers = []
    swings = []
    for line in lines:
        fields = list(map(int, line.split()))
        rfe, ch, group, fc, agc = fields[:5]
        raw, thermal = fields[9:105], fields[105:]
        assert len(raw) == 96 and len(thermal) == 240
        pg_text = block(tables, 'static const struct rtw_phy_pg_cfg_pair rtw8814a_bb_pg' + ('_type0' if not rfe else '') + '[')
        pg = [[0] * 28 for _ in range(4)]
        for row in re.findall(r'\{([^{}]+)\}', pg_text):
            band, path, txnum, addr, mask, data = [int(x.strip(), 0) for x in row.split(',') if x.strip()]
            if band != 1: continue
            address = addr & 255
            first = {0x24:4, 0x28:8, 0x2c:12, 0x30:16, 0x34:20, 0x38:24}.get(address)
            if first is None: continue
            for byte in range(4):
                if mask & (255 << (8 * byte)):
                    bcd = (data >> (8 * byte)) & 255
                    pg[path][first + byte] = (bcd >> 4) * 10 + (bcd & 15)
        limits_text = block(tables, 'static const struct rtw_txpwr_lmt_cfg_pair rtw8814a_txpwr_lmt_type' + str(rfe) + '[')
        limits = [[int(x.strip(), 0) for x in row.split(',') if x.strip()]
                  for row in re.findall(r'\{([^{}]+)\}', limits_text)]
        for domain in range(4):
            values = []
            for path in range(4):
                for rate in range(4, 28):
                    section = 1 if rate < 12 else 2 if rate < 20 else 3
                    base_rate = {1:11, 2:19, 3:27}[section]
                    candidates = [r for r in limits if r[1:5] == [1, 0, section, ch]]
                    world = min(r[5] for r in candidates)
                    selected = min([r[5] for r in candidates if r[0] == domain] + [63])
                    if selected == 63: selected = world
                    ceiling = selected - pg[0][base_rate]
                    offset = min(pg[path][rate] - pg[path][base_rate], ceiling)
                    values.append(max(0, min(63, raw[path * 24 + rate - 4] + offset)) + 2)
                    values[-1] = min(63, values[-1])
            powers.append(' {' + ','.join(map(str, [rfe,domain,ch,group,fc,agc,*fields[5:9]])) + ',{' + ','.join(map(str, values)) + '}},')
        if ch in (36, 52, 64, 100, 140, 149):
            swings.append(' {' + ','.join(map(str, [rfe,ch])) + ',{' + ','.join(map(str, thermal)) + '}},')
    output = '''/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright(c)2018-2019, 2025 Realtek Corporation.
 * BSD-3-Clause; see src/RTL8814AUPHY-LICENSE.txt.
 * Generated by generate-rtl8814au-upper5g-goldens.py using actual pinned
 * Linux channel/group/packed-ROM/thermal-selection functions and complete
 * HT20 set_bw_mode/spur functions, never the port. Spur register callbacks
 * retain unrelated bits on masked writes and implement little-endian I/O.
 * torvalds/linux ''' + manifest['commit'] + '''.
 * Source hashes checked against RTL8814AUPHY-Upstream.json.
 * main.h SHA256: ''' + hashlib.sha256(main_h.encode()).hexdigest() + '''
 * rtw88xxa.h SHA256: ''' + common_hash + ''' */
struct upper5g_golden {unsigned rfe,domain,channel,group,fc,agc,rf[4];unsigned char power[96];};
static const upper5g_golden upper5g_goldens[]={
''' + '\n'.join(powers) + '''
};
struct upper5g_thermal_golden {unsigned rfe,channel;unsigned char value[240];};
static const upper5g_thermal_golden upper5g_thermal_goldens[]={
''' + '\n'.join(swings) + '\n};\n'
    # rf[4] is a subarray in C++; insert its braces explicitly.
    output = re.sub(r'^( \{(?:\d+,){6})(\d+,\d+,\d+,\d+),\{', r'\1{\2},{', output, flags=re.M)
    scale = block(chip, 'static const u32 rtw8814a_txscale_tbl[')
    scale = scale[scale.index('{'):].replace('{', '', 1).rsplit('}', 1)[0]
    output += 'static const unsigned upper5g_swing_scale[37]={' + scale + '};\n'
    output += '''/* Fresh zeroed registers, RFE0/1, cut B; previous_channel=0 means
 * initial tune. Transition rows execute the previous center then the destination
 * without clearing registers. CCASEL/PDMFTH start with nonzero test sentinels. Values are the final upstream register contents after
 * spur_nbi_setting, including its final NBI_ENABLE clear for 5 GHz. */
struct upper5g_spur_golden {unsigned rfe,previous_channel,channel,nbi,csi_setting1,csi_mask0,csi_mask1,csi_mask6,csi_mask7,cca,pdmfth;};
static const upper5g_spur_golden upper5g_spur_goldens[]={
''' + '\n'.join(spurs) + '\n};\n'
    args.output.write_text(output)
    print(f'{len(powers)} power/channel rows, {len(swings)} thermal rows and {len(spurs)} complete HT20 spur rows from actual pinned Linux functions: {args.output}')


if __name__ == '__main__':
    main()
