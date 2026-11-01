#include "rtw/Chip.h"
#include "rtw/USBBulk.h"
#include "rtw/Efuse.h"
#include "rtw/Firmware.h"
#include "rtw/USBPackets.h"
#include "rtw/USBRegisterIO.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <vector>

namespace {
unsigned long assertions=0;
#define CHECK(c) do { ++assertions; if (!(c)) { std::cerr << "FAIL " << __FILE__ << ':' << __LINE__ << ": " << #c << '\n'; std::exit(1); } } while(false)

void testChip() {
    rtw_chip_info info{};
    CHECK(rtw_chip_decode_syscfg(0x04411137,&info));
    CHECK(info.sys_cfg==0x04411137 && info.cut==1);
    CHECK(info.is_usb && !info.is_umc && !info.is_test_chip);
    CHECK(!rtw_chip_decode_syscfg(0,&info) && info.sys_cfg==0);
    CHECK(!rtw_chip_decode_syscfg(0xffffffffU,&info));
    CHECK(!rtw_chip_decode_syscfg(0x04411137,nullptr));
    CHECK(rtw_chip_decode_syscfg(0x00880001,&info));
    CHECK(info.is_umc && info.is_test_chip && !info.is_usb);
}

struct FakeUSB {
    std::array<std::uint8_t,65536> regs{};
    std::array<std::uint8_t,RTW_EFUSE_BYTES> physical{};
    bool access=false;
    bool stuck=false;
    unsigned reads=0;
    unsigned writes=0;
    unsigned fail_write_at=0;
    unsigned fail_read_at=0;
    unsigned last_offset=0;
};
rtw_usb_error fakeIn(void* context,const rtw_usb_setup* req,
                       std::uint8_t* data,std::uint16_t* actual) {
    auto& f=*static_cast<FakeUSB*>(context);
    ++f.reads;
    if (f.fail_read_at && f.reads == f.fail_read_at) return RTW_USB_IO_ERROR;
    if (!req || !data || !actual || req->bm_request_type != 0xC0 ||
        req->b_request != 5 || req->w_index != 0 || req->w_length > 4)
        return RTW_USB_INVALID_ARGUMENT;
    if (static_cast<unsigned>(req->w_value)+req->w_length > f.regs.size())
        return RTW_USB_INVALID_REGISTER;
    for(unsigned i=0;i<req->w_length;++i) data[i]=f.regs[req->w_value+i];
    if (req->w_value==0x30 && req->w_length==4 && f.access && !f.stuck) {
        data[0]=f.physical[f.last_offset];
        data[3]=0x80;
    }
    *actual=req->w_length;
    return RTW_USB_OK;
}
rtw_usb_error fakeOut(void* context,const rtw_usb_setup* req,
                        const std::uint8_t* data,std::uint16_t* actual) {
    auto& f=*static_cast<FakeUSB*>(context);
    ++f.writes;
    if (f.fail_write_at && f.writes == f.fail_write_at) return RTW_USB_IO_ERROR;
    if (!req || !data || !actual || req->bm_request_type != 0x40 ||
        req->b_request != 5 || req->w_index != 0 || !req->w_length ||
        req->w_length > RTW_USB_MAX_VENDOR_PAYLOAD ||
        static_cast<unsigned>(req->w_value)+req->w_length > f.regs.size())
        return RTW_USB_INVALID_ARGUMENT;
    for(unsigned i=0;i<req->w_length;++i) f.regs[req->w_value+i]=data[i];
    if (req->w_value==0xcf && req->w_length==1) f.access=(data[0]==0x69);
    if (req->w_value==0x33 && req->w_length==1 && !(data[0]&0x80))
        f.last_offset=static_cast<unsigned>(f.regs[0x31]) |
                      ((static_cast<unsigned>(f.regs[0x32])&3U)<<8U);
    *actual=req->w_length;
    return RTW_USB_OK;
}

void testWrite() {
    FakeUSB f{};
    rtw_usb_rw_transport bus{&f,fakeIn,fakeOut};
    uint32_t data=88;
    CHECK(rtw_usb_write_register(&bus,0x120,4,0x12345678)==RTW_USB_OK);
    CHECK(f.regs[0x120]==0x78 && f.regs[0x123]==0x12);
    CHECK(rtw_usb_rw_read(&bus,0x120,4,&data)==RTW_USB_OK && data==0x12345678);
    CHECK(rtw_usb_write_register(&bus,0x1,3,1)==RTW_USB_INVALID_ARGUMENT);
    CHECK(rtw_usb_write_register(&bus,0xffff,2,1)==RTW_USB_INVALID_REGISTER);
    std::array<uint8_t,255> buf{};
    CHECK(rtw_usb_write_block(&bus,0x100,buf.data(),255)==RTW_USB_INVALID_ARGUMENT);
    CHECK(rtw_usb_write_block(&bus,0xffff,buf.data(),2)==RTW_USB_INVALID_REGISTER);
    CHECK(rtw_usb_write_block(&bus,0,buf.data(),0)==RTW_USB_INVALID_ARGUMENT);
    CHECK(rtw_usb_write_block(&bus,0,buf.data(),254)==RTW_USB_OK);
    f.fail_write_at=f.writes+1U;
    CHECK(rtw_usb_write_register(&bus,0x100,1,0)==RTW_USB_IO_ERROR);
    CHECK(rtw_usb_rw_read(nullptr,0,1,&data)==RTW_USB_INVALID_ARGUMENT);
}

/* EFUSE packed records at offsets 0xC0, 0xC8, 0xD0, 0xD8.
   D0-D2 encode Realtek VID/PID; D7-DC encode a real-looking unicast MAC. */
std::vector<std::uint8_t> efuseFixture() {
    return {
        0x0f,0x3e, 0xff,0x23,             // block 24, word0 -> C1 board option
        0x2f,0x3c, 0x08,0x00, 0x03,0xff,  // block 25, words0..1 -> C8 plan, CA RFE
        0x4f,0x30, 0xda,0x0b, 0x12,0x88, 0xff,0xff, 0xff,0x02,
        0x6f,0x38, 0x11,0x22, 0x33,0x44, 0x55,0xff,
        0xff
    };
}
void testEfuseDecode() {
    const auto packed=efuseFixture();
    std::array<std::uint8_t,RTW_EFUSE_BYTES> map{};
    std::size_t count=111;
    CHECK(rtw_efuse_decode(packed.data(),packed.size(),map.data(),map.size(),&count)==RTW_EFUSE_OK);
    CHECK(count==10);
    CHECK(map[0xc1]==0x23 && map[0xc8]==8 && map[0xca]==3);
    rtw_efuse_identity identity{};
    CHECK(rtw_efuse_extract_identity(map.data(),map.size(),&identity)==RTW_EFUSE_OK);
    CHECK(identity.eeprom_vendor==0x0bda && identity.eeprom_product==0x8812);
    CHECK(identity.mac[0]==2 && identity.mac[1]==0x11 && identity.mac[5]==0x55);
    CHECK(identity.channel_plan==8 && identity.rfe_type==3 && identity.rf_board_option==0x23);
    CHECK(!identity.thermal_meter_valid && identity.thermal_meter==0xFFU);
    map[0xBAU]=0x18U;
    CHECK(rtw_efuse_extract_identity(map.data(),map.size(),&identity)==RTW_EFUSE_OK);
    CHECK(identity.thermal_meter_valid && identity.thermal_meter==24U);
    map[0xBAU]=0U;
    CHECK(rtw_efuse_extract_identity(map.data(),map.size(),&identity)==RTW_EFUSE_OK);
    CHECK(!identity.thermal_meter_valid);
    map[0xBAU]=63U;
    CHECK(rtw_efuse_extract_identity(map.data(),map.size(),&identity)==RTW_EFUSE_OK);
    CHECK(!identity.thermal_meter_valid && identity.thermal_meter==63U);
    const std::uint8_t brokenHeader[]={0x0f};
    CHECK(rtw_efuse_decode(brokenHeader,sizeof(brokenHeader),map.data(),map.size(),&count)==RTW_EFUSE_TRUNCATED);
    CHECK(map[0xd0]==0xff && count==0);
    const std::uint8_t truncatedWord[]={0x0e,0x12};
    CHECK(rtw_efuse_decode(truncatedWord,sizeof(truncatedWord),map.data(),map.size(),&count)==RTW_EFUSE_TRUNCATED);
    const std::uint8_t outOfRange[]={0x0f,0x8e,0x12,0x34};
    CHECK(rtw_efuse_decode(outOfRange,sizeof(outOfRange),map.data(),map.size(),&count)==RTW_EFUSE_OUT_OF_RANGE);
    CHECK(map[0xd0]==0xff);
    auto erased=packed; erased[0]=0xff;
    CHECK(rtw_efuse_decode(erased.data(),erased.size(),map.data(),map.size(),&count)==RTW_EFUSE_OK && count==0);
    CHECK(rtw_efuse_extract_identity(map.data(),map.size(),&identity)==RTW_EFUSE_INVALID_IDENTITY);
    CHECK(identity.mac[0]==0);
    CHECK(rtw_efuse_decode(nullptr,2,map.data(),map.size(),&count)==RTW_EFUSE_BAD_ARGUMENT);
    CHECK(rtw_efuse_decode(packed.data(),packed.size(),map.data(),300,&count)==RTW_EFUSE_BAD_ARGUMENT);
    CHECK(rtw_efuse_extract_identity(nullptr,0,&identity)==RTW_EFUSE_BAD_ARGUMENT);
}
void testEfuseDevice() {
    FakeUSB dev{};
    const auto packed=efuseFixture();
    dev.physical.fill(0xff);
    std::memcpy(dev.physical.data(),packed.data(),packed.size());
    rtw_usb_rw_transport bus{&dev,fakeIn,fakeOut};
    std::array<std::uint8_t,RTW_EFUSE_BYTES> physical{};
    CHECK(rtw_efuse_read_physical(&bus,physical.data(),physical.size(),2)==RTW_EFUSE_OK);
    CHECK(!dev.access && dev.regs[0xcf]==0);
    CHECK(std::memcmp(physical.data(),packed.data(),packed.size())==0);
    CHECK(physical[511]==0xff);
    std::array<std::uint8_t,RTW_EFUSE_BYTES> map{};
    CHECK(rtw_efuse_decode(physical.data(),physical.size(),map.data(),map.size(),nullptr)==RTW_EFUSE_OK);
    CHECK(map[0xd0]==0xda);
    dev.stuck=true;
    physical.fill(42);
    CHECK(rtw_efuse_read_physical(&bus,physical.data(),physical.size(),3)==RTW_EFUSE_TIMEOUT);
    CHECK(!dev.access && physical[0]==0 && physical[511]==0);
    dev.stuck=false;
    dev.fail_write_at=dev.writes+2U;
    CHECK(rtw_efuse_read_physical(&bus,physical.data(),physical.size(),3)==RTW_EFUSE_USB_ERROR);
    CHECK(!dev.access && physical[0]==0);
    dev.fail_write_at=0;
    CHECK(rtw_efuse_read_physical(&bus,physical.data(),physical.size(),0)==RTW_EFUSE_BAD_ARGUMENT);
    CHECK(rtw_efuse_read_physical(&bus,physical.data(),300,2)==RTW_EFUSE_BAD_ARGUMENT);
}

struct Chunks {
    const rtw_fw_image* image;
    std::vector<uint8_t> reconstructed;
    std::size_t calls=0;
    std::size_t abort_at=0;
};
int chunkSink(void* ctx,uint8_t page,uint16_t addr,const uint8_t* data,uint16_t n) {
    auto& c=*static_cast<Chunks*>(ctx);
    if(c.abort_at && c.calls==c.abort_at) return 0;
    const auto offset=static_cast<std::size_t>(page)*4096U+(addr-0x1000U);
    CHECK(page<8 && addr>=0x1000 && addr<0x2000);
    CHECK(n>=1 && n<=254 && addr+n<=0x2000);
    CHECK(offset==c.reconstructed.size());
    CHECK(data==c.image->payload+offset);
    c.reconstructed.insert(c.reconstructed.end(),data,data+n);
    ++c.calls;
    return 1;
}
void testFirmware() {
    std::vector<std::uint8_t> file(32+4096+257,0);
    file[0]=0x01;file[1]=0x95;file[4]=0x34;file[5]=0x12;file[6]=0x0e;
    for(std::size_t i=32;i<file.size();++i) file[i]=static_cast<uint8_t>((i*13U)^0x5aU);
    rtw_fw_image image{};
    CHECK(rtw_fw_inspect_8812au(file.data(),file.size(),&image)==RTW_FW_OK);
    CHECK(image.signature==0x9501 && image.version==0x1234 && image.subversion==0x0e);
    CHECK(image.payload_length==4353 && image.pages==2 && image.payload==file.data()+32);
    Chunks chunks{&image};
    std::size_t nc=0;
    CHECK(rtw_fw_plan_chunks(&image,chunkSink,&chunks,&nc)==RTW_FW_OK);
    CHECK(chunks.reconstructed.size()==4353 && nc==24 && chunks.calls==24);
    CHECK(std::memcmp(chunks.reconstructed.data(),image.payload,image.payload_length)==0);
    Chunks aborted{&image};aborted.abort_at=2;
    CHECK(rtw_fw_plan_chunks(&image,chunkSink,&aborted,&nc)==RTW_FW_CALLBACK_FAILED && nc==2);
    file[1]=0x21;
    CHECK(rtw_fw_inspect_8812au(file.data(),file.size(),&image)==RTW_FW_INVALID_IMAGE);
    CHECK(image.payload==nullptr && image.pages==0);
    file[1]=0x95;
    CHECK(rtw_fw_inspect_8812au(file.data(),8,&image)==RTW_FW_INVALID_IMAGE);
    CHECK(rtw_fw_inspect_8812au(nullptr,0,&image)==RTW_FW_BAD_ARGUMENT);
    file.resize(32+32769);
    CHECK(rtw_fw_inspect_8812au(file.data(),file.size(),&image)==RTW_FW_TOO_LARGE);
    CHECK(rtw_fw_plan_chunks(nullptr,chunkSink,&chunks,&nc)==RTW_FW_BAD_ARGUMENT);
}

void set32(std::vector<std::uint8_t>& v,std::size_t off,std::uint32_t word) {
    v[off]=static_cast<std::uint8_t>(word);
    v[off+1]=static_cast<std::uint8_t>(word>>8);
    v[off+2]=static_cast<std::uint8_t>(word>>16);
    v[off+3]=static_cast<std::uint8_t>(word>>24);
}
void testPackets() {
    std::vector<std::uint8_t> rx(88+56,0);
    /* frame 1: 40-byte MPDU, two 8-byte PHY words, 1-byte shift, CRC error */
    set32(rx,0,40|(1U<<14)|(2U<<16)|(1U<<24)|(1U<<26));
    set32(rx,12,4);set32(rx,16,0x10);set32(rx,20,0x12345678);
    for(unsigned i=0;i<40;++i) rx[24+16+1+i]=static_cast<uint8_t>(i+1);
    /* frame 2, 32-byte MPDU */
    set32(rx,88,32);set32(rx,88+12,5);
    rtw_rx_iterator it{};
    rtw_rx_iterator_init(&it,rx.data(),rx.size());
    rtw_rx_packet p{};
    CHECK(rtw_rx_next(&it,&p)==RTW_PACKETS_OK);
    CHECK(p.mpdu_length==40 && p.mpdu[0]==1 && p.mpdu[39]==40);
    CHECK(p.phy_info_length==16 && p.phy_info==rx.data()+24);
    CHECK(p.crc_error && !p.icv_error && p.phy_status_present);
    CHECK(p.rate_index==4 && p.bandwidth_code==1 && p.timestamp==0x12345678);
    CHECK(rtw_rx_next(&it,&p)==RTW_PACKETS_OK);
    CHECK(p.mpdu_length==32 && !p.crc_error && p.rate_index==5 && it.frames==2);
    CHECK(rtw_rx_next(&it,&p)==RTW_PACKETS_DONE);
    rx[88]=255;rx[89]=63; // pkt_len 16383 -> truncated
    rtw_rx_iterator_init(&it,rx.data(),rx.size());
    CHECK(rtw_rx_next(&it,&p)==RTW_PACKETS_OK);
    CHECK(rtw_rx_next(&it,&p)==RTW_PACKETS_TRUNCATED);
    rx[88]=0;rx[89]=0;rx[88+12]=1;
    rtw_rx_iterator_init(&it,rx.data()+88,56);
    CHECK(rtw_rx_next(&it,&p)==RTW_PACKETS_MALFORMED);
    std::array<std::uint8_t,RTW_TX_DESC_BYTES> tx{};
    CHECK(rtw_tx_make_descriptor(51,0,0x12,1,tx.data())==RTW_PACKETS_OK);
    CHECK(tx[0]==0x33 && tx[1]==0 && tx[2]==0x28 && tx[3]==0x8d);
    CHECK(tx[4]==1 && tx[5]==0x12 && tx[6]==8);
    CHECK(tx[12]==0 && tx[13]==1 && tx[16]==0 && tx[18]==0x1a);
    uint16_t checksum=0;
    for(unsigned i=0;i<32;i+=2) checksum^=static_cast<uint16_t>(tx[i]|(static_cast<unsigned>(tx[i+1])<<8));
    CHECK(checksum==0);
    CHECK(rtw_tx_make_descriptor(0,0,0x12,0,tx.data())==RTW_PACKETS_BAD_ARGUMENT);
    CHECK(rtw_tx_make_descriptor(65536,0,0x12,0,tx.data())==RTW_PACKETS_BAD_ARGUMENT);
    CHECK(rtw_tx_make_descriptor(5,128,0x12,0,tx.data())==RTW_PACKETS_BAD_ARGUMENT);
    CHECK(rtw_tx_make_descriptor(5,0,32,0,tx.data())==RTW_PACKETS_BAD_ARGUMENT);
    CHECK(rtw_tx_make_descriptor(5,0,0x12,5,tx.data())==RTW_PACKETS_BAD_ARGUMENT);
    std::size_t size=999;
    CHECK(rtw_tx_total_transfer_size(472,512,&size)==RTW_PACKETS_OK && size==513);
    CHECK(rtw_tx_total_transfer_size(473,512,&size)==RTW_PACKETS_OK && size==513);
    CHECK(rtw_tx_total_transfer_size(472,100,&size)==RTW_PACKETS_BAD_ARGUMENT && size==0);
    CHECK(rtw_tx_total_transfer_size(65536,512,&size)==RTW_PACKETS_BAD_ARGUMENT);
    CHECK(rtw_tx_total_transfer_size(1,512,nullptr)==RTW_PACKETS_BAD_ARGUMENT);
}

struct FakeBulk {
    std::vector<std::uint8_t> last_tx;
    std::vector<std::uint8_t> next_rx;
    std::size_t actual_override=0;
    unsigned read_count=0;
    unsigned write_count=0;
    bool fail=false;
};
int fakeBulkWrite(void* ctx, uint8_t ep, const uint8_t* bytes,
                  size_t length, size_t* sent) {
    auto& b=*static_cast<FakeBulk*>(ctx);
    ++b.write_count;
    if (b.fail || ep!=0x02) return 0;
    b.last_tx.assign(bytes,bytes+length);
    *sent=b.actual_override ? b.actual_override : length;
    return 1;
}
int fakeBulkRead(void* ctx, uint8_t ep, uint8_t* out,
                 size_t cap, size_t* actual) {
    auto& b=*static_cast<FakeBulk*>(ctx);
    ++b.read_count;
    if(b.fail || ep!=0x81 || b.next_rx.size()>cap) return 0;
    std::memcpy(out,b.next_rx.data(),b.next_rx.size());
    *actual=b.actual_override ? b.actual_override : b.next_rx.size();
    return 1;
}
void testBulk() {
    FakeBulk fake{};
    rtw_bulk_transport bus{};
    bus.context=&fake;
    bus.read=fakeBulkRead;
    bus.write=fakeBulkWrite;
    bus.endpoint_in=0x81;
    bus.endpoint_out=0x02;
    bus.max_packet_size=512;
    std::vector<uint8_t> mpdu(472,0);
    mpdu[0]=0x40; // IEEE 802.11 probe request, management FC type
    mpdu[4]=0xff; // broadcast destination
    std::vector<uint8_t> staging(600);
    size_t sent=999;
    CHECK(rtw_bulk_send_management(&bus,mpdu.data(),mpdu.size(),4,
                                    staging.data(),staging.size(),&sent)==RTW_BULK_RADIO_NOT_READY);
    CHECK(sent==0 && fake.write_count==0);
    bus.radio_ready=1;
    CHECK(rtw_bulk_send_management(&bus,mpdu.data(),mpdu.size(),4,
                                    staging.data(),staging.size(),&sent)==RTW_BULK_OK);
    CHECK(sent==513 && fake.write_count==1 && fake.last_tx.size()==513);
    CHECK(fake.last_tx[0]==0xd8 && fake.last_tx[3]==0x8d);
    CHECK(fake.last_tx[40]==0x40 && fake.last_tx[513-1]==0);
    CHECK(std::memcmp(fake.last_tx.data()+40,mpdu.data(),mpdu.size())==0);
    const auto before=fake.write_count;
    std::memcpy(staging.data(),mpdu.data(),mpdu.size());
    CHECK(rtw_bulk_send_management(&bus,staging.data(),mpdu.size(),4,
                                    staging.data(),staging.size(),&sent)==RTW_BULK_BAD_ARGUMENT);
    CHECK(fake.write_count==before && sent==0);
    mpdu[0]=0x08; // data frame, not management
    CHECK(rtw_bulk_send_management(&bus,mpdu.data(),mpdu.size(),4,
                                    staging.data(),staging.size(),&sent)==RTW_BULK_BAD_MPDU);
    CHECK(fake.write_count==before && sent==0);
    mpdu[0]=0x40;
    CHECK(rtw_bulk_send_management(&bus,mpdu.data(),mpdu.size(),4,
                                    staging.data(),512,&sent)==RTW_BULK_NO_SPACE);
    CHECK(fake.write_count==before);
    fake.actual_override=12;
    CHECK(rtw_bulk_send_management(&bus,mpdu.data(),mpdu.size(),4,
                                    staging.data(),staging.size(),&sent)==RTW_BULK_SHORT_TRANSFER);
    CHECK(sent==0);
    fake.actual_override=0;
    fake.fail=true;
    CHECK(rtw_bulk_send_management(&bus,mpdu.data(),mpdu.size(),4,
                                    staging.data(),staging.size(),&sent)==RTW_BULK_IO_ERROR);
    fake.fail=false;
    fake.next_rx.resize(24+32,0);
    fake.next_rx[0]=32;
    std::array<uint8_t,128> rx{};
    size_t received=999;
    CHECK(rtw_bulk_receive(&bus,rx.data(),rx.size(),&received)==RTW_BULK_OK);
    CHECK(received==56 && fake.read_count==1);
    rtw_rx_iterator it{};
    rtw_rx_iterator_init(&it,rx.data(),received);
    rtw_rx_packet packet{};
    CHECK(rtw_rx_next(&it,&packet)==RTW_PACKETS_OK && packet.mpdu_length==32);
    CHECK(rtw_rx_next(&it,&packet)==RTW_PACKETS_DONE);
    fake.actual_override=129;
    CHECK(rtw_bulk_receive(&bus,rx.data(),rx.size(),&received)==RTW_BULK_SHORT_TRANSFER);
    CHECK(received==0);
    fake.actual_override=0;
    bus.radio_ready=0;
    const auto read_before=fake.read_count;
    CHECK(rtw_bulk_receive(&bus,rx.data(),rx.size(),&received)==RTW_BULK_RADIO_NOT_READY);
    CHECK(fake.read_count==read_before);
    bus.radio_ready=1;
    bus.endpoint_in=0x01;
    CHECK(rtw_bulk_receive(&bus,rx.data(),rx.size(),&received)==RTW_BULK_BAD_ARGUMENT);
    CHECK(fake.read_count==read_before);
    bus.endpoint_in=0x81;
    CHECK(rtw_bulk_receive(nullptr,rx.data(),rx.size(),&received)==RTW_BULK_BAD_ARGUMENT);
}

void testFuzz() {
    std::mt19937 rng(0x89a261U);
    std::vector<std::uint8_t> data(512);
    std::array<std::uint8_t,RTW_EFUSE_BYTES> map{};
    for(unsigned run=0;run<12000;++run) {
        const auto len=static_cast<std::size_t>(rng()%data.size()+1U);
        for(std::size_t i=0;i<len;++i) data[i]=static_cast<std::uint8_t>(rng());
        rtw_efuse_decode(data.data(),len,map.data(),map.size(),nullptr);
        rtw_rx_iterator it{};
        rtw_rx_iterator_init(&it,data.data(),len);
        rtw_rx_packet p{};
        unsigned steps=0;
        for(;;) {
            const auto err=rtw_rx_next(&it,&p);
            if (err!=RTW_PACKETS_OK) break;
            CHECK(++steps<=len / RTW_RX_DESC_BYTES + 1U);
            CHECK(p.mpdu>=data.data() && p.mpdu+p.mpdu_length<=data.data()+len);
        }
    }
}
} // namespace
int main() {
    testChip();testWrite();testEfuseDecode();testEfuseDevice();
    testFirmware();testPackets();testBulk();testFuzz();
    std::cout << "HAL tests OK; " << assertions << " assertions, "
              << "12000 malformed input pairs (EFUSE/RX)\n";
    return 0;
}
