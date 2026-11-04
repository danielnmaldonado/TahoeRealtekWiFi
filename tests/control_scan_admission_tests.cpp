/* SPDX-License-Identifier: GPL-2.0-only */
#include "RTWControl.h"
#include "rtw/USBWiFiProfiles.h"
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>

using IOReturn=int;
enum {kIOReturnSuccess=0,kIOReturnBadArgument=101,kIOReturnNotReady=102,
      kIOReturnBusy=103,kIOReturnIOError=104,kIOReturnUnsupported=105};
enum ieee80211_state {IEEE80211_S_INIT,IEEE80211_S_SCAN,IEEE80211_S_AUTH,
                      IEEE80211_S_ASSOC,IEEE80211_S_RUN};
enum {IEEE80211_WPA_PROTO_WPA2=1,IEEE80211_WPA_AKM_PSK=1,
      IEEE80211_WPA_CIPHER_CCMP=8,IEEE80211_F_PSK=2};
struct ieee80211com;
struct _ifnet {ieee80211com* owner=nullptr;};
struct ieee80211com {
    _ifnet ic_if{this};ieee80211_state ic_state=IEEE80211_S_INIT;
    unsigned ic_flags=IEEE80211_F_PSK,ic_des_esslen=3;
    uint8_t ic_des_essid[32]{'L','a','b'},ic_psk[32]{};
    bool portValid=false;std::array<uint8_t,32> keys{};unsigned scanEntries=0;
};
struct ieee80211_wpaparams {int i_enabled,i_protos,i_akms,i_ciphers,i_groupcipher;};
static int ieee80211_ioctl_setwpaparms(ieee80211com*,const ieee80211_wpaparams*) {return 0;}
static void ieee80211_begin_scan(_ifnet* ifp) {
    // The real ieee80211_begin_scan calls ieee80211_node_cleanup for a STA.
    // Observe that dangerous boundary without performing RF work in a test.
    auto& ic=*ifp->owner;++ic.scanEntries;ic.portValid=false;ic.keys.fill(0);
    ic.ic_state=IEEE80211_S_SCAN;
}
struct rtwn_softc {int core_error=0;ieee80211com sc_ic;};
static int rtwn_io_error(rtwn_softc* core) {return core->core_error;}
class RTWUSBController {public:
    bool attached_=true,present_=true,sleeping_=false,running_=true,stopping_=false;
    bool joinRequested_=true,userRadioDisabled_=false;int usbError_=0;rtw_wifi_family family_;
    rtwn_softc core_{};struct {bool pending=false;} rxRecovery_;
    unsigned familyRecoveries=0,auRecoveries=0,enables=0,quiesces=0;
    explicit RTWUSBController(rtw_wifi_family family):family_(family) {
        core_.sc_ic.keys.fill(0x5a);std::memset(core_.sc_ic.ic_psk,0x7b,32);
    }
    struct ControlCall {uint32_t selector;const void* input;size_t length;void* output;size_t* outputLength;};
    IOReturn handleControl(ControlCall*);
    void status(RTWControlStatus* out) {out->version=1;out->state=core_.sc_ic.ic_state;}
    void scanResults(RTWControlScanResults* out) {out->version=1;}
    void channelMap(RTWControlChannelMap* out) {out->version=1;}
    int recover8814ForControl() {++auRecoveries;return 0;}
    int recoverFamilyForControl() {++familyRecoveries;return 0;}
    IOReturn enableStation() {++enables;running_=true;return 0;}
    void quiesce(bool erase) {
        ++quiesces;running_=false;core_.sc_ic.portValid=false;core_.sc_ic.keys.fill(0);
        if(erase){joinRequested_=false;core_.sc_ic.ic_des_esslen=0;}
    }
    static IOReturn fromErrno(int error) {return error;}
    void eraseProfile(){joinRequested_=false;core_.sc_ic.ic_des_esslen=0;std::memset(core_.sc_ic.ic_psk,0,sizeof(core_.sc_ic.ic_psk));}
    IOReturn request(uint32_t selector) {
        RTWControlRequest request{};request.version=1;
        RTWControlJoin join{};join.version=1;join.ssidLength=3;std::memcpy(join.ssid,"New",3);
        RTWControlReply reply{};size_t size=sizeof(reply);
        ControlCall call{selector,selector==RTW_CONTROL_JOIN?static_cast<const void*>(&join):&request,
            selector==RTW_CONTROL_JOIN?sizeof(join):sizeof(request),&reply,&size};
        const auto result=handleControl(&call);
        assert(size==sizeof(reply)&&reply.version==1);return result;
    }
};
/* ACTUAL_CONTROL_DISPATCH */
static constexpr rtw_wifi_family families[]={RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8188EU,
    RTW_WIFI_RTL8192EU,RTW_WIFI_RTL8192DU,RTW_WIFI_RTL8192FU,RTW_WIFI_RTL8710BU,
    RTW_WIFI_RTL8723BU,RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8814AU,RTW_WIFI_RTL8821AU,
    RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU};
