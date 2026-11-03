/* SPDX-License-Identifier: GPL-2.0-only
 * Generic protocol headers and opaque bitmap units only. No captured data,
 * radio/USB I/O, ACK generation or reorder state is present in this harness. */
#include "rx_control_accounting_actual.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>

static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { \
    std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); std::exit(1); } } while(0)

/* Baseline RED must compile against build62 and fail its real error count,
 * rather than fail to compile because the new diagnostics are absent. */
template<class T> static auto filtered(const T&d,int) -> decltype(d.blockAckFiltered) { return d.blockAckFiltered; }
template<class T> static uint64_t filtered(const T&,long) { return 0; }
template<class T> static auto subtypeCount(const T&d,unsigned n,int) -> decltype(d.rejectedControlSubtypes[n]) { return d.rejectedControlSubtypes[n]; }
template<class T> static uint64_t subtypeCount(const T&,unsigned,long) { return 0; }
template<class T> static auto lastBAControl(const T&d,int) -> decltype(d.rejectLastBAControl) { return d.rejectLastBAControl; }
template<class T> static uint64_t lastBAControl(const T&,long) { return 65535; }
template<class T> static auto lastBAFragment(const T&d,int) -> decltype(d.rejectLastBAFragment) { return d.rejectLastBAFragment; }
template<class T> static uint64_t lastBAFragment(const T&,long) { return 255; }

static const rtw_wifi_family families[]={RTW_WIFI_RTL8192EU,RTW_WIFI_RTL8192CU,
    RTW_WIFI_RTL8188EU,RTW_WIFI_RTL8192DU,RTW_WIFI_RTL8192FU,RTW_WIFI_RTL8710BU,
    RTW_WIFI_RTL8723BU,RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU,RTW_WIFI_RTL8814AU,
    RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU};
