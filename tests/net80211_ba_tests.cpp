/* Actual net80211 BA/BAR/ADDBA production code; only the OS/next-MAC boundary
 * is supplied here. Every reordered protected MPDU enters real software CCMP. */
#include "net80211_ba_host.hpp"

unsigned rtw_ht_invalid_setdata,rtw_ht_fail_allocations,rtw_ht_live_timers,rtw_ht_live_mbufs;
static unsigned checks,fail_reorder_allocations;
static std::vector<void *> allocations;
static void require(bool ok,const char *what) { ++checks; if(!ok) { fprintf(stderr,"FAIL: %s\n",what); exit(1); } }
void *rtw_ht_allocate(size_t n) {
    if (n==64*sizeof(ieee80211_ba_buf) && fail_reorder_allocations) { --fail_reorder_allocations; return nullptr; }
    void *p=calloc(1,n); if(p) { allocations.push_back(p); if(n==64*sizeof(ieee80211_ba_buf)) memset(p,0xa5,n); } return p;
}
void rtw_ht_deallocate(void *p) { auto it=std::find(allocations.begin(),allocations.end(),p); if(it!=allocations.end()) allocations.erase(it); __builtin_free(p); }
void ieee80211_rx_ba_timeout(void *) {}
static void noop(void *) {}
struct Harness {
    ieee80211com ic={}; ieee80211_node ni={}; RTWHTNetStats stats={};
    ieee80211_key tx={},rx={};
    std::vector<unsigned> delivered;
    unsigned tx_starts=0,rx_starts=0,actions=0,last_action=0,last_action_arg=0;
    unsigned expected_tx_window=16;
    int tx_result=0,rx_result=0;
    bool validate_tx_params=false;
    Harness(); ~Harness();
};
static Harness *active;
static int management(ieee80211com *ic,ieee80211_node *ni,int type,int action,int arg) {
    require(ic==&active->ic && ni==&active->ni && type==IEEE80211_FC0_SUBTYPE_ACTION,"BA response uses correct interface/peer/action frame");
    ++active->actions; active->last_action=action; active->last_action_arg=arg; return 0;
}
static int tx_start(ieee80211com *,ieee80211_node *ni,uint8_t tid) {
    ++active->tx_starts;
    if(active->validate_tx_params) {
    require(ni->ni_tx_ba[tid].ba_winsize==active->expected_tx_window,"TX start observes negotiated peer window before hardware programming");
    require(ni->ni_tx_ba[tid].ba_timeout_val==5*1024,"TX start observes negotiated inactivity timeout");
    }
    return active->tx_result;
}
static int rx_start(ieee80211com *,ieee80211_node *ni,uint8_t tid) {
    ++active->rx_starts;
    auto &ba=ni->ni_rx_ba[tid];
    require(ba.ba_buf!=nullptr,"RX start receives owned reorder allocation");
    for(unsigned i=0;i<64;++i) {
        require(ba.ba_buf[i].m==nullptr,"RX ADDBA zeroes every uninitialized reorder slot");
        const uint8_t *rxi=(const uint8_t *)&ba.ba_buf[i].rxi;
        for(unsigned b=0;b<sizeof(ieee80211_rxinfo);++b) require(rxi[b]==0,"RX ADDBA zeroes every slot metadata byte");
    }
    return active->rx_result;
}
Harness::Harness() {
    active=this; ic.ic_bss=&ni; ic.ic_opmode=IEEE80211_M_STA; ic.ic_state=IEEE80211_S_RUN;
    ic.ic_if.netStat=&stats; ic.ic_send_mgmt=management; ic.ic_ampdu_tx_start=tx_start; ic.ic_ampdu_rx_start=rx_start;
    ni.ni_ic=&ic; ni.ni_flags=IEEE80211_NODE_HT|IEEE80211_NODE_QOS; ni.ni_port_valid=1;
    for(unsigned i=0;i<16;++i) tx.k_key[i]=rx.k_key[i]=i;
    require(!ieee80211_ccmp_set_key(&ic,&tx) && !ieee80211_ccmp_set_key(&ic,&rx),"setup real software CCMP contexts");
}
Harness::~Harness() {
    ieee80211_ba_free(&ni); ieee80211_ccmp_delete_key(&ic,&tx); ieee80211_ccmp_delete_key(&ic,&rx);
    require(allocations.empty() && !rtw_ht_live_mbufs && !rtw_ht_live_timers,"BA fixture releases every timer, encrypted MPDU and allocation");
}
void ieee80211_inputm(_ifnet *ifp,mbuf_t m,ieee80211_node *ni,ieee80211_rxinfo *rxi,mbuf_list *ml) {
    require(ifp==&active->ic.ic_if && ni==&active->ni,"reorder reinjects into correct next MAC stage");
    require(rxi->rxi_flags&IEEE80211_RXI_AMPDU_DONE,"reorder marks reinjection to avoid recursive BA processing");
    require(mtod(m,ieee80211_frame *)->i_fc[1]&IEEE80211_FC1_PROTECTED,"encrypted MPDU remains protected until after reordering");
    mbuf_t decrypted=ieee80211_ccmp_decrypt(&active->ic,m,&active->rx);
    require(decrypted!=nullptr,"ordered PN passes actual CCMP MIC and replay validation");
    ml->packets.push_back(decrypted);
}
int if_input(_ifnet *ifp,mbuf_list *ml) {
    require(ifp==&active->ic.ic_if,"reordered frames reach correct interface handoff");
    for(auto *m:ml->packets) { active->delivered.push_back(LE_READ_2(mtod(m,ieee80211_frame *)->i_seq)>>4); mbuf_freem(m); }
    ml->packets.clear(); return 0;
}
static mbuf_t packet(size_t length) {
    mbuf_t m=nullptr; require(!mbuf_gethdr(0,0,&m),"allocate local wire fixture"); m->length=m->packet_length=length; return m;
}
static mbuf_t encrypted(Harness &h,unsigned tid,unsigned seq,uint64_t pn) {
    auto *m=packet(30); auto *wh=mtod(m,ieee80211_qosframe *);
    wh->i_fc[0]=0x88; wh->i_fc[1]=0x42;
    const uint8_t sta[]={0x66,0x77,0x88,0x99,0xaa,0xbb},ap[]={0,0x11,0x22,0x33,0x44,0x55};
    memcpy(wh->i_addr1,sta,6); memcpy(wh->i_addr2,ap,6); memcpy(wh->i_addr3,ap,6);
    LE_WRITE_2(wh->i_seq,seq<<4); LE_WRITE_2(wh->i_qos,tid);
    memset((uint8_t *)mbuf_data(m)+26,0x5a,4); h.tx.k_tsc=pn-1;
    auto *result=ieee80211_ccmp_encrypt(&h.ic,m,&h.tx); require(result!=nullptr,"construct real encrypted MPDU"); return result;
}
static void rx_window(Harness &h,unsigned tid,unsigned first,unsigned size=8) {
    auto &ba=h.ni.ni_rx_ba[tid]; ba.ba_ni=&h.ni; ba.ba_state=IEEE80211_BA_AGREED;
    ba.ba_winsize=size; ba.ba_winstart=first; ba.ba_winend=(first+size-1)&4095;
    ba.ba_buf=(ieee80211_ba_buf *)malloc(64*sizeof(ieee80211_ba_buf),0,0); memset(ba.ba_buf,0,64*sizeof(ieee80211_ba_buf));
    timeout_set(&ba.ba_to,noop,&ba); timeout_set(&ba.ba_gap_to,ieee80211_input_ba_gap_timeout,&ba);
}
static void receive(Harness &h,unsigned tid,unsigned seq,uint64_t pn) {
    ieee80211_rxinfo info={}; mbuf_list ml={};
    ieee80211_input_ba(&h.ic,encrypted(h,tid,seq,pn),&h.ni,tid,&info,&ml);
    if_input(&h.ic.ic_if,&ml);
}
static mbuf_t action(unsigned action,unsigned tid,unsigned window,unsigned token,unsigned status=0,bool immediate=true,unsigned timeout=5,unsigned ssn=0) {
    auto *m=packet(33); auto *wh=mtod(m,ieee80211_frame *); wh->i_fc[0]=IEEE80211_FC0_SUBTYPE_ACTION;
    uint8_t *body=(uint8_t *)(wh+1); body[0]=IEEE80211_CATEG_BA; body[1]=action; body[2]=token;
    uint16_t params=(window<<6)|(tid<<2)|(immediate ? IEEE80211_ADDBA_BA_POLICY : 0);
    if(action==IEEE80211_ACTION_ADDBA_REQ) { LE_WRITE_2(body+3,params); LE_WRITE_2(body+5,timeout); LE_WRITE_2(body+7,ssn<<4); }
    else { LE_WRITE_2(body+3,status); LE_WRITE_2(body+5,params); LE_WRITE_2(body+7,timeout); }
    return m;
}
static void recv_response(Harness &h,unsigned tid,unsigned window,bool immediate=true,unsigned token=7,unsigned status=0) {
    auto *m=action(IEEE80211_ACTION_ADDBA_RESP,tid,window,token,status,immediate);
    ieee80211_recv_addba_resp(&h.ic,m,&h.ni); mbuf_freem(m);
}
static void requested_tx(Harness &h,unsigned tid) {
    auto &ba=h.ni.ni_tx_ba[tid]; ba.ba_ni=&h.ni; ba.ba_state=IEEE80211_BA_REQUESTED; ba.ba_token=7;
    ba.ba_winsize=64; ba.ba_winstart=4094; ba.ba_winend=61; h.ni.ni_qos_txseqs[tid]=3;
    timeout_set(&ba.ba_to,noop,&ba); ba.ba_to->pending=true;
}
static void encrypted_order() {
    Harness h; rx_window(h,6,100); receive(h,6,101,11);
    require(h.delivered.empty() && !h.rx.k_rsc[6],"N+1 remains encrypted and cannot advance CCMP replay state before N");
    require(h.ni.ni_rx_ba[6].ba_gap_to->pending,"missing N arms the gap timeout");
    receive(h,6,100,10);
    require(h.delivered==std::vector<unsigned>({100,101}),"N and N+1 reach CCMP and Ethernet in sequence order");
    require(h.rx.k_rsc[6]==11 && !h.ic.ic_stats.is_ccmp_replays,"reordered encrypted MPDUs preserve monotonic CCMP PN");
    require(h.ni.ni_rx_ba[6].ba_winstart==102 && !h.ni.ni_rx_ba[6].ba_gap_to->pending,"consecutive flush advances window and cancels gap callback");
}
static void duplicate_and_wrap() {
    Harness h; rx_window(h,5,4095); receive(h,5,0,21); receive(h,5,0,21);
    require(h.stats.inputErrors==1 && h.ic.ic_stats.is_ht_rx_ba_no_buf==1,"duplicate buffered sequence drops before CCMP");
    receive(h,5,4095,20);
    require(h.delivered==std::vector<unsigned>({4095,0}) && h.rx.k_rsc[5]==21,"RX reorder crosses 4095-to-0 sequence wrap before CCMP");
    receive(h,5,4095,20);
    require(h.delivered.size()==2 && h.ic.ic_stats.is_ht_rx_frame_below_ba_winstart==1 && !h.ic.ic_stats.is_ccmp_replays,"late duplicate below wrapped window never reaches cipher");
}
static void bar_gap() {
    Harness h; rx_window(h,2,200); receive(h,2,202,31);
    auto *m=packet(20); uint8_t *wire=mtod(m,uint8_t *); wire[0]=IEEE80211_FC0_TYPE_CTL|IEEE80211_FC0_SUBTYPE_BAR;
    LE_WRITE_2(wire+16,IEEE80211_BA_COMPRESSED|(2<<12)); LE_WRITE_2(wire+18,202<<4);
    ieee80211_recv_bar(&h.ic,m,&h.ni);
    require(h.delivered==std::vector<unsigned>({202}) && h.rx.k_rsc[2]==31,"actual compressed BAR skips gap and decrypts queued MPDU");
    require(h.ni.ni_rx_ba[2].ba_winstart==203 && h.ic.ic_stats.is_ht_rx_ba_frame_lost==2,"BAR accounts lost MPDUs and advances window");
    LE_WRITE_2(wire+18,201<<4); ieee80211_recv_bar(&h.ic,m,&h.ni);
    require(h.ni.ni_rx_ba[2].ba_winstart==203 && h.delivered.size()==1,"stale BAR cannot move window backwards or duplicate output"); mbuf_freem(m);
}
static void gap_timeout() {
    Harness h; rx_window(h,1,300); receive(h,1,302,41); auto *timer=h.ni.ni_rx_ba[1].ba_gap_to;
    timer->pending=false; timer->callback(timer->argument);
    require(h.delivered==std::vector<unsigned>({302}) && h.ic.ic_stats.is_ht_rx_ba_window_gap_timeout==1,"actual gap callback releases protected packet through CCMP");
    require(h.ic.ic_stats.is_ht_rx_ba_frame_lost==2 && h.rx.k_rsc[1]==41,"gap timeout counts only missing leading sequences");
}
static void sliding_window() {
    Harness h; rx_window(h,0,10,4); receive(h,0,14,15);
    require(h.delivered.empty() && h.ni.ni_rx_ba[0].ba_winstart==11,"frame one beyond BA end slides window by one and remains queued");
    receive(h,0,11,12); receive(h,0,12,13); receive(h,0,13,14);
    require(h.delivered==std::vector<unsigned>({11,12,13,14}),"late in-window packets survive minimal forward slide and decrypt before buffered PN");
}
static void full_window_slide() {
    Harness h; rx_window(h,0,10,4); receive(h,0,12,13); receive(h,0,17,18);
    require(h.delivered==std::vector<unsigned>({12}) && h.ni.ni_rx_ba[0].ba_winstart==14,"full-window skip emits only old buffered MPDU and retains new tail behind gap");
    require(h.ni.ni_rx_ba[0].ba_gapwait==1,"full-window slide owns exactly the new trailing MPDU");
    receive(h,0,14,15); receive(h,0,16,17); receive(h,0,15,16);
    require(h.delivered==std::vector<unsigned>({12,14,15,16,17}) && h.rx.k_rsc[0]==18,"new window preserves trailing encrypted MPDU through out-of-order gap closure");
}
static void wrapped_slide() {
    Harness h; rx_window(h,0,4093,4); receive(h,0,1,5);
    require(h.delivered.empty() && h.ni.ni_rx_ba[0].ba_winstart==4094,"minimal BA slide handles end-of-window sequence wrap");
    receive(h,0,4094,2); receive(h,0,0,4); receive(h,0,4095,3);
    require(h.delivered==std::vector<unsigned>({4094,4095,0,1}) && h.rx.k_rsc[0]==5,"wrapped sliding retains every later encrypted MPDU until its gap closes");
}
static void large_jump() {
    Harness h; rx_window(h,0,10,4); receive(h,0,11,12);
    for(unsigned seq=30;seq<=38;++seq) receive(h,0,seq,seq+1);
    require(h.delivered.empty() && h.ni.ni_rx_ba[0].ba_winstart==10,"isolated far-ahead frames do not immediately discard current BA window");
    receive(h,0,39,40);
    require(h.delivered==std::vector<unsigned>({11,39}) && h.rx.k_rsc[0]==40,"confirmed larger-than-window jump flushes owned earlier MPDU before new PN");
    require(h.ic.ic_stats.is_ht_rx_ba_window_jump==1 && h.ni.ni_rx_ba[0].ba_winstart==40,"consistent far-ahead sequence resynchronizes the BA window once");
}
static void bar_past_buffer() {
    Harness h; rx_window(h,2,200); receive(h,2,201,31);
    auto *m=packet(20); uint8_t *wire=mtod(m,uint8_t *); wire[0]=IEEE80211_FC0_TYPE_CTL|IEEE80211_FC0_SUBTYPE_BAR;
    LE_WRITE_2(wire+16,IEEE80211_BA_COMPRESSED|(2<<12)); LE_WRITE_2(wire+18,202<<4);
    ieee80211_recv_bar(&h.ic,m,&h.ni); mbuf_freem(m);
    require(h.delivered==std::vector<unsigned>({201}) && h.ni.ni_rx_ba[2].ba_gapwait==0,"BAR beyond buffered MPDU empties the owned reorder queue");
    require(!h.ni.ni_rx_ba[2].ba_gap_to->pending,"BAR emptying reorder queue cancels stale gap callback before it can skip live sequences");
    receive(h,2,202,32);
    require(h.delivered==std::vector<unsigned>({201,202}) && h.ni.ni_rx_ba[2].ba_winstart==203,"next MPDU following BAR is retained in the live window");
}
static void tx_negotiation() {
    Harness h; h.validate_tx_params=true; requested_tx(h,6); recv_response(h,6,16);
    auto &ba=h.ni.ni_tx_ba[6];
    require(ba.ba_state==IEEE80211_BA_AGREED && h.tx_starts==1,"valid peer ADDBA response activates TX BA once");
    require(h.ni.ni_qos_txseqs[6]==3,"acceptance cannot rewind QoS sequence advanced while ADDBA pending");
    require(ba.ba_winsize==16 && ba.ba_winend==13 && ba.ba_timeout_val==5120,"acceptance honors smaller peer window and timeout across wrap");
    require(((ba.ba_params>>6)&1023)==16 && ba.ba_to->pending,"stored BA parameters and timer match negotiated agreement");
}
static void tx_sequence_no_rewind() {
    Harness h; requested_tx(h,6); h.ic.ic_ampdu_tx_start=nullptr; recv_response(h,6,64);
    require(h.ni.ni_tx_ba[6].ba_state==IEEE80211_BA_AGREED && h.ni.ni_qos_txseqs[6]==3,"TX acceptance preserves sequence after normal QoS crossed wrap while ADDBA pending");
}
static void tx_refusals() {
    Harness h; requested_tx(h,6); recv_response(h,6,16,false);
    require(h.ni.ni_tx_ba[6].ba_state==IEEE80211_BA_INIT && !h.tx_starts,"delayed BA policy rejected before driver activation");
    requested_tx(h,6); recv_response(h,6,0);
    require(h.ni.ni_tx_ba[6].ba_state==IEEE80211_BA_INIT && !h.tx_starts,"zero-sized successful peer response rejected before driver activation");
    requested_tx(h,6); recv_response(h,6,16,true,8);
    require(h.ni.ni_tx_ba[6].ba_state==IEEE80211_BA_REQUESTED && h.ni.ni_tx_ba[6].ba_to->pending && !h.tx_starts,"wrong dialog token leaves outstanding request and timer intact");
}
static void rx_zeroed_slots() {
    Harness h; auto *m=action(IEEE80211_ACTION_ADDBA_REQ,4,16,9,0,true,5,500);
    ieee80211_recv_addba_req(&h.ic,m,&h.ni); mbuf_freem(m);
    require(h.ni.ni_rx_ba[4].ba_state==IEEE80211_BA_AGREED && h.rx_starts==1,"RX ADDBA succeeds with every initially poisoned slot initialized");
}
static void rx_late_refusal() {
    Harness h; rx_window(h,4,500); h.ni.ni_rx_ba[4].ba_state=IEEE80211_BA_REQUESTED;
    receive(h,4,501,51); ieee80211_addba_req_refuse(&h.ic,&h.ni,4);
    auto &ba=h.ni.ni_rx_ba[4];
    require(ba.ba_state==IEEE80211_BA_INIT && !ba.ba_buf && !ba.ba_gapwait && !ba.ba_gap_to->pending && !rtw_ht_live_mbufs,"RX refusal discards pending protected MPDU and cancels its gap callback");
}
static void rx_alloc_and_refuse() {
    Harness h; auto *m=action(IEEE80211_ACTION_ADDBA_REQ,7,16,9,0,true,5,500);
    fail_reorder_allocations=1; ieee80211_recv_addba_req(&h.ic,m,&h.ni);
    auto &ba=h.ni.ni_rx_ba[7];
    require(ba.ba_state==IEEE80211_BA_INIT && !ba.ba_buf && !h.rx_starts,"RX allocation failure rolls back requested agreement without activating driver");
    require(!ba.ba_to->pending && !ba.ba_gap_to->pending && (h.last_action_arg>>16)==IEEE80211_STATUS_REFUSED,"RX allocation failure cancels callbacks and returns refusal");
    h.rx_result=EBUSY; ieee80211_recv_addba_req(&h.ic,m,&h.ni); mbuf_freem(m);
    require(ba.ba_state==IEEE80211_BA_REQUESTED && h.rx_starts==1,"RX asynchronous driver sees initialized slots before accepting");
    receive(h,7,501,51);
    require(ba.ba_gapwait==1 && ba.ba_gap_to->pending && h.delivered.empty(),"protected MPDU can queue while RX BA activation is pending");
    ieee80211_addba_req_refuse(&h.ic,&h.ni,7);
    require(ba.ba_state==IEEE80211_BA_INIT && !ba.ba_buf && !ba.ba_gapwait && !ba.ba_gap_to->pending && !rtw_ht_live_mbufs,"late RX refusal releases queued encrypted MPDU and callbacks");
}
int main(int argc,char **argv) {
    const char *which=argc>1 ? argv[1] : "all";
    const struct { const char *name; void (*run)(); } cases[]={
        {"order",encrypted_order},{"wrap",duplicate_and_wrap},{"bar",bar_gap},{"timeout",gap_timeout},
        {"slide",sliding_window},{"slidefull",full_window_slide},{"slidewrap",wrapped_slide},{"jump",large_jump},{"barpast",bar_past_buffer},{"tx",tx_negotiation},{"rewind",tx_sequence_no_rewind},{"refuse",tx_refusals},{"zero",rx_zeroed_slots},{"rxrefuse",rx_late_refusal},{"alloc",rx_alloc_and_refuse}};
    bool ran=false; for(const auto &test:cases) if(!strcmp(which,"all") || !strcmp(which,test.name)) { test.run(); ran=true; }
    require(ran,"requested case exists"); printf("PASS actual net80211 RX BA/BAR/ADDBA and encrypted reorder (%u assertions)\n",checks);
}
