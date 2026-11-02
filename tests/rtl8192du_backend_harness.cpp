#include "rtw/StationRecovery.hpp"
#include "rtw/StationChannels.h"
/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/RTL8192DUStation.h"
#include "rtw/RTL8192DUUSB.h"
#include "rtw/RTL8192DUDataPath.h"
#include "rtw/RTL8192EU.h"
#include "rtw/RTL8192EUDataPath.h"
#include "rtw/SignalSample.h"
#include "rtw/FCS.h"
#include "rtl8192du_fixture.hpp"
#include <cerrno>
#include <algorithm>
#include <mutex>
#include <thread>
#include <atomic>
using du_test::Fixture;
enum ieee80211_state {IEEE80211_S_INIT,IEEE80211_S_SCAN,IEEE80211_S_AUTH,IEEE80211_S_ASSOC,IEEE80211_S_RUN};
enum {IEEE80211_M_STA=1,IEEE80211_F_SHSLOT=0x20000,IEEE80211_RATE_BASIC=128,IEEE80211_RATE_VAL=127,
    IEEE80211_NODE_HT=1,IEEE80211_NODE_QOS=2,IEEE80211_F_QOS=0x200,IEEE80211_MODE_11A=1,IEEE80211_MODE_11B=2,IEEE80211_MODE_11G=3,IEEE80211_HTCAP_SGI20=32,IEEE80211_HTCAP_SMPS_MASK=12,IEEE80211_HTCAP_SMPS_SHIFT=2};
struct ieee80211_rateset {uint8_t rs_nrates=0,rs_rates[32]{};};
struct ieee80211_channel {int number;unsigned ic_flags=0;};
#define IEEE80211_CHAN_ANYC ((ieee80211_channel *)-1)
struct ieee80211_node {unsigned ni_flags=0,ni_htcaps=0;uint8_t ni_rxmcs[10]{};
    ieee80211_channel *ni_chan=nullptr;uint8_t ni_bssid[6]{};uint16_t ni_associd=0,ni_intval=0;
    ieee80211_rateset ni_rates;bool ni_port_valid=false;unsigned ni_htop1=0;};
struct CoreOps {void*cookie=nullptr;};
struct EDCA {uint8_t ac_aifsn=2,ac_ecwmin=3,ac_ecwmax=4;uint16_t ac_txoplimit=0;};
enum {IFF_RUNNING=1,kIOReturnSuccess=0};
struct ifnet {unsigned if_timer=0,if_flags=IFF_RUNNING;int if_snd=0;};
static void ifq_clr_oactive(int*){}static void task_del(void*,int*){}static void*systq=nullptr;
static const EDCA ieee80211_edca_table[4][4]={
    {},{{3,4,10,0},{7,4,10,0},{2,3,4,94},{2,2,3,47}},
    {{3,5,10,0},{7,5,10,0},{2,4,5,188},{2,3,4,102}},
    {{3,4,10,0},{7,4,10,0},{2,3,4,94},{2,2,3,47}}};
struct ieee80211com {unsigned ic_htcaps=0;int ic_opmode=IEEE80211_M_STA;ieee80211_state ic_state=IEEE80211_S_INIT;
    unsigned ic_flags=0;int ic_rtsthreshold=2347,ic_protmode=0;ieee80211_node *ic_bss=nullptr;void*ic_softc=nullptr;EDCA ic_edca_ac[4];ifnet ic_if;};
struct rtwn_softc {int ac2idx[4]{},sc_tx_timer=0,init_task=0;ieee80211com sc_ic;CoreOps sc_ops;int core_error=0;int(*sc_newstate)(ieee80211com *,ieee80211_state,int)=nullptr;};
static unsigned generic_calls=0;static int generic_error=0;
static int generic(ieee80211com* ic,ieee80211_state state,int) {
    ++generic_calls;if(state==IEEE80211_S_INIT&&ic->ic_bss)ic->ic_bss->ni_port_valid=false;
    if(!generic_error)ic->ic_state=state;return generic_error;
}
static unsigned ieee80211_chan2ieee(ieee80211com*,ieee80211_channel*c){return (unsigned)c->number;}
struct OSObject {virtual ~OSObject()=default;virtual void retain(){};virtual void release(){};};
struct IOService:OSObject {void*client=nullptr;unsigned opens=0,closes=0;void(*onClose)(void*)=nullptr;
    virtual bool open(void*p){if(client&&client!=p)return false;client=p;++opens;return true;}
    bool isOpen(void*p)const{return client==p;}virtual void close(void*p){assert(client==p);client=nullptr;++closes;if(onClose)onClose(p);}};
struct EndpointDescriptor {uint8_t bmAttributes=2;uint16_t wMaxPacketSize=512;};
struct IOUSBHostPipe:OSObject {unsigned refs=1;EndpointDescriptor descriptor;
    const EndpointDescriptor*getEndpointDescriptor(){return &descriptor;}
    void retain()override{++refs;}void release()override{assert(refs);--refs;}};
static uint16_t USBToHost16(uint16_t v){return v;}
struct Descriptor {uint8_t bInterfaceNumber=0,bInterfaceClass=0xff,bInterfaceSubClass=0xff,bInterfaceProtocol=0xff;};
struct Configuration {uint8_t bNumInterfaces=1;};
struct OSIterator:OSObject {std::vector<OSObject*>items;size_t next=0;explicit OSIterator(std::vector<OSObject*> x):items(std::move(x)){}
    OSObject*getNextObject(){return next<items.size()?items[next++]:nullptr;}void release()override{delete this;}};
