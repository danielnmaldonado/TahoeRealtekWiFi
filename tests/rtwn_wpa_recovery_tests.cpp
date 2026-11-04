#include "platform.hpp"
#include <limits>
uint64_t RTWUSBController::clockMS = 100;
static unsigned checks = 0;
#define CHECK(x)                                                                                             \
    do {                                                                                                     \
        ++checks;                                                                                            \
        if (!(x)) {                                                                                          \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);                                \
            std::exit(1);                                                                                    \
        }                                                                                                    \
    } while (0)
static RTWUSBController *controller(ieee80211com *ic) {
    return RTWUSBController::owner(static_cast<rtwn_softc *>(ic->ic_softc)->sc_ops.cookie);
}
RTWUSBController::RTWUSBController() {
    core_.sc_ic.ic_softc = &core_;
    core_.sc_ops.cookie = this;
    core_.sc_ic.ic_if.if_softc = &core_;
    core_.sc_ic.ic_if.if_snd.queue = &pendingQueue_;
    core_.sc_ic.ic_if.if_watchdog = [](_ifnet *p) {
        ++RTWUSBController::owner(static_cast<rtwn_softc *>(p->if_softc)->sc_ops.cookie)->watchdogCalls;
    };
}
int rtwn_io_error(rtwn_softc *c) { return c->core_error ? EIO : 0; }
static int transition_boundary(ieee80211com *ic, ieee80211_state state) {
    auto *c = controller(ic);
    if(c->transitionError){
        if(c->clearChannelOnTransitionError&&ic->ic_bss)ic->ic_bss->ni_chan=nullptr;
        return c->transitionError;
    }
    c->transitions.push_back(state);
    ic->ic_state = state;
    if (state == IEEE80211_S_INIT || state == IEEE80211_S_SCAN) {
        if (ic->ic_bss) {
            ic->ic_bss->ni_port_valid = false;
            ic->ic_bss->ni_rsn_supp_state = 0;
        }
    }
    return 0;
}
int rtwn_newstate(ieee80211com *ic, ieee80211_state state, int) {
    ++controller(ic)->legacyStateCalls;
    return transition_boundary(ic,state);
}
int RTWUSBController::state8814(ieee80211com *ic,ieee80211_state state,int) {
    ++acStateCalls;
    // The hardware state algorithm is tested separately. This boundary
    // certifies a successful RUN, and clears its firmware peer on SCAN/INIT.
    acRateReady_=acMediaConnected_=state==IEEE80211_S_RUN;
    return transition_boundary(ic,state);
}
int RTWUSBController::state8723(ieee80211com*ic,ieee80211_state state,int){
    ++r23StateCalls;r23RateReady_=r23Media_=state==IEEE80211_S_RUN;return transition_boundary(ic,state);
}
void RTWUSBController::stop8723(){++r23StopCalls;(void)state8723(&core_.sc_ic,IEEE80211_S_INIT,-1);running_=false;}
int RTWUSBController::state8710BU(ieee80211com*ic,ieee80211_state state,int){
    ++buStateCalls;buRateReady_=buMedia_=state==IEEE80211_S_RUN;return transition_boundary(ic,state);
}
void RTWUSBController::stop8710BU(){++buStopCalls;(void)state8710BU(&core_.sc_ic,IEEE80211_S_INIT,-1);running_=false;}
int RTWUSBController::state8192FU(ieee80211com*ic,ieee80211_state state,int){
    ++fuStateCalls;fuRateReady_=fuMedia_=state==IEEE80211_S_RUN;return transition_boundary(ic,state);
}
void RTWUSBController::stop8192FU(){++fuStopCalls;(void)state8192FU(&core_.sc_ic,IEEE80211_S_INIT,-1);running_=false;}
int RTWUSBController::state8192DU(ieee80211com*ic,ieee80211_state state,int){
    ++duStateCalls;duRateReady_=duMedia_=state==IEEE80211_S_RUN;return transition_boundary(ic,state);
}
void RTWUSBController::stop8192DU(){++duStopCalls;(void)state8192DU(&core_.sc_ic,IEEE80211_S_INIT,-1);running_=false;}
int RTWUSBController::state88(ieee80211com*ic,ieee80211_state state,int){
    ++r88StateCalls;r88RateReady_=r88Media_=state==IEEE80211_S_RUN;
    return transition_boundary(ic,state);
}
void RTWUSBController::stop88(){++r88StopCalls;(void)state88(&core_.sc_ic,IEEE80211_S_INIT,-1);running_=false;}
int RTWUSBController::state8812(ieee80211com*ic,ieee80211_state state,int){
    ++auStateCalls;auRateReady_=auMediaConnected_=state==IEEE80211_S_RUN;
    return transition_boundary(ic,state);
}
void RTWUSBController::stop8812(){++auStopCalls;(void)state8812(&core_.sc_ic,IEEE80211_S_INIT,-1);running_=false;}
void RTWUSBController::stop8814() {
    ++acStopCalls;
    (void)state8814(&core_.sc_ic,IEEE80211_S_INIT,-1);
    running_=false;
}
void ieee80211_begin_scan(_ifnet *p) {
    auto *c = static_cast<rtwn_softc *>(p->if_softc);
    ++RTWUSBController::owner(c->sc_ops.cookie)->scanCalls;
    RTWUSBController::stateTransition(&c->sc_ic, IEEE80211_S_SCAN, -1);
}
void rtwn_stop(_ifnet *p) {
    auto *core = static_cast<rtwn_softc *>(p->if_softc);
    auto *c = RTWUSBController::owner(core->sc_ops.cookie);
    ++c->stopCalls;
    RTWUSBController::stateTransition(&core->sc_ic, IEEE80211_S_INIT, -1);
    c->running_ = false;
}
// Only the hardware start is a boundary; enableStation's actual admission,
// flags, rollback, and profile handling are compiled from production.
int RTWUSBController::startStationHardware() {
    ++initCalls;
    if(initError)return initError;
    running_ = true;
    return stateTransition(&core_.sc_ic, IEEE80211_S_SCAN, -1);
}
int rtwn_init(_ifnet *p) {
    auto *core=static_cast<rtwn_softc *>(p->if_softc);
    return RTWUSBController::owner(core->sc_ops.cookie)->startStationHardware();
}
static void run(RTWUSBController &c, ieee80211_node &node, uint64_t time = 100) {
    RTWUSBController::clockMS = time;
    c.core_.sc_ic.ic_bss = &node;
    c.core_.sc_ic.ic_state = IEEE80211_S_ASSOC;
    CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic, IEEE80211_S_RUN, 0) == 0);
}
static void tick(RTWUSBController &c, uint64_t time) {
    RTWUSBController::clockMS = time;
    RTWUSBController::watchdogAction(&c, nullptr);
}
// Removing RUN arming or the deadline branch leaves the controlled port blocked forever.
static void missing_m1_retries_without_usb_restart() {
    RTWUSBController c;
    ieee80211_node n;
    c.core_.sc_ic.ic_psk[7] = 0xa5;
    run(c, n);
    tick(c, 15099);
    CHECK(c.core_.sc_ic.ic_state == IEEE80211_S_RUN);
    tick(c, 15100);
    CHECK(c.core_.sc_ic.ic_state == IEEE80211_S_SCAN);
    CHECK(c.wpaRecovery_.retries == 1);
    CHECK(c.wpaRecovery_.lastSupplicantState == 1);
    CHECK(c.joinRequested_);
    CHECK(c.core_.sc_ic.ic_flags & IEEE80211_F_PSK);
    CHECK(c.core_.sc_ic.ic_psk[7] == 0xa5);
    CHECK(c.initCalls == 0);
    CHECK(c.stopCalls == 0);
}

