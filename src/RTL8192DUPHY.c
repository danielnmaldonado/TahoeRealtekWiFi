/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2009-2012, 2024 Realtek Corporation.
 * Linux rtlwifi RTL8192DU USB PHY port. The generated algorithm body retains
 * primary register sequences; this file provides typed state, synchronous
 * register/wait boundaries, failure fencing and operating prerequisites. */
#include "rtw/StationChannels.h"
#include "rtw/RTL8192DUPHY.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;
typedef int8_t s8;typedef int32_t s32;
#define BIT(n) (UINT32_C(1)<<(n))
#define ARRAY_SIZE(a) ((int)(sizeof(a)/sizeof((a)[0])))
#define abs_diff(a,b) ((a)>(b)?(a)-(b):(b)-(a))
#define CHANNEL_MAX_NUMBER 59
#define CHANNEL_MAX_NUMBER_2G 14
#define CHANNEL_GROUP_MAX 12
#define CHANNEL_GROUP_MAX_2G 3
#define MAX_PG_GROUP 13
#define IQK_ADDA_REG_NUM 16
#define IQK_MAC_REG_NUM 4
#define IQK_MATRIX_REG_NUM 8
#define IQK_MATRIX_SETTINGS_NUM 46
#define TARGET_CHNL_NUM_5G 221
#define TARGET_CHNL_NUM_2G 14
#define IQK_BB_REG_NUM 10
#define IQK_DELAY_TIME 1
#define MAX_TOLERANCE 5
#define MAX_TOLERANCE_92D 3
#define INTF_PCI 0
#define INTF_USB 1
#define USB_SPEED_HIGH 3
#define HW_VAR_BASIC_RATE 2
#define OFDM_TABLE_SIZE_92D 43
#define CCK_TABLE_SIZE 33
#define CCK_TABLE_LENGTH 33
#define INDEX_MAPPING_NUM 13
#define RX_INDEX_MAPPING_NUM 15
#define AVG_THERMAL_NUM 8
#define MASKOFDM_D 0xffc00000U
#define MASKCCK 0x3f3f3f3fU
#define MASKDWORD UINT32_MAX
#define RFREG_OFFSET_MASK 0x000fffffU
#define MASKH3BYTES 0xffffff00U
#define MASKHWORD 0xffff0000U
#define MASKH4BITS 0xf0000000U
#define MASKLWORD 0x0000ffffU
#define MASKBYTE0 0xffU
#define MASKBYTE1 0xff00U
#define MASKBYTE2 0xff0000U
#define MASKBYTE3 0xff000000U
#define MAC0_ACCESS_PHY1 0x4000U
#define MAC1_ACCESS_PHY0 0x2000U
#include "RTL8192DURegisters.inc"
#include "RTL8192DUDefinitions.inc"
enum radio_path {RF90_PATH_A=0,RF90_PATH_B,RF90_PATH_C,RF90_PATH_D};
enum rf_type {RF_1T1R=0,RF_1T2R,RF_2T2R};
enum macphy_mode {SINGLEMAC_SINGLEPHY=0,DUALMAC_DUALPHY,DUALMAC_SINGLEPHY};
enum band_type {BAND_ON_2_4G=0,BAND_ON_5G,BAND_ON_BOTH};
enum ht_channel_width {HT_CHANNEL_WIDTH_20=0,HT_CHANNEL_WIDTH_20_40};
enum nl80211_channel_type {NL80211_CHAN_HT20=0};
enum baseband_config_type {BASEBAND_CONFIG_PHY_REG=0,BASEBAND_CONFIG_AGC_TAB};
enum rf_content {radioa_txt=0,radiob_txt,radioc_txt,radiod_txt};
struct bb_reg_def {u32 rfintfs,rfintfi,rfintfo,rfintfe,rf3wire_offset,rflssi_select,
    rftxgain_stage,rfhssi_para1,rfhssi_para2,rfsw_ctrl,rfagc_control1,
    rfagc_control2,rfrxiq_imbal,rfrx_afe,rftxiq_imbal,rftx_afe,rf_rb,rf_rbpi;};