static std::vector<uint8_t> ba(const ControllerHarness&c,bool compressed=true,unsigned tid=0,bool policy=false) {
    std::vector<uint8_t>b(compressed?28:148,0x5a);
    for(unsigned i=0;i<20;++i)b[i]=0;
    b[0]=0x94;std::memcpy(b.data()+4,c.core_.sc_ic.ic_myaddr,6);
    std::memcpy(b.data()+10,c.node_.ni_bssid,6);
    b[16]=uint8_t((compressed?4:0)|(policy?1:0));b[17]=uint8_t(tid<<4);
    b[19]=0x12; // ordinary sequence; fragment nibble remains zero
    return b;
}
static RXControlFrame frame(const std::vector<uint8_t>&b) {
    RXControlFrame f;f.payload.mpdu=b.data();f.payload.mpdu_length=b.size();return f;
}
static uint64_t reasons(const ControllerHarness&c) {
    uint64_t n=0;for(unsigned i=1;i<RTW_RX_REJECT_REASON_COUNT;++i)n+=c.rxDiag_.rejectedReasons[i];return n;
}
static uint64_t controlSubtypes(const ControllerHarness&c) {
    uint64_t n=0;for(unsigned i=0;i<16;++i)n+=subtypeCount(c.rxDiag_,i,0);return n;
}
static void noErrors(const ControllerHarness&c) {
    CHECK(c.rxErrors_==0);CHECK(c.statsStorage_.inputErrors==0);
    CHECK(c.rxDiag_.invalidFrameDrops==0&&reasons(c)==0);
}
static void rejectedOnce(const ControllerHarness&c) {
    CHECK(c.rxErrors_==1&&c.statsStorage_.inputErrors==1);
    CHECK(filtered(c.rxDiag_,0)==0);
    CHECK(controlSubtypes(c)==c.rxDiag_.rejectedReasons[RTW_RX_REJECT_CONTROL]);
}
static void ordinaryBAIsConsumed() {
    for(auto family:families)for(bool compressed:{true,false})for(unsigned tid:{0U,5U,15U})for(bool policy:{false,true}) {
        ControllerHarness c(family);const auto b=ba(c,compressed,tid,policy);
        const bool forwarded=c.dispatch(frame(b),b.size()+56);
        CHECK(c.rxErrors_ == 0); // FIRST executable assertion: build62 RED
        CHECK(!forwarded);noErrors(c);
        CHECK(filtered(c.rxDiag_,0)==1);
        CHECK(subtypeCount(c.rxDiag_,9,0)==0);
        CHECK(c.rxDiag_.rejectLastReason==0);
        CHECK(c.rxDiag_.rejectLastType==255&&c.rxDiag_.rejectLastSubtype==255);
        CHECK(lastBAControl(c.rxDiag_,0)==65535&&lastBAFragment(c.rxDiag_,0)==255);
    }
}
static void priorityAndUnchangedTraffic() {
    for(auto family:families) {
        for(unsigned flags:{1U,2U,3U}) {
            ControllerHarness c(family);auto b=ba(c);auto f=frame(b);
            f.payload.crc_error=uint8_t(flags&1);f.payload.icv_error=uint8_t((flags>>1)&1);
            CHECK(!c.dispatch(f,84));rejectedOnce(c);
            CHECK(c.rxDiag_.crcDrops==((flags&1)?1U:0U));
            CHECK(c.rxDiag_.icvDrops==((flags&1)?0U:1U));
            CHECK(c.rxDiag_.invalidFrameDrops==0&&reasons(c)==0);
            CHECK(subtypeCount(c.rxDiag_,9,0)==0);
        }
        // An unexpected hardware-decrypt disposition cannot become a silent
        // successful control consumption; retain one error at the old gate.
        for(uint8_t sw:{uint8_t(0),uint8_t(1)}) {
            ControllerHarness c(family);auto b=ba(c);auto f=frame(b);
            f.security_code=1;f.sw_decrypt_required=sw;
            CHECK(!c.dispatch(f,84));rejectedOnce(c);
            CHECK(c.rxDiag_.invalidFrameDrops==1&&c.rxDiag_.rejectedReasons[RTW_RX_REJECT_CONTROL]==1);
            CHECK(c.rxDiag_.hardwareDecryptDrops==0&&subtypeCount(c.rxDiag_,9,0)==1);
        }
        if(family!=RTW_WIFI_RTL8192EU) {
            ControllerHarness c(family);auto b=ba(c);auto f=frame(b);
            c.rateAllowed_=false;f.payload.rate_index=127;f.payload.crc_error=1;f.payload.icv_error=1;f.security_code=1;
            CHECK(!c.dispatch(f,84));rejectedOnce(c);
            CHECK(c.rxDiag_.invalidFrameDrops==1&&c.rxDiag_.rejectedReasons[RTW_RX_REJECT_RATE]==1);
            CHECK(c.rxDiag_.crcDrops==0&&c.rxDiag_.icvDrops==0);
        }
        // The BAR exception is still forwarded; data and management retain
        // the original admission and hardware-decrypt drop behavior.
        for(uint8_t type:{uint8_t(0x84),uint8_t(0x08),uint8_t(0x80)}) {
            ControllerHarness c(family);auto b=ba(c);b.resize(type==0x84?20:24);b[0]=type;
            CHECK(c.dispatch(frame(b),84));noErrors(c);CHECK(filtered(c.rxDiag_,0)==0);
            CHECK(subtypeCount(c.rxDiag_,type>>4,0)==0);
            if(type!=0x84) {
                ControllerHarness bad(family);auto f=frame(b);f.security_code=1;
                CHECK(!bad.dispatch(f,84));rejectedOnce(bad);
                CHECK(bad.rxDiag_.hardwareDecryptDrops==1&&bad.rxDiag_.invalidFrameDrops==0);
            }
        }
    }
}
static void malformedOrContextlessControlsRemainErrors() {
    for(auto family:families)for(unsigned kind=0;kind<13;++kind) {
        ControllerHarness c(family);auto b=ba(c);
        switch(kind) {
        case 0:c.auth_=false;break;
        case 1:c.ht_=false;break;
        case 2:c.core_.sc_ic.ic_bss=nullptr;break;
        case 3:b[4]^=1;break;
        case 4:b[10]^=1;break;
        case 5:b[1]=0x40;break;
        case 6:b[16]|=2;break; // multi-TID is outside this narrow exception
        case 7:b[16]|=8;break; // reserved BA-control bit
        case 8:b[18]=1;break; // a fragmented SSC is not ordinary single-TID BA
        case 9:b.resize(27);break;
        case 10:b.resize(29);break;
        case 11:b.resize(148);break; // compressed flag and bitmap size disagree
        case 12:b[0]=0x95;break;
        }
        CHECK(!c.dispatch(frame(b),84));rejectedOnce(c);
        CHECK(c.rxDiag_.invalidFrameDrops==1&&reasons(c)==1);
        // Subtype history belongs to reason CONTROL; a wrong version retains
        // its distinct VERSION error and must not inflate that control sum.
        CHECK(subtypeCount(c.rxDiag_,9,0)==(kind==12?0U:1U));
        if(kind==12)CHECK(c.rxDiag_.rejectedReasons[RTW_RX_REJECT_VERSION]==1&&
                         c.rxDiag_.rejectedReasons[RTW_RX_REJECT_CONTROL]==0);
        if(kind==8) {CHECK(lastBAControl(c.rxDiag_,0)==4);CHECK(lastBAFragment(c.rxDiag_,0)==1);}
    }
    for(auto family:families) {
        ControllerHarness c(family);auto b=ba(c);b.resize(10);b[0]=0xd4;
        CHECK(!c.dispatch(frame(b),66));rejectedOnce(c);
        CHECK(c.rxDiag_.rejectedReasons[RTW_RX_REJECT_CONTROL]==1);
        CHECK(subtypeCount(c.rxDiag_,13,0)==1);
        CHECK(lastBAControl(c.rxDiag_,0)==65535&&lastBAFragment(c.rxDiag_,0)==255);
    }
}
static void boundedMetadataAndCounterSeparation() {
    for(auto family:families)for(size_t n:{size_t(0),size_t(1),size_t(2),size_t(9),size_t(10),size_t(15),size_t(16),size_t(17),size_t(18),size_t(19)}) {
        ControllerHarness c(family);const auto full=ba(c);
        // Construct a fresh exact-sized allocation: shrinking the original
        // vector would leave accessible backing storage beyond the wire end.
        const std::vector<uint8_t>b(full.begin(),full.begin()+n);
        CHECK(!c.dispatch(frame(b),n+56));rejectedOnce(c);
        CHECK(reasons(c)==1);
        if(n<18)CHECK(lastBAControl(c.rxDiag_,0)==65535);
        if(n<20)CHECK(lastBAFragment(c.rxDiag_,0)==255);
    }
    for(auto family:families) {
        ControllerHarness c(family);auto b=ba(c);
        CHECK(!c.dispatch(frame(b),84)); // accounted by consume-only counter
        b[1]=0x40;CHECK(!c.dispatch(frame(b),84));
        CHECK(filtered(c.rxDiag_,0)==1&&c.rxErrors_==1&&c.statsStorage_.inputErrors==1);
        CHECK(c.rxDiag_.invalidFrameDrops==1&&reasons(c)==1&&subtypeCount(c.rxDiag_,9,0)==1);
        b[1]=0;CHECK(!c.dispatch(frame(b),84));
        CHECK(filtered(c.rxDiag_,0)==2&&c.rxErrors_==1&&c.statsStorage_.inputErrors==1);
        CHECK(c.rxDiag_.rejectLastFlags==0x40&&subtypeCount(c.rxDiag_,9,0)==1);
    }
}
int main() {
    ordinaryBAIsConsumed();priorityAndUnchangedTraffic();
    malformedOrContextlessControlsRemainErrors();boundedMetadataAndCounterSeparation();
    CHECK(sizeof(families)/sizeof(families[0])==13);
    std::printf("RX control accounting: %u checks across13 station families passed; actual receive block, scalar census and unchanged error priority\n",checks);
}