// M2 progress cannot extend the association deadline indefinitely.
static void stalled_m2_uses_same_deadline() {
    RTWUSBController c;
    ieee80211_node n;
    run(c, n);
    n.ni_rsn_supp_state = 2;
    tick(c, 10100);
    CHECK(c.core_.sc_ic.ic_state == IEEE80211_S_RUN);
    CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic, IEEE80211_S_RUN, 0) == 0);
    tick(c, 15100);
    CHECK(c.wpaRecovery_.retries == 1);
    CHECK(c.wpaRecovery_.lastSupplicantState == 2);
}
// Each retry either reaches a new RUN or consumes its finite scan/association budget.
static void retries_exhaust_and_stay_terminal() {
    RTWUSBController c;
    ieee80211_node n;
    run(c, n);
    for (unsigned retry = 1; retry <= 3; ++retry) {
        tick(c, 100 + retry * 15000);
        CHECK(c.core_.sc_ic.ic_state == IEEE80211_S_SCAN);
        CHECK(c.wpaRecovery_.retries == retry);
    }
    tick(c, 60100);
    CHECK(c.core_.sc_ic.ic_state == IEEE80211_S_INIT);
    CHECK(c.wpaRecovery_.state == RTWUSBController::WPARecoveryFailed);
    CHECK(c.wpaRecovery_.error == ETIMEDOUT);
    CHECK(!c.wpaRecovery_.active);
    CHECK(c.wpaRecovery_.timeouts == 4);
    CHECK(c.scanTimerValue_.cancelled > 0);
    CHECK(c.joinRequested_);
    CHECK(c.core_.sc_ic.ic_des_esslen == 1);
    const auto transitions = c.transitions.size();
    for (unsigned i = 0; i < 20; ++i)
        tick(c, 100000 + i * 15000);
    CHECK(c.transitions.size() == transitions);
    CHECK(c.initCalls == 0);
    CHECK(c.stopCalls == 0);
    CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic, IEEE80211_S_AUTH, 0) == ETIMEDOUT);
    CHECK(c.core_.sc_ic.ic_state == IEEE80211_S_INIT);
}
static void retry_run_gets_one_handshake_window() {
    RTWUSBController c;
    ieee80211_node n;
    run(c, n);
    tick(c, 15100);
    run(c, n, 29100);
    n.ni_rsn_supp_state = 1;
    tick(c, 43100);
    CHECK(c.wpaRecovery_.retries == 1);
    tick(c, 44100);
    CHECK(c.wpaRecovery_.retries == 2);
}
// Opening the actual controlled port is the only success condition.
static void authorization_cancels_deadline() {
    RTWUSBController c;
    ieee80211_node n;
    run(c, n);
    tick(c, 10100);
    n.ni_port_valid = true;
    tick(c, 15100);
    CHECK(!c.wpaRecovery_.active);
    CHECK(c.wpaRecovery_.state == RTWUSBController::WPARecoveryAuthorized);
    CHECK(c.wpaRecovery_.retries == 0);
    tick(c, 900000);
    CHECK(c.core_.sc_ic.ic_state == IEEE80211_S_RUN);
    CHECK(c.transitions.size() == 1);
}
static void erased_profile_never_retries() {
    RTWUSBController c;
    ieee80211_node n;
    run(c, n);
    c.eraseProfile();
    tick(c, 900000);
    CHECK(!c.wpaRecovery_.active);
    CHECK(c.wpaRecovery_.retries == 0);
    CHECK(!c.joinRequested_);
    CHECK(c.core_.sc_ic.ic_des_esslen == 0);
    for (auto b : c.core_.sc_ic.ic_psk)
        CHECK(b == 0);
    CHECK(c.transitions.size() == 1);
}
static void invalid_profile_and_lifecycle_cancel() {
    for (unsigned reason = 0; reason < 8; ++reason) {
        RTWUSBController c;
        ieee80211_node n;
        run(c, n);
        switch (reason) {
        case 0:
            c.present_ = false;
            break;
        case 1:
            c.sleeping_ = true;
            break;
        case 2:
            c.usbError_ = kIOReturnNoDevice;
            break;
        case 3:
            c.core_.core_error = true;
            break;
        case 4:
            c.core_.sc_ic.ic_flags &= ~IEEE80211_F_PSK;
            break;
        case 5:
            c.core_.sc_ic.ic_flags &= ~IEEE80211_F_RSNON;
            break;
        case 6:
            c.core_.sc_ic.ic_des_esslen = 0;
            break;
        case 7:
            c.radioReady_ = false;
            break;
        }
        tick(c, 900000);
        CHECK(!c.wpaRecovery_.active);
        CHECK(c.wpaRecovery_.retries == 0);
        for (auto state : c.transitions)
            CHECK(state != IEEE80211_S_SCAN);
    }
}
static void lifecycle_stops_cancel_wpa_without_erasing_pm_intent() {
    for (auto operation :
         {RTWUSBController::Disable, RTWUSBController::Sleep, RTWUSBController::Terminated}) {
        RTWUSBController c;
        ieee80211_node n;
        run(c, n);
        CHECK(RTWUSBController::gated(&c, reinterpret_cast<void *>(operation), nullptr, nullptr, nullptr) ==
              0);
        tick(c, 900000);
        CHECK(!c.wpaRecovery_.active);
        CHECK(c.joinRequested_ == (operation != RTWUSBController::Terminated));
        CHECK(!c.running_);
        CHECK(c.wpaRecovery_.retries == 0);
    }
    RTWUSBController c;
    ieee80211_node n;
    run(c, n);
    RTWControlRequest request{};
    request.version = 1;
    RTWControlReply reply{};
    size_t size = sizeof(reply);
    RTWUSBController::ControlCall call{RTW_CONTROL_LEAVE, &request, sizeof(request), &reply, &size};
    CHECK(c.handleControl(&call) == 0);
    tick(c, 900000);
    CHECK(!c.wpaRecovery_.active);
    CHECK(!c.joinRequested_);
}