struct iqk_matrix_regs {bool iqk_done;long value[1][8];};
struct rtl_phy {
    struct bb_reg_def phyreg_def[4];
    u8 rf_mode,rf_type,current_chan_bw,current_channel;
    u8 set_bwmode_inprogress,sw_chnl_inprogress,lck_inprogress;
    s32 reg_e94,reg_e9c,reg_eb4,reg_ebc;
    u32 adda_backup[16],iqk_mac_backup[4],iqk_bb_backup[10];
    struct iqk_matrix_regs iqk_matrix[46];
    bool rfpi_enable;u8 pwrgroup_cnt,cck_high_power;
    u32 mcs_offset[13][16];u8 default_initialgain[4];
    u8 cur_cck_txpwridx,cur_ofdm24g_txpwridx;
    u32 rfreg_chnlval[2],reg_rf3c[2];u8 framesync;u32 framesync_c34;
    u8 num_total_rfpath;
};
struct rtl_hal {
    enum version_8192d version;
    enum macphy_mode macphymode;
    enum band_type current_bandtype,bandset;
    u8 interface,interfaceindex;
    bool during_mac0init_radiob,during_mac1init_radioa;
    bool load_imrandiqk_setting_for2g,reloadtxpowerindex;
};
struct rtl_efuse {
    bool autoload_failflag,txpwr_fromeprom;
    u8 crystalcap,delta_iqk,delta_lck,eeprom_c9,eeprom_cc,eeprom_regulatory;
    u8 eeprom_thermalmeter,internal_pa_5g[2],thermalmeter[2];
    u8 pwrgroup_ht20[2][59],pwrgroup_ht40[2][59],txpwrlevel_cck[2][14];
    u8 txpwrlevel_ht40_1s[2][59],txpwrlevel_ht40_2s[2][59];
    s8 txpwr_ht20diff[2][59],txpwr_legacyhtdiff[2][59];
};
struct rtl_mac {bool act_scanning;u8 cur_40_prime_sc,mode;};
struct rtl_dm {
    bool txpower_trackinginit,cck_inch14,txpower_tracking,done_txpower;
    u8 thermalvalue_rxgain,thermalvalue_iqk,thermalvalue_lck,thermalvalue;
    u8 thermalvalue_avg[8],thermalvalue_avg_index,tm_trigger,txpower_track_control;
    s8 ofdm_index[2],cck_index;
};
struct ieee80211_hw;struct rtw_92du_phy;
struct rtl_ops {
    void (*set_hw_reg)(struct ieee80211_hw*,u8,u8*);
    void (*phy_lc_calibrate)(struct ieee80211_hw*,bool);
    void (*phy_iq_calibrate)(struct ieee80211_hw*);
};
struct rtl_cfg {const struct rtl_ops *ops;};
struct rtl_priv {
    struct rtw_92du_phy *owner;
    struct rtl_phy phy;struct rtl_hal rtlhal;struct rtl_efuse efuse;
    struct rtl_mac mac80211;struct rtl_dm dm;const struct rtl_cfg *cfg;
    u32 curveindex_2g[14],curveindex_5g[221];
};
struct usb_device {unsigned speed;};
struct rtl_usb {struct usb_device *udev;};
struct ieee80211_hw {struct rtw_92du_phy *context;};
struct rtw_92du_phy {
    rtw_92du_boot *boot;rtw_92du_rom rom;
    struct rtl_priv priv;struct ieee80211_hw hw;
    struct usb_device usb_device;struct rtl_usb usb;
    bool mac_loaded,loaded,channel_set,lc_valid,iq_valid,active,collect_curves,passive_scan;
    unsigned curve_samples[2],nonzero_curve_samples[2];
    bool cleanup_done,power_needs_tune;u8 power_reduction_db;int32_t power_limit_mbm;
};
#define rtl_priv(hw) (&(hw)->context->priv)
#define rtl_hal(priv) (&(priv)->rtlhal)
#define rtl_efuse(priv) (&(priv)->efuse)
#define rtl_mac(priv) (&(priv)->mac80211)
#define rtl_usbpriv(hw) (&(hw)->context->usb)
#define rtl_usbdev(usb) (usb)
#define get_rf_type(phy) ((phy)->rf_type)
#define is_hal_stop(hal) (false)
#define RT_CANNOT_IO(hw) ((hw)->context->boot->last_error!=RTW_92DU_OK)
#define rtl92d_pci_lock(priv) ((void)(priv))
#define rtl92d_pci_unlock(priv) ((void)(priv))
#define rtl92d_acquire_cckandrw_pagea_ctl(hw,flag) ((void)(hw),(void)(flag))
#define rtl92d_release_cckandrw_pagea_ctl(hw,flag) ((void)(hw),(void)(flag))
static void unused_log(struct rtl_priv *priv,const char *format,...) {(void)priv;(void)format;}
#define rtl_dbg(priv,comp,level,...) do {if(false)unused_log((priv),__VA_ARGS__);} while(0)
#define RTPRINT(priv,comp,level,...) do {if(false)unused_log((priv),__VA_ARGS__);} while(0)
#define pr_err(...) do {if(false)unused_log(NULL,__VA_ARGS__);} while(0)
#define WARN_ONCE(condition,...) (!!(condition))
static unsigned calculate_bit_shift(u32 mask) {unsigned n=0;while(mask && !(mask&1U)){++n;mask>>=1U;}return n;}
static void u32p_replace_bits(u32 *word,u32 value,u32 mask) {*word=(*word&~mask)|((value<<calculate_bit_shift(mask))&mask);}
static void du_algorithm_failure(struct ieee80211_hw *hw,rtw_92du_result error) {
    if(hw->context->boot->last_error==RTW_92DU_OK)hw->context->boot->last_error=error;
}
static u32 du_get_le32(const u8 *p) {
    return (u32)p[0]|((u32)p[1]<<8U)|((u32)p[2]<<16U)|((u32)p[3]<<24U);
}
static void du_rf_sample(struct ieee80211_hw *hw,enum radio_path path,u32 reg,u32 value) {
    struct rtw_92du_phy *p=hw->context;
    if(p->collect_curves && (reg==0x4fU || reg==0x50U)) {
        if((unsigned)path>=2U){du_algorithm_failure(hw,RTW_92DU_BAD_ARGUMENT);return;}
        ++p->curve_samples[path];
        if(value&(reg==0x50U?0x000ffc00U:0x000fffffU))++p->nonzero_curve_samples[path];
    }
}
static u32 read_io(struct rtl_priv *priv,u32 address,u8 width) {
    rtw_92du_boot *b=priv->owner->boot;u32 value=0;
    if(b->last_error!=RTW_92DU_OK)return 0;
    if(address>65535U){b->last_error=RTW_92DU_BAD_ARGUMENT;return 0;}
    b->last_register=(u16)address;
    if(rtw_usb_rw_read(&b->bus,(u16)address,width,&value)!=RTW_USB_OK)b->last_error=RTW_92DU_IO_ERROR;
    return value;
}
static void write_io(struct rtl_priv *priv,u32 address,u8 width,u32 value) {
    rtw_92du_boot *b=priv->owner->boot;
    if(b->last_error!=RTW_92DU_OK)return;
    if(address>65535U){b->last_error=RTW_92DU_BAD_ARGUMENT;return;}
    b->last_register=(u16)address;
    /* Imported LC/IQK restoration can request unpause. Only activate() is
     * permitted to release queues after both calibrations actually pass. */
    if(address==REG_TXPAUSE && width==1U)value=0xffU;
    /* Realtek8192D phy_DbmToTxPwrIdx/TxPwrIdxToDbm use2indices/dB.
     * Intercept final OFDM/HT TXAGC writes, including thermal refreshes.
     * A requested drop below index0 is rejected while queues stay paused. */
    if(width==4U && (address==RTXAGC_A_RATE18_06||address==RTXAGC_A_RATE54_24||
        address==RTXAGC_A_MCS03_MCS00||address==RTXAGC_A_MCS07_MCS04||
        address==RTXAGC_A_MCS11_MCS08||address==RTXAGC_A_MCS15_MCS12||
        address==RTXAGC_B_RATE18_06||address==RTXAGC_B_RATE54_24||
        address==RTXAGC_B_MCS03_MCS00||address==RTXAGC_B_MCS07_MCS04||
        address==RTXAGC_B_MCS11_MCS08||address==RTXAGC_B_MCS15_MCS12)){
        unsigned reduction=(unsigned)priv->owner->power_reduction_db*2U;u32 next=0;
        for(unsigned i=0;i<4U;++i){unsigned index=(value>>(8U*i))&255U;
            if(priv->owner->power_limit_mbm!=INT32_MAX){
                int64_t mbm=priv->owner->power_limit_mbm;int absolute=(int)(mbm>=0?mbm/50:-((-mbm+49)/50));
                if(absolute<0){b->last_error=RTW_92DU_BAD_ARGUMENT;return;}
                if(index>(unsigned)absolute)index=(unsigned)absolute;
            }
            if(index<reduction){b->last_error=RTW_92DU_BAD_ARGUMENT;return;}
            next|=(index-reduction)<<(8U*i);}
        value=next;
    }
    if(rtw_usb_write_register(&b->bus,(u16)address,width,value)!=RTW_USB_OK)b->last_error=RTW_92DU_IO_ERROR;
}
static u8 rtl_read_byte(struct rtl_priv *p,u32 r) {return (u8)read_io(p,r,1U);}
static u16 rtl_read_word(struct rtl_priv *p,u32 r) {return (u16)read_io(p,r,2U);}
static u32 rtl_read_dword(struct rtl_priv *p,u32 r) {return read_io(p,r,4U);}
static void rtl_write_byte(struct rtl_priv *p,u32 r,u32 v) {write_io(p,r,1U,v);}
static void rtl_write_word(struct rtl_priv *p,u32 r,u32 v) {write_io(p,r,2U,v);}
static void rtl_write_dword(struct rtl_priv *p,u32 r,u32 v) {write_io(p,r,4U,v);}
static void du_delay(struct ieee80211_hw *hw,u32 us) {
    rtw_92du_boot *b=hw->context->boot;
    if(b->last_error==RTW_92DU_OK && !b->waiter.wait_us(b->waiter.context,us))b->last_error=RTW_92DU_WAIT_ERROR;
}
#define udelay(us) du_delay(hw,(us))
#define mdelay(ms) du_delay(hw,(ms)*1000U)
static void du_addr_delay(struct ieee80211_hw *hw,u32 address) {
    static const u32 waits[6]={1U,5U,50U,1000U,5000U,50000U};
    if(address>=0xf9U && address<=0xfeU)du_delay(hw,waits[address-0xf9U]);
}
#define rtl_addr_delay(address) du_addr_delay(hw,(address))
static u32 rtl92du_phy_query_bb_reg(struct ieee80211_hw*,u32,u32);
static void rtl92du_phy_set_bb_reg(struct ieee80211_hw*,u32,u32,u32);
static u32 rtl92d_phy_query_rf_reg(struct ieee80211_hw*,enum radio_path,u32,u32);
static void rtl92d_phy_set_rf_reg(struct ieee80211_hw*,enum radio_path,u32,u32,u32);
#define rtl_get_bbreg rtl92du_phy_query_bb_reg
#define rtl_set_bbreg rtl92du_phy_set_bb_reg
#define rtl_get_rfreg rtl92d_phy_query_rf_reg
#define rtl_set_rfreg rtl92d_phy_set_rf_reg
static void rtl_rfreg_delay(struct ieee80211_hw *hw,enum radio_path path,u32 address,u32 mask,u32 value) {
    if(address>=0xf9U && address<=0xfeU)du_addr_delay(hw,address);
    else {rtl_set_rfreg(hw,path,address,mask,value);du_delay(hw,1U);}
}
static void read_efuse_byte(struct ieee80211_hw *hw,u16 offset,u8 *out) {
    /* init_pa_bias reads physical OTP offset0x3fa; it is captured by the ROM
     * reader, not an arbitrary logical PROM byte or guessed board default. */
    if(offset==0x3faU)*out=hw->context->rom.pa_bias;
    else {*out=0xffU;du_algorithm_failure(hw,RTW_92DU_BAD_PROM);}
}
static void basic_rate(struct ieee80211_hw *hw,u8 variable,u8 *bytes) {
    u16 mask;u8 highest=0;
    if(variable!=HW_VAR_BASIC_RATE){du_algorithm_failure(hw,RTW_92DU_BAD_ARGUMENT);return;}
    memcpy(&mask,bytes,sizeof(mask));mask&=0x15fU;
    rtl_write_byte(rtl_priv(hw),REG_RRSR,mask&255U);
    rtl_write_byte(rtl_priv(hw),REG_RRSR+1U,(unsigned)mask>>8U);
    while(mask>1U){mask>>=1U;++highest;}
    if(hw->context->boot->firmware_version>0x0eU)
        rtl_write_byte(rtl_priv(hw),REG_INIRTS_RATE_SEL,highest);
}
static void checked_lc(struct ieee80211_hw *hw,bool is2t);
static void checked_iq(struct ieee80211_hw *hw);
static const struct rtl_ops ops={basic_rate,checked_lc,checked_iq};
static const struct rtl_cfg config={&ops};
static const u8 channel5g[49]={36,38,40,42,44,46,48,52,54,56,58,60,62,64,
    100,102,104,106,108,110,112,116,118,120,122,124,126,128,
    132,134,136,138,140,142,144,149,151,153,155,157,159,161,
    165,167,169,171,173,175,177};