struct IOUSBHostDevice:IOService {std::vector<OSObject*>children;OSIterator*getChildIterator(const char*){return new OSIterator(children);}};
struct IOUSBHostInterface:IOService {Descriptor descriptor;Configuration configuration;IOUSBHostDevice*device=nullptr;
    IOUSBHostPipe*pipes[256]{};
    IOUSBHostPipe*copyPipe(uint8_t address){auto*p=pipes[address];if(p)p->retain();return p;}
    IOUSBHostDevice*getDevice(){return device;}const Descriptor*getInterfaceDescriptor(){return &descriptor;}
    const Configuration*getConfigurationDescriptor(){return &configuration;}};
#define OSDynamicCast(type,object) dynamic_cast<type*>(object)
static const char*gIOServicePlane="IOService";
static void IOFree(void*,size_t){assert(false&&"the fixture has no kernel allocation");}
static void IOLockLock(std::mutex*p){p->lock();}
static void IOLockUnlock(std::mutex*p){p->unlock();}
enum {IEEE80211_FC0_TYPE_MASK=12,IEEE80211_FC0_TYPE_DATA=8,IEEE80211_FC0_TYPE_MGT=0,IEEE80211_FC0_SUBTYPE_QOS=128,
    IEEE80211_FC1_DIR_MASK=3,IEEE80211_FC1_DIR_DSTODS=3,IEEE80211_FC1_PROTECTED=64,IEEE80211_QOS_TID=15,
    IEEE80211_CIPHER_CCMP=3,EDCA_AC_BE=0,EDCA_AC_BK=1,EDCA_AC_VI=2,EDCA_AC_VO=3,
    IEEE80211_F_SHPREAMBLE=0x80,IEEE80211_F_USEPROT=0x100,IEEE80211_HTOP1_PROT_MASK=3,
    IEEE80211_PROT_CTSONLY=1,IEEE80211_PROT_RTSCTS=2};
struct ieee80211_frame {uint8_t i_fc[2],i_dur[2],i_addr1[6],i_addr2[6],i_addr3[6],i_seq[2];};
struct Packet {std::vector<uint8_t>bytes;};using mbuf_t=Packet*;
static unsigned packet_frees=0,node_releases=0,encrypt_calls=0;
static size_t mbuf_pkthdr_len(mbuf_t p){return p->bytes.size();}
static int mbuf_copydata(mbuf_t p,size_t offset,size_t n,void*out){if(offset>p->bytes.size()||n>p->bytes.size()-offset)return EINVAL;memcpy(out,p->bytes.data()+offset,n);return 0;}
static void mbuf_freem(mbuf_t p){++packet_frees;delete p;}
struct ieee80211_key {int k_cipher=IEEE80211_CIPHER_CCMP;};static ieee80211_key key;
static ieee80211_key*ieee80211_get_txkey(ieee80211com*,const ieee80211_frame*,ieee80211_node*){return &key;}
static mbuf_t ieee80211_encrypt(ieee80211com*,mbuf_t p,ieee80211_key*){++encrypt_calls;p->bytes.resize(p->bytes.size()+16);return p;}
static void ieee80211_release_node(ieee80211com*,ieee80211_node*){++node_releases;}
static unsigned ieee80211_up_to_ac(ieee80211com*,int tid){return tid==1||tid==2?1:tid>=6?3:tid>=4?2:0;}
using IOInterruptState=unsigned;static IOInterruptState IOSimpleLockLockDisableInterrupt(void*){return 0;}
static void IOSimpleLockUnlockEnableInterrupt(void*,IOInterruptState){}
struct Memory {std::vector<uint8_t>bytes;Memory():bytes(4352){}void*getBytesNoCopy(){return bytes.data();}};
struct Transfer {Memory*memory=nullptr;};
class RTWUSBController {public:

 bool permitsDFSChannelByCountry(uint8_t)const{return true;}

