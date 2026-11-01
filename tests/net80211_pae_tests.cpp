/* Execute imported PAE input/output and crypto, using independent AP fixtures. */
#include "net80211_pae_host/platform.hpp"
#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_priv.h>
#include "pae_fixtures.hpp"

unsigned rtw_pae_invalid_setdata;
unsigned rtw_pae_fail_allocations;
static unsigned checks;
static void require(bool ok,const char *what) {
    ++checks;
    if(!ok) { fprintf(stderr,"FAIL: %s\n",what); exit(1); }
}
static std::vector<std::pair<void *,size_t>> contexts;
void *rtw_pae_allocate(size_t n) {
    void *context=calloc(1,n);
    if(context) contexts.push_back({context,n});
    return context;
}
void rtw_pae_deallocate(void *p) {
    for(auto at=contexts.begin();at!=contexts.end();++at) {
        if(at->first!=p) continue;
        const auto *bytes=(const uint8_t *)p;
        bool erased=true;
        for(size_t n=0;n<at->second;++n) erased=erased && bytes[n]==0;
        require(erased,"actual software CCMP context erased before free");
        contexts.erase(at); break;
    }
    __builtin_free(p);
}
void rtw_pae_random(void *out,size_t n) {
    require(n==sizeof(fixture_snonce),"supplicant requests 32-byte SNonce");
    memcpy(out,fixture_snonce,n);
}
struct Installed { unsigned id,flags; uint8_t key[16]; };
struct Harness {
    ieee80211com ic={};
    ieee80211_node node={};
    std::vector<mbuf_t> output;
    std::vector<Installed> installed;
    unsigned deletes=0,link_ups=0,deauths=0,state_changes=0;
    int key_result=0;
};
static Harness *active;
static int install(ieee80211com *ic,ieee80211_node *,ieee80211_key *key) {
    require(ic==&active->ic,"installation uses correct interface");
    require(key->k_cipher==IEEE80211_CIPHER_CCMP && key->k_len==16,"CCMP-128 key installation");
    Installed event={key->k_id,key->k_flags,{}};
    memcpy(event.key,key->k_key,sizeof(event.key));
    active->installed.push_back(event);
    if(active->key_result) return active->key_result;
    int result=ieee80211_ccmp_set_key(ic,key);
    if(!result) key->k_flags|=IEEE80211_KEY_SWCRYPTO;
    return result;
}
static void remove_key(ieee80211com *ic,ieee80211_node *,ieee80211_key *key) {
    ++active->deletes;
    ieee80211_ccmp_delete_key(ic,key);
    explicit_bzero(key,sizeof(*key));
}
static int management(ieee80211com *,ieee80211_node *,int subtype,int,int) {
    require(subtype==IEEE80211_FC0_SUBTYPE_DEAUTH,"rejection requests deauthentication");
    ++active->deauths; return 0;
}
static int transition(ieee80211com *,ieee80211_state state,int) {
    require(state==IEEE80211_S_SCAN,"rejection requests scan state");
    ++active->state_changes; return 0;
}
void ieee80211_set_link_state(ieee80211com *,int state) {
    require(state==LINK_STATE_UP,"supplicant only signals a validated link");
    require(active->node.ni_port_valid,"controlled port opens before carrier callback");
    ++active->link_ups;
}
/* These authenticator-only paths are unreachable for this station fixture. */
void ieee80211_iterate_nodes(ieee80211com *,ieee80211_iter_func *,void *) { abort(); }
void ieee80211_count_rekeysta(void *,ieee80211_node *) { abort(); }
void ieee80211_setkeysdone(ieee80211com *) { abort(); }
void ieee80211_ba_del(ieee80211_node *) {}
void ieee80211_ba_free(ieee80211_node *) {}
static void start(_ifnet *) {}
static void setup(Harness &h) {
    active=&h;
    h.ic.ic_if.if_snd.packets=&h.output;
    h.ic.ic_if.if_start=start;
    h.ic.ic_set_key=install; h.ic.ic_delete_key=remove_key;
    h.ic.ic_send_mgmt=management; h.ic.ic_newstate=transition;
    h.ic.ic_caps=IEEE80211_C_RSN;
    h.ic.ic_flags=IEEE80211_F_RSNON|IEEE80211_F_PSK;
    h.ic.ic_state=IEEE80211_S_RUN; h.ic.ic_opmode=IEEE80211_M_STA;
    h.ic.ic_rsnprotos=IEEE80211_PROTO_RSN;
    h.ic.ic_rsnakms=IEEE80211_AKM_PSK;
    h.ic.ic_rsnciphers=IEEE80211_CIPHER_CCMP;
    h.ic.ic_rsngroupcipher=IEEE80211_CIPHER_CCMP;
    memcpy(h.ic.ic_myaddr,fixture_station,6); memcpy(h.ic.ic_psk,fixture_pmk,32);
    TAILQ_INIT(&h.ic.ic_pmksa);
    h.ic.ic_bss=&h.node;
    h.node.ni_ic=&h.ic;
    memcpy(h.node.ni_macaddr,fixture_ap,6);
    h.node.ni_rsnprotos=IEEE80211_PROTO_RSN;
    h.node.ni_rsnakms=IEEE80211_AKM_PSK;
    h.node.ni_rsncipher=IEEE80211_CIPHER_CCMP;
    h.node.ni_rsnciphers=IEEE80211_CIPHER_CCMP;
    h.node.ni_rsngroupcipher=IEEE80211_CIPHER_CCMP;
    h.node.ni_rsn_supp_state=RSNA_SUPP_PTKSTART;
    h.node.ni_rsnie=(uint8_t *)malloc(sizeof(fixture_rsn));
    memcpy(h.node.ni_rsnie,fixture_rsn,sizeof(fixture_rsn));
}
static void teardown(Harness &h) {
    for(auto *m:h.output) mbuf_freem(m);
    h.output.clear();
    free(h.node.ni_rsnie); h.node.ni_rsnie=nullptr;
    ieee80211_node_cleanup(&h.ic,&h.node);
    ieee80211_crypto_detach(&h.ic.ic_if);
    require(contexts.empty(),"cleanup releases every real software CCMP context");
}
static mbuf_t packet(const uint8_t *data,size_t size) {
    mbuf_t m=nullptr;
    require(mbuf_gethdr(0,0,&m)==0,"allocate input frame");
    memcpy(m->storage,data,size); m->length=m->packet_length=size;
    return m;
}
static void receive(Harness &h,const uint8_t *data,size_t size) {
    active=&h;
    ieee80211_eapol_key_input(&h.ic,packet(data,size),&h.node);
    require(rtw_pae_invalid_setdata==0,"real PAE uses valid KPI mbuf_setdata range");
}
static std::vector<uint8_t> sent(Harness &h,size_t index) {
    require(index<h.output.size(),"expected EAPOL response exists");
    mbuf_t m=h.output[index];
    std::vector<uint8_t> bytes(mbuf_pkthdr_len(m));
    mbuf_copydata(m,0,bytes.size(),bytes.data()); return bytes;
}
static void message1(Harness &h) {
    receive(h,fixture_msg1,sizeof(fixture_msg1));
    require(h.output.size()==1,"Message1 produces exactly Message2");
    require(h.node.ni_rsn_supp_state==RSNA_SUPP_PTKNEGOTIATING,"Message1 enters PTK negotiation");
    require(!h.node.ni_port_valid && !h.link_ups && h.installed.empty(),"Message1 cannot open port or install keys");
    auto response=sent(h,0);
    require(response.size()==14+99+sizeof(fixture_rsn),"Message2 encoded length");
    auto *key=(ieee80211_eapol_key *)(response.data()+14);
    require(BE_READ_2(key->info)==0x010a,"Message2 flags and descriptor");
    require(BE_READ_8(key->replaycnt)==1,"Message2 echoes replay counter");
    require(memcmp(key->nonce,fixture_snonce,32)==0,"Message2 carries SNonce");
    require(memcmp(key+1,fixture_rsn,sizeof(fixture_rsn))==0,"Message2 uses actual RSN IE encoder");
    require(ieee80211_eapol_key_check_mic(key,fixture_ptk)==0,"Message2 MIC matches independent AP-derived KCK");
}
static void successful_exchange() {
    Harness h; setup(h); message1(h);
    receive(h,fixture_msg3,sizeof(fixture_msg3));
    require(h.output.size()==2,"authenticated Message3 produces Message4");
    require(h.node.ni_port_valid && h.link_ups==1,"valid handshake opens controlled port once");
    require(h.node.ni_rsn_supp_state==RNSA_SUPP_PTKDONE,"handshake completes supplicant state");
    require(h.installed.size()==2,"valid handshake installs PTK and GTK");
    require(memcmp(h.installed[0].key,fixture_ptk+32,16)==0,"PTK temporal key matches independent AP");
    require(h.installed[1].id==1 && (h.installed[1].flags&IEEE80211_KEY_GROUP),"GTK key index and type");
    require(memcmp(h.installed[1].key,fixture_gtk,16)==0,"decrypted GTK matches AP");
    require(memcmp(&h.node.ni_ptk,fixture_ptk,64)==0,"full PTK derivation matches independent AP");
    require(h.node.ni_replaycnt==2 && h.node.ni_replaycnt_ok,"authenticated replay counter commits");
    auto response=sent(h,1);
    auto *key=(ieee80211_eapol_key *)(response.data()+14);
    require(response.size()==14+99 && BE_READ_2(key->info)==0x030a,"Message4 flags and length");
    require(BE_READ_8(key->replaycnt)==2,"Message4 echoes authenticated counter");
    require(ieee80211_eapol_key_check_mic(key,fixture_ptk)==0,"Message4 MIC matches AP KCK");
    receive(h,fixture_msg3,sizeof(fixture_msg3));
    require(h.output.size()==2 && h.installed.size()==2,"equal replay counter rejected without reinstall");
    require(h.ic.ic_stats.is_rx_eapol_replay==1,"replay is accounted");
    h.node.ni_pairwise_key.k_tsc=1234;
    receive(h,fixture_msg3_later,sizeof(fixture_msg3_later));
    require(h.output.size()==3 && h.installed.size()==2,"higher-counter repeated Message3 is acknowledged without reinstall");
    require(h.node.ni_pairwise_key.k_tsc==1234,"repeated Message3 does not reset transmit nonce");
    receive(h,fixture_group1,sizeof(fixture_group1));
    require(h.output.size()==4 && h.installed.size()==3,"group rekey acknowledged and installed");
    require(h.installed.back().id==2 && memcmp(h.installed.back().key,fixture_gtk2,16)==0,"new group key decrypted correctly");
    require(h.ic.ic_nw_keys[2].k_rsc[0]==7,"group replay sequence installed");
    receive(h,fixture_rekey1,sizeof(fixture_rekey1));
    require(h.installed.size()==3,"new Message1 does not prematurely replace PTK");
    receive(h,fixture_rekey3,sizeof(fixture_rekey3));
    require(h.installed.size()==5 && memcmp(h.installed[3].key,fixture_new_ptk+32,16)==0,"fresh ANonce installs new PTK only after valid Message3");
    unsigned deletes=h.deletes;
    ieee80211_node_leave_rsn(&h.ic,&h.node);
    require(!h.node.ni_port_valid && !(h.node.ni_flags&IEEE80211_NODE_PMK),"actual RSN leave helper closes port and invalidates PMK");
    require(!(h.node.ni_flags&IEEE80211_NODE_TXRXPROT) && h.deletes==deletes+1,"RSN leave removes pairwise protection/key");
    uint8_t no_pmk[32]={},no_ptk[64]={};
    require(memcmp(h.node.ni_pmk,no_pmk,32)==0 && memcmp(&h.node.ni_ptk,no_ptk,64)==0,"RSN leave erases PMK and PTK bytes");
    ieee80211_node_leave_rsn(&h.ic,&h.node);
    ieee80211_node_cleanup(&h.ic,&h.node);
    require(h.deletes==deletes+1,"repeated leave and cleanup do not delete key twice");
    size_t replies=h.output.size();
    receive(h,fixture_rekey3,sizeof(fixture_rekey3));
    require(h.output.size()==replies && !h.node.ni_port_valid,"old Message3 cannot reopen disconnected port");
    ieee80211_crypto_detach(&h.ic.ic_if);
    uint8_t zeros[32]={};
    require(memcmp(h.ic.ic_psk,zeros,32)==0,"crypto detach erases configured PSK");
    for(auto &group:h.ic.ic_nw_keys) require(group.k_cipher==IEEE80211_CIPHER_NONE && group.k_len==0,"detach clears group-key metadata");
    teardown(h);
}
static void cleanup_session() {
    Harness h; setup(h); message1(h);
    receive(h,fixture_msg3,sizeof(fixture_msg3));
    require(h.node.ni_pairwise_key.k_priv!=nullptr,"real software CCMP context installed");
    h.node.ni_flags|=IEEE80211_NODE_PMKID;
    memset(h.node.ni_pmkid,0x55,sizeof(h.node.ni_pmkid));
    ieee80211_node_cleanup(&h.ic,&h.node);
    require(!h.node.ni_port_valid && !(h.node.ni_flags&(IEEE80211_NODE_PMK|IEEE80211_NODE_PMKID|IEEE80211_NODE_TXRXPROT|IEEE80211_NODE_RSN_NEW_PTK)),"node cleanup closes port and clears RSN flags");
    uint8_t zeros[64]={};
    require(memcmp(h.node.ni_pmk,zeros,sizeof(h.node.ni_pmk))==0 && memcmp(&h.node.ni_ptk,zeros,sizeof(h.node.ni_ptk))==0,"node cleanup erases PMK and PTK");
    require(memcmp(h.node.ni_pmkid,zeros,sizeof(h.node.ni_pmkid))==0 && memcmp(h.node.ni_nonce,zeros,sizeof(h.node.ni_nonce))==0,"node cleanup erases PMKID and handshake nonce");
    require(h.node.ni_pairwise_key.k_priv==nullptr && h.node.ni_pairwise_key.k_len==0 && !h.node.ni_replaycnt_ok && !h.node.ni_replaycnt && h.node.ni_rsn_supp_state==RSNA_SUPP_INITIALIZE,"cleanup releases key and resets replay/supplicant state");
    unsigned deletes=h.deletes;
    ieee80211_node_cleanup(&h.ic,&h.node);
    require(h.deletes==deletes,"node cleanup is idempotent");
    teardown(h);
}
static void rejected_messages() {
    for(unsigned scenario=0;scenario<9;++scenario) {
        Harness h; setup(h); message1(h);
        std::vector<uint8_t> incoming(fixture_msg3,fixture_msg3+sizeof(fixture_msg3));
        if(scenario==0) incoming[14+81]^=1; // wrong MIC
        if(scenario==1) incoming.assign(fixture_msg3_bad_rsn,fixture_msg3_bad_rsn+sizeof(fixture_msg3_bad_rsn));
        if(scenario==2) incoming.assign(fixture_msg3_plain_gtk,fixture_msg3_plain_gtk+sizeof(fixture_msg3_plain_gtk));
        if(scenario==3) h.key_result=EIO;
        if(scenario==4) h.key_result=EBUSY;
        if(scenario==5) incoming.assign(fixture_msg3_bad_wrap,fixture_msg3_bad_wrap+sizeof(fixture_msg3_bad_wrap));
        if(scenario==6) incoming.assign(fixture_msg3_wrong_nonce,fixture_msg3_wrong_nonce+sizeof(fixture_msg3_wrong_nonce));
        if(scenario==7) { h.node.ni_replaycnt_ok=1; h.node.ni_replaycnt=2; }
        if(scenario==8) incoming.resize(incoming.size()-1);
        receive(h,incoming.data(),incoming.size());
        require(!h.node.ni_port_valid && !h.link_ups,"invalid/deferred handshake keeps controlled port closed");
        if(scenario<3 || scenario>=5) require(h.installed.empty() && h.output.size()==1,"invalid MIC/replay/key data produces no Message4 or installation");
        if(scenario==0) require(h.ic.ic_stats.is_rx_eapol_badmic==1,"wrong MIC accounted");
        if(scenario==1 || scenario==3) require(h.deauths==1 && h.state_changes==1,"RSN mismatch/key install error requests deauth and scan");
        if(scenario==7) require(h.ic.ic_stats.is_rx_eapol_replay==1,"replayed Message3 before authorization fails closed");
        teardown(h);
    }
}
static void output_allocation_failure() {
    for(unsigned message=1;message<=3;message+=2) {
        Harness h; setup(h);
        if(message==3) message1(h);
        mbuf_t input=packet(message==1 ? fixture_msg1 : fixture_msg3,
                            message==1 ? sizeof(fixture_msg1) : sizeof(fixture_msg3));
        size_t responses=h.output.size();
        rtw_pae_fail_allocations=1;
        ieee80211_eapol_key_input(&h.ic,input,&h.node);
        require(rtw_pae_fail_allocations==0,"real EAPOL response allocation failure exercised");
        require(h.output.size()==responses && h.installed.empty() && !h.node.ni_port_valid && !h.link_ups,"response allocation failure keeps port closed and installs no keys");
        teardown(h);
    }
}
int main() {
    cleanup_session(); successful_exchange(); rejected_messages(); output_allocation_failure();
    printf("PASS actual imported WPA2 PAE exchange, rejection, rekey and RSN teardown (%u assertions)\n",checks);
}
