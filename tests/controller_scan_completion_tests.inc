// SPDX-License-Identifier: GPL-2.0-only
static void browseCompletesForEveryFamilyWithoutCandidates() {
    const rtw_wifi_family families[]={RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8188EU,RTW_WIFI_RTL8192EU,
        RTW_WIFI_RTL8814AU,RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU,RTW_WIFI_RTL8192DU,
        RTW_WIFI_RTL8192FU,RTW_WIFI_RTL8710BU,RTW_WIFI_RTL8723BU,
        RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU};
    for(auto family:families)for(bool incompatible:{false,true}) {
        const bool dual=family==RTW_WIFI_RTL8814AU||family==RTW_WIFI_RTL8812AU||family==RTW_WIFI_RTL8821AU||
            family==RTW_WIFI_RTL8192DU||family==RTW_WIFI_RTL8821CU||family==RTW_WIFI_RTL8822BU||family==RTW_WIFI_RTL8822CU;
        Fixture f(dual?IEEE80211_C_SCANALLBAND:0,dual);
        RTWUSBController c(f.ic,&f.pending);c.family_=family;
        f.observeTwo=f.observeFive=incompatible;f.rejectCandidates=incompatible;
        f.start();RTWControlStatus status{};c.status(&status);
        require(status.flags&RTW_CONTROL_SCANNING,"STATUS reports an active browse scan");
        f.finish();c.status(&status);
        require(!(status.flags&RTW_CONTROL_SCANNING),"STATUS clears SCANNING at the end of a browse sweep");
        require(c.browseScanComplete_&&c.scanDiag_.state==rtw::ScanDiagnosticState::Complete,"controller commits complete diagnostic state");
        require(f.visits==(dual?std::vector<unsigned>{1,6,11,36,149}:std::vector<unsigned>{1,6,11}),"browse scans every enabled active/passive channel once");
        require(f.completions==1&&f.joins==0&&!f.pending,"one completion stops timer hopping without auto-association");
        require(f.probes==std::vector<unsigned>{1,6,11},"browse emits no passive-channel probe");
        if(incompatible)require(RB_FIND(ieee80211_tree,&f.ic.ic_tree,&f.two)!=nullptr,"incompatible cached BSS remains available to userspace");
    }
}
static void repeatBrowseAndSubsequentJoinPreserveTheCache() {
    Fixture f(IEEE80211_C_SCANALLBAND);RTWUSBController c(f.ic,&f.pending);
    f.start();f.finish();require(c.scanDiag_.generation==1,"first browse has one generation");
    require(RB_FIND(ieee80211_tree,&f.ic.ic_tree,&f.two)&&RB_FIND(ieee80211_tree,&f.ic.ic_tree,&f.five),"completion preserves both bands in cache");
    f.visits.clear();f.probes.clear();f.start();
    require(!c.browseScanComplete_&&c.scanDiag_.generation==2,"new SCAN resets one-shot completion");
    f.finish();require(f.completions==2&&f.visits==std::vector<unsigned>({1,6,11,36,149}),"repeat SCAN completes one new sweep");
    c.joinRequested_=true;f.start();f.finish();
    require(!c.browseScanComplete_&&f.joins==1&&f.joinedChannel==149,"subsequent JOIN ignores old browse completion and chooses a cached AP");
}
static void joinKeepsRetryingWhenItsAPIsAbsent() {
    Fixture f(IEEE80211_C_SCANALLBAND);RTWUSBController c(f.ic,&f.pending);
    f.observeTwo=f.observeFive=false;c.joinRequested_=true;f.start();
    for(unsigned i=0;i<30;++i){require(f.pending,"JOIN keeps its existing retry timer");f.pending=false;ieee80211_next_scan(&f.ic.ic_if);}
    require(f.completions>=3&&!c.browseScanComplete_&&f.joins==0,"one-shot browse hook never consumes JOIN retries");
    f.pending=false;
}
static void dfsOnlyBrowseCompletesAfterEveryPassiveChannel() {
    Fixture f(IEEE80211_C_SCANALLBAND);RTWUSBController c(f.ic,&f.pending);
    std::memset(f.ic.ic_chan_active,0,sizeof(f.ic.ic_chan_active));
    for(unsigned ch:{52U,56U,60U,64U,100U,104U,108U,112U,116U,120U,124U,128U,132U,136U,140U}){
        f.ic.ic_channels[ch].ic_flags=IEEE80211_CHAN_5GHZ|IEEE80211_CHAN_PASSIVE;setbit(f.ic.ic_chan_active,ch);
    }
    f.observeTwo=f.observeFive=false;f.start();f.finish();
    require(f.visits==std::vector<unsigned>({52,56,60,64,100,104,108,112,116,120,124,128,132,136,140}),"DFS-only browse observes all registered passive channels once");
    require(f.probes.empty()&&f.completions==1&&c.browseScanComplete_,"DFS sweep finishes without directed/active probes");
}
static void completionHookDoesNotHandleBackgroundOrCancelledOwners() {
    Fixture f(IEEE80211_C_SCANALLBAND);RTWUSBController c(f.ic,&f.pending);
    require(f.ic.ic_scan_done!=nullptr,"actual initializePlatform wires completion hook");
    f.ic.ic_state=IEEE80211_S_RUN;f.ic.ic_flags=IEEE80211_F_ASCAN|IEEE80211_F_BGSCAN|IEEE80211_F_DISABLE_BG_AUTO_CONNECT;
    f.cache(f.two);ieee80211_end_scan(&f.ic.ic_if);
    require(!c.browseScanComplete_&&f.completions==1&&f.ic.ic_state==IEEE80211_S_RUN,"background completion preserves existing event and RUN state");
    f.ic.ic_flags=0;f.ic.ic_state=IEEE80211_S_SCAN;
    for(unsigned guard=0;guard<12;++guard){
        c.attached_=c.running_=c.present_=true;c.sleeping_=c.stopping_=c.closing_=c.joinRequested_=false;
        c.usbError_=0;c.core_.core_error=0;
        c.scanDiag_.state=rtw::ScanDiagnosticState::Active;c.scanDiag_.lastError=0;
        if(guard==0)c.attached_=false;
        if(guard==1)c.running_=false;
        if(guard==2)c.present_=false;
        if(guard==3)c.sleeping_=true;
        if(guard==4)c.stopping_=true;
        if(guard==5)c.closing_=true;
        if(guard==6)c.joinRequested_=true;
        if(guard==7)c.usbError_=5;
        if(guard==8)c.core_.core_error=5;
        if(guard==9)c.scanDiag_.state=rtw::ScanDiagnosticState::Failed;
        if(guard==10)c.scanDiag_.lastError=5;
        if(guard==11)c.scanDiag_.state=rtw::ScanDiagnosticState::Cancelled;
        const auto publications=c.publications,cancelled=c.timer.cancelled;
        require(f.ic.ic_scan_done(&f.ic)==0,"late/sleep/stop/JOIN callback stays unconsumed");
        require(!c.browseScanComplete_&&c.publications==publications&&c.timer.cancelled==cancelled,"rejected callback does not publish completion or touch a timer");
        if(guard==10)require(c.scanDiag_.lastError==5,"completion cannot clear the failed-hop error");
    }
}
int main() {
    browseCompletesForEveryFamilyWithoutCandidates();
    repeatBrowseAndSubsequentJoinPreserveTheCache();joinKeepsRetryingWhenItsAPIsAbsent();
    dfsOnlyBrowseCompletesAfterEveryPassiveChannel();completionHookDoesNotHandleBackgroundOrCancelledOwners();
    std::printf("Controller finite browse scan: %u checks passed (13 families, actual net80211/controller, empty/incompatible, passive/DFS, repeat/cache, JOIN, cancellation)\n",checks);
}