static IOReturn gate(RTWUSBController &c, RTWUSBController::GateOperation operation, void *argument=nullptr) {
    return RTWUSBController::gated(&c,reinterpret_cast<void *>(operation),argument,nullptr,nullptr);
}
static IOReturn radioRequest(RTWUSBController &c,uint32_t selector,bool valid=true) {
    RTWControlRequest request{};request.version=valid?RTW_CONTROL_VERSION:0;
    RTWControlJoin join{};join.version=request.version;join.ssidLength=3;
    std::memcpy(join.ssid,"New",3);std::memset(join.pmk,0x39,sizeof(join.pmk));
    RTWControlReply reply{};size_t size=sizeof(reply);
    RTWUSBController::ControlCall call{selector,
        selector==RTW_CONTROL_JOIN?static_cast<const void *>(&join):&request,
        selector==RTW_CONTROL_JOIN?sizeof(join):sizeof(request),&reply,&size};
    return gate(c,RTWUSBController::Control,&call);
}
static bool manualRadioOffStatus(RTWUSBController &c) {
    RTWControlRequest request{};request.version=RTW_CONTROL_VERSION;
    RTWControlStatus status{};size_t size=sizeof(status);
    RTWUSBController::ControlCall call{RTW_CONTROL_STATUS,&request,sizeof(request),&status,&size};
    CHECK(gate(c,RTWUSBController::Control,&call)==kIOReturnSuccess);
    CHECK(status.version==RTW_CONTROL_VERSION&&size==sizeof(status));
    for(auto reserved:status.reserved)CHECK(reserved==0);
    return (status.flags&(1U<<13))!=0; // Additive ABI v1 manual-radio-off bit.
}
static void configuredProfile(RTWUSBController &c,uint8_t key) {
    auto &ic=c.core_.sc_ic;
    c.joinRequested_=true;ic.ic_flags|=IEEE80211_F_RSNON|IEEE80211_F_PSK;
    ic.ic_des_esslen=3;std::memcpy(ic.ic_des_essid,"Lab",3);
    std::memset(ic.ic_psk,key,sizeof(ic.ic_psk));
}
static void expectProfile(const RTWUSBController &c,uint8_t key,const char *ssid="Lab") {
    const auto &ic=c.core_.sc_ic;
    CHECK(c.joinRequested_&&ic.ic_des_esslen==3);
    CHECK(!std::memcmp(ic.ic_des_essid,ssid,3));
    CHECK((ic.ic_flags&(IEEE80211_F_RSNON|IEEE80211_F_PSK))==(IEEE80211_F_RSNON|IEEE80211_F_PSK));
    for(auto byte:ic.ic_psk)CHECK(byte==key);
}
static void expectErasedProfile(const RTWUSBController &c) {
    const auto &ic=c.core_.sc_ic;
    CHECK(!c.joinRequested_&&ic.ic_des_esslen==0);
    CHECK(!(ic.ic_flags&(IEEE80211_F_RSNON|IEEE80211_F_PSK|IEEE80211_F_AUTO_JOIN)));
    for(auto byte:ic.ic_psk)CHECK(byte==0);
    for(auto byte:ic.ic_des_essid)CHECK(byte==0);
}
// IOEthernetInterface disables a running controller BEFORE setPowerState(0).
// Either erasing Disable or erasing Sleep must fail this test, even if Wake
// were able to ask the app for a new profile later.
static void pm_cycle_preserves_configured_profile() {
    for(auto family:{RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8188EU,RTW_WIFI_RTL8192EU,
        RTW_WIFI_RTL8192DU,RTW_WIFI_RTL8192FU,RTW_WIFI_RTL8710BU,
        RTW_WIFI_RTL8723BU,RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU,
        RTW_WIFI_RTL8814AU,RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}) {
        RTWUSBController c;ieee80211_node node;c.family_=family;configuredProfile(c,0xa7);run(c,node);
        for(unsigned cycle=0;cycle<3;++cycle) {
            CHECK(gate(c,RTWUSBController::Disable)==kIOReturnSuccess);
            expectProfile(c,0xa7);
            CHECK(!c.running_&&!c.enabled_&&!c.radioReady_&&!c.wpaRecovery_.active);
            CHECK(c.core_.sc_ic.ic_state==IEEE80211_S_INIT&&!node.ni_port_valid);
            const unsigned starts=c.initCalls;
            CHECK(gate(c,RTWUSBController::Sleep)==kIOReturnSuccess);
            CHECK(c.sleeping_);expectProfile(c,0xa7);
            CHECK(gate(c,RTWUSBController::Wake)==kIOReturnSuccess);
            CHECK(!c.sleeping_&&!c.running_&&c.initCalls==starts);
            // Host re-enable, after the usable-state notification, owns I/O.
            CHECK(gate(c,RTWUSBController::Enable)==kIOReturnSuccess);
            CHECK(c.running_&&c.enabled_&&c.radioReady_&&c.initCalls==starts+1);
            CHECK(c.core_.sc_ic.ic_state==IEEE80211_S_SCAN);expectProfile(c,0xa7);
            CHECK(gate(c,RTWUSBController::Enable)==kIOReturnSuccess&&c.initCalls==starts+1);
        }
    }
    RTWUSBController direct;configuredProfile(direct,0x63);
    CHECK(gate(direct,RTWUSBController::Sleep)==0);expectProfile(direct,0x63);
    CHECK(!direct.running_&&direct.sleeping_);
}
// LEAVE expresses manual radio-off both awake and during a PM power drop.
// A later host enable must succeed without starting radio or restoring keys.
static void manual_radio_off_survives_pm_cycles() {
    for(bool leaveWhileSleeping:{false,true}) {
        RTWUSBController c;configuredProfile(c,0x72);
        if(leaveWhileSleeping) {
            CHECK(gate(c,RTWUSBController::Disable)==0);
            CHECK(gate(c,RTWUSBController::Sleep)==0);
        }
        CHECK(radioRequest(c,RTW_CONTROL_LEAVE)==kIOReturnSuccess);expectErasedProfile(c);
        CHECK(manualRadioOffStatus(c));
        for(unsigned cycle=0;cycle<3;++cycle) {
            CHECK(gate(c,RTWUSBController::Disable)==0);
            CHECK(gate(c,RTWUSBController::Sleep)==0);
            const unsigned starts=c.initCalls;
            CHECK(gate(c,RTWUSBController::Wake)==0);
            CHECK(gate(c,RTWUSBController::Enable)==kIOReturnSuccess);
            CHECK(c.initCalls==starts&&!c.running_&&!c.enabled_&&!c.radioReady_);
            CHECK(manualRadioOffStatus(c));
            expectErasedProfile(c);
        }
        // Clearing a profile alone is not permission to turn radio back on.
        c.eraseProfile();CHECK(gate(c,RTWUSBController::Enable)==0);
        CHECK(c.initCalls==0&&!c.running_);
    }
}
// Only admitted explicit JOIN/SCAN can resume a manual radio-off. Malformed
// messages and a rejected request during sleep must not change that intent.
static void explicit_radio_resume_requires_valid_admitted_command() {
    RTWUSBController malformed;configuredProfile(malformed,0x4a);
    CHECK(radioRequest(malformed,RTW_CONTROL_LEAVE,false)==kIOReturnBadArgument);
    CHECK(!manualRadioOffStatus(malformed));
    CHECK(malformed.running_);expectProfile(malformed,0x4a);
    CHECK(gate(malformed,RTWUSBController::Disable)==0);
    CHECK(gate(malformed,RTWUSBController::Enable)==0&&malformed.running_);
    for(uint32_t selector:{uint32_t(RTW_CONTROL_JOIN),uint32_t(RTW_CONTROL_SCAN)}) {
        RTWUSBController c;CHECK(radioRequest(c,RTW_CONTROL_LEAVE)==0);
        CHECK(radioRequest(c,selector,false)==kIOReturnBadArgument);
        CHECK(manualRadioOffStatus(c));
        CHECK(gate(c,RTWUSBController::Enable)==0&&!c.running_&&c.initCalls==0);
        CHECK(gate(c,RTWUSBController::Sleep)==0);
        CHECK(radioRequest(c,selector)==kIOReturnNotReady);
        CHECK(gate(c,RTWUSBController::Wake)==0);
        CHECK(gate(c,RTWUSBController::Enable)==0&&!c.running_&&c.initCalls==0);
        c.rxRecovery_.pending=selector==RTW_CONTROL_SCAN;
        if(c.rxRecovery_.pending) {
            CHECK(radioRequest(c,selector)==kIOReturnBusy);
            CHECK(gate(c,RTWUSBController::Enable)==0&&!c.running_&&c.initCalls==0);
            c.rxRecovery_.pending=false;
        }
        CHECK(radioRequest(c,selector)==kIOReturnSuccess);
        CHECK(!manualRadioOffStatus(c));
        CHECK(c.running_&&c.enabled_&&c.radioReady_&&c.initCalls==1);
        if(selector==RTW_CONTROL_JOIN)expectProfile(c,0x39,"New");
        else expectErasedProfile(c);
        CHECK(gate(c,RTWUSBController::Disable)==0);
        CHECK(gate(c,RTWUSBController::Sleep)==0);
        CHECK(gate(c,RTWUSBController::Wake)==0);
        CHECK(gate(c,RTWUSBController::Enable)==0&&c.initCalls==2&&c.running_);
    }
    RTWUSBController failed;configuredProfile(failed,0x24);failed.initError=EIO;
    CHECK(radioRequest(failed,RTW_CONTROL_JOIN)==EIO);expectErasedProfile(failed);
    CHECK(!failed.running_&&!failed.enabled_&&!failed.radioReady_);
}
// Per-adapter intent cannot leak, and stale wake/enable callbacks after
// shutdown or termination must not resurrect a retained configured profile.
static void pm_intent_is_per_adapter_and_teardown_wins() {
    RTWUSBController disabled,connected;configuredProfile(disabled,0x15);configuredProfile(connected,0x2b);
    CHECK(radioRequest(disabled,RTW_CONTROL_LEAVE)==0);
    for(auto *c:{&disabled,&connected}) {
        CHECK(gate(*c,RTWUSBController::Disable)==0);
        CHECK(gate(*c,RTWUSBController::Sleep)==0);
        CHECK(gate(*c,RTWUSBController::Wake)==0);
        CHECK(gate(*c,RTWUSBController::Enable)==0);
    }
    CHECK(!disabled.running_&&disabled.initCalls==0);expectErasedProfile(disabled);
    CHECK(manualRadioOffStatus(disabled)&&!manualRadioOffStatus(connected));
    CHECK(connected.running_&&connected.initCalls==1);expectProfile(connected,0x2b);
    for(auto operation:{RTWUSBController::Shutdown,RTWUSBController::Terminated}) {
        RTWUSBController c;configuredProfile(c,0x5d);
        CHECK(gate(c,operation)==0);expectErasedProfile(c);
        CHECK(gate(c,RTWUSBController::Wake)==kIOReturnNotReady);
        CHECK(gate(c,RTWUSBController::Enable)==kIOReturnNotReady);
        CHECK(!c.running_&&c.initCalls==0);
    }
}
static void manual_join_resets_exhausted_campaign() {
    RTWUSBController c;
    ieee80211_node n;
    run(c, n);
    for (unsigned i = 1; i <= 4; ++i)
        tick(c, 100 + i * 15000);
    CHECK(c.wpaRecovery_.error == ETIMEDOUT);
    RTWControlJoin join{};
    join.version = 1;
    join.ssidLength = 1;
    join.ssid[0] = 'z';
    RTWControlReply reply{};
    size_t size = sizeof(reply);
    RTWUSBController::ControlCall call{RTW_CONTROL_JOIN, &join, sizeof(join), &reply, &size};
    CHECK(c.handleControl(&call) == 0);
    CHECK(c.wpaRecovery_.retries == 0);
    CHECK(c.wpaRecovery_.error == 0);
    CHECK(c.joinRequested_);
    CHECK(c.core_.sc_ic.ic_des_essid[0] == 'z');
    run(c, n, 100000);
    tick(c, 115000);
    CHECK(c.wpaRecovery_.retries == 1);
}
static void clock_wrap_and_backwards_are_bounded() {
    RTWUSBController wrapped;
    ieee80211_node n;
    run(wrapped, n, std::numeric_limits<uint64_t>::max() - 999);
    tick(wrapped, 13999);
    CHECK(wrapped.wpaRecovery_.retries == 0);
    tick(wrapped, 14000);
    CHECK(wrapped.wpaRecovery_.retries == 1);
    RTWUSBController backwards;
    ieee80211_node b;
    run(backwards, b, 10000);
    tick(backwards, 15000);
    CHECK(backwards.wpaRecovery_.retries == 0);
    tick(backwards, 14000);
    CHECK(backwards.wpaRecovery_.retries == 0);
    tick(backwards, 23999);
    CHECK(backwards.wpaRecovery_.retries == 0);
    tick(backwards, 24000);
    CHECK(backwards.wpaRecovery_.retries == 1);
}
// Imported dequeue transfers ownership once; a later recovery flush must not free it again.
static void dequeued_tx_is_not_owned_by_recovery_queue() {
    RTWUSBController c;
    ieee80211_node n;
    run(c, n);
    Packet packet;
    packet.node = &n;
    c.pendingQueue_.lockEnqueue(&packet);
    c.core_.sc_ops.is_oactive = [](void *) { return 0; };
    c.core_.sc_ops.tx = [](void *, mbuf_t m, ieee80211_node *node) {
        mbuf_freem(m);
        --node->refs;
        return 0;
    };
    rtwn_start(&c.core_.sc_ic.ic_if);
    CHECK(packet.frees == 1);
    CHECK(n.refs == 0);
    CHECK(c.pendingQueue_.packets.empty());
    tick(c, 15100);
    CHECK(packet.frees == 1);
    CHECK(n.refs == 0);
}
// Status is read-only even when polled much faster than the watchdog.
static void status_polling_does_not_extend_deadline() {
    RTWUSBController c;
    ieee80211_node n;
    run(c, n);
    for (unsigned i = 0; i < 100; ++i) {
        RTWUSBController::clockMS = 100 + i * 150;
        RTWControlStatus status{};
        c.status(&status);
        CHECK(status.state == IEEE80211_S_RUN);
        CHECK(!(status.flags & RTW_CONTROL_AUTHORIZED));
    }
    CHECK(c.wpaRecovery_.lastMS == 100);
    CHECK(c.wpaRecovery_.remainingMS == 15000);
    CHECK(c.properties.at("RTWDiagScanGeneration") == 0);
    CHECK(c.properties.at("RTWDiagWPARecoveryActive") == 1);
    tick(c, 15100);
    CHECK(c.wpaRecovery_.retries == 1);
    for (unsigned i = 2; i <= 4; ++i)
        tick(c, 100 + i * 15000);
    RTWControlStatus status{};
    c.status(&status);
    CHECK(status.state == IEEE80211_S_INIT);
    CHECK(status.usbError == 0);
    CHECK(!(status.flags & RTW_CONTROL_AUTHORIZED));
    CHECK(c.properties.at("RTWDiagWPARecoveryState") == 4);
    CHECK(c.properties.at("RTWDiagWPARecoveryError") == ETIMEDOUT);
    CHECK(c.properties.at("RTWDiagWPARecoveryTimeouts") == 4);
    CHECK(c.properties.at("RTWDiagWPARecoveryRetries") == 3);
    CHECK(c.properties.at("RTWDiagWPARecoveryRemainingMS") == 0);
    CHECK(c.properties.at("RTWDiagWPARecoveryLastSupplicantState") == 0);
}
// A completed exchange cancels its budget at the actual link-up callback.
static void link_authorization_cancels_before_next_watchdog() {
    RTWUSBController c;
    ieee80211_node n;
    run(c, n);
    n.ni_port_valid = true;
    CHECK(c.setLinkStatus(kIONetworkLinkValid | kIONetworkLinkActive));
    CHECK(!c.wpaRecovery_.active);
    CHECK(c.wpaRecovery_.state == RTWUSBController::WPARecoveryAuthorized);
}
// Reauthentication loops within a failing attempt cannot replenish its clock.
static void auth_assoc_loops_keep_original_deadline() {
    RTWUSBController c;
    ieee80211_node n;
    run(c, n);
    tick(c, 10100);
    RTWUSBController::clockMS = 11100;
    CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic, IEEE80211_S_AUTH, 0) == 0);
    CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic, IEEE80211_S_ASSOC, 0) == 0);
    CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic, IEEE80211_S_RUN, 0) == 0);
    tick(c, 15099);
    CHECK(c.wpaRecovery_.retries == 0);
    tick(c, 15100);
    CHECK(c.wpaRecovery_.retries == 1);
}
// A user can scan after failure, but only a fresh JOIN can restart authorization.
static void manual_scan_after_failure_does_not_restart_join() {
    RTWUSBController c;
    ieee80211_node n;
    run(c, n);
    for (unsigned i = 1; i <= 4; ++i)
        tick(c, 100 + i * 15000);
    RTWControlRequest request{};
    request.version = 1;
    RTWControlReply reply{};
    size_t size = sizeof(reply);
    RTWUSBController::ControlCall call{RTW_CONTROL_SCAN, &request, sizeof(request), &reply, &size};
    CHECK(c.handleControl(&call) == 0);
    CHECK(c.core_.sc_ic.ic_state == IEEE80211_S_SCAN);
    for (auto state : {IEEE80211_S_AUTH, IEEE80211_S_ASSOC, IEEE80211_S_RUN})
        CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic, state, 0) == ETIMEDOUT);
    tick(c, 900000);
    CHECK(!c.wpaRecovery_.active);
    CHECK(c.wpaRecovery_.retries == 3);
    CHECK(c.wpaRecovery_.error == ETIMEDOUT);
    call.selector = RTW_CONTROL_LEAVE;
    size = sizeof(reply);
    CHECK(c.handleControl(&call) == 0);
    CHECK(!c.joinRequested_);
    CHECK(c.wpaRecovery_.error == 0);
    CHECK(c.wpaRecovery_.state == RTWUSBController::WPARecoveryIdle);
}
static void manual_scan_waits_for_rx_recovery_without_changing_hardware_state() {
    const auto backendCalls=[](const RTWUSBController&c){
        return c.legacyStateCalls+c.acStateCalls+c.auStateCalls+c.r88StateCalls+
               c.duStateCalls+c.fuStateCalls+c.buStateCalls+c.r23StateCalls;
    };
    for(auto family:{RTW_WIFI_RTL8192EU,RTW_WIFI_RTL8188EU,RTW_WIFI_RTL8192CU,
                     RTW_WIFI_RTL8814AU,RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU,
                     RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU,
                     RTW_WIFI_RTL8192DU,RTW_WIFI_RTL8192FU,RTW_WIFI_RTL8710BU,RTW_WIFI_RTL8723BU}){
        for(bool running:{false,true}){
            RTWUSBController c;ieee80211_node n;c.family_=family;c.acCore_.phy_initialized=1;run(c,n);
            c.running_=running;c.rxRecovery_.pending=true;
            const auto transitions=c.transitions.size();const auto calls=backendCalls(c);
            RTWControlRequest request{};request.version=RTW_CONTROL_VERSION;
            RTWControlReply reply{};size_t size=sizeof(reply);
            RTWUSBController::ControlCall call{RTW_CONTROL_SCAN,&request,sizeof(request),&reply,&size};
            CHECK(c.handleControl(&call)==kIOReturnBusy);
            CHECK(c.scanCalls==0&&c.initCalls==0&&c.transitions.size()==transitions&&backendCalls(c)==calls);
            CHECK(c.core_.sc_ic.ic_state==IEEE80211_S_RUN&&c.rxRecovery_.pending&&c.running_==running);
            RTWControlStatus status{};size=sizeof(status);
            call.selector=RTW_CONTROL_STATUS;call.output=&status;
            CHECK(c.handleControl(&call)==kIOReturnSuccess&&status.state==IEEE80211_S_RUN);
            RTWControlScanResults results{};size=sizeof(results);
            call.selector=RTW_CONTROL_SCAN_RESULTS;call.output=&results;
            CHECK(c.handleControl(&call)==kIOReturnSuccess);
            CHECK(c.scanCalls==0&&c.initCalls==0&&c.transitions.size()==transitions&&backendCalls(c)==calls);
            c.rxRecovery_.pending=false;c.running_=true;size=sizeof(reply);
            call.selector=RTW_CONTROL_SCAN;call.output=&reply;
            CHECK(c.handleControl(&call)==kIOReturnBusy);
            CHECK(c.scanCalls==0&&c.initCalls==0&&c.transitions.size()==transitions&&backendCalls(c)==calls);
            CHECK(c.core_.sc_ic.ic_state==IEEE80211_S_RUN);
            // Once the live attempt has ended, normal idle discovery resumes.
            c.core_.sc_ic.ic_state=IEEE80211_S_INIT;size=sizeof(reply);
            CHECK(c.handleControl(&call)==kIOReturnSuccess);
            CHECK(c.scanCalls==1&&c.initCalls==0&&c.transitions.size()==transitions+1&&backendCalls(c)==calls+1);
            CHECK(c.core_.sc_ic.ic_state==IEEE80211_S_SCAN);
        }
    }
}
static void scan_diagnostics_follow_only_successful_backend_transitions(){
    for(auto family:{RTW_WIFI_RTL8192EU,RTW_WIFI_RTL8814AU,RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821CU,
                     RTW_WIFI_RTL8192DU,RTW_WIFI_RTL8192FU,RTW_WIFI_RTL8710BU,RTW_WIFI_RTL8723BU}){
        RTWUSBController c;ieee80211_node n;c.family_=family;
        c.core_.sc_ic.ic_bss=&n;c.core_.sc_ic.ic_state=IEEE80211_S_INIT;
        n.ni_chan=reinterpret_cast<void*>(149);
        CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic,IEEE80211_S_SCAN,0)==0);
        CHECK(c.scanDiag_.generation==1&&c.scanDiag_.hops==1&&c.scanDiag_.visited[2]==2097152);
        CHECK(c.scanDiag_.state==rtw::ScanDiagnosticState::Active&&c.scanDiag_.lastChannel==149);
        n.ni_chan=reinterpret_cast<void*>(153);c.transitionError=EIO;c.clearChannelOnTransitionError=true;
        CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic,IEEE80211_S_SCAN,0)==EIO);
        CHECK(c.scanDiag_.state==rtw::ScanDiagnosticState::Failed&&c.scanDiag_.lastError==EIO);
        CHECK(c.scanDiag_.hops==1&&c.scanDiag_.visited[2]==2097152&&c.scanDiag_.lastVisitedChannel==149&&c.scanDiag_.lastChannel==153);
        c.transitionError=0;n.ni_chan=reinterpret_cast<void*>(196);
        CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic,IEEE80211_S_SCAN,0)==0);
        CHECK(c.scanDiag_.generation==2&&c.scanDiag_.hops==1&&c.scanDiag_.visited[2]==0&&c.scanDiag_.visited[3]==16);
        CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic,IEEE80211_S_AUTH,0)==0);
        CHECK(c.scanDiag_.state==rtw::ScanDiagnosticState::Complete&&c.scanDiag_.visited[3]==16);
        CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic,IEEE80211_S_INIT,0)==0);
        CHECK(c.scanDiag_.state==rtw::ScanDiagnosticState::Complete);
        n.ni_chan=reinterpret_cast<void*>(36);
        CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic,IEEE80211_S_SCAN,0)==0);
        c.quiesce(false);
        CHECK(c.scanDiag_.state==rtw::ScanDiagnosticState::Cancelled&&c.scanDiag_.visited[0]==UINT64_C(68719476736));
        CHECK(c.properties.at("RTWDiagScanGeneration")==3&&c.properties.at("RTWDiagScanState")==3);
    }
}
static void scan_publication_separates_frame_counts_from_bounded_cached_bss_records(){
    RTWUSBController c;ieee80211_node current;current.ni_chan=reinterpret_cast<void*>(64);
    c.core_.sc_ic.ic_bss=&current;
    for(unsigned channel:{1U,2U,3U,4U,5U,6U,7U,8U,9U,10U,11U})c.core_.sc_ic.ic_channels[channel].ic_flags=IEEE80211_CHAN_2GHZ;
    for(unsigned channel:{36U,40U,44U,48U,149U,153U,157U,161U,165U})c.core_.sc_ic.ic_channels[channel].ic_flags=IEEE80211_CHAN_5GHZ;
    std::vector<ieee80211_node> nodes(1030);
    for(unsigned i=0;i<nodes.size();++i){
        nodes[i].hostTreeIndex=i;nodes[i].ni_chan=reinterpret_cast<void*>(i%2?1:149);
        c.core_.sc_ic.ic_tree.nodes.push_back(&nodes[i]);
    }
    nodes[1].ni_chan=IEEE80211_CHAN_ANYC;
    uint8_t frame[36]{};frame[0]=0x80;
    rtw::scanDiagnosticsTransition(c.scanDiag_,false,rtw::ScanDiagnosticTransition::Scan,149,0);
    for(unsigned i=0;i<3;++i)rtw::scanDiagnosticsFrame(c.scanDiag_,true,frame,sizeof(frame),149,1);
    c.publishScanDiagnostics();
    CHECK(c.properties["RTWDiagScanCacheBSS24"]==511&&c.properties["RTWDiagScanCacheBSS5"]==512);
    CHECK(c.properties["RTWDiagScanCacheBSSUnknown"]==1&&c.properties["RTWDiagScanCacheBSSVisited"]==1024);
    CHECK(c.properties["RTWDiagScanCacheBSSTruncated"]==1&&c.core_.sc_ic.ic_tree.nodes.size()==1030);
    CHECK(c.properties["RTWDiagScanBeaconFrames"]==3&&c.properties["RTWDiagScanSoftware5Frames"]==3);
    CHECK(c.properties["RTWDiagScanPHY24Frames"]==3&&c.properties["RTWDiagScanPHYChannelMismatches"]==3);
    CHECK(c.properties["RTWDiagCurrentChannel"]==64);
    CHECK(c.properties["RTWDiagSupportedBandFlags"]==(RTW_CONTROL_BANDS_EXPLICIT|RTW_CONTROL_BAND_2GHZ|RTW_CONTROL_BAND_5GHZ|RTW_CONTROL_BAND_5GHZ_HIGH));
    c.attached_=false;c.publishScanDiagnostics();
    CHECK(c.properties["RTWDiagScanCacheBSSVisited"]==0&&c.properties["RTWDiagScanCacheBSSTruncated"]==0);
    CHECK(c.properties["RTWDiagCurrentChannel"]==0&&c.properties["RTWDiagSupportedBandFlags"]==0);
    CHECK(c.properties["RTWDiagScanBeaconFrames"]==3);
}
static void scan_diagnostics_do_not_claim_a_tune_with_latched_io_failure(){
    RTWUSBController c;ieee80211_node n;c.core_.sc_ic.ic_bss=&n;
    c.core_.sc_ic.ic_state=IEEE80211_S_INIT;n.ni_chan=reinterpret_cast<void*>(149);
    c.core_.core_error=true;
    // Preserve the backend's existing return/state behavior; only telemetry sees the latched error.
    CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic,IEEE80211_S_SCAN,0)==0);
    CHECK(c.core_.sc_ic.ic_state==IEEE80211_S_SCAN&&c.scanDiag_.state==rtw::ScanDiagnosticState::Failed);
    CHECK(c.scanDiag_.generation==1&&c.scanDiag_.lastChannel==149&&c.scanDiag_.lastError==EIO);
    CHECK(c.scanDiag_.hops==0&&c.scanDiag_.hopAttempts==1&&c.scanDiag_.visited[2]==0);
    c.core_.core_error=false;n.ni_chan=reinterpret_cast<void*>(36);
    CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic,IEEE80211_S_SCAN,0)==0);
    CHECK(c.scanDiag_.generation==2&&c.scanDiag_.hops==1&&c.scanDiag_.visited[0]==UINT64_C(68719476736));
}
static void ac8814_common_state_and_stop_dispatch() {
    RTWUSBController c;ieee80211_node n;
    c.family_=RTW_WIFI_RTL8814AU;c.acCore_.phy_initialized=1;
    run(c,n);
    CHECK(c.acStateCalls==1 && c.legacyStateCalls==0);
    CHECK(c.acRateReady_ && c.acMediaConnected_ && !c.authorized());
    tick(c,15100);
    CHECK(c.acStateCalls==3 && c.legacyStateCalls==0 && c.wpaRecovery_.retries==1); // RUN, INIT, SCAN.
    CHECK(c.core_.sc_ic.ic_state==IEEE80211_S_SCAN && !c.acRateReady_ && !c.acMediaConnected_);
    CHECK(c.stopCalls==0 && c.acStopCalls==0);
    for(auto operation:{RTWUSBController::Disable,RTWUSBController::Sleep,RTWUSBController::Terminated}) {
        RTWUSBController stopped;ieee80211_node station;
        stopped.family_=RTW_WIFI_RTL8814AU;stopped.acCore_.phy_initialized=1;run(stopped,station);
        CHECK(RTWUSBController::gated(&stopped,reinterpret_cast<void *>(operation),nullptr,nullptr,nullptr)==0);
        CHECK(stopped.acStopCalls==1 && stopped.stopCalls==0 && stopped.legacyStateCalls==0);
        CHECK(!stopped.running_ && !stopped.acRateReady_ && !stopped.acMediaConnected_);
        CHECK(stopped.joinRequested_==(operation!=RTWUSBController::Terminated));
        CHECK(stopped.core_.sc_ic.ic_state==IEEE80211_S_INIT);
    }
}
static void au_common_state_stop_and_port_dispatch(){
    for(auto family:{RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU}){
        RTWUSBController c;ieee80211_node n;c.family_=family;run(c,n);
        CHECK(c.auStateCalls==1&&c.acStateCalls==0&&c.legacyStateCalls==0);
        CHECK(c.auRateReady_&&c.auMediaConnected_&&!c.authorized());
        n.ni_port_valid=true;CHECK(c.authorized());c.auRateReady_=false;CHECK(!c.authorized());
        c.auRateReady_=true;n.ni_port_valid=false;tick(c,15100);
        CHECK(c.auStateCalls==3&&c.legacyStateCalls==0&&!c.auRateReady_&&!c.auMediaConnected_);
        for(auto op:{RTWUSBController::Disable,RTWUSBController::Sleep,RTWUSBController::Terminated}){
            RTWUSBController stopped;ieee80211_node station;stopped.family_=family;run(stopped,station);
            CHECK(RTWUSBController::gated(&stopped,reinterpret_cast<void*>(op),nullptr,nullptr,nullptr)==0);
            CHECK(stopped.auStopCalls==1&&stopped.acStopCalls==0&&stopped.stopCalls==0);
            CHECK(!stopped.running_&&!stopped.auRateReady_&&!station.ni_port_valid);
        }
    }
}
static void bc_common_state_stop_and_port_dispatch(){
    for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
        RTWUSBController c;ieee80211_node n;c.family_=family;run(c,n);
        CHECK(c.r88StateCalls==1&&c.auStateCalls==0&&c.legacyStateCalls==0);
        CHECK(c.r88RateReady_&&c.r88Media_&&!c.authorized());
        n.ni_port_valid=true;CHECK(c.authorized());c.r88RateReady_=false;CHECK(!c.authorized());
        c.r88RateReady_=true;n.ni_port_valid=false;tick(c,15100);
        CHECK(c.r88StateCalls==3&&c.legacyStateCalls==0&&!c.r88RateReady_&&!c.r88Media_);
        for(auto op:{RTWUSBController::Disable,RTWUSBController::Sleep,RTWUSBController::Terminated}){
            RTWUSBController stopped;ieee80211_node station;stopped.family_=family;run(stopped,station);
            CHECK(RTWUSBController::gated(&stopped,reinterpret_cast<void*>(op),nullptr,nullptr,nullptr)==0);
            CHECK(stopped.r88StopCalls==1&&stopped.auStopCalls==0&&stopped.stopCalls==0);
            CHECK(!stopped.running_&&!stopped.r88RateReady_&&!station.ni_port_valid);
        }
    }
}
static void ac8814_carrier_requires_phy_and_firmware_peer() {
    for(unsigned reason=0;reason<4;++reason) {
        RTWUSBController c;ieee80211_node n;
        c.family_=RTW_WIFI_RTL8814AU;c.acCore_.phy_initialized=1;
        run(c,n);n.ni_port_valid=true;CHECK(c.authorized());
        if(reason==0)c.acCore_.phy_initialized=0;
        if(reason==1)c.acCore_.io_error=EIO;
        if(reason==2)c.acRateReady_=false;
        if(reason==3)c.acMediaConnected_=false;
        CHECK(!c.authorized());
        CHECK(c.setLinkStatus(kIONetworkLinkValid|kIONetworkLinkActive));
        CHECK(c.lastLinkState==kIONetworkLinkValid);
        RTWControlStatus status{};c.status(&status);CHECK(!(status.flags&RTW_CONTROL_AUTHORIZED));
        CHECK(c.wpaRecovery_.state!=RTWUSBController::WPARecoveryAuthorized);
    }
    RTWUSBController c;ieee80211_node n;c.family_=RTW_WIFI_RTL8814AU;c.acCore_.phy_initialized=1;
    run(c,n);n.ni_port_valid=true;
    CHECK(c.setLinkStatus(kIONetworkLinkValid|kIONetworkLinkActive));
    CHECK(c.lastLinkState==(kIONetworkLinkValid|kIONetworkLinkActive));
    CHECK(c.authorized() && c.wpaRecovery_.state==RTWUSBController::WPARecoveryAuthorized);
}
// Compile and exercise the diagnostic fields used by the real gated output.
static IOReturn gated_output(RTWUSBController &c, RTWUSBController::OutputCall &call) {
    return RTWUSBController::gated(&c, reinterpret_cast<void *>(RTWUSBController::Output),
                                   &call, nullptr, nullptr);
}
static void unauthorized_output_counts_only_link_drops() {
    RTWUSBController c;
    ieee80211_node n;
    run(c, n);
    CHECK(!c.authorized());
    Packet packet;
    RTWUSBController::OutputCall call{&packet, false};
    CHECK(gated_output(c, call) == kIOReturnOutputDropped);
    CHECK(call.consumed);
    CHECK(packet.frees == 1);
    CHECK(c.txLinkDrops_ == 1);
    CHECK(c.txQueueDrops_ == 0);
    CHECK(c.txErrors_ == 0);
    CHECK(c.statsValue_.outputErrors == 0);
    CHECK(c.pendingQueue_.packets.empty());
}
static void null_output_does_not_count_a_packet_drop() {
    for (bool authorized : {false, true}) {
        RTWUSBController c;
        ieee80211_node n;
        run(c, n);
        n.ni_port_valid = authorized;
        RTWUSBController::OutputCall call{nullptr, false};
        CHECK(gated_output(c, call) == kIOReturnOutputDropped);
        CHECK(call.consumed);
        CHECK(c.txLinkDrops_ == 0);
        CHECK(c.txQueueDrops_ == 0);
        CHECK(c.txErrors_ == 0);
        CHECK(c.statsValue_.outputErrors == 0);
        CHECK(c.pendingQueue_.packets.empty());
    }
}
static void rejected_output_counts_queue_errors_with_optional_stats() {
    for (bool missingQueue : {false, true}) {
        for (bool haveStats : {false, true}) {
            RTWUSBController c;
            ieee80211_node n;
            run(c, n);
            n.ni_port_valid = true;
            CHECK(c.authorized());
            c.pendingQueue_.rejectEnqueue = !missingQueue;
            if (missingQueue)
                c.core_.sc_ic.ic_if.if_snd.queue = nullptr;
            if (!haveStats)
                c.stats_ = nullptr;
            Packet packet;
            RTWUSBController::OutputCall call{&packet, false};
            CHECK(gated_output(c, call) == (missingQueue ? kIOReturnOutputDropped : kIOReturnOutputStall));
            CHECK(call.consumed == missingQueue);
            CHECK(packet.frees == (missingQueue ? 1U : 0U));
            CHECK(c.txLinkDrops_ == 0);
            CHECK(c.txQueueDrops_ == (missingQueue ? 1U : 0U));
            CHECK(c.txErrors_ == (missingQueue ? 1U : 0U));
            CHECK(c.statsValue_.outputErrors == (haveStats && missingQueue ? 1U : 0U));
            CHECK(c.pendingQueue_.packets.empty());
            CHECK(c.pendingQueue_.dequeues == 0);
            if(!missingQueue)mbuf_freem(&packet); // The outer queue retains retry ownership.
        }
    }
}
static void accepted_output_preserves_counters_and_queue_ownership() {
    RTWUSBController c;
    ieee80211_node n;
    run(c, n);
    n.ni_port_valid = true;
    // Keep the packet queued, as when all hardware TX slots are occupied.
    c.core_.sc_ops.is_oactive = [](void *) { return 1; };
    Packet packet;
    RTWUSBController::OutputCall call{&packet, false};
    CHECK(gated_output(c, call) == kIOReturnOutputSuccess);
    CHECK(call.consumed);
    CHECK(packet.frees == 0);
    CHECK(c.pendingQueue_.packets.size() == 1);
    CHECK(c.pendingQueue_.packets.front() == &packet);
    CHECK(c.txLinkDrops_ == 0);
    CHECK(c.txQueueDrops_ == 0);
    CHECK(c.txErrors_ == 0);
    CHECK(c.statsValue_.outputErrors == 0);
    ifq_flush(&c.core_.sc_ic.ic_if.if_snd);
    CHECK(packet.frees == 1);
    CHECK(c.pendingQueue_.packets.empty());
}
static void closing_output_frees_packet_without_queueing() {
    RTWUSBController c;
    c.closing_ = true;
    Packet packet;
    RTWUSBController::OutputCall call{&packet, false};
    CHECK(gated_output(c, call) == kIOReturnOutputDropped);
    CHECK(call.consumed);
    CHECK(packet.frees == 1);
    CHECK(c.pendingQueue_.packets.empty());
    CHECK(c.txLinkDrops_ == 0);
    CHECK(c.txQueueDrops_ == 0);
    CHECK(c.txErrors_ == 0);
    CHECK(c.statsValue_.outputErrors == 0);
}
static void r23_common_state_stop_and_port_dispatch(){
    RTWUSBController c;ieee80211_node n;c.family_=RTW_WIFI_RTL8723BU;run(c,n);
    CHECK(c.r23StateCalls==1&&c.legacyStateCalls==0);
    CHECK(c.r23RateReady_&&c.r23Media_&&!c.authorized());
    n.ni_port_valid=true;CHECK(c.authorized());c.r23RateReady_=false;CHECK(!c.authorized());
    c.r23RateReady_=true;n.ni_port_valid=false;tick(c,15100);
    CHECK(c.r23StateCalls==3&&c.legacyStateCalls==0&&!c.r23RateReady_&&!c.r23Media_);
    for(auto op:{RTWUSBController::Disable,RTWUSBController::Sleep,RTWUSBController::Terminated}){
        RTWUSBController stopped;ieee80211_node station;stopped.family_=RTW_WIFI_RTL8723BU;run(stopped,station);
        CHECK(RTWUSBController::gated(&stopped,reinterpret_cast<void*>(op),nullptr,nullptr,nullptr)==0);
        CHECK(stopped.r23StopCalls==1&&stopped.stopCalls==0&&!station.ni_port_valid);
    }
}
static void bu_common_state_stop_and_port_dispatch(){
    RTWUSBController c;ieee80211_node n;c.family_=RTW_WIFI_RTL8710BU;run(c,n);
    CHECK(c.buStateCalls==1&&c.legacyStateCalls==0);
    CHECK(c.buRateReady_&&c.buMedia_&&!c.authorized());
    n.ni_port_valid=true;CHECK(c.authorized());c.buRateReady_=false;CHECK(!c.authorized());
    c.buRateReady_=true;n.ni_port_valid=false;tick(c,15100);
    CHECK(c.buStateCalls==3&&c.legacyStateCalls==0&&!c.buRateReady_&&!c.buMedia_);
    for(auto op:{RTWUSBController::Disable,RTWUSBController::Sleep,RTWUSBController::Terminated}){
        RTWUSBController stopped;ieee80211_node station;stopped.family_=RTW_WIFI_RTL8710BU;run(stopped,station);
        CHECK(RTWUSBController::gated(&stopped,reinterpret_cast<void*>(op),nullptr,nullptr,nullptr)==0);
        CHECK(stopped.buStopCalls==1&&stopped.stopCalls==0&&!station.ni_port_valid);
    }
}
static void fu_common_state_stop_and_port_dispatch(){
    RTWUSBController c;ieee80211_node n;c.family_=RTW_WIFI_RTL8192FU;run(c,n);
    CHECK(c.fuStateCalls==1&&c.legacyStateCalls==0);
    CHECK(c.fuRateReady_&&c.fuMedia_&&!c.authorized());
    n.ni_port_valid=true;CHECK(c.authorized());c.fuRateReady_=false;CHECK(!c.authorized());
    c.fuRateReady_=true;n.ni_port_valid=false;tick(c,15100);
    CHECK(c.fuStateCalls==3&&c.legacyStateCalls==0&&!c.fuRateReady_&&!c.fuMedia_);
    for(auto op:{RTWUSBController::Disable,RTWUSBController::Sleep,RTWUSBController::Terminated}){
        RTWUSBController stopped;ieee80211_node station;stopped.family_=RTW_WIFI_RTL8192FU;run(stopped,station);
        CHECK(RTWUSBController::gated(&stopped,reinterpret_cast<void*>(op),nullptr,nullptr,nullptr)==0);
        CHECK(stopped.fuStopCalls==1&&stopped.stopCalls==0&&!station.ni_port_valid);
    }
}
static void du_common_state_stop_and_port_dispatch(){
    RTWUSBController c;ieee80211_node n;c.family_=RTW_WIFI_RTL8192DU;run(c,n);
    CHECK(c.duStateCalls==1&&c.legacyStateCalls==0);
    CHECK(c.duRateReady_&&c.duMedia_&&!c.authorized());
    n.ni_port_valid=true;CHECK(c.authorized());c.duRateReady_=false;CHECK(!c.authorized());
    c.duRateReady_=true;n.ni_port_valid=false;tick(c,15100);
    CHECK(c.duStateCalls==3&&c.legacyStateCalls==0&&!c.duRateReady_&&!c.duMedia_);
    for(auto op:{RTWUSBController::Disable,RTWUSBController::Sleep,RTWUSBController::Terminated}){
        RTWUSBController stopped;ieee80211_node station;stopped.family_=RTW_WIFI_RTL8192DU;run(stopped,station);
        CHECK(RTWUSBController::gated(&stopped,reinterpret_cast<void*>(op),nullptr,nullptr,nullptr)==0);
        CHECK(stopped.duStopCalls==1&&stopped.stopCalls==0&&!station.ni_port_valid);
    }
}
static void live_registered_band_groups(){
    const uint32_t low=RTW_CONTROL_BAND_5GHZ,high=RTW_CONTROL_BAND_5GHZ_HIGH,groups=low|high;
    const uint8_t lo[]={36,40,44,48},hi[]={149,153,157,161,165};
    for(auto family:{RTW_WIFI_RTL8192DU,RTW_WIFI_RTL8814AU,RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU,RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
        RTWUSBController c;c.family_=family;RTWControlStatus status{};
        c.status(&status);CHECK(!(status.flags&groups));
        for(uint32_t flags:{low,high,groups}){
            for(auto ch:lo)c.core_.sc_ic.ic_channels[ch].ic_flags=flags&low?IEEE80211_CHAN_5GHZ:0;
            for(auto ch:hi)c.core_.sc_ic.ic_channels[ch].ic_flags=flags&high?IEEE80211_CHAN_5GHZ:0;
            c.status(&status);CHECK((status.flags&groups)==flags);
        }
        for(auto ch:lo){c.core_.sc_ic.ic_channels[ch].ic_flags=0;c.status(&status);CHECK((status.flags&groups)==high);c.core_.sc_ic.ic_channels[ch].ic_flags=IEEE80211_CHAN_5GHZ;}
        for(auto ch:hi){c.core_.sc_ic.ic_channels[ch].ic_flags=0;c.status(&status);CHECK((status.flags&groups)==low);c.core_.sc_ic.ic_channels[ch].ic_flags=IEEE80211_CHAN_5GHZ;}
        c.attached_=false;c.status(&status);CHECK(!(status.flags&groups));
    }
}
int main(int argc,char **argv) {
    const std::string selected=argc>1?argv[1]:"all";
    if(selected=="all"||selected=="pm-profile")pm_cycle_preserves_configured_profile();
    if(selected=="all"||selected=="pm-manual")manual_radio_off_survives_pm_cycles();
    if(selected=="all"||selected=="pm-admission")explicit_radio_resume_requires_valid_admitted_command();
    if(selected=="all"||selected=="pm-isolation")pm_intent_is_per_adapter_and_teardown_wins();
    if(selected!="all") {
        std::printf("Controller %s: %u checks passed on actual gate/control/quiesce/enable methods\n",selected.c_str(),checks);
        return 0;
    }
    r23_common_state_stop_and_port_dispatch();
    bu_common_state_stop_and_port_dispatch();
    fu_common_state_stop_and_port_dispatch();
    du_common_state_stop_and_port_dispatch();
    live_registered_band_groups();
    unauthorized_output_counts_only_link_drops();
    null_output_does_not_count_a_packet_drop();
    rejected_output_counts_queue_errors_with_optional_stats();
    accepted_output_preserves_counters_and_queue_ownership();
    closing_output_frees_packet_without_queueing();
    bc_common_state_stop_and_port_dispatch();
    au_common_state_stop_and_port_dispatch();
    ac8814_common_state_and_stop_dispatch();
    ac8814_carrier_requires_phy_and_firmware_peer();
    auth_assoc_loops_keep_original_deadline();
    manual_scan_after_failure_does_not_restart_join();
    manual_scan_waits_for_rx_recovery_without_changing_hardware_state();
    scan_diagnostics_follow_only_successful_backend_transitions();
    scan_publication_separates_frame_counts_from_bounded_cached_bss_records();
    scan_diagnostics_do_not_claim_a_tune_with_latched_io_failure();
    link_authorization_cancels_before_next_watchdog();
    status_polling_does_not_extend_deadline();
    missing_m1_retries_without_usb_restart();
    stalled_m2_uses_same_deadline();
    retries_exhaust_and_stay_terminal();
    retry_run_gets_one_handshake_window();
    authorization_cancels_deadline();
    erased_profile_never_retries();
    invalid_profile_and_lifecycle_cancel();
    lifecycle_stops_cancel_wpa_without_erasing_pm_intent();
    manual_join_resets_exhausted_campaign();
    clock_wrap_and_backwards_are_bounded();
    dequeued_tx_is_not_owned_by_recovery_queue();
    std::printf("WPA recovery: %u checks passed on real controller methods\n", checks);
    return 0;
}
