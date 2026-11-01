// SPDX-License-Identifier: GPL-2.0-only
#include "net80211_scan_host.hpp"

static unsigned checks;
static void require(bool condition,const char *description) {
    ++checks;
    if(!condition){std::fprintf(stderr,"FAIL: %s\n",description);std::exit(1);}
}
struct Fixture {
    ieee80211com ic{};
    ieee80211_node bss{},two{},five{};
    scan_media_entry media{};
    std::vector<unsigned> visits,probes;
    unsigned completions=0,cleanup=0,joins=0,joinedChannel=0;
    bool observeTwo=true,observeFive=true,pending=false;
    Fixture(unsigned capabilities,bool withFive=true):ic{} {
        ic.ic_caps=capabilities;ic.ic_bss=&bss;ic.ic_media.ifm_cur=&media;ic.test_context=this;
        bss.ni_chan=IEEE80211_CHAN_ANYC;RB_INIT(&ic.ic_tree);
        for(unsigned ch:{1U,6U,11U}){ic.ic_channels[ch].ic_flags=IEEE80211_CHAN_2GHZ;setbit(ic.ic_chan_active,ch);}
        if(withFive)for(unsigned ch:{36U,149U}){ic.ic_channels[ch].ic_flags=IEEE80211_CHAN_5GHZ|IEEE80211_CHAN_PASSIVE;setbit(ic.ic_chan_active,ch);}
        two.ni_chan=&ic.ic_channels[6];two.ni_macaddr[5]=1;two.ni_rssi=85;
        five.ni_chan=&ic.ic_channels[149];five.ni_macaddr[5]=2;five.ni_rssi=55;
        ic.ic_node_checkrssi=ieee80211_node_checkrssi;
        ic.ic_event_handler=[](ieee80211com *i,int event,void *){
            require(event==IEEE80211_EVT_SCAN_DONE,"completion event type");
            ++static_cast<Fixture *>(i->test_context)->completions;
        };
    }
    void cache(ieee80211_node &node){if(!ieee80211_tree_RB_FIND(&ic.ic_tree,&node))RB_INSERT(ieee80211_tree,&ic.ic_tree,&node);}
    void start(){ieee80211_begin_scan(&ic.ic_if);}
    void finish(){
        unsigned steps=0;
        while(pending&&steps++<40){pending=false;ieee80211_next_scan(&ic.ic_if);}
        require(steps<40,"scan finishes without revisiting channels indefinitely");
    }
};
static Fixture &fixture(ieee80211com *ic){return *static_cast<Fixture *>(ic->test_context);}
int ieee80211_node_cmp(ieee80211_node *a,ieee80211_node *b){return std::memcmp(a->ni_macaddr,b->ni_macaddr,6);}
RB_GENERATE(ieee80211_tree,ieee80211_node,ni_node,ieee80211_node_cmp)
unsigned ieee80211_chan2ieee(ieee80211com *ic,const ieee80211_channel *ch){return unsigned(ch-ic->ic_channels);}
void ieee80211_node_cleanup(ieee80211com *,ieee80211_node *){}
void ieee80211_node_raise_inact(void *,ieee80211_node *n){++n->ni_inact;}
void ieee80211_iterate_nodes(ieee80211com *ic,void (*callback)(void *,ieee80211_node *),void *arg){
    ieee80211_node *n;RB_FOREACH(n,ieee80211_tree,&ic->ic_tree)callback(arg,n);
}
void ieee80211_clean_inactive_nodes(ieee80211com *ic,int){++fixture(ic).cleanup;}
void ieee80211_free_node(ieee80211com *ic,ieee80211_node *n){RB_REMOVE(ieee80211_tree,&ic->ic_tree,n);}
// Fixtures are already parsed, compatible APs for the requested ESS. Keep
// the production active-channel eligibility at this unrelated RSN boundary.
int ieee80211_match_bss(ieee80211com *ic,ieee80211_node *n,int){return isclr(ic->ic_chan_active,ieee80211_chan2ieee(ic,n->ni_chan));}
void ieee80211_switch_ess(ieee80211com *){}
void ieee80211_node_join_bss(ieee80211com *ic,ieee80211_node *n){
    auto &f=fixture(ic);++f.joins;f.joinedChannel=ieee80211_chan2ieee(ic,n->ni_chan);ic->ic_state=IEEE80211_S_AUTH;f.pending=false;
}
void ieee80211_node_switch_bss(ieee80211com *,ieee80211_node *){}
void ieee80211_stop_ampdu_tx(ieee80211com *,ieee80211_node *,int){}
int scan_send_mgmt(ieee80211com *,ieee80211_node *,int,int){return 0;}
void ieee80211_new_state(ieee80211com *ic,ieee80211_state state,int){
    auto &f=fixture(ic);require(state==IEEE80211_S_SCAN,"channel hop stays in scan");
    ic->ic_state=state;const unsigned channel=ieee80211_chan2ieee(ic,ic->ic_bss->ni_chan);
    f.visits.push_back(channel);f.pending=true;
    if(ic->ic_flags&IEEE80211_F_ASCAN)f.probes.push_back(channel);
    if(channel==6&&f.observeTwo)f.cache(f.two);
    if(channel==ieee80211_chan2ieee(ic,f.five.ni_chan)&&f.observeFive)f.cache(f.five);
}
// Mode changes and timer/channel I/O are outside this closure. The real
// begin/reset/next/end and choose_bss functions own every scan bitmap hop.
int ieee80211_setmode(ieee80211com *ic,ieee80211_phymode mode){ic->ic_curmode=mode;ieee80211_reset_scan(&ic->ic_if);return 0;}
ieee80211_phymode ieee80211_next_mode(_ifnet *ifp){auto *ic=reinterpret_cast<ieee80211com *>(ifp);ieee80211_setmode(ic,IEEE80211_MODE_AUTO);return IEEE80211_MODE_AUTO;}
ieee80211_phymode ieee80211_chan2mode(ieee80211com *,ieee80211_channel *){return IEEE80211_MODE_AUTO;}

