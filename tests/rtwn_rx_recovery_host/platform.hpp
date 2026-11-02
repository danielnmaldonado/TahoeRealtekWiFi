#pragma once
#include "rtw/RTL8192EUDataPath.h"
#include "rtw/USBDescriptors.h"
#include "rtw/FamilyBackend.h"
#include "rtw/USBRXCapacity.h"
#ifdef RTW_RX_CAPACITY_PREFLIGHT
#include "rtw/RTL8192EU.h"
#endif
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <vector>
using IOReturn = int;
using IOInterruptState = int;
struct IOSimpleLock {};
inline int IOSimpleLockLockDisableInterrupt(IOSimpleLock*) { return 0; }
inline std::function<void()> hostAfterUnlock;
inline bool hostUnlockRunning=false;
inline void IOSimpleLockUnlockEnableInterrupt(IOSimpleLock*,int) {
    if(hostAfterUnlock&&!hostUnlockRunning){
        hostUnlockRunning=true;auto action=hostAfterUnlock;action();hostUnlockRunning=false;
    }
}
enum { kIOReturnSuccess=0, kIOReturnNotReady=-1, kIOReturnNotResponding=-2, kIOReturnAborted=-3,
       kIOReturnUnderrun=-4, kIOReturnIOError=-5, kIOReturnNoDevice=-6, kIOReturnTimeout=-7,
       IFF_RUNNING=1, IEEE80211_S_SCAN=1, IEEE80211_S_RUN=2, LINK_STATE_DOWN=0 };
struct OSObject { virtual ~OSObject()=default; };
struct IOTimerEventSource { unsigned scheduled=0,cancelled=0,lastDelay=0;void setTimeoutMS(unsigned delay){++scheduled;lastDelay=delay;}void cancelTimeout(){++cancelled;} };
struct IOInterruptEventSource { unsigned events=0;bool pending=false;void interruptOccurred(void*,void*,int){++events;pending=true;} };
struct IOBufferMemoryDescriptor {
    std::vector<uint8_t> bytes=std::vector<uint8_t>(32768);
    inline static unsigned allocationCalls=0,failAllocation=0,liveAllocations=0;
    bool heapAllocated=false;
    unsigned prepares=0,completes=0;
    bool prepared=false;
    IOReturn prepareError=0;
    IOReturn prepare(){if(prepareError)return prepareError;if(prepared)std::abort();prepared=true;++prepares;return 0;}
    void complete(){if(!prepared)std::abort();prepared=false;++completes;}
    size_t getCapacity()const{return bytes.size();}
    void* getBytesNoCopy(){return bytes.data();}
    static IOBufferMemoryDescriptor* withCapacity(size_t capacity,unsigned){
        if(++allocationCalls==failAllocation)return nullptr;
        auto*p=new IOBufferMemoryDescriptor;p->bytes.resize(capacity);p->heapAllocated=true;++liveAllocations;return p;
    }
    void release(){if(!heapAllocated||prepared)std::abort();--liveAllocations;delete this;}
};
struct IOUSBHostCompletion { void* owner;void(*action)(void*,void*,IOReturn,uint32_t);void* parameter; };
struct IOUSBHostIOSource { enum { kAbortSynchronous=1 }; };
struct IOUSBHostPipe {
    struct Request { IOUSBHostCompletion callback; IOBufferMemoryDescriptor* memory; uint32_t length; bool done=false; };
    struct Endpoint { uint8_t bEndpointAddress=0x81; } endpoint;
    std::vector<Request> requests;
    unsigned clearCalls=0,abortCalls=0,ioCalls=0;
    IOReturn clearError=0,ioError=0;
    std::vector<IOReturn> clearResults;
    std::map<unsigned,IOReturn> ioResults;
    unsigned failIOCall=0;
    bool deferAbort=false,withRequest=false,halted=false;
    std::function<void()> onClear,onIO,onAccepted,onAbort;
    IOReturn io(IOBufferMemoryDescriptor*memory,uint32_t length,IOUSBHostCompletion*callback,unsigned){
        ++ioCalls;if(onIO)onIO();
        if(halted)return kIOReturnNotResponding;
        IOReturn result=ioResults.count(ioCalls)?ioResults[ioCalls]:ioError;
        if(!result&&failIOCall&&ioCalls==failIOCall)result=kIOReturnIOError;
        if(result){if(result!=kIOReturnTimeout)halted=true;return result;}
        requests.push_back({*callback,memory,length,false});if(onAccepted)onAccepted();return 0;
    }
    void finish(unsigned index,IOReturn result,uint32_t length){
        auto&r=requests.at(index);if(r.done)std::abort();r.done=true;
        if(result!=kIOReturnSuccess&&result!=kIOReturnTimeout&&result!=kIOReturnAborted)halted=true;
        r.callback.action(r.callback.owner,r.callback.parameter,result,length);
    }
    void duplicate(unsigned index,IOReturn result,uint32_t length){auto&r=requests.at(index);r.callback.action(r.callback.owner,r.callback.parameter,result,length);}
    IOReturn abort(unsigned){++abortCalls;if(onAbort)onAbort();if(!deferAbort){for(unsigned i=0,n=requests.size();i<n;++i)if(!requests[i].done)finish(i,kIOReturnAborted,0);}return 0;}
    IOReturn clearStall(bool request){
        ++clearCalls;withRequest=request;if(onClear)onClear();abort(1);
        IOReturn result=clearCalls<=clearResults.size()?clearResults[clearCalls-1]:clearError;
        if(result==kIOReturnSuccess)halted=false;
        return result;
    }
    const Endpoint* getEndpointDescriptor()const{return &endpoint;}
};
enum {kIODirectionIn=1,kIODirectionOut=2,R92C_ACLK_MON=0,R92C_USB_HRPWM=1,R88E_TX_RPT_CTRL=2,R88E_TX_RPT_CTRL_EN=1};
struct IONetworkStats { uint64_t inputErrors=0,outputPackets=0,outputErrors=0; };
struct ieee80211_node {unsigned ni_port_valid=0,ni_intval=100;};
inline void hostWatchdog(void*){}
struct ieee80211com {struct { unsigned if_flags=IFF_RUNNING;int if_snd=0;void(*if_watchdog)(void*)=hostWatchdog;void*iface=reinterpret_cast<void*>(1); } ic_if;unsigned ic_state=IEEE80211_S_RUN;ieee80211_node node,*ic_bss=&node;};
struct rtwn_softc { int core_error=0,sc_tx_timer=0,thcal_lctemp=0,thcal_state=0;unsigned timerHardwareCalls=0;ieee80211com sc_ic; };
inline int rtwn_io_error(rtwn_softc*c){return c->core_error;}
inline void rtwn_next_scan(rtwn_softc*c){++c->timerHardwareCalls;}
inline void rtwn_calib(rtwn_softc*c){++c->timerHardwareCalls;}
struct HostACCore {unsigned timerHardwareCalls=0;};
inline int rtw_8814au_phy_tick(HostACCore*c){++c->timerHardwareCalls;return 0;}
inline bool rtw_8814au_make_rssi_monitor(uint8_t,uint8_t*){return false;}
inline int rtw_8814au_h2c_mailbox(HostACCore*c,uint8_t*){++c->timerHardwareCalls;return 0;}
inline void ifq_clr_oactive(int*){}
inline void rtwn_start(void*){}
inline void ieee80211_set_link_state(ieee80211com*,int){}
struct HostOutputQueue {void stop(){}void flush(){}};
class RTWUSBController: public OSObject {
public:
    bool dfsTXBlocked()const{return false;}
    void advanceDFSRecovery(){}

