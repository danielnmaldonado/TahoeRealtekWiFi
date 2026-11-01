#ifndef RTW_HT_TX_HOST_HPP
#define RTW_HT_TX_HOST_HPP
/* Offline dependency boundary for verbatim controller TX/BA methods. The
 * transport records bytes without USB or RF. The crypto boundary models
 * ownership and changes the SNAP offset/content; cipher correctness is tested
 * separately by the real net80211 crypto fixtures. */
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <vector>
#include <array>
#include <algorithm>
#include "rtw/RTL8192EUDataPath.h"
#include "rtw/RTL8192CDataPath.h"
#include "rtw/RTL8814AUDataPath.h"
#include "rtw/RTL8814AUPHY.h"
#include "rtw/RTL8812AURuntime.h"
#include "rtw/RTL8723BU.h"
#include "rtw/RTW88RX.h"
#include "rtw/USBWiFiProfiles.h"
#include "rtw/RTL8192EUHT.h"
#include "rtw/USBPackets.h"
#define IEEE80211_RATE_VAL 0x7f
#define IEEE80211_RATE_BASIC 0x80
#define IEEE80211_NODE_HT 0x0400U
#define IEEE80211_S_RUN 4
#define IEEE80211_BA_AGREED 2
#define IEEE80211_BA_MAX_WINSZ 64
#define IEEE80211_AMPDU_PARAM_LE 0x03
#define IEEE80211_AMPDU_PARAM_SS 0x1c
#define IEEE80211_FC0_TYPE_MASK 0x0c
#define IEEE80211_FC0_TYPE_DATA 0x08
#define IEEE80211_FC0_SUBTYPE_QOS 0x80
#define IEEE80211_FC0_SUBTYPE_NODATA 0x40
#define IEEE80211_FC1_DIR_MASK 0x03
#define IEEE80211_FC1_DIR_DSTODS 0x03
#define IEEE80211_FC1_PROTECTED 0x40
#define IEEE80211_FC1_MORE_FRAG 0x04
#define IEEE80211_QOS_TID 0x0f
#define IEEE80211_QOS_ACK_POLICY_MASK 0x60
#define IEEE80211_QOS_ACK_POLICY_NORMAL 0
#define IEEE80211_HTOP1_PROT_MASK 0x03
#define IEEE80211_HTOP0_SCO_MASK 0x03
#define IEEE80211_HTOP0_SCO_SCA 1
#define IEEE80211_HTOP0_SCO_SCB 3
#define IEEE80211_F_SHPREAMBLE 0x00040000U
#define IEEE80211_F_USEPROT 0x00100000U
#define IEEE80211_PROT_CTSONLY 1
#define IEEE80211_PROT_RTSCTS 2
#define IEEE80211_CIPHER_CCMP 8
#define IEEE80211_ADDR_EQ(a,b) (std::memcmp(a,b,6)==0)
#define IEEE80211_STA_ONLY 1
#define MIN(a,b) ((a)<(b)?(a):(b))
#define nitems(a) (sizeof(a)/sizeof((a)[0]))
#define bzero(p,n) std::memset(p,0,n)
enum ieee80211_edca_ac {EDCA_AC_BE=0,EDCA_AC_BK=1,EDCA_AC_VI=2,EDCA_AC_VO=3};
struct ieee80211_frame {uint8_t i_fc[2]{},i_dur[2]{},i_addr1[6]{},i_addr2[6]{},i_addr3[6]{},i_seq[2]{};};
static_assert(sizeof(ieee80211_frame)==24,"802.11 base header size");
struct ieee80211_rateset {uint8_t rs_nrates=0,rs_rates[15]{};};
struct host_timeout {unsigned calls=0,last_usec=0;};
struct host_ba {unsigned ba_state=0,ba_winsize=0,ba_timeout_val=0;void *ba_buf=nullptr;host_timeout ba_to;};
struct ieee80211_channel {unsigned ic_flags=0;};
#define IEEE80211_CHAN_5GHZ 0x0100U
#define IEEE80211_CHAN_ANYC ((ieee80211_channel *)-1)
struct ieee80211_node {
    ieee80211_channel *ni_chan=nullptr;
    ieee80211_rateset ni_rates;
    uint8_t ni_txrate=0,ni_bssid[6]{2,0,0,0,0,1},ni_ampdu_param=31;
    unsigned ni_flags=IEEE80211_NODE_HT,ni_htop0=0,ni_htop1=0;
    bool ni_port_valid=true;
    host_ba ni_tx_ba[16],ni_rx_ba[16];
};
struct ieee80211com {
    int ic_state=IEEE80211_S_RUN;
    ieee80211_node *ic_bss=nullptr;
    void *ic_softc=nullptr;
    unsigned ic_flags=0,ic_protmode=0,ic_rtsthreshold=2347;
    unsigned ic_tid_noack=0;
    struct {unsigned ac_acm=0;} ic_edca_ac[4];
};
struct rtwn_softc {
    ieee80211com sc_ic;
    struct {void *cookie=nullptr;} sc_ops;
    rtw_92e_rate_config rate_config{};
    uint8_t fw_station_connected=1;
    uint8_t channel_bw40=0;
    unsigned ac2idx[4]{2,2,1,0};
    int ampdu_error=0;
    unsigned ampdu_calls=0,last_factor=0,last_density=0;
};
struct host_mbuf {std::vector<uint8_t> bytes;bool copy_error=false;};
using mbuf_t=host_mbuf*;
struct host_effects {unsigned live=0,frees=0,encryptions=0,releases=0,phy_updates=0;uint8_t phy_rate=0;bool key_missing=false,encrypt_failure=false;unsigned cipher=IEEE80211_CIPHER_CCMP;
    std::vector<std::array<uint32_t,3>> ac_writes;int ac_write_error=0;
};
inline host_effects effects;
inline int host_ac_write(void *,uint16_t address,unsigned width,uint32_t value) {
    effects.ac_writes.push_back({address,width,value});return effects.ac_write_error;
}
#ifndef RTW_HT_TX_NATIVE_8814_PHY
inline int rtw_8814au_phy_set_tx_rate(rtw_8814au *,uint8_t rate) {++effects.phy_updates;effects.phy_rate=rate;return 0;}
#endif
inline size_t mbuf_pkthdr_len(mbuf_t m) {return m->bytes.size();}
inline int mbuf_copydata(mbuf_t m,size_t offset,size_t length,void *out) {
    if(!m || m->copy_error || offset>m->bytes.size() || length>m->bytes.size()-offset)return EINVAL;
    std::memcpy(out,m->bytes.data()+offset,length);return 0;
}
inline void mbuf_freem(mbuf_t m) {if(m){++effects.frees;--effects.live;delete m;}}
struct ieee80211_key {unsigned k_cipher=IEEE80211_CIPHER_CCMP;};
inline ieee80211_key *ieee80211_get_txkey(ieee80211com *,ieee80211_frame *,ieee80211_node *) {
    static ieee80211_key key;key.k_cipher=effects.cipher;return effects.key_missing?nullptr:&key;
}
inline mbuf_t ieee80211_encrypt(ieee80211com *,mbuf_t m,ieee80211_key *) {
    ++effects.encryptions;
    if(effects.encrypt_failure){mbuf_freem(m);return nullptr;}
    size_t header=(m->bytes[0]&IEEE80211_FC0_SUBTYPE_QOS)?26U:24U;
    for(size_t i=header;i<m->bytes.size();++i)m->bytes[i]^=0x5aU;
    m->bytes.insert(m->bytes.begin()+header,8,0xcc);m->bytes.insert(m->bytes.end(),8,0xdd);
    return m;
}
inline void ieee80211_release_node(ieee80211com *,ieee80211_node *) {++effects.releases;}
inline void timeout_add_usec(host_timeout *t,unsigned usec) {++t->calls;t->last_usec=usec;}
inline int rtwn_r92e_ampdu_params(rtwn_softc *s,uint8_t factor,uint8_t density) {
    ++s->ampdu_calls;s->last_factor=factor;s->last_density=density;return s->ampdu_error;
}
using IOInterruptState=unsigned;
inline IOInterruptState IOSimpleLockLockDisableInterrupt(void *) {return 0;}
inline void IOSimpleLockUnlockEnableInterrupt(void *,IOInterruptState) {}
constexpr int kIOReturnSuccess=0;
struct IOUSBHostPipe {unsigned number=0;};
struct host_memory {std::array<uint8_t,4352> bytes{};void *getBytesNoCopy(){return bytes.data();}};
class RTWUSBController {
public:
    bool dfsTXBlocked()const{return false;}
    void advanceDFSRecovery(){}
    bool dfsAllowed_=true;
    bool dfsFrameAllowed(mbuf_t,ieee80211_node*)const{return dfsAllowed_;}