    Fixture test;
    rtwn_softc core_{};rtw_92du_boot&duCore_=test.boot;rtw_92du_rom&duROM_=test.identity;rtw_92du_mac duMAC_{};
    std::mutex lifecycle_storage_;std::mutex*lifecycleLock_=&lifecycle_storage_;
    rtw_92du_rom duPortROM_[2]{};rtw_backend_usb_plan duPlan_=du_test::plan();
    struct DUDataPort {rtw_usb_endpoints endpoints{};IOUSBHostPipe*rx=nullptr,*tx[4]{};uint16_t mps[4]{};}duPorts_[2]{};
    uint8_t duPortCount_=0,duActivePort_=0,duStationAddress_[6]{};
    struct Recovery {unsigned retries=0,remainingMS=0;bool active=false;}wpaRecovery_{};
    bool radioReady_=true,abort_drains=true,sleeping_=false;int boot_error=0,usbError_=kIOReturnSuccess;
    unsigned boot_calls=0,bus_starts=0,aborts=0,free_calls=0;
    std::vector<uint8_t>dual_configuration;
    rtw_wifi_family family_=RTW_WIFI_RTL8192DU;
    bool duWholeDeviceOwned_=true,running_=true,stopping_=false,duRSSIValid_=false;uint8_t duRSSIPWDB_=0;
    IOUSBHostInterface*usb_=nullptr,*duSibling_=nullptr;IOService*duDevice_=nullptr;
    bool usbOpened_=false;void*duPHYStorage_=nullptr;size_t duPHYSize_=0;
    uint8_t duPowerReductionDB_=0;int set8192DUPowerReduction(uint8_t);int32_t duPowerLimitMBM_=INT32_MAX;int set8192DUPowerLimitMBM(int32_t);
    enum {RXSlots=4,TXCapacity=4352};bool present_=true;int lock_storage_=0;void*completionLock_=&lock_storage_;
    rtw_transfer_ledger ledger_{};rtw_usb_endpoints endpoints_{};uint16_t txMPS_[4]{512,512,512,512};
    IOUSBHostPipe*rxPipe_=nullptr,*txPipes_[4]{};std::array<Memory,RTW_TRANSFER_SLOTS>memories;Transfer transfers_[RTW_TRANSFER_SLOTS]{};
    uint64_t txHTPackets_=0;int submit_error=0;std::vector<uint8_t>submitted;
    static int ioError(void*p){return owner(p)->test.bus.fail_at?EIO:0;}
    int submit(unsigned index,void*,uint32_t n){submitted.assign(memories[index].bytes.begin(),memories[index].bytes.begin()+n);return submit_error;}
    int send8192DUFrame(mbuf_t,ieee80211_node*);
    RTWUSBController(){endpoints_.num_bulk_out=1;
        for(unsigned i=0;i<RTW_TRANSFER_SLOTS;++i)transfers_[i].memory=&memories[i];
        core_.sc_ops.cookie=this;core_.sc_ic.ic_softc=&core_;core_.sc_newstate=generic;
        auto p=du_test::plan();test.boot.firmware_ready=0;
        assert(rtw_92du_mac_init(&duMAC_,&duCore_,&p)==RTW_92DU_OK);
        assert(rtw_92du_mac_prepare(&duMAC_)==RTW_92DU_OK);test.boot.firmware_ready=1;test.boot.firmware_version=38;
        assert(rtw_92du_mac_start(&duMAC_)==RTW_92DU_OK);
        assert(rtw_92du_phy_load_tables(&duCore_)==RTW_92DU_OK);
        assert(rtw_92du_phy_set_channel(&duCore_,1)==RTW_92DU_OK && rtw_92du_phy_calibrate(&duCore_)==RTW_92DU_OK);
        test.bus.regs[0x80]=2;test.bus.regs[0x18]=test.bus.regs[0x1a]=1;
    }
    static RTWUSBController*owner(void*p){return static_cast<RTWUSBController*>(p);}
    bool claim8192DUDevice(),is8192DUUSBProvider(IOService*)const;void close8192DUProviders(),release8192DUStorage();
    rtw_92du_bss station8192DUParams()const;int set8192DUChannel(uint8_t),state8192DU(ieee80211com*,ieee80211_state,int),tick8192DU();
    void stop8192DU();void freeTransfers(){assert(rtw_transfer_drained(&ledger_));++free_calls;}
    bool permits8192DUChannel(uint8_t)const,supports8192DUBand(bool)const,uses8192DUFirmwareRate()const;
    static void update8192DUEDCA(ieee80211com*);
    static void cancelScan(void*){}static void cancelCalibration(void*){}static void nextScan(void*){}static void nextCalibration(void*){}
    bool authorized()const{return uses8192DUFirmwareRate()&&core_.sc_ic.ic_bss->ni_port_valid;}
    rtw_packet_error decode8192DUUSBFrame(rtw_rx_iterator*,rtw_8192eu_rx_frame*,uint8_t*)const;
    int sample8192DURSSI(const rtw_rx_packet&);
    int prepare8192DUPorts(const uint8_t*,size_t),select8192DUPort(uint8_t),switch8192DUBand(uint8_t);
    void setProperty(const char*,unsigned long long,unsigned){}
    void abortTransfers(){++aborts;running_=radioReady_=false;wpaRecovery_.active=false;rtw_transfer_stop(&ledger_);
        if(abort_drains)for(unsigned i=0;i<RTW_TRANSFER_SLOTS;i++)if(ledger_.pending&(1U<<i)){
            if(!(ledger_.completed&(1U<<i)))assert(rtw_transfer_complete(&ledger_,i));assert(rtw_transfer_take(&ledger_,i));}}
    static int busInitialize(void*p){auto*c=owner(p);++c->bus_starts;
        assert(rtw_transfer_restart(&c->ledger_));c->running_=true;return 0;}
    // Boot/firmware are independently exercised with the complete embedded
    // payload by boot_tests. This orchestration boundary starts after those
    // stages, then executes the actual per-MAC MAC/LLT/PHY/calibration code.
    int initialize8192DUCore(){const rtw_usb_rw_transport t{&test.bus,du_test::Bus::in,du_test::Bus::out};
        const rtw_waiter w{&test.bus,du_test::Bus::wait};const rtw_92du_boot_options o{1,1,10};
        auto e=rtw_92du_boot_init(&duCore_,&duROM_.board,&t,&w,&o);if(e!=RTW_92DU_OK)return EIO;
        duCore_.power_ready=1;return rtw_92du_mac_init_for_interface(&duMAC_,&duCore_,&duPlan_,dual_configuration.data(),dual_configuration.size())==RTW_92DU_OK?0:EIO;}
    int start8192DUHardware(){++boot_calls;if(boot_error)return boot_error;
        assert(rtw_92du_mac_prepare(&duMAC_)==RTW_92DU_OK);duCore_.firmware_ready=1;duCore_.firmware_version=38;
        assert(rtw_92du_phy_init(&duCore_,&duROM_,test.storage.data(),test.storage.size()*8,RTW_92DU_USB_HIGH)==RTW_92DU_OK);
        assert(rtw_92du_mac_start(&duMAC_)==RTW_92DU_OK);
        assert(rtw_92du_mac_set_address(&duMAC_,duStationAddress_)==RTW_92DU_OK);
        test.bus.regs[0x80]=2;test.bus.regs[0x18]=test.bus.regs[0x1a]=1;
        assert(rtw_92du_phy_load_tables(&duCore_)==RTW_92DU_OK);assert(rtw_92du_phy_set_power_reduction(&duCore_,duPowerReductionDB_)==RTW_92DU_OK);assert(rtw_92du_phy_set_power_limit_mbm(&duCore_,duPowerLimitMBM_)==RTW_92DU_OK);return 0;}

#include "family_recovery_backend_host.hpp"
};
/* ACTUAL_BACKEND_METHODS */
static void put(uint8_t*p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(v>>(8*i));}
static mbuf_t tx(bool eapol,bool qos,size_t n=64,bool protected_frame=false){
    auto*p=new Packet;p->bytes.resize(n);p->bytes[0]=uint8_t(8|(qos?128:0));p->bytes[1]=uint8_t(protected_frame?64:0);
    p->bytes[4]=2;p->bytes[22]=0x30;p->bytes[23]=0x12;
    if(eapol){const uint8_t snap[]={0xaa,0xaa,3,0,0,0,0x88,0x8e};memcpy(p->bytes.data()+24+(qos?2:0),snap,8);}return p;
}
static std::vector<uint8_t> frame(){std::vector<uint8_t>b(24+32+32+4);put(b.data(),36U|(4U<<16U)|(1U<<26U)|(1U<<27U));
    put(b.data()+12,4);b[28]=0x80;b[56]=0x80;put(b.data()+b.size()-4,rtw_fcs32(b.data()+56,32));return b;}
