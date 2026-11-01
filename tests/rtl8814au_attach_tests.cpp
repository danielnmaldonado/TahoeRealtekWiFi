/* SPDX-License-Identifier: BSD-3-Clause */
/* Compile the real attach prefix, context initializer and PHY precondition.
 * USB OTP reads and allocation are boundaries; no RF/USB operation occurs. */
#include "rtw/RTL8814AU.h"
#include "rtw/RTL8814AUPHY.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static unsigned checks,board_reads,allocations,phy_calls;
static int board_error;
static bool allocation_error;
static uint8_t physical_streams=2;
static rtw_8814au_speed configured_speed=RTW_8814AU_HIGH_SPEED;
#define CHECK(x) do {++checks;if(!(x)){std::fprintf(stderr,"attach line %d: %s\n",__LINE__,#x);std::exit(1);}} while(0)
static void *IOMalloc(size_t size) {++allocations;return allocation_error?nullptr:std::malloc(size);}
#define bzero(p,size) std::memset(p,0,size)
class RTWUSBController {
public:
    rtw_8814au acCore_{};
    void *acPHYStorage_=nullptr;
    size_t acPHYSize_=0;
    ~RTWUSBController(){std::free(acPHYStorage_);}
    static int acRead(void *,uint16_t,unsigned,uint32_t *) {std::abort();}
    static int acWrite(void *,uint16_t,unsigned,uint32_t) {std::abort();}
    static int acBulk(void *,uint8_t,const uint8_t *,size_t) {std::abort();}
    static void delayUS(void *,uint32_t) {std::abort();}
    rtw_8814au_speed acSpeed()const{return configured_speed;}
    int attach8814();
};
/* ACTUAL_CONTEXT_INITIALIZER */
/* ACTUAL_PHY_BOARD_PRECONDITION */
extern "C" size_t rtw_8814au_phy_size(void) {return 256;}
extern "C" int rtw_8814au_phy_init(rtw_8814au *c,void *storage,size_t size) {
    ++phy_calls;
    CHECK(storage && size==256);
    // This exact production precondition must not be weakened to make attach
    // succeed. Full PHY initialization belongs after power/FW/MAC in start.
    return validate_board(c);
}
extern "C" int rtw_8814au_identify_read_board(rtw_8814au *c,rtw_8814au_speed speed) {
    ++board_reads;CHECK(speed==configured_speed);
    if(board_error)return board_error;
    c->identified=c->board_valid=1;c->board.physical_rf_paths=4;
    c->board.tx_streams=c->board.rx_streams=physical_streams;c->board.cut=1;c->board.rfe_option=1;
    c->board.antenna_tx=c->board.antenna_rx=physical_streams==3?7:3;
    c->speed=speed;c->board.logical[0xc9]=0xee;
    c->board.mac[0]=2;c->board.mac[5]=7;
    return 0;
}
/* ACTUAL_ATTACH_PREFIX */
static void reset() {board_reads=allocations=phy_calls=0;board_error=0;allocation_error=false;physical_streams=2;configured_speed=RTW_8814AU_HIGH_SPEED;}
int main() {
    reset();
    {RTWUSBController c;
        CHECK(c.attach8814()==0);
        CHECK(board_reads==1 && allocations==1 && phy_calls==0);
        CHECK(c.acCore_.identified && c.acCore_.board_valid && c.acPHYStorage_ && c.acPHYSize_==256);
        CHECK(!c.acCore_.powered && !c.acCore_.fw_running && !c.acCore_.mac_initialized && !c.acCore_.phy_initialized);
        CHECK(c.acCore_.bus.cookie==&c && c.acCore_.board.mac[5]==7);
        CHECK(validate_board(&c.acCore_)==EINVAL);
        c.acCore_.powered=c.acCore_.fw_running=c.acCore_.mac_initialized=1;
        CHECK(validate_board(&c.acCore_)==0);
    }
    reset();board_error=EIO;
    {RTWUSBController c;CHECK(c.attach8814()==EIO);CHECK(board_reads==1 && !allocations && !phy_calls && !c.acPHYStorage_);}
    reset();allocation_error=true;
    {RTWUSBController c;CHECK(c.attach8814()==ENOMEM);CHECK(board_reads==1 && allocations==1 && !phy_calls && !c.acPHYStorage_);}
    reset();physical_streams=3;configured_speed=RTW_8814AU_SUPER_SPEED;
    {RTWUSBController c;CHECK(c.attach8814()==0);
        CHECK(c.acCore_.board.tx_streams==2 && c.acCore_.board.rx_streams==2);
        CHECK(c.acCore_.board.antenna_tx==3 && c.acCore_.board.antenna_rx==3);
        CHECK(c.acCore_.board.physical_rf_paths==4 && c.acCore_.board.logical[0xc9]==0xee && c.acCore_.speed==RTW_8814AU_SUPER_SPEED);
        c.acCore_.powered=c.acCore_.fw_running=c.acCore_.mac_initialized=1;CHECK(validate_board(&c.acCore_)==0);
    }
    std::printf("RTL8814AU attach prefix: %u checks passed (actual initializer, attach and PHY precondition)\n",checks);
}