static void complete_dual_band(unsigned capabilities){
    Fixture f(capabilities);f.start();f.finish();
    require(f.joinedChannel==149,"5 GHz observed after 2.4 GHz must participate in the first BSS choice");
    require(f.visits==std::vector<unsigned>({1,6,11,36,149}),"complete scan visits each enabled channel once");
    require(f.probes==std::vector<unsigned>({1,6,11}),"passive 5 GHz channels emit no probe");
    require(f.completions==1&&f.cleanup==1&&f.joins==1,"complete scan emits one completion and chooses once");
    require(RB_FIND(ieee80211_tree,&f.ic.ic_tree,&f.two)&&RB_FIND(ieee80211_tree,&f.ic.ic_tree,&f.five),"passive phase preserves both cached APs");
}
static void legacy_24(){
    Fixture f(0,false);f.observeFive=false;f.start();f.finish();
    require(f.visits==std::vector<unsigned>({1,6,11}),"legacy 2.4 GHz scanner keeps its existing channel sequence");
    require(f.joinedChannel==6&&f.completions==1&&f.cleanup==1,"legacy scanner concludes normally");
}
static void allband_24_only(){
    Fixture f(IEEE80211_C_SCANALLBAND,false);f.start();f.finish();
    require(f.visits==std::vector<unsigned>({1,6,11})&&f.joins==1&&f.completions==1,"all-band scan with no passive channels concludes without another round");
}
static void absent_five(){
    Fixture f(IEEE80211_C_SCANALLBAND);f.observeFive=false;f.start();f.finish();
    require(f.visits==std::vector<unsigned>({1,6,11,36,149}),"missing 5 GHz AP still gets one complete observation");
    require(f.joinedChannel==6&&f.completions==1,"available 2.4 GHz AP is chosen after the complete scan");
}
static void passive_only(){
    Fixture f(IEEE80211_C_SCANALLBAND);f.observeTwo=false;
    for(unsigned ch:{1U,6U,11U})clrbit(f.ic.ic_chan_active,ch);
    f.start();f.finish();
    require(f.visits==std::vector<unsigned>({36,149})&&f.probes.empty(),"passive-only enabled channels are scanned without an active round");
    require(f.joinedChannel==149&&f.completions==1,"passive-only scan concludes once");
}
static void cached_user_scan(){
    Fixture f(IEEE80211_C_SCANALLBAND);f.cache(f.two);f.cache(f.five);
    f.bss.ni_chan=&f.ic.ic_channels[149];f.start();f.finish();
    require(f.visits==std::vector<unsigned>({1,6,11,36,149}),"explicit user scan observes passive channels even with cached candidate and previous 5 GHz channel");
    require(f.joinedChannel==149&&f.completions==1,"cached candidate does not cause premature scan completion");
}
static void disabled_passive(){
    Fixture f(IEEE80211_C_SCANALLBAND);f.cache(f.two);
    f.bss.ni_chan=&f.ic.ic_channels[11];f.ic.ic_flags|=IEEE80211_F_ASCAN;
    setbit(f.ic.ic_chan_scan,36);clrbit(f.ic.ic_chan_active,36);
    ieee80211_end_scan(&f.ic.ic_if);f.finish();
    require(f.visits.empty()&&f.joinedChannel==6&&f.completions==1,"disabled passive residual bit never authorizes a channel hop");
}
static void background_cache(){
    Fixture f(IEEE80211_C_SCANALLBAND);f.cache(f.two);
    f.bss.ni_chan=&f.ic.ic_channels[6];f.ic.ic_state=IEEE80211_S_RUN;
    f.ic.ic_flags=IEEE80211_F_ASCAN|IEEE80211_F_BGSCAN|IEEE80211_F_DISABLE_BG_AUTO_CONNECT;
    setbit(f.ic.ic_chan_scan,36);
    ieee80211_end_scan(&f.ic.ic_if);
    require(f.visits.empty()&&f.joins==0&&f.ic.ic_state==IEEE80211_S_RUN,"background cache completion never starts a foreground passive phase");
    require(f.completions==1&&isset(f.ic.ic_chan_scan,36),"background completion preserves unrelated scan bitmap");
}
struct Family { const char *name;void (*registration)(ieee80211com &); };
static const Family families[]={
    {"8192du",register_8192du_scan_channels},
    {"8812au",register_8812_scan_channels},
    {"8814au",register_8814_scan_channels},
    {"8821au",register_8812_scan_channels},
    {"8821cu",register_88_scan_channels},
    {"8822bu",register_88_scan_channels},
    {"8822cu",register_88_scan_channels}
};
enum ScanProfile { AllBands,FiveOnly,LowerFiveOnly,UpperFiveOnly,TwoOnly,DFSOnly };
static void family_scan(const Family &family,ScanProfile profile){
    Fixture f(0,false);
    for(auto &channel:f.ic.ic_channels)channel={};
    std::memset(f.ic.ic_chan_active,0,sizeof(f.ic.ic_chan_active));
    family.registration(f.ic);
    const std::vector<unsigned> allowed5={36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165};
    std::vector<unsigned> expectedVisits,expectedProbes;
    for(unsigned ch=0;ch<=IEEE80211_CHAN_MAX;++ch){
        const bool two=ch>=1&&ch<=11;
        const bool five=std::find(allowed5.begin(),allowed5.end(),ch)!=allowed5.end();
        const auto flags=f.ic.ic_channels[ch].ic_flags;
        require(bool(flags)==(two||five),"actual backend registration remains limited to 1-11 and24 qualified5GHz centers");
        if(!flags)continue;
        require(bool(flags&IEEE80211_CHAN_PASSIVE)==five,"actual backend registration marks every 5 GHz channel passive and keeps 2.4 GHz active");
        const bool active=profile==AllBands||(profile==FiveOnly&&five)||
            (profile==LowerFiveOnly&&five&&ch<100)||(profile==UpperFiveOnly&&five&&ch>100)||
            (profile==TwoOnly&&two)||(profile==DFSOnly&&five&&ch>=52&&ch<=140);
        if(active){setbit(f.ic.ic_chan_active,ch);expectedVisits.push_back(ch);if(two)expectedProbes.push_back(ch);}
    }
    f.observeTwo=profile==AllBands||profile==TwoOnly;
    if(profile==LowerFiveOnly)f.five.ni_chan=&f.ic.ic_channels[36];
    if(profile==DFSOnly)f.five.ni_chan=&f.ic.ic_channels[100];
    f.start();f.finish();
    require(f.visits==expectedVisits,"family scan visits all and only enabled channels before choosing its AP");
    require(f.probes==expectedProbes,"family scan keeps passive 5 GHz channels out of the probe path");
    const unsigned expectedJoin=profile==TwoOnly?6:profile==LowerFiveOnly?36:profile==DFSOnly?100:149;
    require(f.joinedChannel==expectedJoin,"family scan chooses a 5 GHz candidate first observed after the 2.4 GHz round");
    require(f.completions==1&&f.joins==1&&f.cleanup==1,"family profile completes and selects once without repeating scan rounds");
}
int main(int argc,char **argv){
    const char *selected=argc>1?argv[1]:"all";
    if(!std::strcmp(selected,"all")||!std::strcmp(selected,"au"))complete_dual_band(au_scan_capabilities());
    if(!std::strcmp(selected,"all")||!std::strcmp(selected,"dual"))complete_dual_band(IEEE80211_C_SCANALLBAND);
    if(!std::strcmp(selected,"all")){legacy_24();allband_24_only();absent_five();passive_only();cached_user_scan();disabled_passive();background_cache();}
    for(const auto &family:families)if(!std::strcmp(selected,"all")||!std::strcmp(selected,"families")||!std::strcmp(selected,family.name)){
        for(auto profile:{AllBands,FiveOnly,LowerFiveOnly,UpperFiveOnly,TwoOnly,DFSOnly})family_scan(family,profile);
        std::printf("modeled scan profiles: %s (both bands, 5-only, lower/upper5-only,2.4-only,DFS-only)\n",family.name);
    }
    std::printf("net80211 scan: %u checks passed (actual scan/BSS choice; no USB)\n",checks);
}