#include "RTL8192DUTables.inc"
/* These unchanged primary routines use Linux's deliberate integer narrowing
 * and discarded log/unused callback parameters. Keep warnings scoped to that
 * imported body; the adapter and public preconditions remain warning-clean. */
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#endif
#include "RTL8192DUAlgorithms.inc"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

static struct rtw_92du_phy *context(rtw_92du_boot *boot) {
    struct rtw_92du_phy *p=boot ? boot->phy:NULL;
    return p && p->boot==boot ? p:NULL;
}
static rtw_92du_result finish(struct rtw_92du_phy *p) {
    if(p->boot->last_error!=RTW_92DU_OK){
        p->active=false;p->lc_valid=false;p->iq_valid=false;
        if(!p->cleanup_done){
            p->cleanup_done=true;
            (void)rtw_usb_write_register(&p->boot->bus,REG_TXPAUSE,1U,0xffU);
        }
    }
    return p->boot->last_error;
}
size_t rtw_92du_phy_size(void) {return sizeof(struct rtw_92du_phy);}
static int overlaps(const void *a,size_t an,const void *b,size_t bn) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y ? y-x<an:x-y<bn;
}
rtw_92du_result rtw_92du_phy_init(rtw_92du_boot *boot,const rtw_92du_rom *rom,void *storage,size_t size,rtw_92du_usb_speed speed) {
    struct rtw_92du_phy *p;
    if(!boot || !rom || !storage || size<sizeof(*p) ||
        (uintptr_t)storage%_Alignof(struct rtw_92du_phy) ||
        (speed!=RTW_92DU_USB_FULL && speed!=RTW_92DU_USB_HIGH) ||
        overlaps(storage,sizeof(*p),boot,sizeof(*boot)) ||
        overlaps(storage,sizeof(*p),rom,sizeof(*rom)))return RTW_92DU_BAD_ARGUMENT;
    if(!boot->power_ready || !boot->firmware_ready)return RTW_92DU_WRONG_STATE;
    if(boot->phy)return RTW_92DU_WRONG_STATE;
    if(boot->last_error!=RTW_92DU_OK)return boot->last_error;
    if(boot->board.profile!=rom->board.profile || boot->board.sys_cfg!=rom->board.sys_cfg ||
        boot->board.mode!=rom->board.mode || boot->board.mac_index!=rom->board.mac_index ||
        boot->board.rf_paths!=rom->board.rf_paths || boot->board.initial_band!=rom->board.initial_band ||
        memcmp(boot->board.mac,rom->board.mac,6U) ||
        !rom->thermal_valid || !rom->pa_valid || rom->crystal==0xffU ||
        rom->regulatory>3U || rom->logical[0xc9U]==0xffU || rom->logical[0xccU]==0xffU)
        return RTW_92DU_BAD_PROM;
    {
        static const unsigned base_offsets[5]={0x61U,0x67U,0x7cU,0x91U,0xa6U};
        unsigned group,path;
        if(rom->cut<RTW_92DU_CUT_C || rom->cut>RTW_92DU_CUT_E ||
            rom->logical[0xbdU]!=rom->crystal || (rom->logical[0xc3U]&31U)!=rom->thermal ||
            rom->thermal<6U || rom->thermal>0x1cU ||
            (rom->logical[0xc4U]&7U)!=rom->regulatory)return RTW_92DU_BAD_PROM;
        if(rom->logical[0]!=0x29U || rom->logical[1]!=0x81U || rom->logical[0x10U]==0xffU ||
            boot->board.mode!=((rom->logical[0x10U]&1U)?RTW_92DU_DUAL_MAC:RTW_92DU_SINGLE_MAC) ||
            boot->board.rf_paths!=(boot->board.mode==RTW_92DU_SINGLE_MAC?2U:1U) ||
            boot->board.mac_index>1U || (boot->board.mode==RTW_92DU_SINGLE_MAC && boot->board.mac_index) ||
            boot->board.initial_band!=(boot->board.mode==RTW_92DU_DUAL_MAC && !boot->board.mac_index?
                RTW_WIFI_BAND_5GHZ:RTW_WIFI_BAND_2GHZ) ||
            memcmp(rom->logical+(boot->board.mac_index?0x5bU:0x19U),boot->board.mac,6U))
            return RTW_92DU_BAD_PROM;
        if((rom->cut==RTW_92DU_CUT_C && rom->cut_word!=0xaa55U) ||
            (rom->cut==RTW_92DU_CUT_D && rom->cut_word!=0x9966U) ||
            (rom->cut==RTW_92DU_CUT_E && rom->cut_word!=0xcc33U && rom->cut_word!=0x33ccU))
            return RTW_92DU_BAD_PROM;
        for(path=0;path<2U;++path) {
            const u8 raw=rom->logical[0xbeU+path];
            const unsigned expected=rom->cut==RTW_92DU_CUT_C?0U:!(raw&0x40U);
            if((rom->cut!=RTW_92DU_CUT_C && raw==0xffU) || rom->internal_pa_5g[path]!=expected)
                return RTW_92DU_BAD_PROM;
        }
        /* All three groups for both physical paths must be programmed.
         * The imported Linux base-power fallback is never authorized. */
        for(group=0;group<5U;++group)for(path=0;path<6U;++path)
            if(rom->logical[base_offsets[group]+path]>63U)return RTW_92DU_BAD_PROM;
    }
    p=storage;memset(p,0,sizeof(*p));p->power_limit_mbm=INT32_MAX;p->boot=boot;p->rom=*rom;
    p->hw.context=p;p->priv.owner=p;p->priv.cfg=&config;
    p->usb_device.speed=(unsigned)speed;p->usb.udev=&p->usb_device;
    p->priv.rtlhal.interface=INTF_USB;p->priv.rtlhal.interfaceindex=boot->board.mac_index;
    p->priv.rtlhal.macphymode=boot->board.mode==RTW_92DU_SINGLE_MAC?SINGLEMAC_SINGLEPHY:DUALMAC_DUALPHY;
    p->priv.rtlhal.version=boot->board.mode==RTW_92DU_SINGLE_MAC?VERSION_NORMAL_CHIP_92D_SINGLEPHY:VERSION_NORMAL_CHIP_92D_DUALPHY;
    p->priv.rtlhal.version|=rom->cut==RTW_92DU_CUT_C?CHIP_92D_C_CUT:rom->cut==RTW_92DU_CUT_D?CHIP_92D_D_CUT:CHIP_92D_E_CUT;
    p->priv.rtlhal.current_bandtype=boot->board.initial_band==RTW_WIFI_BAND_5GHZ?BAND_ON_5G:BAND_ON_2_4G;
    p->priv.rtlhal.bandset=boot->board.mode==RTW_92DU_SINGLE_MAC?BAND_ON_BOTH:p->priv.rtlhal.current_bandtype;
    p->priv.phy.rf_type=boot->board.rf_paths==2U?RF_2T2R:RF_1T1R;
    p->priv.phy.rf_mode=RF_OP_BY_SW_3WIRE;
    p->priv.phy.current_channel=p->priv.rtlhal.current_bandtype==BAND_ON_5G?36U:1U;
    _rtl92d_read_txpower_info(&p->hw,false,p->rom.logical);
    boot->phy=p;return RTW_92DU_OK;
}
rtw_92du_result rtw_92du_phy_load_mac_table(rtw_92du_boot *boot) {
    struct rtw_92du_phy *p=context(boot);
    if(!p)return RTW_92DU_WRONG_STATE;
    if(boot->last_error!=RTW_92DU_OK)return boot->last_error;
    if(p->mac_loaded)return RTW_92DU_OK;
    rtl_write_byte(&p->priv,REG_TXPAUSE,0xffU);
    if(!rtl92du_phy_mac_config(&p->hw))du_algorithm_failure(&p->hw,RTW_92DU_IO_ERROR);
    p->mac_loaded=boot->last_error==RTW_92DU_OK;return finish(p);
}
rtw_92du_result rtw_92du_phy_load_tables(rtw_92du_boot *boot) {
    struct rtw_92du_phy *p=context(boot);unsigned path;
    if(!p || p->loaded)return RTW_92DU_WRONG_STATE;
    if(boot->last_error!=RTW_92DU_OK)return boot->last_error;
    rtl_write_byte(&p->priv,REG_TXPAUSE,0xffU);
    if(rtw_92du_phy_load_mac_table(boot)!=RTW_92DU_OK || !rtl92du_phy_bb_config(&p->hw))
        du_algorithm_failure(&p->hw,RTW_92DU_IO_ERROR);
    rtl_set_bbreg(&p->hw,RFPGA0_ANALOGPARAMETER4,0x00f00000U,15U);
    if(!rtl92du_phy_rf_config(&p->hw))du_algorithm_failure(&p->hw,RTW_92DU_IO_ERROR);
    rtl_set_bbreg(&p->hw,RFPGA0_ANALOGPARAMETER4,0x00f00000U,0U);
    rtl92du_update_bbrf_configuration(&p->hw);
    for(path=0;path<p->priv.phy.num_total_rfpath;++path)
        p->priv.phy.rfreg_chnlval[path]=rtl_get_rfreg(&p->hw,(enum radio_path)path,RF_CHNLBW,RFREG_OFFSET_MASK);
    rtl_set_bbreg(&p->hw,RFPGA0_RFMOD,BCCKEN,p->priv.rtlhal.current_bandtype==BAND_ON_2_4G?1U:0U);
    rtl_set_bbreg(&p->hw,RFPGA0_RFMOD,BOFDMEN,1U);
    /* rtl92du_hw_init post-BB settings, before IQ/LC and the snapshot used
     * by thermal tracking. C87 is CCA, C8C OFDM RX weighting, not IQ matrices. */
    rtl_write_byte(&p->priv,REG_AFE_XTAL_CTRL,(u8)(rtl_read_byte(&p->priv,REG_AFE_XTAL_CTRL)|2U));
    rtl_write_dword(&p->priv,REG_BAR_MODE_CTRL,0xffffU);
    rtl_write_byte(&p->priv,REG_NAV_CTRL+2U,0U);
    rtl_write_byte(&p->priv,ROFDM0_XATXAFE+3U,0x50U);
    if(boot->board.mode==RTW_92DU_SINGLE_MAC) {
        rtl_set_bbreg(&p->hw,RFPGA1_TXINFO,BIT(30),0U);
        rtl_set_bbreg(&p->hw,RFPGA0_TXGAINSTAGE,BIT(31),0U);
        rtl_write_dword(&p->priv,ROFDM0_XBTXAFE,0xa0e40000U);
    }
    rtl92d_phy_get_hw_reg_originalvalue(&p->hw);
    rtl92d_dm_initialize_txpower_tracking(&p->hw);
    rtl92du_phy_init_pa_bias(&p->hw);
    p->loaded=boot->last_error==RTW_92DU_OK;return finish(p);
}
rtw_92du_result rtw_92du_phy_set_power_limit_mbm(rtw_92du_boot *boot,int32_t mbm) {
    struct rtw_92du_phy *p=context(boot);rtw_92du_result e;if(!p)return RTW_92DU_WRONG_STATE;
    e=rtw_92du_phy_pause(boot);if(e!=RTW_92DU_OK)return e;
    p->power_limit_mbm=mbm;p->power_needs_tune=true;p->iq_valid=false;return RTW_92DU_OK;
}
rtw_92du_result rtw_92du_phy_set_power_reduction(rtw_92du_boot *boot,u8 db) {
    struct rtw_92du_phy *p=context(boot);rtw_92du_result e;
    if(!p)return RTW_92DU_WRONG_STATE;
    e=rtw_92du_phy_pause(boot);if(e!=RTW_92DU_OK)return e;
    p->power_reduction_db=db;p->power_needs_tune=true;p->iq_valid=false;return RTW_92DU_OK;
}
rtw_92du_result rtw_92du_phy_set_channel(rtw_92du_boot *boot,u8 channel) {
    struct rtw_92du_phy *p=context(boot);bool band5=rtw_station_5ghz_channel(channel)!=0;
    if(!p || !p->loaded)return RTW_92DU_WRONG_STATE;
    if(!band5 && (channel<1U || channel>11U))return RTW_92DU_BAD_ARGUMENT;
    if(boot->board.mode==RTW_92DU_DUAL_MAC && band5!=(boot->board.mac_index==0U))return RTW_92DU_UNSUPPORTED_CHIP;
    if(boot->last_error!=RTW_92DU_OK)return boot->last_error;
    p->active=p->lc_valid=p->iq_valid=false;p->channel_set=p->passive_scan=false;
    rtl_write_byte(&p->priv,REG_TXPAUSE,0xffU);p->priv.phy.current_channel=channel;
    if(!rtl92du_phy_sw_chnl(&p->hw))du_algorithm_failure(&p->hw,RTW_92DU_WRONG_STATE);
    p->priv.phy.current_chan_bw=HT_CHANNEL_WIDTH_20;
    rtl92du_phy_set_bw_mode(&p->hw,NL80211_CHAN_HT20);
    p->channel_set=boot->last_error==RTW_92DU_OK;if(p->channel_set)p->power_needs_tune=false;return finish(p);
}
rtw_92du_result rtw_92du_phy_set_scan_channel(rtw_92du_boot *boot,u8 channel) {
    struct rtw_92du_phy *p=context(boot);rtw_92du_result result;u8 index;
    if(!p || !p->loaded || !p->lc_valid)return RTW_92DU_WRONG_STATE;
    p->priv.mac80211.act_scanning=true;
    result=rtw_92du_phy_set_channel(boot,channel);
    p->priv.mac80211.act_scanning=false;
    if(result!=RTW_92DU_OK)return result;
    /* A successful source LC operation captured curves for every supported
     * channel on both paths (or the fixed startup band on a dual-MAC PHY). */
    p->lc_valid=true;p->passive_scan=channel>14U;
    index=rtl92d_get_rightchnlplace_for_iqk(channel);
    p->iq_valid=p->priv.phy.iqk_matrix[index].iqk_done &&
        p->priv.phy.iqk_matrix[index].value[0][0]!=0 &&
        p->priv.phy.iqk_matrix[index].value[0][2]!=0 &&
        (boot->board.rf_paths!=2U ||
            (p->priv.phy.iqk_matrix[index].value[0][4]!=0 &&
             p->priv.phy.iqk_matrix[index].value[0][6]!=0));
    return RTW_92DU_OK;
}
int rtw_92du_phy_receive_ready(const rtw_92du_boot *boot) {
    const struct rtw_92du_phy *p=boot ? boot->phy:NULL;
    return p && p->boot==boot && boot->last_error==RTW_92DU_OK &&
        boot->power_ready && boot->firmware_ready && p->loaded && p->channel_set && p->lc_valid;
}
int rtw_92du_phy_tx_active(const rtw_92du_boot *boot) {
    const struct rtw_92du_phy *p=boot ? boot->phy:NULL;
    return rtw_92du_phy_radio_ready(boot) && p && p->active && !p->passive_scan;
}
u8 rtw_92du_phy_channel(const rtw_92du_boot *boot) {
    const struct rtw_92du_phy *p=boot ? boot->phy:NULL;
    return p && p->boot==boot ? p->priv.phy.current_channel:0U;
}
int rtw_92du_phy_cck_high_power(const rtw_92du_boot *boot) {
    const struct rtw_92du_phy *p=boot ? boot->phy:NULL;
    return p && p->boot==boot && p->loaded && p->priv.phy.cck_high_power;
}
static void checked_lc(struct ieee80211_hw *hw,bool is2t) {
    struct rtw_92du_phy *p=hw->context;rtw_92du_boot *boot=p->boot;unsigned path;
    if(boot->last_error!=RTW_92DU_OK)return;
    if(is2t!=(boot->board.rf_paths==2U)){du_algorithm_failure(hw,RTW_92DU_BAD_ARGUMENT);return;}
    p->lc_valid=false;
    rtl_write_byte(&p->priv,REG_TXPAUSE,0xffU);
    memset(p->curve_samples,0,sizeof(p->curve_samples));
    memset(p->nonzero_curve_samples,0,sizeof(p->nonzero_curve_samples));p->collect_curves=true;
    rtl92du_phy_lc_calibrate(&p->hw,boot->board.rf_paths==2U);
    p->collect_curves=false;
    for(path=0;path<boot->board.rf_paths;++path)
        if(p->curve_samples[path]!=128U || p->nonzero_curve_samples[path]<64U)
            du_algorithm_failure(&p->hw,RTW_92DU_POLL_TIMEOUT);
    p->lc_valid=boot->last_error==RTW_92DU_OK;
}
static void checked_iq(struct ieee80211_hw *hw) {
    struct rtw_92du_phy *p=hw->context;rtw_92du_boot *boot=p->boot;
    u8 index;
    if(boot->last_error!=RTW_92DU_OK)return;
    p->iq_valid=false;
    index=rtl92d_get_rightchnlplace_for_iqk(p->priv.phy.current_channel);
    p->priv.phy.iqk_matrix[index].iqk_done=false;
    rtl92du_phy_iq_calibrate(&p->hw);
    p->iq_valid=p->priv.phy.iqk_matrix[index].iqk_done &&
        p->priv.phy.iqk_matrix[index].value[0][0]!=0 &&
        p->priv.phy.iqk_matrix[index].value[0][2]!=0 &&
        (boot->board.rf_paths!=2U ||
            (p->priv.phy.iqk_matrix[index].value[0][4]!=0 &&
             p->priv.phy.iqk_matrix[index].value[0][6]!=0));
    if(!p->iq_valid)du_algorithm_failure(&p->hw,RTW_92DU_POLL_TIMEOUT);
}
rtw_92du_result rtw_92du_phy_calibrate(rtw_92du_boot *boot) {
    struct rtw_92du_phy *p=context(boot);
    if(!p || !p->loaded || !p->channel_set || p->power_needs_tune)return RTW_92DU_WRONG_STATE;
    if(boot->last_error!=RTW_92DU_OK)return boot->last_error;
    p->active=p->lc_valid=p->iq_valid=false;
    checked_lc(&p->hw,boot->board.rf_paths==2U);checked_iq(&p->hw);
    return finish(p);
}
rtw_92du_result rtw_92du_phy_tick(rtw_92du_boot *boot) {
    struct rtw_92du_phy *p=context(boot);bool resume;
    if(!p || !rtw_92du_phy_radio_ready(boot))return RTW_92DU_WRONG_STATE;
    resume=p->active;p->active=false;
    rtl_write_byte(&p->priv,REG_TXPAUSE,0xffU);
    rtl92d_dm_check_txpower_tracking_thermal_meter(&p->hw);
    if(finish(p)!=RTW_92DU_OK)return boot->last_error;
    return resume ? rtw_92du_phy_activate(boot):RTW_92DU_OK;
}
int rtw_92du_phy_radio_ready(const rtw_92du_boot *boot) {
    const struct rtw_92du_phy *p=boot ? boot->phy:NULL;
    return p && p->boot==boot && boot->last_error==RTW_92DU_OK &&
        boot->power_ready && boot->firmware_ready && p->loaded && p->channel_set && p->lc_valid && p->iq_valid && !p->passive_scan;
}
int rtw_92du_phy_supports_channel(const rtw_92du_boot *boot,u8 channel) {
    const struct rtw_92du_phy *p=boot?boot->phy:NULL;
    const bool band5=rtw_station_5ghz_channel(channel)!=0;
    if(!p || p->boot!=boot || !p->loaded || boot->last_error!=RTW_92DU_OK ||
        !boot->power_ready || !boot->firmware_ready || (!band5 && (channel<1U || channel>11U)))return 0;
    return boot->board.mode!=RTW_92DU_DUAL_MAC || band5==(boot->board.mac_index==0U);
}
rtw_92du_result rtw_92du_phy_activate(rtw_92du_boot *boot) {
    struct rtw_92du_phy *p=context(boot);
    if(!rtw_92du_phy_radio_ready(boot) || !p || p->passive_scan)return RTW_92DU_WRONG_STATE;
    boot->last_register=REG_TXPAUSE;
    if(rtw_usb_write_register(&boot->bus,REG_TXPAUSE,1U,0U)!=RTW_USB_OK)boot->last_error=RTW_92DU_IO_ERROR;
    p->active=boot->last_error==RTW_92DU_OK;return finish(p);
}
rtw_92du_result rtw_92du_phy_pause(rtw_92du_boot *boot) {
    struct rtw_92du_phy *p=context(boot);
    if(!p)return RTW_92DU_WRONG_STATE;
    p->active=false;rtl_write_byte(&p->priv,REG_TXPAUSE,0xffU);return finish(p);
}
rtw_92du_result rtw_92du_phy_stop(rtw_92du_boot *boot) {
    struct rtw_92du_phy *p=context(boot);rtw_usb_error error;
    if(!p)return RTW_92DU_WRONG_STATE;
    p->active=p->lc_valid=p->iq_valid=p->channel_set=false;
    if(p->loaded && boot->last_error==RTW_92DU_OK) {
        p->loaded=false;_rtl92du_phy_set_rfsleep(&p->hw);return finish(p);
    }
    p->loaded=false;
    if(boot->last_error!=RTW_92DU_OK)return finish(p);
    error=rtw_usb_write_register(&boot->bus,REG_TXPAUSE,1U,0xffU);
    if(error!=RTW_USB_OK && boot->last_error==RTW_92DU_OK)boot->last_error=RTW_92DU_IO_ERROR;
    return finish(p);
}