    enum { RXSlots=4,RXCapacity=16384,TXCapacity=4352 };
    struct Transfer {RTWUSBController*owner=nullptr;IOBufferMemoryDescriptor*memory=nullptr;IOUSBHostPipe*pipe=nullptr;unsigned index=0;uint32_t submitted=0,actual=0,generation=0;IOReturn result=0;bool prepared=false;};
    Transfer transfers_[RTW_TRANSFER_SLOTS]{};
    IOBufferMemoryDescriptor memory_[RTW_TRANSFER_SLOTS];
    IOUSBHostPipe rx,tx;
    IOUSBHostPipe*rxPipe_=&rx,*txPipes_[4]{&tx,nullptr,nullptr,nullptr};
    rtw_usb_endpoints endpoints_{};
    IOSimpleLock lock;IOSimpleLock*completionLock_=&lock;
    IOInterruptEventSource event;IOInterruptEventSource*completionEvent_=&event;
    IOTimerEventSource timer;IOTimerEventSource*scanTimer_=&timer,*calibTimer_=&timer,*watchdogTimer_=&timer;
    rtw_transfer_ledger ledger_{};rtwn_softc core_{};IONetworkStats stats;IONetworkStats*stats_=&stats;
    bool running_=true,stopping_=false,present_=true,sleeping_=false,attached_=true,radioReady_=true,enabled_=true,rateTransportFresh_=true,closing_=false;
    IOReturn usbError_=0;rtw_wifi_family family_=RTW_WIFI_RTL8814AU;
    uint64_t txPackets_=0,rxPackets_=0,txErrors_=0,rxErrors_=0,txUSBTransferErrors_=0;
    unsigned references=1,quiesces=0,receivedCalls=0,cleanupCalls=0;
    bool usefulReceive=true;
    std::vector<unsigned> received;
    std::map<std::string,uint64_t> diagnostics;
    inline static uint64_t clockMS=100;
#include "rx_recovery_state.hpp"
#include "calibration_state.hpp"
#ifdef RTW_RX_CAPACITY_PREFLIGHT
#include "rx_diagnostics_state.hpp"
    void*netif_=reinterpret_cast<void*>(1);
    rtw_packet_error decodeUSBFrame(rtw_rx_iterator*,rtw_8192eu_rx_frame*,uint8_t*)const;
#endif
    HostACCore acCore_;uint8_t acRSSI_=0;unsigned familyTimerCalls=0;
    bool is8723Family()const{return family_==RTW_WIFI_RTL8723BU;}
    bool is8710BUFamily()const{return family_==RTW_WIFI_RTL8710BU;}
    bool is8192FUFamily()const{return family_==RTW_WIFI_RTL8192FU;}
    bool is88Family()const{return family_==RTW_WIFI_RTL8821CU||family_==RTW_WIFI_RTL8822BU||family_==RTW_WIFI_RTL8822CU;}
    bool is8812Family()const{return family_==RTW_WIFI_RTL8812AU||family_==RTW_WIFI_RTL8821AU;}
    int tick8723(){++familyTimerCalls;return 0;}int tick8710BU(){++familyTimerCalls;return 0;}
    int tick8192FU(){++familyTimerCalls;return 0;}int tick8192DU(){++familyTimerCalls;return 0;}
    int tick88(){++familyTimerCalls;return 0;}int tick8812(){++familyTimerCalls;return 0;}
    bool uses8814FirmwareRate()const{return false;}void chooseRateControl(){}
    RTWUSBController(){endpoints_.bulk_in[0]=0x81;for(unsigned i=0;i<RTW_TRANSFER_SLOTS;++i){transfers_[i].owner=this;transfers_[i].index=i;transfers_[i].memory=&memory_[i];}}
    void retain(){++references;}void release(){if(!references)std::abort();--references;}
    void fault(IOReturn error){if(!usbError_)usbError_=error;radioReady_=false;}
    void quiesce(bool){++quiesces;stopping_=true;abortTransfers();stopping_=false;}
#ifdef RTW_RX_CAPACITY_PREFLIGHT
    void receive(const uint8_t*,size_t);
#else
    void receive(const uint8_t*bytes,size_t){++receivedCalls;received.push_back(bytes[0]);if(usefulReceive)++rxPackets_;}
#endif
    void scheduleDeferredCleanup(){++cleanupCalls;}
    void cancelWpaRecovery(){}
    bool hasFreeTX()const{return true;}
    bool authorized()const{return false;}
    void serviceTXOutputQueue(){}
    void advanceWpaRecovery(){}
    HostOutputQueue* getOutputQueue(){return nullptr;}
    static int stateTransition(ieee80211com*,int,int){return 0;}
    uint64_t lastBeaconMs_=0;
    void publishDiagnosticCounter(const char*name,uint64_t value){diagnostics[name]=value;}
    static uint64_t nowMS(){return clockMS;}
    static RTWUSBController*owner(void*p){return static_cast<RTWUSBController*>(p);}
    static int ioError(void*p){return owner(p)->usbError_?EIO:0;}
    static void write1(void*,uint16_t,uint8_t){}
    static uint8_t read1(void*,uint16_t){return 0;}
    static int busInitialize(void*);
    int allocateTransfers();
    void freeTransfers();
    int submit(unsigned,IOUSBHostPipe*,uint32_t);
    void recordCompletion(Transfer*,IOReturn,uint32_t,uint32_t);
    static void usbCompletion(void*,void*,IOReturn,uint32_t);
    static void completionAction(OSObject*,IOInterruptEventSource*,int);
    void drainCompletions(bool);
    // Actual deferred AU power-off is covered by rtl8814au_recovery_tests.
    void finish8814StopAfterDrain(){}
    void finishFamilyStopAfterDrain(){}
    enum{AUThermal=19};
    void record8814Failure(uint32_t,int){}
    void abortTransfers();
    bool beginRXRecovery(IOReturn);
    void advanceRXRecovery();
    void failRXRecovery(bool exhausted=false);
    void publishRXRecoveryDiagnostics();
    static void watchdogAction(OSObject*,IOTimerEventSource*);
    static void scanAction(OSObject*,IOTimerEventSource*);
    static void calibAction(OSObject*,IOTimerEventSource*);
};