struct DualFixture {
    RTWUSBController c;IOUSBHostDevice device;IOUSBHostInterface first,second;IOUSBHostPipe pipes[5];
    explicit DualFixture(bool bad_mps=false) {
        c.duROM_=du_test::rom(true,0);c.running_=false;c.duWholeDeviceOwned_=false;
        first.device=second.device=&device;first.configuration.bNumInterfaces=2;
        second.descriptor.bInterfaceNumber=1;first.client=&c;device.children={&first,&second};
        first.pipes[0x81]=&pipes[0];first.pipes[2]=&pipes[1];
        second.pipes[0x84]=&pipes[2];second.pipes[5]=&pipes[3];second.pipes[6]=&pipes[4];
        c.usb_=&first;c.usbOpened_=true;assert(c.claim8192DUDevice());
        c.dual_configuration={9,2,62,0,2,c.duPlan_.profile->configuration,0,0x80,50,
            9,4,0,0,2,0xff,0xff,0xff,0,7,5,0x81,2,0,2,0,7,5,2,2,0,2,0,
            9,4,1,0,3,0xff,0xff,0xff,0,7,5,0x84,2,0,2,0,7,5,5,2,0,2,0,7,5,6,2,0,2,0};
        if(bad_mps)pipes[4].descriptor.wMaxPacketSize=65;
        const auto before=c.test.bus.io;
        const int result=c.prepare8192DUPorts(c.dual_configuration.data(),c.dual_configuration.size());
        assert(c.test.bus.io==before); // descriptor/pipe preparation issues no register operations
        if(bad_mps){assert(result==EINVAL&&!c.duPortCount_);return;}
        assert(!result&&c.duActivePort_==1&&c.duPortCount_==2);
        assert(c.rxPipe_==&pipes[2]&&c.txPipes_[0]==&pipes[3]&&c.txPipes_[1]==&pipes[4]);
        assert(c.core_.ac2idx[EDCA_AC_BE]==1&&c.endpoints_.num_bulk_out==2);
        assert(!c.initialize8192DUCore()&&!c.start8192DUHardware());
        assert(rtw_92du_phy_set_channel(&c.duCore_,1)==RTW_92DU_OK&&rtw_92du_phy_calibrate(&c.duCore_)==RTW_92DU_OK);
        c.running_=c.radioReady_=true;
    }
    ~DualFixture(){
        c.abortTransfers();assert(rtw_transfer_drained(&c.ledger_));
        if(c.rxPipe_){c.rxPipe_->release();c.rxPipe_=nullptr;}
        for(auto*&p:c.txPipes_)if(p){p->release();p=nullptr;}
        c.release8192DUStorage();
        for(auto&p:pipes)assert(p.refs==1);
        if(first.isOpen(&c))first.close(&c);
        assert(!second.client&&!device.client);
    }
};
int main(){
    // A poisoned disconnect mailbox must still flush the old host RUN/keys.
    {
        RTWUSBController c;ieee80211_channel channel{1};ieee80211_node node{};
        node.ni_chan=&channel;node.ni_bssid[0]=2;node.ni_associd=1;node.ni_intval=100;
        node.ni_flags=IEEE80211_NODE_HT|IEEE80211_NODE_QOS;node.ni_htcaps=3U<<2;
        node.ni_rxmcs[0]=node.ni_rxmcs[1]=255;
        node.ni_rates={3,{0x82,12,108}};auto&ic=c.core_.sc_ic;ic.ic_bss=&node;
        assert(!c.state8192DU(&ic,IEEE80211_S_AUTH,0));
        assert(!c.state8192DU(&ic,IEEE80211_S_ASSOC,0));
        assert(!c.state8192DU(&ic,IEEE80211_S_RUN,0));node.ni_port_valid=true;
        assert(du_test::control_admitted(c.test.bus,8));
        const auto before=generic_calls;c.test.bus.h2c_busy=15;
        assert(c.state8192DU(&ic,IEEE80211_S_SCAN,0)==ETIMEDOUT);
        assert(ic.ic_state==IEEE80211_S_INIT&&!node.ni_port_valid&&generic_calls==before+1);
        assert(c.core_.core_error==ETIMEDOUT&&!c.radioReady_&&!c.duMAC_.joined&&!c.duMAC_.rate_ready);
        assert(!rtw_92du_phy_tx_active(&c.duCore_)&&!c.authorized());
        assert(!du_test::control_admitted(c.test.bus,8)&&!du_test::control_admitted(c.test.bus,10));
        c.test.bus.regs[0x81]=0x80;c.stop8192DU();
        assert(!(c.test.bus.regs[0x81]&0x80)&&c.test.bus.regs[0x1c]==0x0e);
        assert(!c.duCore_.power_ready&&!c.duCore_.firmware_ready&&!c.duMAC_.started);
        assert(c.core_.core_error==ETIMEDOUT&&!c.running_&&c.free_calls==1&&c.duWholeDeviceOwned_);
    }
    for(const auto health:std::array<std::array<uint8_t,2>,5>{{{{0,0}},{{3,1}},{{2,0}},{{0xff,0xff}},{{2,1}}}}){
        RTWUSBController c;ieee80211_channel channel{1};ieee80211_node node{};
        node.ni_chan=&channel;node.ni_bssid[0]=2;node.ni_associd=1;node.ni_intval=100;
        node.ni_rates={3,{0x82,12,108}};auto&ic=c.core_.sc_ic;ic.ic_bss=&node;
        assert(!c.state8192DU(&ic,IEEE80211_S_AUTH,0));
        assert(!c.state8192DU(&ic,IEEE80211_S_ASSOC,0));
        assert(!c.state8192DU(&ic,IEEE80211_S_RUN,0));node.ni_port_valid=true;
        assert(c.authorized());
        c.test.bus.regs[0x80]=health[0];c.test.bus.regs[0x18]=health[1];
        if(health[0]==2&&health[1]==1)c.test.bus.fail_at=c.test.bus.io+1;
        const auto writes=c.test.bus.writes.size();
        assert(c.tick8192DU()==EIO);
        assert(!c.duCore_.firmware_ready&&!c.duMAC_.joined&&!c.duMAC_.rate_ready&&!c.radioReady_);
        assert(ic.ic_state==IEEE80211_S_INIT&&!node.ni_port_valid&&c.core_.core_error==EIO&&!c.authorized());
        assert(!rtw_92du_phy_tx_active(&c.duCore_));
        for(size_t i=writes;i<c.test.bus.writes.size();++i)
            assert(c.test.bus.writes[i].address!=0x1d0 && !(c.test.bus.writes[i].address==0x522 && !c.test.bus.writes[i].value));
    }
    {
        DualFixture owned;auto&c=owned.c;c.test.bus.regs[0x53]=1;
        c.duCore_.last_error=RTW_92DU_POLL_TIMEOUT;c.stop8192DU();
        assert(!(c.test.bus.regs[0x53]&1)&&c.test.bus.regs[0x1c]==0x0e);
        assert(c.duWholeDeviceOwned_&&owned.device.client==&c&&owned.first.client==&c&&owned.second.client==&c);
        assert(!c.running_&&rtw_transfer_drained(&c.ledger_)&&c.free_calls==1&&c.core_.core_error==ETIMEDOUT);
    }
    for(bool duplicate:{false,true}){
        RTWUSBController c;IOUSBHostDevice device;IOUSBHostInterface first,second;
        first.device=second.device=&device;first.client=&c;first.configuration.bNumInterfaces=2;
        second.descriptor.bInterfaceNumber=duplicate?0:1;device.children={&first,&second};
        c.duWholeDeviceOwned_=false;c.usb_=&first;c.usbOpened_=true;
        assert(c.claim8192DUDevice()==!duplicate);
        if(!duplicate){assert(second.client==&c&&c.duWholeDeviceOwned_&&c.is8192DUUSBProvider(&second));c.release8192DUStorage();}
        assert(!device.client&&!second.client&&first.client==&c);
    }
    {
        RTWUSBController c;ieee80211_channel channel{6};ieee80211_node node{};node.ni_chan=&channel;
        node.ni_rates.rs_nrates=1;node.ni_rates.rs_rates[0]=12;c.core_.sc_ic.ic_bss=&node;
        for(auto& ac:c.core_.sc_ic.ic_edca_ac)ac=EDCA{0,0,0,0};
        c.core_.sc_ic.ic_flags=IEEE80211_F_QOS;
        assert(ieee80211_edca_table[IEEE80211_MODE_11G][0].ac_aifsn==3);
        RTWUSBController::update8192DUEDCA(&c.core_.sc_ic);
        assert(du_test::Bus::word(c.test.bus.regs.data()+0x508,4)==0x0000a446U && !c.core_.core_error);
    }
    { // An AUTH switch from DMDP MAC1 to MAC0 must retain the AP backoff.
        DualFixture f;auto&c=f.c;ieee80211_channel channel{52};ieee80211_node node{};
        node.ni_chan=&channel;node.ni_bssid[0]=2;node.ni_bssid[5]=4;node.ni_associd=42;node.ni_intval=100;node.ni_rates={3,{0x8c,24,108}};
        node.ni_flags=IEEE80211_NODE_HT;node.ni_htcaps=(3U<<2);node.ni_rxmcs[0]=255;auto&ic=c.core_.sc_ic;ic.ic_bss=&node;
        assert(c.duActivePort_==1&&!c.set8192DUPowerReduction(2)&&!c.set8192DUPowerLimitMBM(949));
        assert(!c.state8192DU(&ic,IEEE80211_S_AUTH,0)&&c.duActivePort_==0&&rtw_92du_phy_tx_active(&c.duCore_));
        assert(du_test::Bus::word(c.test.bus.regs.data()+0xe00,4)==0x0e0e0e0eU);
        assert(!c.state8192DU(&ic,IEEE80211_S_ASSOC,0)&&!c.state8192DU(&ic,IEEE80211_S_RUN,0));
        assert(!c.set8192DUPowerReduction(0)&&!c.set8192DUPowerLimitMBM(INT32_MAX)&&!c.set8192DUChannel(52));
        assert(du_test::Bus::word(c.test.bus.regs.data()+0xe00,4)==0x18181818U);
    }
    for(uint8_t channel:{uint8_t(1),uint8_t(36),uint8_t(52),uint8_t(56),uint8_t(60),uint8_t(64),uint8_t(100),uint8_t(104),uint8_t(108),uint8_t(112),uint8_t(116),uint8_t(120),uint8_t(124),uint8_t(128),uint8_t(132),uint8_t(136),uint8_t(140),uint8_t(165)}){
        RTWUSBController c;ieee80211_channel ch{channel};ieee80211_node node{};node.ni_chan=&ch;
        node.ni_bssid[0]=2;node.ni_bssid[5]=4;node.ni_associd=0xc02a;node.ni_intval=100;
        node.ni_rates.rs_nrates=3;node.ni_rates.rs_rates[0]=uint8_t(0x80|(channel>14?12:2));
        node.ni_rates.rs_rates[1]=24;node.ni_rates.rs_rates[2]=108;
        node.ni_flags=IEEE80211_NODE_HT|IEEE80211_NODE_QOS;node.ni_htcaps=IEEE80211_HTCAP_SGI20|(3U<<2U);node.ni_rxmcs[0]=node.ni_rxmcs[1]=255;
        auto&ic=c.core_.sc_ic;ic.ic_bss=&node;ic.ic_flags=IEEE80211_F_QOS;ic.ic_htcaps=IEEE80211_HTCAP_SGI20;
        assert(c.permits8192DUChannel(channel)&&!c.permits8192DUChannel(144));
        assert(!c.state8192DU(&ic,IEEE80211_S_SCAN,0));
        assert(rtw_92du_phy_receive_ready(&c.duCore_));
        assert(bool(rtw_92du_phy_tx_active(&c.duCore_))==(channel==1));
        if(channel>14)assert(c.send8192DUFrame(tx(true,false),&node)==ENETDOWN);
        assert(!c.state8192DU(&ic,IEEE80211_S_AUTH,0)&&rtw_92du_phy_tx_active(&c.duCore_));
        assert(!c.state8192DU(&ic,IEEE80211_S_ASSOC,0));
        assert(!c.state8192DU(&ic,IEEE80211_S_RUN,0)&&c.uses8192DUFirmwareRate()&&!c.authorized());
        assert(du_test::control_admitted(c.test.bus,8)&&!du_test::control_admitted(c.test.bus,10));
        assert(!(du_test::Bus::word(c.test.bus.regs.data()+0x608,4)&((1U<<12)|1U)));
        assert((du_test::Bus::word(c.test.bus.regs.data()+0x608,4)&0xc0)==0xc0&&!memcmp(c.duMAC_.bssid,node.ni_bssid,6));
        std::array<uint8_t,48> bar{};put(bar.data(),24);put(bar.data()+12,4);bar[24]=0x84;
        memcpy(bar.data()+28,c.duROM_.board.mac,6);memcpy(bar.data()+34,node.ni_bssid,6);
        put(bar.data()+44,rtw_fcs32(bar.data()+24,20));rtw_rx_iterator bar_it{};rtw_8192eu_rx_frame bar_frame{};uint8_t report=0;
        rtw_rx_iterator_init(&bar_it,bar.data(),bar.size());
        assert(c.decode8192DUUSBFrame(&bar_it,&bar_frame,&report)==RTW_PACKETS_OK&&bar_frame.payload.mpdu_length==20&&!bar_frame.payload.crc_error&&!report);
        const auto acceptsBAR=[&](){return rtw_92e_rx_frame_allowed(bar_frame.payload.mpdu,bar_frame.payload.mpdu_length,
            c.authorized()&&c.uses8192DUFirmwareRate()&&c.duMAC_.rates.ht,c.duROM_.board.mac,node.ni_bssid);};
        assert(!acceptsBAR()); // hardware admission never bypasses the controlled port
        assert(c.duMAC_.rates.ht&&c.duMAC_.rates.initial_rate==27&&c.duMAC_.rates.sgi20);
        const auto free_before=packet_frees,release_before=node_releases;
        assert(c.send8192DUFrame(tx(false,true),&node)==EACCES && packet_frees==free_before+1 && node_releases==release_before);
        assert(!c.send8192DUFrame(tx(true,false),&node));
        assert((du_test::Bus::word(c.submitted.data()+16,4)&0x500)==0x500);
        node.ni_port_valid=true;assert(c.authorized());
        assert(acceptsBAR());bar[28]^=1;assert(!acceptsBAR());bar[28]^=1;
        bar[34]^=1;assert(!acceptsBAR());bar[34]^=1;
        assert(!c.send8192DUFrame(tx(false,true,480),&node));
        assert(c.submitted.size()==520 && ((du_test::Bus::word(c.submitted.data()+4,4)>>26)&31)==1);
        assert(c.submitted[40]==0x88 && ((du_test::Bus::word(c.submitted.data()+12,4)>>16)&4095)==0x123);
        assert(!(du_test::Bus::word(c.submitted.data()+16,4)&0x500) && c.txHTPackets_==1);
        const auto encrypted=encrypt_calls;
        assert(!c.send8192DUFrame(tx(true,false,64,true),&node)&&encrypt_calls==encrypted+1);
        c.submit_error=EIO;const auto refs=node_releases;
        assert(c.send8192DUFrame(tx(false,true),&node)==EIO && node_releases==refs);c.submit_error=0;
        assert(!c.tick8192DU());
        assert(!c.state8192DU(&ic,IEEE80211_S_INIT,0)&&!node.ni_port_valid&&!c.uses8192DUFirmwareRate());
        assert(!du_test::control_admitted(c.test.bus,8)&&!du_test::control_admitted(c.test.bus,10));
        assert(!rtw_92du_phy_tx_active(&c.duCore_));
        // The generic key/port flush must still run with a poisoned USB HAL.
        node.ni_port_valid=true;c.duCore_.last_error=RTW_92DU_IO_ERROR;const auto before=generic_calls;
        assert(c.state8192DU(&ic,IEEE80211_S_INIT,0)==EIO&&generic_calls==before+1&&!node.ni_port_valid);
        assert(c.core_.core_error==EIO&&ic.ic_state==IEEE80211_S_INIT);
    }
    {
        RTWUSBController c;auto raw=frame();rtw_rx_iterator it{};rtw_8192eu_rx_frame out{};uint8_t type=9;
        rtw_rx_iterator_init(&it,raw.data(),raw.size());
        assert(c.decode8192DUUSBFrame(&it,&out,&type)==RTW_PACKETS_OK&&out.payload.mpdu_length==32&&!out.payload.crc_error&&!type);
        assert(c.decode8192DUUSBFrame(&it,&out,&type)==RTW_PACKETS_DONE);
        raw.back()^=1;rtw_rx_iterator_init(&it,raw.data(),raw.size());
        assert(c.decode8192DUUSBFrame(&it,&out,&type)==RTW_PACKETS_OK&&out.payload.crc_error);
        raw=frame();put(raw.data()+12,4U|512U);rtw_rx_iterator_init(&it,raw.data(),raw.size());
        assert(c.decode8192DUUSBFrame(&it,&out,&type)==RTW_PACKETS_OK&&!out.payload.bandwidth_code);
        put(raw.data()+12,12U|64U|512U);rtw_rx_iterator_init(&it,raw.data(),raw.size());
        assert(c.decode8192DUUSBFrame(&it,&out,&type)==RTW_PACKETS_MALFORMED);
    }
    {
        DualFixture f;auto&c=f.c;ieee80211_channel channel{1};ieee80211_node node{};node.ni_chan=&channel;
        node.ni_bssid[0]=2;node.ni_bssid[5]=4;node.ni_associd=0xc02a;node.ni_intval=100;
        node.ni_flags=IEEE80211_NODE_HT|IEEE80211_NODE_QOS;node.ni_htcaps=IEEE80211_HTCAP_SGI20|(3U<<2U);
        node.ni_rxmcs[0]=node.ni_rxmcs[1]=255;node.ni_rates.rs_nrates=3;
        node.ni_rates.rs_rates[0]=0x8c;node.ni_rates.rs_rates[1]=24;node.ni_rates.rs_rates[2]=108;
        auto&ic=c.core_.sc_ic;ic.ic_bss=&node;ic.ic_flags=IEEE80211_F_QOS;ic.ic_htcaps=IEEE80211_HTCAP_SGI20;
        const auto address=c.duPortROM_[0].board;unsigned switches=0;
        for(uint8_t ch:{uint8_t(1),uint8_t(36),uint8_t(52),uint8_t(64),uint8_t(100),uint8_t(140),uint8_t(165),uint8_t(6)}){
            const unsigned target=ch>14?0:1;const bool change=target!=c.duActivePort_;channel.number=ch;
            const auto boots=c.boot_calls;
            assert(rtw_transfer_begin(&c.ledger_,0));
            c.wpaRecovery_={2,15000,true};
            assert(!c.state8192DU(&ic,IEEE80211_S_SCAN,0));
            assert(c.boot_calls==boots+unsigned(change));if(change)++switches;
            if(!change){assert(rtw_transfer_complete(&c.ledger_,0));assert(rtw_transfer_take(&c.ledger_,0));}
            assert(c.duActivePort_==target&&c.duROM_.board.mac_index==target&&c.duCore_.board.mac_index==target);
            assert(c.usb_==&f.first&&c.duWholeDeviceOwned_&&f.second.isOpen(&c));
            assert(c.rxPipe_==&f.pipes[target?2:0]&&c.endpoints_.num_bulk_out==(target?2:1));
            assert(c.wpaRecovery_.active&&c.wpaRecovery_.retries==2&&c.wpaRecovery_.remainingMS==15000);
            assert(!memcmp(c.duMAC_.station_address,address.mac,6)&&!memcmp(c.test.bus.regs.data()+0x610,address.mac,6));
            assert(bool(rtw_92du_phy_tx_active(&c.duCore_))==(ch<14));
            assert(!c.state8192DU(&ic,IEEE80211_S_AUTH,0));assert(!c.state8192DU(&ic,IEEE80211_S_ASSOC,0));
            assert(!c.state8192DU(&ic,IEEE80211_S_RUN,0)&&c.duMAC_.rates.initial_rate==19&&!c.authorized());
            assert(!c.send8192DUFrame(tx(true,false),&node));
            assert(c.send8192DUFrame(tx(false,true),&node)==EACCES);
            assert(!c.state8192DU(&ic,IEEE80211_S_INIT,0));
        }
        assert(switches==2&&c.bus_starts==2);
    }
    {
        DualFixture f;auto&c=f.c;const auto boots=c.boot_calls;auto*receive=c.rxPipe_;
        assert(rtw_transfer_begin(&c.ledger_,0));c.abort_drains=false;
        assert(c.switch8192DUBand(36)==EBUSY&&c.duActivePort_==1&&c.boot_calls==boots&&c.rxPipe_==receive);
        assert(c.duCore_.power_ready&&!c.running_&&!rtw_92du_phy_tx_active(&c.duCore_));
        assert(rtw_transfer_complete(&c.ledger_,0)&&rtw_transfer_take(&c.ledger_,0));c.abort_drains=true;
    }
    {
        DualFixture f;auto&c=f.c;c.boot_error=EIO;
        assert(c.switch8192DUBand(36)==EIO&&!c.running_&&!c.radioReady_&&!c.bus_starts&&!c.duMAC_.rate_ready&&!c.duMAC_.joined);
    }
    {
        DualFixture f;auto&c=f.c;c.test.bus.regs[0x81]=0x80;const auto writes=c.test.bus.rf_writes.size();
        assert(c.switch8192DUBand(36)==EBUSY&&c.duActivePort_==1&&!c.bus_starts);
        assert(c.test.bus.rf_writes.size()==writes);
    }
    {DualFixture invalid_mps(true);}
    for(uint8_t ch:{uint8_t(1),uint8_t(36)}){
        DualFixture f;auto&c=f.c;ieee80211_channel channel{ch};ieee80211_node node{};
        node.ni_chan=&channel;node.ni_bssid[0]=2;node.ni_bssid[5]=4;node.ni_associd=0xc02a;node.ni_intval=100;
        node.ni_flags=IEEE80211_NODE_HT|IEEE80211_NODE_QOS;node.ni_htcaps=3U<<2U; // peer has no SGI20
        node.ni_rxmcs[0]=node.ni_rxmcs[1]=255;node.ni_rates.rs_nrates=3;
        node.ni_rates.rs_rates[0]=0x8c;node.ni_rates.rs_rates[1]=24;node.ni_rates.rs_rates[2]=108;
        auto&ic=c.core_.sc_ic;ic.ic_bss=&node;ic.ic_flags=IEEE80211_F_QOS;ic.ic_htcaps=IEEE80211_HTCAP_SGI20;
        assert(!c.state8192DU(&ic,IEEE80211_S_SCAN,0)&&!c.state8192DU(&ic,IEEE80211_S_AUTH,0));
        assert(!c.state8192DU(&ic,IEEE80211_S_ASSOC,0)&&!c.state8192DU(&ic,IEEE80211_S_RUN,0));
        // Primary DU trx.c forces SGI for DMDP MCS7, so an LGI-only peer
        // receives the valid MCS0–6 firmware mask and a usable initial MCS6.
        assert(c.duMAC_.rates.ht&&!c.duMAC_.rates.sgi20&&c.duMAC_.rates.initial_rate==18);
        assert((c.duMAC_.rates.mask&0x0ffff000U)==0x0007f000U);
        node.ni_port_valid=true;const auto before=encrypt_calls;
        assert(!c.send8192DUFrame(tx(false,true,64,true),&node)&&encrypt_calls==before+1);
        assert((du_test::Bus::word(c.submitted.data()+20,4)&0x7f)==18);
        assert(!(du_test::Bus::word(c.submitted.data()+16,4)&0x500));
    }
    {
        DualFixture f;auto raw=frame();rtw_rx_iterator it{};rtw_8192eu_rx_frame out{};uint8_t type=9;
        put(raw.data()+12,20U|64U);rtw_rx_iterator_init(&it,raw.data(),raw.size());
        assert(f.c.decode8192DUUSBFrame(&it,&out,&type)==RTW_PACKETS_MALFORMED);
        put(raw.data()+12,19U|64U);rtw_rx_iterator_init(&it,raw.data(),raw.size());
        assert(f.c.decode8192DUUSBFrame(&it,&out,&type)==RTW_PACKETS_OK);
    }
    {
        DualFixture f;auto&c=f.c;
        assert(rtw_transfer_begin(&c.ledger_,3));c.duPHYStorage_=c.test.storage.data();c.duPHYSize_=c.test.storage.size()*8;
        auto*phy=c.duCore_.phy;
        f.second.onClose=[](void*p){static_cast<RTWUSBController*>(p)->close8192DUProviders();};
        f.device.onClose=f.second.onClose;
        std::atomic<bool>go{false};std::array<std::thread,16>threads;
        for(auto&t:threads)t=std::thread([&]{while(!go.load(std::memory_order_acquire))std::this_thread::yield();
            for(unsigned retry=0;retry<16;retry++){(void)c.is8192DUUSBProvider(&f.device);c.close8192DUProviders();}});
        go.store(true,std::memory_order_release);for(auto&t:threads)t.join();
        assert(!c.duWholeDeviceOwned_&&!c.duSibling_&&!c.duDevice_&&f.second.closes==1&&f.device.closes==1);
        assert(c.duPHYStorage_==c.test.storage.data()&&c.duCore_.phy==phy&&!rtw_transfer_drained(&c.ledger_));
        assert(f.pipes[2].refs==3);c.close8192DUProviders();assert(f.second.closes==1&&f.device.closes==1);
        assert(rtw_transfer_complete(&c.ledger_,3)&&rtw_transfer_take(&c.ledger_,3));c.duPHYStorage_=nullptr;c.duPHYSize_=0;
    }
    puts("RTL8192DU DMDP: actual pipe selection, fresh per-MAC HAL,2G/5G scan+RUN,one-stream MCS,stable MAC,drain/failure/reentrant-close passed");
    puts("RTL8192DU actual kernel methods + real C HAL: ownership,2G/low5G/high5G,HT20,RSN port,TX pause,RX FCS passed");
}