    enum {RXSlots=4,TXCapacity=4352};
    rtw_wifi_family family_=RTW_WIFI_RTL8192EU;
    bool running_=true,stopping_=false,present_=true,rateTransportFresh_=true;
    int usbError_=kIOReturnSuccess;
    void *completionLock_=nullptr;
    rtwn_softc core_;
    // Other hardware backends are isolated scheduling/readiness boundaries.
    rtw_r12au_boot auCore_{};
    rtw88_rx_phy_config r88RX_{};
    rtw88_station_rate r88Rates_{};
    unsigned r23TXCalls=0;bool r23FirmwareReady_=false;rtw_8723bu_rates r23Rates_{};
    bool is8723Family()const{return family_==RTW_WIFI_RTL8723BU;}
    bool uses8723FirmwareRate()const{return is8723Family()&&r23FirmwareReady_;}
    int send8723Frame(mbuf_t p,ieee80211_node*){++r23TXCalls;mbuf_freem(p);return ENOTSUP;}
    unsigned buTXCalls=0,buBACalls=0;bool buFirmwareReady_=false,buHT_=false;
    bool is8710BUFamily()const{return family_==RTW_WIFI_RTL8710BU;}
    bool uses8710BUFirmwareRate()const{return is8710BUFamily()&&buFirmwareReady_;}
    bool uses8710BUHighThroughput()const{return uses8710BUFirmwareRate()&&buHT_;}
    static int ampdu8710BUTXStart(ieee80211com*ic,ieee80211_node*,uint8_t){auto*c=static_cast<RTWUSBController*>(static_cast<rtwn_softc*>(ic->ic_softc)->sc_ops.cookie);++c->buBACalls;return ENOTSUP;}
    int send8710BUFrame(mbuf_t p,ieee80211_node*){++buTXCalls;mbuf_freem(p);return ENOTSUP;}
    unsigned fuTXCalls=0,fuBACalls=0;bool fuFirmwareReady_=false,fuHT_=false;
    bool is8192FUFamily()const{return family_==RTW_WIFI_RTL8192FU;}
    bool uses8192FUFirmwareRate()const{return is8192FUFamily()&&fuFirmwareReady_;}
    bool uses8192FUHighThroughput()const{return uses8192FUFirmwareRate()&&fuHT_;}
    static int ampdu8192FUTXStart(ieee80211com*ic,ieee80211_node*,uint8_t){auto*c=static_cast<RTWUSBController*>(static_cast<rtwn_softc*>(ic->ic_softc)->sc_ops.cookie);++c->fuBACalls;return ENOTSUP;}
    int send8192FUFrame(mbuf_t p,ieee80211_node*){++fuTXCalls;mbuf_freem(p);return ENOTSUP;}
    bool auFirmwareReady_=false,r88FirmwareReady_=false;
    bool is8812Family()const{return family_==RTW_WIFI_RTL8812AU||family_==RTW_WIFI_RTL8821AU;}
    bool is88Family()const{return family_==RTW_WIFI_RTL8821CU||family_==RTW_WIFI_RTL8822BU||family_==RTW_WIFI_RTL8822CU;}
    bool uses8812FirmwareRate()const{return is8812Family()&&auFirmwareReady_;}
    bool uses8812HighThroughput()const{return false;}
    static int ampdu8812TXStart(ieee80211com*,ieee80211_node*,uint8_t){return ENOTSUP;}
    bool uses88FirmwareRate()const{return is88Family()&&r88FirmwareReady_;}
    rtw_8814au acCore_{};
    rtw_8814au_rate_config acRateConfig_{};
    bool acRateReady_=false,acMediaConnected_=false;
    rtw_transfer_ledger ledger_{};
    struct {unsigned num_bulk_out=3;} endpoints_;
    host_memory storage_[RTW_TRANSFER_SLOTS];
    struct {host_memory *memory=nullptr;} transfers_[RTW_TRANSFER_SLOTS];
    IOUSBHostPipe pipeStorage_[3]{{0},{1},{2}},*txPipes_[3]{};
    uint16_t txMPS_[3]{512,512,512};
    struct {bool active=true,requestedSinceReset=false;uint64_t requests=0,pendingReports=0,htSampleCounter=0,reports=0,invalid=0,ignored=0;uint8_t finalRate=0,bssid[6]{2,0,0,0,0,1};} rateControl_;
    uint64_t txHTPackets_=0,txAMPDUPackets_=0,rxBARPackets_=0;
    unsigned submitted=0,last_slot=0,last_pipe=0;
    int submit_error=0;
    std::vector<uint8_t> last_transfer;
    RTWUSBController() {
        core_.sc_ic.ic_softc=&core_;core_.sc_ops.cookie=this;
        for(unsigned i=0;i<RTW_TRANSFER_SLOTS;++i)transfers_[i].memory=&storage_[i];
        for(unsigned i=0;i<3;++i)txPipes_[i]=&pipeStorage_[i];
    }
    bool authorized() const {return core_.sc_ic.ic_bss&&core_.sc_ic.ic_bss->ni_port_valid;}
    static RTWUSBController *owner(void *p){return static_cast<RTWUSBController*>(p);}
    int submit(unsigned slot,IOUSBHostPipe *pipe,uint32_t length) {
        ++submitted;last_slot=slot;last_pipe=pipe->number;
        auto *bytes=storage_[slot].bytes.data();last_transfer.assign(bytes,bytes+length);
        return submit_error;
    }
    int send8192DUFrame(mbuf_t packet,ieee80211_node*){if(packet)mbuf_freem(packet);return ENOTSUP;}
    int send8812Frame(mbuf_t packet,ieee80211_node*){mbuf_freem(packet);return ENOTSUP;}
    int send88Frame(mbuf_t packet,ieee80211_node*){mbuf_freem(packet);return ENOTSUP;}
    int sendFrame(mbuf_t,ieee80211_node *);
    bool usesFirmwareRate() const;
    bool usesLegacyFirmwareRate() const;
    bool uses8814FirmwareRate() const;
    void handle8814C2H(const uint8_t *,size_t);
    static int ampduRXStart(ieee80211com *,ieee80211_node *,uint8_t);
    static int ampduTXStart(ieee80211com *,ieee80211_node *,uint8_t);
    static void ampduRXStop(ieee80211com *,ieee80211_node *,uint8_t);
    static void ampduTXStop(ieee80211com *,ieee80211_node *,uint8_t);
};
enum ieee80211_edca_ac ieee80211_up_to_ac(ieee80211com *,int);
#endif
