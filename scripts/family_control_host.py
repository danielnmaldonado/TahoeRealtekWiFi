"""Add actual controller control/enable methods to dedicated native HAL fixtures.

The existing fixtures retain their real native startup/stop. Only initial USB
submission is doubled here; common RX transport has its independent full suite.
"""
def function(source, marker):
    a=source.index(marker);b=source.index('{',a);end=b+1;depth=1
    while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[a:end]

def augment_control(text, root, suffix):
    source=(root/'kext/RTWUSBController.cpp').read_text()
    prelude='''
#include <cerrno>
#include <cassert>
using IOReturn=int;
enum {kIOReturnBadArgument=101,kIOReturnNoMemory=102,kIOReturnBusy=103,kIOReturnNoDevice=104,kIOReturnNotReady=105,kIOReturnTimeout=106,kIOReturnIOError=107,kIOReturnUnsupported=108};
#ifndef IEEE80211_F_PSK
#define IEEE80211_F_PSK 0x40000000U
#endif
enum {IEEE80211_WPA_PROTO_WPA2=1,IEEE80211_WPA_AKM_PSK=1,IEEE80211_WPA_CIPHER_CCMP=8};
constexpr unsigned RTWN_FLAG_BUSY=1;
struct ieee80211_wpaparams {int i_enabled,i_protos,i_akms,i_ciphers,i_groupcipher;};
template<class T>int ieee80211_ioctl_setwpaparms(T*,const ieee80211_wpaparams*){return 0;}
template<class T>void ieee80211_begin_scan(T*){}
template<class T>int rtwn_init(T*){return ENOTSUP;}
template<class T>int rtwn_io_error(T*c){return c->core_error;}
'''
    # Definitions precede existing OS host shapes; kIOReturnSuccess remains
    # supplied by each fixture's existing boundary.
    text='#include "'+str(root/'kext/RTWControl.h')+'"\n'+prelude+text
    text=text.replace('struct ieee80211com {','struct ieee80211com {uint8_t ic_psk[32]{},ic_des_essid[32]{};unsigned ic_des_esslen=0;',1)
    text=text.replace('struct rtwn_softc {','struct rtwn_softc {unsigned iq_calibration_mask=0,sc_flags=0;',1)
    current={'8812':'is8812Family','88':'is88Family','8192FU':'is8192FUFamily','8710BU':'is8710BUFamily','8723':'is8723Family','8192DU':None}[suffix]
    members='''
    struct ControlCall {uint32_t selector;const void*input;size_t length;void*output;size_t*outputLength;};
    bool attached_=true,closing_=false,enabled_=false,joinRequested_=false,userRadioDisabled_=false;
    void*netif_=this,*stats_=this;int initialRXError_=0;struct{bool pending=false;}rxRecovery_;
    bool isLegacyRecoveryFamily()const{return false;}
    int recover8814ForControl(){return ENOTSUP;}
    int recoverFamilyForControl(){return familyRecovery_.recover(core_.core_error,[&]{return present_&&!sleeping_&&!closing_&&!stopping_&&!usbError_;},[&]{return true;},[&]{quiesce(false);},[&]{return recoveryDrained_;},[&]{return 0;},[&]{core_.core_error=0;});}
    void status(RTWControlStatus*){}void scanResults(RTWControlScanResults*){}void channelMap(RTWControlChannelMap*){}
    template<class...T>void setProperty(T...){}
    IOReturn enableStation();IOReturn handleControl(ControlCall*);
    void eraseProfile(){joinRequested_=false;core_.sc_ic.ic_des_esslen=0;memset(core_.sc_ic.ic_psk,0,sizeof(core_.sc_ic.ic_psk));}
    IOReturn request(uint32_t selector){RTWControlRequest request{};request.version=RTW_CONTROL_VERSION;RTWControlJoin join{};join.version=RTW_CONTROL_VERSION;join.ssidLength=1;join.ssid[0]='x';RTWControlReply reply{};size_t size=sizeof(reply);ControlCall call{selector,selector==RTW_CONTROL_JOIN?(const void*)&join:(const void*)&request,selector==RTW_CONTROL_JOIN?sizeof(join):sizeof(request),&reply,&size};return handleControl(&call);}
'''
    if 'rxRecovery_;' in text:members=members.replace('struct{bool pending=false;}rxRecovery_;','')
    for name in ['is8812Family','is88Family','is8192FUFamily','is8710BUFamily','is8723Family']:
        if name!=current:members+='    bool '+name+'()const{return false;}\n'
    for name in ['8812','88','8192FU','8710BU','8723','8192DU','8814']:
        if name!=suffix:members+='    int start'+name+'(){return ENOTSUP;}\n'
    members+='    void quiesce(bool erase){stopping_=true;enabled_=radioReady_=false;core_.sc_ic.ic_if.if_flags&=~IFF_UP;stop'+suffix+'();if(erase)joinRequested_=false;stopping_=false;}\n'
    a=text.index('class RTWUSBController');b=text.index('{',a);e=b+1;depth=1
    while depth:depth+=(text[e]=='{')-(text[e]=='}');e+=1
    text=text[:e-1]+members+text[e-1:]
    a=text.index('static int busInitialize(void*p)');b=text.index('{',a)+1
    text=text[:b]+'''auto*control=owner(p);if(control->initialRXError_){control->running_=false;if(control->initialRXError_==EBUSY)control->recoveryDrained_=false;return control->initialRXError_;}'''+text[b:]
    # Actual methods sit after the complete class, before its fixture/tests.
    if suffix=='8192DU':
        text=text.replace('    IOReturn enableStation();','    int start8192DU();static void initTask8192DU(void*);int allocateTransfers(){return 0;}\n    IOReturn enableStation();',1)
        text=text.replace('enum {IFF_RUNNING=1,','enum {IFF_UP=2,IFF_RUNNING=1,',1)
        text=text.replace('struct ieee80211com {','struct ieee80211com {ieee80211_channel*ic_ibss_chan=nullptr;int(*ic_newstate)(ieee80211com*,ieee80211_state,int)=nullptr;',1)
    a=text.index('static void put(uint8_t*p') if suffix=='8192DU' else text.index('struct Fixture')
    actual='\n'.join(function(source,m) for m in ['static IOReturn fromErrno(','IOReturn RTWUSBController::enableStation(','IOReturn RTWUSBController::handleControl('])
    backend=(root/'kext'/('RTW'+('8723BU' if suffix=='8723' else suffix)+'Backend.inc')).read_text()
    actual+='\n'+function(backend,'void RTWUSBController::initTask'+suffix+'(')
    if suffix=='8192DU':actual+='\n'+function((root/'kext/RTW8192DUBackend.inc').read_text(),'int RTWUSBController::start8192DU(')
    text=text[:a]+actual+'\n'+text[a:]
    cold='f.cold();' if suffix in ['8812','88'] else ''
    loop='for(auto family:{RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU})' if suffix=='8812' else 'for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})' if suffix=='88' else 'for(unsigned family:{0U})'
    ctor='Fixture f(family);' if suffix in ['8812','88'] else '(void)family;Fixture f;'
    if suffix=='8192DU':
        ctor='''DualFixture f;ieee80211_channel channel{1};ieee80211_node node{};node.ni_chan=&channel;node.ni_bssid[0]=2;node.ni_associd=1;node.ni_intval=100;node.ni_rates.rs_nrates=1;node.ni_rates.rs_rates[0]=0x82;f.c.core_.sc_ic.ic_bss=&node;f.c.core_.sc_ic.ic_ibss_chan=&channel;f.c.core_.sc_ic.ic_newstate=[](ieee80211com*ic,ieee80211_state state,int arg){auto*core=static_cast<rtwn_softc*>(ic->ic_softc);return RTWUSBController::owner(core->sc_ops.cookie)->state8192DU(ic,state,arg);};f.c.duPHYStorage_=f.c.test.storage.data();f.c.duPHYSize_=f.c.test.storage.size()*8;f.c.quiesce(false);(void)family;'''
    checks='''
    for(int transient:{EAGAIN,EBUSY})for(uint32_t selector:{uint32_t(RTW_CONTROL_SCAN),uint32_t(RTW_CONTROL_JOIN)})LOOP {
        CTOR COLD
        f.c.initialRXError_=transient;
        assert(f.c.request(selector)==kIOReturnBusy);
        assert(!f.c.core_.core_error&&!f.c.running_&&!f.c.radioReady_);
        f.c.initialRXError_=0;
        if(transient==EBUSY){assert(f.c.request(selector)==kIOReturnBusy);assert(!f.c.running_);f.c.recoveryDrained_=true;f.c.STOP();assert(!f.c.running_&&!f.c.radioReady_);}
        assert(f.c.request(selector)==kIOReturnSuccess);
        assert(f.c.running_&&f.c.radioReady_&&!f.c.core_.core_error);
        f.c.quiesce(false);
    }
    std::puts("Actual SCAN/JOIN + native startup/stop: initial RX EAGAIN/EBUSY retires old callbacks then explicitly restarts without a sticky latch");
'''.replace('LOOP',loop).replace('CTOR',ctor).replace('COLD',cold).replace('STOP','stop'+suffix)
    if suffix=='8192DU':checks=checks.replace('f.c.quiesce(false);\n    }','f.c.quiesce(false);f.c.duPHYStorage_=nullptr;f.c.duPHYSize_=0;\n    }')
    watchdog='''
    for(int transient:{EAGAIN,EBUSY})LOOP {
        CTOR COLD
        f.c.initialRXError_=transient;f.c.core_.sc_ic.ic_if.if_flags|=IFF_UP;
        RTWUSBController::TASK(&f.c);
        assert(!f.c.core_.core_error&&!f.c.running_&&!f.c.radioReady_);
        f.c.initialRXError_=0;
        if(transient==EBUSY){f.c.recoveryDrained_=true;f.c.STOP();assert(!f.c.running_);}
        assert(f.c.request(RTW_CONTROL_SCAN)==kIOReturnSuccess);
        assert(f.c.running_&&!f.c.core_.core_error);f.c.quiesce(false);
        CLEANUP
    }
'''.replace('LOOP',loop).replace('CTOR',ctor).replace('COLD',cold).replace('TASK','initTask'+suffix).replace('STOP','stop'+suffix).replace('CLEANUP','f.c.duPHYStorage_=nullptr;f.c.duPHYSize_=0;' if suffix=='8192DU' else '')
    checks+=watchdog
    if suffix=='88':checks+="""
    for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){Fixture f(family);f.cold();failEvent=11;
        assert(f.c.request(RTW_CONTROL_SCAN)==kIOReturnIOError);
        assert(f.c.core_.core_error==EIO&&!f.c.running_&&!f.c.familyRecovery_.blocked&&f.c.familyRecovery_.eligible&&f.c.familyRecovery_.diag.failures==1);
        failEvent=0;assert(f.c.request(RTW_CONTROL_SCAN)==kIOReturnSuccess&&f.c.running_);f.c.quiesce(false);
    }
"""
    if suffix=='8723':checks+="""
    {Fixture f;f.c.hw.power_ready=false;
        assert(f.c.request(RTW_CONTROL_SCAN)==kIOReturnTimeout);
        assert(f.c.core_.core_error==ETIMEDOUT&&f.c.familyRecovery_.eligible&&!f.c.familyRecovery_.blocked);
        const auto calls=f.c.hw.calls;RTWUSBController::initTask8723(&f.c);assert(f.c.hw.calls==calls&&f.c.core_.core_error==ETIMEDOUT);
        f.c.hw.power_ready=true;assert(f.c.request(RTW_CONTROL_SCAN)==kIOReturnSuccess&&f.c.running_);f.c.quiesce(false);
    }
"""

    a=text.index('int main(');b=text.index('{',a)+1;text=text[:b]+checks+text[b:]
    return text
