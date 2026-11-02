#pragma once
#include "RTWControl.h"
#include "RTWBandCapabilities.hpp"
#include "rtw/USBWiFiProfiles.h"
#include "rtw/SignalSample.h"
#include "rtw/DFSStation.h"
#include "rtw/ScanDiagnostics.hpp"
#include "rtw/RTL8814AU.h"
#include "rtw/RTL8814AUPHY.h"
#ifdef RTW_DFS_ACTUAL
#include "rtw/RTL8812AUPHY.h"
#include "rtw/RTW88PHYOperating.h"
#include "rtw/DFSRegulatory.h"
#endif
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include "rtw/DiagnosticPublishPolicy.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <vector>
using IOReturn = int;
using UInt32 = uint32_t;
using UInt64 = uint64_t;
struct IONetworkMedium {};
struct OSData {};
struct OSObject {
    virtual ~OSObject() = default;
};
struct IOWorkLoop { unsigned releases = 0; void release() { ++releases; } };
struct IOCommandGate {
    OSObject *target = nullptr;
    bool reject = false;
    unsigned releases = 0;
    using Action = IOReturn (*)(OSObject *, void *, void *, void *, void *);
    IOReturn runAction(Action action, void *op, void *arg) {
        return reject ? -2 : action(target, op, arg, nullptr, nullptr);
    }
    void release() { ++releases; }
};
struct IOBasicOutputQueue { enum { kServiceAsync = 1 }; };
struct IOInterruptEventSource {};
struct IOTimerEventSource {
    unsigned scheduled = 0, cancelled = 0;
    void setTimeoutMS(unsigned) { ++scheduled; }
    void cancelTimeout() { ++cancelled; }
};
enum ieee80211_state {
    IEEE80211_S_INIT,
    IEEE80211_S_SCAN,
    IEEE80211_S_AUTH,
    IEEE80211_S_ASSOC,
    IEEE80211_S_RUN
};
enum {
    kIOReturnSuccess = 0,
    kIOReturnBadArgument = -1,
    kIOReturnNotReady = -2,
    kIOReturnUnsupported = -3,
    kIOReturnNoDevice = -4,
    kIOReturnIOError = -5,
    kIOReturnBusy = -6,
    kIOReturnUnderrun = -7,
    kIOReturnOutputDropped = 1,
    kIOReturnOutputSuccess = 0,
    kIOReturnOutputStall = 0x102,
    kIONetworkLinkValid = 1,
    kIONetworkLinkActive = 2,
    IFF_UP = 1,
    IFF_RUNNING = 2,
    LINK_STATE_DOWN = 0,
    IEEE80211_F_RSNON = 0x100,
    IEEE80211_F_PSK = 0x200,
    IEEE80211_F_AUTO_JOIN = 0x400,
    IEEE80211_WPA_PROTO_WPA2 = 2,
    IEEE80211_WPA_AKM_PSK = 2,
    IEEE80211_WPA_CIPHER_CCMP = 8
};
struct Packet {
    std::vector<uint8_t> bytes;
    unsigned frees = 0;
    void *node = nullptr;
};
using mbuf_t = Packet *;
inline void mbuf_freem(mbuf_t m) {
    if (++m->frees != 1)
        std::abort();
}
inline void *mbuf_pkthdr_rcvif(mbuf_t m) { return m->node; }
struct IOPacketQueue {
    std::deque<mbuf_t> packets;
    unsigned stops = 0, flushes = 0, dequeues = 0, capacity = 2048, services = 0;
    unsigned serviceOptions = 0;
    bool rejectEnqueue = false;
    bool lockEnqueue(mbuf_t m) {
        if (rejectEnqueue || packets.size() >= capacity)
            return false;
        packets.push_back(m);
        return true;
    }
    mbuf_t lockDequeue() {
        ++dequeues;
        if (packets.empty())
            return nullptr;
        auto *m = packets.front();
        packets.pop_front();
        return m;
    }
    void lockFlush() {
        ++flushes;
        while (!packets.empty())
            mbuf_freem(lockDequeue());
    }
    void stop() { ++stops; }
    void start() {}
    void flush() { lockFlush(); }
    unsigned getSize() const { return unsigned(packets.size()); }
    unsigned getCapacity() const { return capacity; }
    bool service(unsigned options) { ++services; serviceOptions = options; return true; }
};
struct _ifqueue {
    unsigned ifq_oactive = 0;
    IOPacketQueue *queue = nullptr;
};
struct ieee80211_node {
    bool ni_port_valid = false;
    unsigned ni_intval = 100, ni_rsn_supp_state = 1, refs = 1;
    uint8_t ni_bssid[6]{}, ni_essid[32]{};
    unsigned ni_esslen = 1, ni_rssi = 0;
    unsigned hostTreeIndex = 0;
    void *ni_chan = nullptr;
};
struct HostNodeTree {std::vector<ieee80211_node*> nodes;};
inline ieee80211_node* hostRBMin(HostNodeTree*tree){return tree->nodes.empty()?nullptr:tree->nodes[0];}
inline ieee80211_node* hostRBNext(HostNodeTree*tree,ieee80211_node*node){
    const size_t index=(size_t)node->hostTreeIndex+1;
    return index<tree->nodes.size()?tree->nodes[index]:nullptr;
}
#define RB_MIN(name,tree) hostRBMin(tree)
#define RB_NEXT(name,tree,node) hostRBNext(tree,node)
#define IEEE80211_CHAN_ANYC reinterpret_cast<void *>(~uintptr_t(0))
inline unsigned ieee80211_chan2ieee(void *, void *channel) {
    const uintptr_t number=reinterpret_cast<uintptr_t>(channel);
    return number>=1&&number<=196?(unsigned)number:1;
}
struct _ifnet {
    unsigned if_flags = IFF_UP | IFF_RUNNING, if_timer = 0, if_oerrors = 0;
    void *if_softc = nullptr;
    _ifqueue if_snd;
    void (*if_watchdog)(_ifnet *) = nullptr;
};
constexpr unsigned IEEE80211_CHAN_5GHZ=0x0100,IEEE80211_CHAN_2GHZ=0x0080;
struct ieee80211com {
    HostNodeTree ic_tree;
    struct {unsigned ic_flags=0;} ic_channels[256];
    void *ic_softc = nullptr;
    _ifnet ic_if;
    ieee80211_state ic_state = IEEE80211_S_ASSOC;
    ieee80211_node *ic_bss = nullptr;
    unsigned ic_flags = IEEE80211_F_RSNON | IEEE80211_F_PSK, ic_des_esslen = 1;
    uint8_t ic_psk[32]{}, ic_des_essid[32]{'x'},ic_myaddr[6]{2,6,7,8,9,10};
    std::deque<mbuf_t> ic_mgtq;
};
struct rtwn_softc {
    ieee80211com sc_ic;
    bool core_error = false;
    unsigned sc_tx_timer = 0;
    struct {
        void *cookie = nullptr;
        int (*is_oactive)(void *) = nullptr;
        int (*tx)(void *, mbuf_t, ieee80211_node *) = nullptr;
    } sc_ops;
};
struct ieee80211_wpaparams {
    int i_enabled = 0, i_protos = 0, i_akms = 0, i_ciphers = 0, i_groupcipher = 0;
};
inline void bzero(void *p, size_t n) { std::memset(p, 0, n); }
inline void explicit_bzero(void *p, size_t n) { std::memset(p, 0, n); }
class IOEthernetController {
  public:
    unsigned lastLinkState = 0;
    bool setLinkStatus(unsigned state, const IONetworkMedium * = nullptr, uint64_t = 0, OSData * = nullptr) {
        lastLinkState = state;
        return true;
    }
};
#define super IOEthernetController
#ifdef RTW_DFS_ACTUAL
struct DFSPowerBoundary{static unsigned calls;static uint8_t db;static int error;static int32_t limit;static std::vector<int> events;static int cap(int32_t value){limit=value;events.push_back(0);return error;}static int apply(uint8_t value){++calls;db=value;events.push_back(1);return error;}};
namespace StandardUSB {struct DeviceRequest{uint8_t bmRequestType,bRequest;uint16_t wValue,wIndex,wLength;};}
inline uint16_t OSSwapHostToLittleInt16(uint16_t n){return n;}
struct DFSUSB {
 struct Wire{uint16_t address;std::vector<uint8_t> bytes;};std::vector<Wire>wires;
 IOReturn deviceRequest(const StandardUSB::DeviceRequest&r,uint8_t*b,uint32_t&actual,unsigned){actual=r.wLength;if(!(r.bmRequestType&0x80))wires.push_back({r.wValue,std::vector<uint8_t>(b,b+r.wLength)});return 0;}
};
#endif
class RTWUSBController : public IOEthernetController, public OSObject {
  public:
    enum GateOperation {
        Initialize = 1,
        Shutdown,
        Enable,
        Disable,
        Output,
        Control,
        Terminated,
        Sleep,
        Wake,
        TeardownReady
    };
#include "wpa_recovery_state.hpp"
    struct OutputCall {
        mbuf_t packet;
        bool consumed;
    };
    struct ControlCall {
        uint32_t selector;
        const void *input;
        size_t length;
        void *output;
        size_t *outputLength;
    };
    rtw_dfs_station dfsStation_{};
    uint8_t dfsTunedChannel_=0,dfsBoundChannel_=0,dfsBoundBSSID_[6]{};
    bool dfsBound_=false,dfsTXHold_=false;
    bool permitsDFSChannelByCountry(uint8_t)const{return true;}
    int16_t factoryMBM_=2000;
    int16_t factoryDFSChannelLimitMBM(uint8_t)const{return factoryMBM_;}
#ifdef RTW_DFS_ACTUAL
    bool dfsTXBlocked()const;
    bool dfsStateAllowed(ieee80211_state,unsigned)const;
    void dfsStateChanged(ieee80211_state,unsigned,int);
    bool dfsFrameAllowed(mbuf_t,ieee80211_node*)const;
    void observeDFSFrame(const uint8_t*,size_t,unsigned);
    void resetDFSStation(bool=false);
    void stopDFSStation();
    void advanceDFSRecovery();
    DFSUSB usbValue_;DFSUSB*usb_=&usbValue_;bool usbOpened_=true;
    int vendorRequest(bool,uint16_t,uint8_t*,size_t);
    uint8_t dfsAppliedReduction_=0;
    int32_t dfsAppliedLimitMBM_=INT32_MAX;
    rtw_r12au_phy*auPHY_=nullptr;rtw88_phy_context r88PHY_{};
    int prepareDFSPower(unsigned);
    int candidateDFSPower(uint8_t,int32_t*,uint8_t*)const;
    int set8192DUPowerReduction(uint8_t db){return DFSPowerBoundary::apply(db);}
    int set8192DUPowerLimitMBM(int32_t mbm){return DFSPowerBoundary::cap(mbm);}
    unsigned pauseCalls=0;int pauseError=0;
    int pauseDFSStationTX(){++pauseCalls;uint8_t pause=255;(void)vendorRequest(false,0x522,&pause,1);return pauseError;}
#else
    bool dfsTXBlocked()const{return false;}
    bool dfsStateAllowed(ieee80211_state,unsigned)const{return true;}
    void dfsStateChanged(ieee80211_state,unsigned,int){}
    bool dfsFrameAllowed(mbuf_t,ieee80211_node*)const{return true;}
    void resetDFSStation(bool=false){}
    void stopDFSStation(){}
    void advanceDFSRecovery(){}
    int prepareDFSPower(unsigned){return 0;}
#endif
    rtwn_softc core_;
    rtw::ScanDiagnostics scanDiag_{};
    bool browseScanComplete_=false;
    int transitionError=0;
    bool clearChannelOnTransitionError=false;
    void publishScanDiagnostics();
    rtw_signal_sample signal_{};
    rtw_wifi_family family_ = RTW_WIFI_RTL8192EU;
    rtw_8814au acCore_{};
    bool acRateReady_ = false, acMediaConnected_ = false;
    bool attached_ = true, running_ = true, radioReady_ = true, joinRequested_ = true, present_ = true,
         sleeping_ = false, stopping_ = false, enabled_ = true, closing_ = false;
    IOReturn usbError_ = kIOReturnSuccess;
    unsigned txErrors_ = 0;
    uint64_t txLinkDrops_ = 0, txQueueDrops_ = 0;
    struct Stats {
        uint32_t outputErrors = 0;
    } statsValue_;
    Stats *stats_ = &statsValue_;
    uint64_t lastBeaconMs_ = 0;
    uint64_t txPackets_ = 0, rxPackets_ = 0, rxErrors_ = 0;
    struct {
        uint8_t bytes[6]{};
    } mac_;
    void *completionLock_ = nullptr;
    IOTimerEventSource scanTimerValue_, calibTimerValue_, watchdogTimerValue_;
    IOTimerEventSource *scanTimer_ = &scanTimerValue_, *calibTimer_ = &calibTimerValue_,
                       *watchdogTimer_ = &watchdogTimerValue_;
    IOPacketQueue outputQueue_, pendingQueue_;
    IOWorkLoop hostLoop_;
    IOCommandGate hostGate_;
    bool hostNoGate_ = false;
    std::map<std::string, uint64_t> properties;
    std::vector<ieee80211_state> transitions;
    unsigned watchdogCalls = 0, initCalls = 0, stopCalls = 0, rateResets = 0, scanCalls = 0;
    unsigned legacyStateCalls = 0, acStateCalls = 0, acStopCalls = 0;
    unsigned r23StateCalls=0,r23StopCalls=0;
    bool r23RateReady_=false,r23Media_=false;
    bool is8723Family()const{return family_==RTW_WIFI_RTL8723BU;}
    bool uses8723FirmwareRate()const{return is8723Family()&&r23RateReady_&&r23Media_&&running_&&!stopping_&&core_.sc_ic.ic_state==IEEE80211_S_RUN;}
    int state8723(ieee80211com*,ieee80211_state,int);
    void stop8723();
    unsigned buStateCalls=0,buStopCalls=0;
    bool buRateReady_=false,buMedia_=false;
    bool is8710BUFamily()const{return family_==RTW_WIFI_RTL8710BU;}
    bool uses8710BUFirmwareRate()const{return is8710BUFamily()&&buRateReady_&&buMedia_&&running_&&!stopping_&&core_.sc_ic.ic_state==IEEE80211_S_RUN;}
    int state8710BU(ieee80211com*,ieee80211_state,int);
    void stop8710BU();
    unsigned fuStateCalls=0,fuStopCalls=0;
    bool fuRateReady_=false,fuMedia_=false;
    bool is8192FUFamily()const{return family_==RTW_WIFI_RTL8192FU;}
    bool uses8192FUFirmwareRate()const{return is8192FUFamily()&&fuRateReady_&&fuMedia_&&running_&&!stopping_&&core_.sc_ic.ic_state==IEEE80211_S_RUN;}
    int state8192FU(ieee80211com*,ieee80211_state,int);
    void stop8192FU();
    unsigned duStateCalls=0,duStopCalls=0;
    bool duRateReady_=false,duMedia_=false;
    bool uses8192DUFirmwareRate()const{return family_==RTW_WIFI_RTL8192DU&&duRateReady_&&duMedia_&&running_&&!stopping_&&core_.sc_ic.ic_state==IEEE80211_S_RUN;}
    int state8192DU(ieee80211com*,ieee80211_state,int);
    void stop8192DU();
    unsigned r88StateCalls=0,r88StopCalls=0;
    bool r88RateReady_=false,r88Media_=false;
    bool is88Family()const{return family_==RTW_WIFI_RTL8821CU||family_==RTW_WIFI_RTL8822BU||family_==RTW_WIFI_RTL8822CU;}
    bool uses88FirmwareRate()const{return is88Family()&&r88RateReady_&&r88Media_&&running_&&!stopping_&&core_.sc_ic.ic_state==IEEE80211_S_RUN;}
    bool supports88Band(bool)const{return false;}
    int state88(ieee80211com*,ieee80211_state,int);
    void stop88();
    unsigned auStateCalls=0,auStopCalls=0;
    bool auRateReady_=false,auMediaConnected_=false;
    bool is8812Family()const{return family_==RTW_WIFI_RTL8812AU||family_==RTW_WIFI_RTL8821AU;}
    bool uses8812FirmwareRate()const{return is8812Family()&&auRateReady_&&auMediaConnected_&&running_&&!stopping_&&core_.sc_ic.ic_state==IEEE80211_S_RUN;}
    bool supports8812Band(bool)const{return false;} // Radio capability is isolated in the AU HAL suites.
    int state8812(ieee80211com*,ieee80211_state,int);
    void stop8812();
    static uint64_t clockMS;
    RTWUSBController();
    static RTWUSBController *owner(void *p) { return static_cast<RTWUSBController *>(p); }
    static uint64_t nowMS() { return clockMS; }
    bool authorized() const;
    IOCommandGate *copyGate(IOWorkLoop **);
    UInt32 outputPacket(mbuf_t, void *);
    void serviceTXOutputQueue();
    uint32_t supported5GHzFlags() const;
    bool uses8814FirmwareRate() const {
        return family_ == RTW_WIFI_RTL8814AU && acRateReady_ && acMediaConnected_ &&
            running_ && !stopping_ && core_.sc_ic.ic_state == IEEE80211_S_RUN && core_.sc_ic.ic_bss;
    }
    int state8814(ieee80211com *, ieee80211_state, int);
    void stop8814();
    // AU recovery has its own actual-startup/control/native-HAL fixture.
    int recover8814ForControl(){return core_.core_error;}
    int recoverFamilyForControl(){return core_.core_error;}
    void finish8814StopAfterDrain(){}
    enum{AUThermal=19};
    void record8814Failure(uint32_t,int){}
    bool setLinkStatus(UInt32, const IONetworkMedium * = nullptr, UInt64 = 0, OSData * = nullptr);
    static int stateTransition(ieee80211com *, ieee80211_state, int);
    void observeScanTransition(ieee80211_state,ieee80211_state,unsigned,int);
    static void watchdogAction(OSObject *, IOTimerEventSource *);
    static IOReturn gated(OSObject *, void *, void *, void *, void *);
    void resetWpaRecovery();
    void cancelWpaRecovery();
    void wpaStateChanged(ieee80211_state);
    /* USB recovery is exercised by the separate real transport-method suite. */
    struct { bool pending = false; } rxRecovery_;
    void advanceRXRecovery() {}
    void advanceWpaRecovery();
    void publishWpaRecoveryDiagnostics();
    void eraseProfile();
    void quiesce(bool);
    IOReturn handleControl(ControlCall *);
    void resetRateControl() { ++rateResets; }
    IOPacketQueue *getOutputQueue() { return &outputQueue_; }
    void abortTransfers() { running_ = false; }
    void fault(IOReturn e) { usbError_ = e; }
    IOReturn initializePlatform() { return 0; }
    void shutdownPlatform() { quiesce(true); }
    bool teardownReady_ = false;
    void scheduleDeferredCleanup() {}
    IOReturn enableStation();
    void status(RTWControlStatus *);
    void scanResults(RTWControlScanResults *) {}
    void channelMap(RTWControlChannelMap *) const {}
    void publishDiagnostics() { publishWpaRecoveryDiagnostics();publishScanDiagnostics(); }
    void setProperty(const char *n, uint64_t v, unsigned) { properties[n] = v; }
    void publishDiagnosticCounter(const char *name, uint64_t value) {
        const auto current = properties.find(name);
        if (current == properties.end() ||
            rtw::diagnosticCounterNeedsWrite(true, 64, current->second, value)) {
            setProperty(name, value, 64);
        }
    }
};
int rtwn_newstate(ieee80211com *, ieee80211_state, int);
int rtwn_io_error(rtwn_softc *);
void rtwn_stop(_ifnet *);
inline void ieee80211_set_link_state(ieee80211com *, int) {}
void ieee80211_begin_scan(_ifnet *);
inline int ieee80211_ioctl_setwpaparms(ieee80211com *ic, ieee80211_wpaparams *) {
    ic->ic_flags |= IEEE80211_F_RSNON;
    return ENETRESET;
}
inline IOReturn fromErrno(int e) { return e; }
inline bool rtw_92e_beacon_expired(uint64_t, uint64_t, unsigned) { return false; }
inline void ifq_flush(_ifqueue *q) {
    if (q->queue)
        q->queue->lockFlush();
}
inline void ifq_set_oactive(_ifqueue *q) { q->ifq_oactive = 1; }
inline unsigned ifq_is_oactive(_ifqueue *q) { return q->ifq_oactive; }
mbuf_t ifq_dequeue(_ifqueue *);
inline mbuf_t mq_dequeue(std::deque<mbuf_t> *q) {
    if (q->empty())
        return nullptr;
    auto *m = q->front();
    q->pop_front();
    return m;
}
inline void ieee80211_release_node(ieee80211com *, ieee80211_node *n) {
    if (!n->refs)
        std::abort();
    --n->refs;
}
inline mbuf_t ieee80211_encap(_ifnet *, mbuf_t m, ieee80211_node **n) {
    *n = static_cast<ieee80211_node *>(m->node);
    return m;
}
void rtwn_start(_ifnet *);

#ifdef RTW_DFS_ACTUAL
struct ieee80211_frame {uint8_t i_fc[2],i_dur[2],i_addr1[6],i_addr2[6],i_addr3[6],i_seq[2];};
constexpr uint8_t IEEE80211_FC0_TYPE_MGT=0,IEEE80211_FC0_SUBTYPE_PROBE_REQ=0x40;
inline size_t mbuf_pkthdr_len(mbuf_t m){return m->bytes.size();}
inline int mbuf_copydata(mbuf_t m,size_t o,size_t n,void*p){if(o>m->bytes.size()||n>m->bytes.size()-o)return 1;std::memcpy(p,m->bytes.data()+o,n);return 0;}
#endif
