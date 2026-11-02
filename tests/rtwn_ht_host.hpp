#ifndef RTW_HT_HOST_HPP
#define RTW_HT_HOST_HPP
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <vector>
#define __packed __attribute__((__packed__))
#define IEEE80211_ADDR_LEN 6
#define nitems(a) (sizeof(a)/sizeof((a)[0]))
#define DPRINTF(x) ((void)0)
#include "vendor/rtwn/r92creg.h"
#include "rtw/RTL8192EUHT.h"
#include "rtw/RTL8192CDataPath.h"
#define RTWN_CHIP_88E 0x20U
#define RTWN_CHIP_92C 0x01U
#define RTWN_CHIP_92C_1T2R 0x02U
#define RTWN_CHIP_UMC 0x04U
#define RTWN_CHIP_UMC_A_CUT 0x08U
#define RTWN_CHIP_88C 0x10U
#define RTWN_CHIP_23A 0x80U
#define RTWN_CHIP_88F 0x200U
#define RTWN_CHIP_92E 0x40U
#define RTWN_CHIP_PCI 0x40000000U
#define RTWN_FLAG_FORCE_RAID_11B 4U
#define RTWN_FLAG_EXT_HDR 8U
#define RTWN_POWER_COUNT 28
#define RTWN_RIDX_CCK1 0
#define RTWN_RIDX_CCK11 3
#define RTWN_RIDX_OFDM6 4
#define RTWN_RIDX_OFDM54 11
#define RTWN_RIDX_MCS0 12
#define RTWN_RIDX_MCS8 20
#define RTWN_RIDX_MCS15 27
#define R92C_MAX_TX_PWR 0x3f
#define RTWN_SIGN4TO8(v) (((v)&8U)?((v)|0xf0U):(v))
#define IEEE80211_MODE_11B 1
#define IEEE80211_MODE_11G 2
#define IEEE80211_RATE_VAL 0x7f
#define IEEE80211_RATE_BASIC 0x80
#define IEEE80211_NODE_HT 0x0400U
#define IEEE80211_NODE_HT_SGI20 0x4000U
#define IEEE80211_NODE_HT_SGI40 0x8000U
#define IEEE80211_HTCAP_CBW20_40 0x0002U
#define IEEE80211_HTCAP_SGI20 0x0020U
#define IEEE80211_HTCAP_SGI40 0x0040U
#define IEEE80211_HTCAP_SMPS_DIS 3U
#define IEEE80211_HTCAP_SMPS_SHIFT 2U
#define IEEE80211_HTCAP_SMPS_MASK (3U<<2)
#define IEEE80211_C_QOS 0x100U
#define IEEE80211_C_TX_AMPDU 0x200U
#define IEEE80211_F_QOS 0x400U
#define IEEE80211_AMPDU_PARAM_LE 3U
#define IEEE80211_AMPDU_PARAM_SS_16 (7U<<2)
#define IEEE80211_TX_MCS_SET_DEFINED 1U
#define IEEE80211_TX_RX_MCS_NOT_EQUAL 2U
#define IEEE80211_HTCAP_40INTOLERANT 0x4000U
#define IEEE80211_HTOP0_SCO_MASK 0x03U
#define IEEE80211_HTOP0_CHW 0x04U
#define IEEE80211_HTOP0_SCO_SCA 0x01U
#define IEEE80211_HTOP0_SCO_SCB 0x03U
#define IEEE80211_CHAN_2GHZ 0x0080U
#define IEEE80211_CHAN_HT40U 0x00020000U
#define IEEE80211_CHAN_HT40D 0x00040000U
#define IEEE80211_F_NOHT40 0x20U
#define IEEE80211_CHAN_WIDTH_20 1
#define IEEE80211_CHAN_WIDTH_40 2
#define IEEE80211_M_MONITOR 1
#define IEEE80211_M_STA 2
#define RTWN_LED_LINK 0
#define rtwn_bb_read rtwn_read_4
#define rtwn_bb_write rtwn_write_4
enum ieee80211_state {IEEE80211_S_INIT,IEEE80211_S_SCAN,IEEE80211_S_AUTH,IEEE80211_S_ASSOC,IEEE80211_S_RUN};
struct ieee80211_channel {unsigned ic_freq=2412,ic_flags=IEEE80211_CHAN_2GHZ;};
#define IEEE80211_CHAN_ANYC ((ieee80211_channel *)-1)
struct ieee80211_rateset { uint8_t rs_nrates=0, rs_rates[16]{}; };
struct ieee80211_node { ieee80211_rateset ni_rates; uint8_t ni_txrate=0, ni_rxmcs[10]{},ni_bssid[6]{2,0,0,0,0,1}; uint32_t ni_flags=0; uint16_t ni_intval=100,ni_htcaps=0; uint8_t ni_htop0=0,ni_chw=IEEE80211_CHAN_WIDTH_20; ieee80211_channel *ni_chan=nullptr; };
struct ieee80211com { ieee80211_node *ic_bss=nullptr; void *ic_softc=nullptr; int ic_curmode=IEEE80211_MODE_11G,ic_opmode=IEEE80211_M_STA; enum ieee80211_state ic_state=IEEE80211_S_ASSOC; uint16_t ic_htcaps=IEEE80211_HTCAP_CBW20_40|IEEE80211_HTCAP_SGI20|IEEE80211_HTCAP_SGI40; uint32_t ic_userflags=0,ic_caps=0,ic_flags=0;uint8_t ic_ampdu_params=0,ic_sup_mcs[10]{},ic_tx_mcs_set=0; ieee80211_channel ic_channels[12]{}; ieee80211_channel *ic_ibss_chan=nullptr; };
static inline int host_newstate(ieee80211com *ic,enum ieee80211_state state,int) {ic->ic_state=state;return 0;}
struct fw_command { uint8_t id; std::vector<uint8_t> payload; };
struct rtwn_softc {
    struct {const char *dv_xname="offline";} device;decltype(device)*sc_pdev=&device;
    uint32_t chip=RTWN_CHIP_92E, sc_flags=0;
    ieee80211com sc_ic;
    std::vector<fw_command> commands;
    uint32_t rrsr=0;
    uint8_t inirts=0;
    int fw_error=0,fw_fail_id=-1,io_error=0;
    int read4_fail_reg=-1;
    unsigned read4_count=0;
    int ntxchains=2;
    int nrxchains=2;
    struct r92e_rom sc_r92e_rom{};
    uint32_t rf_chnlbw[2]{},rf_output[2]{};
    bool extension_power=false;
    uint8_t channel_bw40=0;
    rtw_92e_rate_config rate_config{};
    uint8_t fw_station_connected=0, registers[0x1000]{};
    std::vector<uint16_t> byte_writes;
    struct {void *cookie=nullptr;void (*cancel_calib)(void*)=[](void*){};void (*next_calib)(void*)=[](void*){};void (*next_scan)(void*)=[](void*){};} sc_ops;
    int (*sc_newstate)(ieee80211com *,enum ieee80211_state,int)=host_newstate;
    int ledlink=0,avg_pwdb=0,thcal_state=0,thcal_lctemp=0,nettype=0;
};
static inline void rtwn_write_1(rtwn_softc *s,uint16_t reg,uint8_t v) { if(reg==R92C_INIRTS_RATE_SEL) s->inirts=v; s->registers[reg]=v;s->byte_writes.push_back(reg); }
static inline uint8_t rtwn_read_1(rtwn_softc *s,uint16_t reg) { return s->registers[reg]; }
static inline void rtwn_write_4(rtwn_softc *s,uint16_t reg,uint32_t v) { if(reg==R92C_RRSR) s->rrsr=v;for(unsigned i=0;i<4;i++)s->registers[reg+i]=(uint8_t)(v>>(8*i)); }
static inline void rtwn_write_2(rtwn_softc *s,uint16_t reg,uint16_t v) { s->registers[reg]=(uint8_t)v;s->registers[reg+1]=(uint8_t)(v>>8); }
static inline uint16_t rtwn_read_2(rtwn_softc *s,uint16_t reg) {return (uint16_t)(s->registers[reg]|((uint16_t)s->registers[reg+1]<<8));}
static inline uint32_t rtwn_read_4(rtwn_softc *s,uint16_t reg) {
    ++s->read4_count;
    if(s->read4_fail_reg==reg){s->io_error=EIO;return 0;}
    return reg==R92C_RRSR?s->rrsr:(uint32_t)rtwn_read_2(s,reg)|((uint32_t)rtwn_read_2(s,reg+2)<<16);
}
static inline int rtwn_fw_cmd(rtwn_softc *s,uint8_t id,const void *p,int n) {
    const auto *b=static_cast<const uint8_t *>(p); s->commands.push_back({id,{b,b+n}}); return (s->fw_fail_id<0 || s->fw_fail_id==id)?s->fw_error:0;
}
int rtwn_r92c_ra_init(rtwn_softc *,uint8_t,uint32_t,int,uint32_t,int);
static inline int rtwn_io_error(rtwn_softc *s) {return s->io_error;}
int rtwn_r92e_ampdu_params(rtwn_softc *,uint8_t,uint8_t);
int rtwn_r92e_media_status(rtwn_softc *,int);
int rtwn_read_chipid(rtwn_softc *);
int rtwn_newstate(ieee80211com *,enum ieee80211_state,int);
static inline int splnet() {return 0;}
static inline void splx(int) {}
static inline void rtwn_set_led(rtwn_softc *s,int,int value) {s->ledlink=value;}
static inline void rtwn_set_nettype(rtwn_softc *s,int type) {s->nettype=type;}
static inline void rtwn_edca_init(rtwn_softc *) {}
static inline void rtwn_updateslot(ieee80211com *) {}
static inline void rtwn_update_short_preamble(ieee80211com *) {}
void rtwn_set_chan(rtwn_softc *,ieee80211_channel *,ieee80211_channel *);
static inline unsigned ieee80211_chan2ieee(ieee80211com *,ieee80211_channel *c) {return c?(c->ic_freq-2407)/5:1;}
static inline void rtwn_set_txpower(rtwn_softc *s,ieee80211_channel *,ieee80211_channel *extc) {s->extension_power=extc!=nullptr;}
static inline void rtwn_rf_write(rtwn_softc *s,int chain,uint16_t reg,uint32_t value) {if(reg==R92C_RF_CHNLBW) s->rf_output[chain]=value;}
static inline void rtwn_delay(rtwn_softc *,uint32_t) {}
static inline void rtwn_tsf_sync_enable(rtwn_softc *) {}
int rtwn_r88e_ra_init(rtwn_softc *,uint8_t,uint32_t,int,uint32_t,int);
int rtwn_ra_init(rtwn_softc *);
void rtwn_ht_capabilities(rtwn_softc *);
int rtwn_chan2group(int);
void rtwn_r92e_get_txpower(rtwn_softc *,int,ieee80211_channel *,ieee80211_channel *,uint16_t *);
static inline void require(bool ok,const char *what) { if(!ok) {std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);} }
#endif
