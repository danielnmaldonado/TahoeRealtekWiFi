/* Tests execute verbatim production functions, with only the OS boundary replaced. */
#include "net80211_ht_host/platform.hpp"
#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_priv.h>

unsigned rtw_ht_invalid_setdata,rtw_ht_fail_allocations,rtw_ht_live_timers,rtw_ht_live_mbufs;
static unsigned checks,requests;
static int requested_tid=-1;
static uint16_t requested_ssn;
static std::vector<void *> allocations;
static void require(bool ok,const char *description) {
    ++checks; if (!ok) { fprintf(stderr,"FAIL: %s\n",description); exit(1); }
}
void *rtw_ht_allocate(size_t n) { void *p=calloc(1,n); if (p) allocations.push_back(p); return p; }
void rtw_ht_deallocate(void *p) {
    auto at=std::find(allocations.begin(),allocations.end(),p);
    if (at!=allocations.end()) allocations.erase(at);
    __builtin_free(p);
}
int ieee80211_node_cmp(ieee80211_node *a,ieee80211_node *b) { return memcmp(a->ni_macaddr,b->ni_macaddr,6); }
RB_GENERATE(ieee80211_tree,ieee80211_node,ni_node,ieee80211_node_cmp)
int ieee80211_addba_request(ieee80211com *,ieee80211_node *ni,uint16_t ssn,uint8_t tid) { ++requests; requested_tid=tid; requested_ssn=ssn; ni->ni_tx_ba[tid].ba_state=IEEE80211_BA_REQUESTED; return 0; }
void ieee80211_clear_htcaps(ieee80211_node *ni) { ni->ni_flags&=~IEEE80211_NODE_HT; }
void ieee80211_release_node(ieee80211com *,ieee80211_node *ni) { --ni->ni_refcnt; }
ieee80211_node *ieee80211_find_txnode(ieee80211com *ic,const uint8_t *) { return ieee80211_ref_node(ic->ic_bss); }
int ieee80211_save_ie(const uint8_t *ie,uint8_t **out) {
    *out=(uint8_t *)malloc(ie[1]+2,0,0); memcpy(*out,ie,ie[1]+2); return 0;
}
int ieee80211_save_ie_tlv(const uint8_t *ie,uint8_t **out,u_int *length,u_int n) {
    *out=(uint8_t *)malloc(n,0,0); memcpy(*out,ie,n); *length=n; return 0;
}
static void empty_callback(void *) {}
static mbuf_t ethernet(uint16_t type=ETHERTYPE_IP,uint8_t tos=0) {
    mbuf_t m=nullptr; require(mbuf_gethdr(0,0,&m)==0,"allocate Ethernet fixture");
    m->offset=96; m->length=m->packet_length=34;
    uint8_t frame[]={0,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0x08,0x00,
        0x45,0,0,20,0,0,0,0,64,17,0,0,192,0,2,1,192,0,2,2};
    frame[12]=type>>8; frame[13]=type; frame[15]=tos;
    memcpy(mbuf_data(m),frame,sizeof(frame)); return m;
}
struct Fixture {
    ieee80211com ic={}; ieee80211_node ni={};
    Fixture() { ic.ic_bss=&ni; ic.ic_opmode=IEEE80211_M_STA; ic.ic_node_free=ieee80211_node_free; ni.ni_ic=&ic;
        memcpy(ni.ni_bssid,"\x00\x11\x22\x33\x44\x55",6); RB_INIT(&ic.ic_tree); }
};
static void buffered_encrypted(Fixture &f,unsigned tid,int state=IEEE80211_BA_AGREED,ieee80211_node *target=nullptr) {
    auto *owner=target ? target : &f.ni;
    auto &ba=owner->ni_rx_ba[tid]; ba.ba_ni=owner; ba.ba_state=state; ba.ba_winsize=16;
    ba.ba_winstart=4093; ba.ba_winend=12; ba.ba_head=3; ba.ba_gapwait=2;
    ba.ba_buf=(ieee80211_ba_buf *)malloc(IEEE80211_BA_MAX_WINSZ*sizeof(ieee80211_ba_buf),0,0);
    timeout_set(&ba.ba_to,empty_callback,&ba); ba.ba_to->pending=true;
    timeout_set(&ba.ba_gap_to,empty_callback,&ba); ba.ba_gap_to->pending=true;
    ieee80211_key key={}; key.k_tsc=31; memset(key.k_key,0x42,16);
    require(ieee80211_ccmp_set_key(&f.ic,&key)==0,"install reorder fixture CCMP key");
    const uint8_t raw[]={0x88,0x41,0,0,0,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,
        0,0x11,0x22,0x33,0x44,0x55,0xd0,0xff,0,0,0xaa,0xaa,3,0,0,0,8,0};
    for (unsigned index : {3u,5u,63u}) {
        mbuf_t m=nullptr; require(mbuf_gethdr(0,0,&m)==0,"allocate pending protected MPDU");
        memcpy(mbuf_data(m),raw,sizeof(raw)); m->length=m->packet_length=sizeof(raw);
        ba.ba_buf[index].m=ieee80211_ccmp_encrypt(&f.ic,m,&key);
        require(ba.ba_buf[index].m!=nullptr,"real encrypted MPDU buffered awaiting gap");
    }
    ieee80211_ccmp_delete_key(&f.ic,&key);
}
static void cleanup_buffered() {
    Fixture f; buffered_encrypted(f,0); buffered_encrypted(f,7,IEEE80211_BA_INIT);
    ieee80211_node_cleanup(&f.ic,&f.ni);
    require(rtw_ht_live_mbufs==0,"node_cleanup releases buffered encrypted MPDUs in every BA state");
    require(allocations.empty(),"node_cleanup releases every reorder allocation");
    require(rtw_ht_live_timers==0,"node_cleanup releases BA timer ownership");
    ieee80211_node_cleanup(&f.ic,&f.ni);
    require(rtw_ht_live_mbufs==0 && allocations.empty(),"repeated cleanup is idempotent");
}
static void cancel_init_timer() {
    Fixture f; auto &ba=f.ni.ni_rx_ba[0];
    timeout_set(&ba.ba_gap_to,empty_callback,&ba); ba.ba_gap_to->pending=true;
    ieee80211_ba_del(&f.ni);
    require(!ba.ba_gap_to->pending,"BA_INIT still cancels a previously armed gap callback");
    ieee80211_ba_free(&f.ni);
}
static void reset_then_new_ba() {
    Fixture f; buffered_encrypted(f,6);
    ieee80211_ba_del(&f.ni);
    require(f.ni.ni_rx_ba[6].ba_buf==nullptr && !rtw_ht_live_mbufs && allocations.empty(),"ba_del reset drops old encrypted reorder window before a fresh agreement");
    require(!f.ni.ni_rx_ba[6].ba_gap_to->pending && f.ni.ni_rx_ba[6].ba_state==IEEE80211_BA_INIT,"reset cancels callback before dropping its reorder window");
    buffered_encrypted(f,6); ieee80211_node_cleanup(&f.ic,&f.ni);
    require(!rtw_ht_live_timers && !rtw_ht_live_mbufs && allocations.empty(),"fresh BA following direct reset owns its own buffers");
}
static void leave_then_cleanup() {
    Fixture f; f.ni.ni_flags|=IEEE80211_NODE_HT; buffered_encrypted(f,15);
    ieee80211_node_leave_ht(&f.ic,&f.ni); ieee80211_node_cleanup(&f.ic,&f.ni);
    require(allocations.empty() && rtw_ht_live_mbufs==0 && rtw_ht_live_timers==0,"node_leave_ht then cleanup releases resources once");
}
static void rejoin_copy() {
    Fixture src,dst; ieee80211_node_set_timeouts(&src.ni);
    buffered_encrypted(src,3); buffered_encrypted(dst,2);
    unsigned source_mbufs=rtw_ht_live_mbufs/2;
    ieee80211_node_copy(&dst.ic,&dst.ni,&src.ni);
    require(dst.ni.ni_rx_ba[3].ba_buf==nullptr && dst.ni.ni_rx_ba[3].ba_to==nullptr && dst.ni.ni_rx_ba[3].ba_gap_to==nullptr,"node_copy never aliases source RX BA ownership");
    require(dst.ni.ni_rx_ba[3].ba_state==IEEE80211_BA_INIT && dst.ni.ni_rx_ba[3].ba_gapwait==0,"node_copy begins fresh reordering agreement");
    require(rtw_ht_live_mbufs==source_mbufs,"node_copy discards only destination pending encrypted MPDUs");
    require(dst.ni.ni_addba_req_to[0]!=src.ni.ni_addba_req_to[0],"node_copy owns fresh AddBA callback timers");
    ieee80211_node_cleanup(&dst.ic,&dst.ni);
    require(src.ni.ni_rx_ba[3].ba_buf && src.ni.ni_rx_ba[3].ba_to->pending,"copy cleanup preserves source session");
    ieee80211_node_cleanup(&src.ic,&src.ni);
    require(allocations.empty() && !rtw_ht_live_mbufs && !rtw_ht_live_timers,"both copy owners clean independently");
}
static void free_all_and_rejoin() {
    Fixture f; buffered_encrypted(f,1);
    auto *cached=(ieee80211_node *)malloc(sizeof(ieee80211_node),0,0);
    cached->ni_ic=&f.ic; cached->ni_macaddr[5]=1;
    buffered_encrypted(f,3,IEEE80211_BA_AGREED,cached);
    RB_INSERT(ieee80211_tree,&f.ic.ic_tree,cached); f.ic.ic_nnodes=1;
    ieee80211_free_allnodes(&f.ic,1);
    require(!f.ic.ic_nnodes && RB_EMPTY(&f.ic.ic_tree),"INIT/free_allnodes releases cache nodes");
    require(allocations.empty() && !rtw_ht_live_mbufs && !rtw_ht_live_timers,"INIT/free_allnodes discards BSS encrypted reorder buffer");
    buffered_encrypted(f,1); ieee80211_node_cleanup(&f.ic,&f.ni);
    require(allocations.empty() && !rtw_ht_live_mbufs && !rtw_ht_live_timers,"rejoin can own and clean a new BA window");
}
static void detach_buffered() {
    Fixture f;
    auto *bss=(ieee80211_node *)malloc(sizeof(ieee80211_node),0,0);
    bss->ni_ic=&f.ic; f.ic.ic_bss=bss;
    buffered_encrypted(f,5,IEEE80211_BA_AGREED,bss);
    ieee80211_node_detach(&f.ic.ic_if);
    require(f.ic.ic_bss==nullptr && !f.ic.ic_nnodes,"detach releases owned BSS and clears node reference");
    require(allocations.empty() && !rtw_ht_live_mbufs && !rtw_ht_live_timers,"detach releases encrypted pending MPDUs and BA callbacks");
}
void ieee80211_del_ess(ieee80211com *,char *,int,int) {}
static void timeout_bounds() {
    Fixture f; f.ni.ni_tx_ba[15].ba_winstart=177;
    f.ni.ni_qos_txseqs[4]=42; f.ni.ni_flags=IEEE80211_NODE_HT|IEEE80211_NODE_QOS;
    ieee80211_node_set_timeouts(&f.ni);
    require(f.ni.ni_tx_ba[15].ba_winstart==177 && f.ni.ni_qos_txseqs[4]==42 && (f.ni.ni_flags&IEEE80211_NODE_HT),"timeout initialization stays within its pointer array");
    ieee80211_node_cleanup(&f.ic,&f.ni);
}
static mbuf_t encapsulate(Fixture &f,uint16_t type=ETHERTYPE_IP,uint8_t tos=0) {
    ieee80211_node *node=nullptr; auto *m=ieee80211_encap(&f.ic.ic_if,ethernet(type,tos),&node);
    require(m && node==&f.ni,"actual encap returns associated node"); --node->ni_refcnt; return m;
}
static void all_tid_timers() {
    Fixture f; ieee80211_node_set_timeouts(&f.ni);
    for (unsigned tid=0;tid<16;++tid) {
        f.ni.ni_qos_txseqs[tid]=4095-tid;
        require(f.ni.ni_addba_req_to[tid]!=nullptr,"all 16 TIDs have an AddBA trigger callback");
        ieee80211_node_trigger_addba_req(&f.ni,tid);
        auto *timer=f.ni.ni_addba_req_to[tid];
        require(timer->pending,"actual AddBA trigger arms each TID timer");
        timer->pending=false; timer->callback(timer->argument);
        require(requested_tid==int(tid) && requested_ssn==4095-tid,"actual callback requests correct TID and current sequence");
    }
    ieee80211_node_cleanup(&f.ic,&f.ni); requests=0;
}
static void qos_sequences() {
    Fixture f; ieee80211_node_set_timeouts(&f.ni); f.ic.ic_flags=IEEE80211_F_QOS; f.ni.ni_flags=IEEE80211_NODE_QOS|IEEE80211_NODE_HT;
    f.ni.ni_qos_txseqs[0]=4095; f.ni.ni_txseq=77;
    for(unsigned expected : {4095u,0u}) {
        auto *m=encapsulate(f); auto *wh=mtod(m,ieee80211_qosframe *);
        require(ieee80211_has_qos((ieee80211_frame *)wh),"negotiated QoS uses QoS data without any TX BA agreement");
        require((LE_READ_2(wh->i_qos)&0x60)==0,"QoS without TX BA uses normal ACK");
        require(LE_READ_2(wh->i_seq)==(expected<<4),"per-TID sequence wraps modulo 4096"); mbuf_freem(m);
    }
    require(f.ni.ni_txseq==77 && !requests,"normal QoS does not use legacy sequence or request unsupported aggregation");
    auto *eapol=encapsulate(f,ETHERTYPE_PAE);
    require(!ieee80211_has_qos(mtod(eapol,ieee80211_frame *)) && LE_READ_2(mtod(eapol,ieee80211_frame *)->i_seq)==(77<<4),"EAPOL stays legacy and keeps independent sequence space"); mbuf_freem(eapol);
    auto *voice=encapsulate(f,ETHERTYPE_IP,0xe0); auto *qwh=mtod(voice,ieee80211_qosframe *);
    require((LE_READ_2(qwh->i_qos)&15)==6 && LE_READ_2(qwh->i_seq)==0,"DSCP classification uses independent voice TID sequence"); mbuf_freem(voice);
    f.ic.ic_flags|=IEEE80211_F_RSNON; f.ic.ic_caps|=IEEE80211_C_TX_AMPDU;
    f.ni.ni_port_valid=1; f.ni.ni_rsnprotos=IEEE80211_PROTO_RSN;
    auto *request=encapsulate(f); mbuf_freem(request); require(timeout_pending(&f.ni.ni_addba_req_to[0]),"eligible unagreed BA still triggers existing AddBA negotiation");
    f.ni.ni_tx_ba[0].ba_state=IEEE80211_BA_AGREED;
    auto *agreed=encapsulate(f); require(ieee80211_has_qos(mtod(agreed,ieee80211_frame *)) && requests==0,"active BA remains QoS without repeated negotiation"); mbuf_freem(agreed); ieee80211_node_cleanup(&f.ic,&f.ni);
}
static void qos_ccmp() {
    Fixture f; f.ic.ic_flags=IEEE80211_F_QOS|IEEE80211_F_RSNON; f.ni.ni_flags=IEEE80211_NODE_QOS|IEEE80211_NODE_TXPROT;
    f.ni.ni_port_valid=1; f.ni.ni_qos_txseqs[6]=4095;
    ieee80211_key key={}; for(unsigned i=0;i<16;++i) key.k_key[i]=i; key.k_tsc=0x010203040505ULL;
    require(!ieee80211_ccmp_set_key(&f.ic,&key),"QoS CCMP key setup");
    auto *encrypted=ieee80211_ccmp_encrypt(&f.ic,encapsulate(f,ETHERTYPE_IP,0xe0),&key);
    require(encrypted!=nullptr && mbuf_pkthdr_len(encrypted)==70,"normal ACK QoS packet software encrypts");
    /* Independently generated with Node/OpenSSL AES-128-CCM: TID=6 in nonce and AAD. */
    const uint8_t expected[]={0xc2,0x93,0xa5,0x4e,0x4e,0x55,0x4b,0x15,0xc3,0xd4,0xaf,0xd1,0x39,0x4d,0xae,0xe5,0x21,0x96,0x35,0x04,0xea,0xa7,0xc1,0xc0,0x93,0x12,0x56,0x85,0x88,0x9c,0xf2,0x6e,0xab,0x1b,0x84,0xe8};
    uint8_t wire[70]; mbuf_copydata(encrypted,0,sizeof(wire),wire);
    require(memcmp(wire+34,expected,sizeof(expected))==0,"QoS CCMP ciphertext and MIC match independent OpenSSL vector");
    require(wire[24]==6 && wire[26]==6 && wire[27]==5 && wire[30]==4 && wire[33]==1,"CCMP retains QoS TID and PN wire header");
    auto *plain=ieee80211_ccmp_decrypt(&f.ic,encrypted,&key);
    require(plain && mbuf_pkthdr_len(plain)==54,"software CCMP decrypt restores QoS packet");
    require(key.k_rsc[6]==0x010203040506ULL && !key.k_rsc[0],"CCMP replay state uses QoS TID from nonce/AAD");
    mbuf_freem(plain); ieee80211_ccmp_delete_key(&f.ic,&key);
}
int main(int argc,char **argv) {
    const char *which=argc>1 ? argv[1] : "all";
    if (!strcmp(which,"all") || !strcmp(which,"cleanup")) cleanup_buffered();
    if (!strcmp(which,"all") || !strcmp(which,"timer")) cancel_init_timer();
    if (!strcmp(which,"all") || !strcmp(which,"reset")) reset_then_new_ba();
    if (!strcmp(which,"all") || !strcmp(which,"leave")) leave_then_cleanup();
    if (!strcmp(which,"all") || !strcmp(which,"copy")) rejoin_copy();
    if (!strcmp(which,"all") || !strcmp(which,"freeall")) free_all_and_rejoin();
    if (!strcmp(which,"all") || !strcmp(which,"detach")) detach_buffered();
    if (!strcmp(which,"all") || !strcmp(which,"bounds")) timeout_bounds();
    if (!strcmp(which,"all") || !strcmp(which,"tids")) all_tid_timers();
    if (!strcmp(which,"all") || !strcmp(which,"qos")) qos_sequences();
    if (!strcmp(which,"all") || !strcmp(which,"ccmp")) qos_ccmp();
    require(allocations.empty() && !rtw_ht_live_mbufs && !rtw_ht_live_timers,"no final host allocations, packets or timers remain");
    printf("PASS actual net80211 HT BA lifecycle, QoS sequences and CCMP (%u assertions)\n",checks);
}
