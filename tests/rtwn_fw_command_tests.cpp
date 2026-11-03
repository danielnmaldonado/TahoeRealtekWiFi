#include "rtwn_fw_command_host.hpp"
#include <array>
#include <climits>
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); std::exit(1); } } while (0)
int rtwn_io_error(rtwn_softc *s) { return s->core_error; }
uint8_t rtwn_read_1(rtwn_softc *s, uint16_t address) {
    CHECK(address == R92C_HMETFR);
    ++s->reads;
    if (s->reads == s->fail_read_at) { s->core_error = s->fault_error; return 0; }
    return s->reads <= s->busy_reads ? uint8_t(1U << s->fwcur) : 0;
}
static void write(rtwn_softc *s, unsigned width, uint16_t address, uint32_t value) {
    s->writes.push_back({width,address,value});
    if (s->writes.size() == s->fail_write_at) s->core_error = s->fault_error;
}
void rtwn_write_2(rtwn_softc *s, uint16_t address, uint16_t value) { write(s,2,address,value); }
void rtwn_write_4(rtwn_softc *s, uint16_t address, uint32_t value) { write(s,4,address,value); }
void rtwn_delay(rtwn_softc *s, uint32_t value) { s->delays.push_back(value); }
void rtwn_temp_calib(rtwn_softc*s){++s->thermalCalls;}
static void scheduleCalib(void*cookie){++static_cast<rtwn_softc*>(cookie)->scheduled;}
static void expected(const FWWrite &w, unsigned width, uint16_t address, uint32_t value) {
    CHECK(w.width == width); CHECK(w.address == address); CHECK(w.value == value);
}
static void unchanged(const rtwn_softc &s, int index) {
    CHECK(s.fwcur == index); CHECK(s.reads == 0); CHECK(s.writes.empty()); CHECK(s.delays.empty());
}
int main() {
    /* Public 92E RSSI_REPORT layout: MACID, reserved, PWDB, reserved.
     * The nonzero PWDB byte reproduces the former byte-dereference loss. */
    { rtwn_softc s; uint8_t rssi[]{0,0,57,0};
      CHECK(rtwn_fw_cmd(&s,0x42,rssi,sizeof(rssi)) == 0);
      CHECK(s.writes.size() == 2);
      expected(s.writes[0],4,0x1f0,0); expected(s.writes[1],4,0x1d0,0x39000042);
      CHECK(s.fwcur == 1); }
    /* Public gen2 MACID_CFG_RAID payload, 7 bytes; ID is 0x40, never 0xc0. */
    { rtwn_softc s; s.fwcur=3; const uint8_t mask[]{0,6,0,0xff,0x0f,0,0};
      CHECK(rtwn_fw_cmd(&s,0x40,mask,sizeof(mask)) == 0);
      CHECK(s.writes.size() == 2);
      expected(s.writes[0],4,0x1fc,0x00000fff); expected(s.writes[1],4,0x1dc,0x00060040);
      CHECK(s.fwcur == 0); }
    /* Every supported payload length, all mailbox indices and unaligned input. */
    for (unsigned gen2=0; gen2<2; ++gen2) for(int index=0; index<4; ++index)
      for (int length=0; length <= (gen2?7:5); ++length) {
        rtwn_softc s; s.chip=gen2?RTWN_CHIP_92E:0; s.fwcur=index;
        uint8_t storage[]{0xee,0x11,0x22,0x33,0x44,0x55,0x66,0x77};
        const uint8_t *payload=storage+1;
        uint8_t message[8]{}; message[0]=uint8_t(0x42|(!gen2&&length>3?0x80:0));
        std::memcpy(message+1,payload,size_t(length));
        CHECK(rtwn_fw_cmd(&s,0x42,length?payload:nullptr,length) == 0);
        CHECK(s.writes.size() == (length>3?2U:1U));
        uint32_t low=uint32_t(message[0])|(uint32_t(message[1])<<8)|(uint32_t(message[2])<<16)|(uint32_t(message[3])<<24);
        expected(s.writes.back(),4,uint16_t(0x1d0+index*4),low);
        if(length>3) {
          uint32_t high=uint32_t(message[4])|(uint32_t(message[5])<<8)|(uint32_t(message[6])<<16)|(uint32_t(message[7])<<24);
          expected(s.writes[0],gen2?4:2,uint16_t(gen2?0x1f0+index*4:0x88+index*2),high); }
        CHECK(s.fwcur == (index+1)%4);
      }
    const uint8_t payload[]{1,2,3,4,5,6,7,8};
    for (unsigned gen2=0;gen2<2;++gen2) for(int length : {-1,gen2?8:6,INT_MAX}) {
        rtwn_softc s; s.chip=gen2?RTWN_CHIP_92E:0;
        CHECK(rtwn_fw_cmd(&s,0x42,payload,length)==EINVAL); unchanged(s,0);
    }
    for(int index : {-1,4,INT_MAX}) { rtwn_softc s; s.fwcur=index;
      CHECK(rtwn_fw_cmd(&s,0x42,payload,1)==EINVAL); unchanged(s,index); }
    { rtwn_softc s; CHECK(rtwn_fw_cmd(&s,0x42,nullptr,1)==EINVAL); unchanged(s,0); }
    CHECK(rtwn_fw_cmd(nullptr,0x42,payload,1)==EINVAL);
    { rtwn_softc s; s.core_error=EIO; CHECK(rtwn_fw_cmd(&s,0x42,payload,1)==EIO); unchanged(s,0); }
    { rtwn_softc s; s.busy_reads=2; CHECK(rtwn_fw_cmd(&s,0x42,payload,1)==0);
      CHECK(s.reads==3); CHECK(s.delays.size()==2); CHECK(s.delays[0]==10); }
    { rtwn_softc s; s.busy_reads=100; CHECK(rtwn_fw_cmd(&s,0x42,payload,1)==ETIMEDOUT);
      CHECK(s.reads==100); CHECK(s.writes.empty()); CHECK(s.fwcur==0); CHECK(s.core_error==0);
      CHECK(rtwn_fw_cmd(&s,0x42,payload,1)==0&&s.fwcur==1); }
    // CU/88EU use gen1 and EU gen2. Their existing calibration path already
    // treats firmware mailbox ownership as optional, without latching it.
    for(unsigned chip:{0U,RTWN_CHIP_92E}){rtwn_softc s;s.chip=chip;s.busy_reads=100;
      s.sc_ops={&s,scheduleCalib};rtwn_calib(&s);
      CHECK(!s.core_error&&s.fwcur==0&&s.writes.empty()&&s.thermalCalls==1&&s.scheduled==1);
      rtwn_calib(&s);CHECK(!s.core_error&&s.fwcur==1&&s.thermalCalls==2&&s.scheduled==2);
      for(int error:{EIO,ETIMEDOUT,EBUSY})for(bool read:{false,true}){
       rtwn_softc bad;bad.chip=chip;bad.fault_error=error;if(read)bad.fail_read_at=1;else bad.fail_write_at=1;
       bad.sc_ops={&bad,scheduleCalib};rtwn_calib(&bad);
       CHECK(bad.core_error==error&&bad.fwcur==0);if(read)CHECK(bad.writes.empty());}}
    { rtwn_softc s; s.fail_read_at=1; CHECK(rtwn_fw_cmd(&s,0x42,payload,1)==EIO);
      CHECK(s.writes.empty()); CHECK(s.fwcur==0); }
    { rtwn_softc s; s.fail_write_at=1; CHECK(rtwn_fw_cmd(&s,0x42,payload,7)==EIO);
      CHECK(s.writes.size()==1); CHECK(s.writes[0].address==0x1f0); CHECK(s.fwcur==0); }
    { rtwn_softc s; s.fail_write_at=2; CHECK(rtwn_fw_cmd(&s,0x42,payload,7)==EIO);
      CHECK(s.writes.size()==2); CHECK(s.fwcur==0); }
    { rtwn_softc s; s.chip=RTWN_CHIP_PCI; CHECK(rtwn_fw_cmd(&s,5,payload,3)==0);
      CHECK(s.delays.size()==1); CHECK(s.delays[0]==2000); }
    std::printf("firmware mailbox: %u checks passed (actual core function, offline trace)\n",checks);
}