int main() {
    unsigned cases=0;
    for(auto family:families) {
        // A command queued from an earlier idle STATUS must obey the live
        // state under the controller gate, before any family recovery/reset.
        for(auto state:{IEEE80211_S_AUTH,IEEE80211_S_ASSOC,IEEE80211_S_RUN}) {
            RTWUSBController c(family);auto& ic=c.core_.sc_ic;ic.ic_state=state;
            ic.portValid=state==IEEE80211_S_RUN;const auto keys=ic.keys;
            std::array<uint8_t,32> psk;std::memcpy(psk.data(),ic.ic_psk,psk.size());
            const auto result=c.request(RTW_CONTROL_SCAN);
            if(result!=kIOReturnBusy||ic.scanEntries||c.familyRecoveries||c.auRecoveries)
                std::fprintf(stderr,"SCAN preempted live state: family=%u state=%u result=%d scan=%u recovery=%u\n",
                    unsigned(family),unsigned(state),result,ic.scanEntries,c.familyRecoveries+c.auRecoveries);
            assert(result==kIOReturnBusy&&ic.scanEntries==0);
            assert(c.familyRecoveries==0&&c.auRecoveries==0&&c.enables==0&&c.quiesces==0);
            assert(ic.ic_state==state&&ic.portValid==(state==IEEE80211_S_RUN)&&ic.keys==keys);
            assert(c.joinRequested_&&ic.ic_des_esslen==3&&!std::memcmp(ic.ic_des_essid,"Lab",3));
            assert(!std::memcmp(ic.ic_psk,psk.data(),psk.size()));++cases;
        }
        // Real idle SCANs remain available, both before and after enabling.
        for(auto state:{IEEE80211_S_INIT,IEEE80211_S_SCAN})for(bool running:{false,true}) {
            RTWUSBController c(family);c.running_=running;c.core_.sc_ic.ic_state=state;
            assert(c.request(RTW_CONTROL_SCAN)==kIOReturnSuccess);
            assert(c.familyRecoveries+c.auRecoveries==1);
            assert(c.enables==unsigned(!running)&&c.core_.sc_ic.scanEntries==unsigned(running));++cases;
        }
        // Fault recovery must not be hidden by stale association metadata.
        RTWUSBController fault(family);fault.core_.sc_ic.ic_state=IEEE80211_S_RUN;
        fault.core_.core_error=EIO;assert(fault.request(RTW_CONTROL_SCAN)==kIOReturnNotReady);
        assert(fault.familyRecoveries+fault.auRecoveries==1&&fault.core_.sc_ic.scanEntries==0);++cases;
        RTWUSBController stopped(family);stopped.core_.sc_ic.ic_state=IEEE80211_S_RUN;stopped.running_=false;
        assert(stopped.request(RTW_CONTROL_SCAN)==kIOReturnSuccess&&stopped.enables==1);++cases;
        // Explicit JOIN and LEAVE retain their existing authority.
        for(uint32_t selector:{uint32_t(RTW_CONTROL_JOIN),uint32_t(RTW_CONTROL_LEAVE)}) {
            RTWUSBController c(family);c.core_.sc_ic.ic_state=IEEE80211_S_RUN;c.core_.sc_ic.portValid=true;
            assert(c.request(selector)==kIOReturnSuccess&&c.quiesces==1);
            if(selector==RTW_CONTROL_JOIN)assert(c.joinRequested_&&c.enables==1);
            else assert(!c.joinRequested_&&!c.running_);++cases;
        }
    }
    std::printf("Actual shared SCAN admission: %u cases across %zu families; live link/keys preserved, idle/recovery/JOIN/LEAVE retained\n",cases,std::size(families));
}
