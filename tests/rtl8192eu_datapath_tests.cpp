#include "rtw/RTL8192EUDataPath.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); std::exit(1); } } while(0)
static uint32_t le32(const uint8_t* p) { return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24); }
int main() {
    CHECK(rtw_92e_beacon_timeout_ms(0)==2000);
    CHECK(rtw_92e_beacon_timeout_ms(100)==2000);
    CHECK(rtw_92e_beacon_timeout_ms(200)==2048);
    CHECK(rtw_92e_beacon_timeout_ms(201)==2059);
    CHECK(rtw_92e_beacon_timeout_ms(65535)==10000);
    CHECK(!rtw_92e_beacon_expired(2999,1000,100));
    CHECK(rtw_92e_beacon_expired(3000,1000,100));
    CHECK(!rtw_92e_beacon_expired(3047,1000,200));
    CHECK(rtw_92e_beacon_expired(3048,1000,200));
    CHECK(rtw_92e_beacon_expired(999,1000,100));
    CHECK(!rtw_92e_beacon_expired(UINT64_MAX,UINT64_MAX-1999,100));
    CHECK(rtw_92e_beacon_expired(UINT64_MAX,UINT64_MAX-2000,100));
    CHECK(rtw_92e_beacon_expired(UINT64_MAX,0,65535));
    rtw_transfer_ledger l{}; rtw_transfer_reset(&l);
    CHECK(rtw_transfer_drained(&l));
    CHECK(rtw_transfer_begin(&l,0)); CHECK(!rtw_transfer_begin(&l,0));
    CHECK(!rtw_transfer_take(&l,0));
    CHECK(rtw_transfer_begin(&l,15)); CHECK(!rtw_transfer_begin(&l,16));
    rtw_transfer_stop(&l); CHECK(!rtw_transfer_begin(&l,1));
    CHECK(!rtw_transfer_restart(&l)); CHECK(!rtw_transfer_drained(&l));
    CHECK(rtw_transfer_complete(&l,15)); CHECK(!rtw_transfer_complete(&l,15));
    CHECK(!rtw_transfer_complete(&l,1)); CHECK(rtw_transfer_take(&l,15));
    CHECK(!rtw_transfer_drained(&l)); CHECK(rtw_transfer_complete(&l,0));
    CHECK(rtw_transfer_take(&l,0)); CHECK(!rtw_transfer_take(&l,0));
    CHECK(rtw_transfer_drained(&l)); CHECK(rtw_transfer_restart(&l));
    CHECK(rtw_transfer_begin(&l,0)); CHECK(rtw_transfer_complete(&l,0));
    CHECK(rtw_transfer_take(&l,0));
    CHECK(!rtw_transfer_begin(nullptr,0)); CHECK(!rtw_transfer_drained(nullptr));
    /* A pending TX before RX cannot block RX selection, and a completed
     * later RX cannot pass its incomplete oldest submission. */
    rtw_transfer_reset(&l);unsigned slot=99;
    CHECK(rtw_transfer_begin(&l,15));
    CHECK(rtw_transfer_begin(&l,2));CHECK(rtw_transfer_begin(&l,0));
    CHECK(rtw_transfer_complete(&l,0));
    CHECK(!rtw_transfer_oldest_ready(&l,0x0f));
    CHECK(!rtw_transfer_take_oldest(&l,0x0f,&slot));CHECK(slot==99);
    CHECK(rtw_transfer_complete(&l,2));
    CHECK(rtw_transfer_oldest_ready(&l,0x0f));
    CHECK(!rtw_transfer_take_oldest(&l,0x0f,nullptr));
    CHECK(rtw_transfer_take_oldest(&l,0x0f,&slot));CHECK(slot==2);
    CHECK(rtw_transfer_take_oldest(&l,0x0f,&slot));CHECK(slot==0);
    CHECK(!rtw_transfer_oldest_ready(&l,0x0f));
    CHECK(!rtw_transfer_take_oldest(&l,0,&slot));
    CHECK(!rtw_transfer_oldest_ready(nullptr,~0U));
    CHECK(!rtw_transfer_take_oldest(nullptr,~0U,&slot));
    CHECK(rtw_transfer_complete(&l,15));CHECK(rtw_transfer_take(&l,15));
    CHECK(rtw_transfer_drained(&l));
    /* Full queues and repeated reuse cannot append duplicate submissions or
     * retain reclaimed slots across stop/restart. */
    for(unsigned cycle=0;cycle<32;cycle++){
        for(unsigned i=0;i<RTW_TRANSFER_SLOTS;i++)CHECK(rtw_transfer_begin(&l,i));
        CHECK(!rtw_transfer_begin(&l,0));
        CHECK(!rtw_transfer_begin(&l,RTW_TRANSFER_SLOTS));
        for(unsigned i=RTW_TRANSFER_SLOTS;i>0;i--)CHECK(rtw_transfer_complete(&l,i-1));
        CHECK(!rtw_transfer_complete(&l,0));
        rtw_transfer_stop(&l);CHECK(!rtw_transfer_restart(&l));
        for(unsigned i=0;i<RTW_TRANSFER_SLOTS;i++){
            CHECK(rtw_transfer_take_oldest(&l,~0U,&slot));CHECK(slot==i);
        }
        CHECK(rtw_transfer_drained(&l));CHECK(rtw_transfer_restart(&l));
        CHECK(!rtw_transfer_take_oldest(&l,~0U,&slot));
    }
    std::array<uint8_t,40> d{};
    CHECK(rtw_92e_make_tx_descriptor(60,1,0,0,4,0x123,d.data()));
    CHECK(le32(d.data())==0x8c28003cU);
    CHECK(le32(d.data()+4)==0x00060000U);
    CHECK(le32(d.data()+8)==0x00010000U);
    CHECK(le32(d.data()+12)==0x00000100U);
    CHECK(le32(d.data()+16)==0x08001f04U);
    CHECK(le32(d.data()+32)==0 && le32(d.data()+36)==0x00123000U);
    unsigned sum=0; for(unsigned i=0;i<32;i+=2) sum^=unsigned(d[i])|(unsigned(d[i+1])<<8);
    CHECK(sum==0); CHECK((le32(d.data()+4)&0x00c00000U)==0);
    CHECK(rtw_92e_make_tx_descriptor(32,0,1,1,11,4095,d.data()));
    CHECK(le32(d.data())==0x8d280020U);
    CHECK(le32(d.data()+4)==0x00081200U);
    CHECK(le32(d.data()+16)==0x00001f00U);
    CHECK(le32(d.data()+36)==0x00fff000U);
    CHECK(rtw_92e_tx_protection(1,d.data()));
    CHECK(le32(d.data()+12)==0x3100U);
    CHECK(rtw_92e_tx_protection(2,d.data()));
    /* Linux rtl8xxxu fill_txdesc_v2 and Realtek92E fill_txdesc_vcs:
     * CTS-to-self is selected without enabling the hardware RTS engine. */
    CHECK(le32(d.data()+12)==0x0900U);
    CHECK((le32(d.data()+12)&0x2000U)==0);
    CHECK(rtw_92e_tx_protection(0,d.data()));
    CHECK(le32(d.data()+12)==0x0100U);
    sum=0; for(unsigned i=0;i<32;i+=2) sum^=unsigned(d[i])|(unsigned(d[i+1])<<8);
    CHECK(sum==0);
    CHECK(!rtw_92e_tx_protection(3,d.data()));
    CHECK(!rtw_92e_tx_protection(0,nullptr));
    d.fill(0xa5);
    CHECK(!rtw_92e_make_tx_descriptor(0,1,0,0,0,0,d.data()));
    CHECK(!rtw_92e_make_tx_descriptor(4097,1,0,0,0,0,d.data()));
    CHECK(!rtw_92e_make_tx_descriptor(24,1,1,0,0,0,d.data()));
    CHECK(!rtw_92e_make_tx_descriptor(24,1,0,0,12,0,d.data()));
    CHECK(!rtw_92e_make_tx_descriptor(24,1,0,0,0,4096,d.data()));
    CHECK(!rtw_92e_make_tx_descriptor(24,1,0,0,0,0,nullptr));
    CHECK(d[0]==0xa5);
    std::printf("92e datapath: %u checks passed\n",checks);
}
