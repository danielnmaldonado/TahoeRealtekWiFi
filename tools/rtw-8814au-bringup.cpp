/* SPDX-License-Identifier: BSD-3-Clause
 * Single2357:0106 firmware/MAC bring-up test. No station frames are sent.
 * Uses production HAL. Always powers down before releasing the USB device. */
#define RTW_BOARD_HELPER_ONLY
#include "rtw-8814au-board.cpp"
#include <fstream>
#include <vector>
#include "rtw/RTL8814AUPHY.h"
#include "rtw/RTL8814AUDataPath.h"
#include "rtw/RTL8814AUStation.h"
struct BootUSB: USBBoard {IOUSBInterfaceInterface500 **interface=nullptr;UInt8 out=0,in=0;unsigned bulks=0;};
static int bootRead(void*p,uint16_t a,unsigned w,uint32_t*v){auto*b=static_cast<BootUSB*>(p);*v=0;if(std::chrono::steady_clock::now()>b->deadline)return ETIMEDOUT;++b->reads;return request(b,true,a,w,v);}
static int bootWrite(void*p,uint16_t a,unsigned w,uint32_t v){auto*b=static_cast<BootUSB*>(p);if(a==0x30||a==0x34||a==0xcf)return EPERM;++b->writes;return request(b,false,a,w,&v);}
static int bootBulk(void*p,uint8_t qsel,const uint8_t*bytes,size_t n){
    auto*b=static_cast<BootUSB*>(p);if(!b->interface||!b->out||!bytes||!n||n>4137||(qsel!=16&&qsel!=19))return EINVAL;
    ++b->bulks;std::vector<uint8_t> copy(bytes,bytes+n);if(n%512==0)copy.push_back(0);
    auto r=(*b->interface)->WritePipeTO(b->interface,b->out,copy.data(),UInt32(copy.size()),500,1000);
    if(r!=kIOReturnSuccess){std::fprintf(stderr,"BULK_ERROR=0x%x QSEL=%u LENGTH=%zu\n",r,qsel,n);return EIO;}return 0;
}
static int openInterface(BootUSB &b){
    IOUSBFindInterfaceRequest match{kIOUSBFindInterfaceDontCare,kIOUSBFindInterfaceDontCare,kIOUSBFindInterfaceDontCare,kIOUSBFindInterfaceDontCare};
    io_iterator_t it=0;auto result=(*b.device)->CreateInterfaceIterator(b.device,&match,&it);if(result)return EIO;
    io_service_t service=IOIteratorNext(it);auto second=IOIteratorNext(it);IOObjectRelease(it);
    if(!service||second){if(service)IOObjectRelease(service);if(second)IOObjectRelease(second);return ENODEV;}
    IOCFPlugInInterface **plugin=nullptr;SInt32 score=0;
    result=IOCreatePlugInInterfaceForService(service,kIOUSBInterfaceUserClientTypeID,kIOCFPlugInInterfaceID,&plugin,&score);IOObjectRelease(service);
    if(result||!plugin)return EIO;
    auto query=(*plugin)->QueryInterface(plugin,CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID500),reinterpret_cast<LPVOID*>(&b.interface));(*plugin)->Release(plugin);
    if(query||!b.interface)return EIO;
    UInt8 number=255,alternate=255;result=(*b.interface)->GetInterfaceNumber(b.interface,&number);
    if(!result)result=(*b.interface)->GetAlternateSetting(b.interface,&alternate);
    if(result||number||alternate){(*b.interface)->Release(b.interface);b.interface=nullptr;return ENODEV;}
    result=(*b.interface)->USBInterfaceOpen(b.interface);if(result){std::printf("INTERFACE_OPEN_ERROR=0x%x\n",result);(*b.interface)->Release(b.interface);b.interface=nullptr;return EBUSY;}
    UInt8 count=0;result=(*b.interface)->GetNumEndpoints(b.interface,&count);unsigned outs=0,ins=0;
    for(UInt8 pipe=1;!result&&pipe<=count;pipe++){
        UInt8 direction=0,number=0,type=0,interval=0;UInt16 mps=0;
        result=(*b.interface)->GetPipeProperties(b.interface,pipe,&direction,&number,&type,&mps,&interval);
        if(!result&&type==kUSBBulk){if(mps!=512)return ENODEV;if(direction==kUSBOut){if(!outs)b.out=pipe;++outs;}else{b.in=pipe;++ins;}}
    }
    return !result&&outs==3&&ins==1?0:ENODEV;
}
int main(int argc,char**argv){
    const bool high=argc==3&&!std::strcmp(argv[1],"--rx-5g-high-check");
    const bool five=high||(argc==3&&(!std::strcmp(argv[1],"--radio-5g-check")||!std::strcmp(argv[1],"--rx-5g-check")));
    const bool rx=high||(argc==3&&(!std::strcmp(argv[1],"--rx-check")||!std::strcmp(argv[1],"--rx-5g-check")));
    const bool radio=five||rx||(argc==3&&std::strcmp(argv[1],"--radio-check")==0);
    if(argc!=3||(!radio&&std::strcmp(argv[1],"--boot-check"))){std::fputs("Usage: rtw-8814au-bringup --boot-check|--radio-check|--rx-check|--radio-5g-check|--rx-5g-check|--rx-5g-high-check rtw8814a_fw.bin\n",stderr);return 1;}
    std::ifstream file(argv[2],std::ios::binary|std::ios::ate);if(!file||file.tellg()!=68320)return 1;
    std::vector<uint8_t> firmware(68320);file.seekg(0);if(!file.read(reinterpret_cast<char*>(firmware.data()),firmware.size()))return 1;
    rtw_8814au_fw_info info{};if(rtw_8814au_inspect_fw(firmware.data(),firmware.size(),&info)||info.version!=33||info.subversion!=6)return 1;
    auto match=IOServiceMatching(kIOUSBDeviceClassName);if(!match)return 2;number(match,CFSTR("idVendor"),0x2357);number(match,CFSTR("idProduct"),0x0106);
    io_iterator_t iterator=0;auto result=IOServiceGetMatchingServices(kIOMainPortDefault,match,&iterator);if(result)return 2;
    io_service_t service=IOIteratorNext(iterator);auto second=IOIteratorNext(iterator);IOObjectRelease(iterator);
    if(!service||second){if(service)IOObjectRelease(service);if(second)IOObjectRelease(second);return 2;}
    IOCFPlugInInterface **plugin=nullptr;SInt32 score=0;
    result=IOCreatePlugInInterfaceForService(service,kIOUSBDeviceUserClientTypeID,kIOCFPlugInInterfaceID,&plugin,&score);IOObjectRelease(service);if(result||!plugin)return 3;
    IOUSBDeviceInterface500 **device=nullptr;auto query=(*plugin)->QueryInterface(plugin,CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID500),reinterpret_cast<LPVOID*>(&device));(*plugin)->Release(plugin);if(query||!device)return 3;
    UInt16 vendor=0,product=0;UInt8 speed=0,config=0;
    result=(*device)->GetDeviceVendor(device,&vendor);if(!result)result=(*device)->GetDeviceProduct(device,&product);if(!result)result=(*device)->GetDeviceSpeed(device,&speed);if(!result)result=(*device)->GetConfiguration(device,&config);
    if(result||vendor!=0x2357||product!=0x106||speed!=kUSBDeviceSpeedHigh||config!=1){(*device)->Release(device);return 3;}
    result=(*device)->USBDeviceOpen(device);if(result){(*device)->Release(device);return 4;}
    BootUSB b{};b.device=device;b.deadline=std::chrono::steady_clock::now()+std::chrono::seconds(45);
    rtw_8814au c{};rtw_8814au_bus_ops ops{&b,readBoard,writeBoard,nullptr,delayBoard};rtw_8814au_init(&c,&ops);
    int error=rtw_8814au_identify_read_board(&c,RTW_8814AU_HIGH_SPEED);
    if(!error)error=openInterface(b);
    bool attempted=false;
    if(!error){c.bus={&b,bootRead,bootWrite,bootBulk,delayBoard};attempted=true;error=rtw_8814au_power_on(&c);std::printf("POWER=%d\n",error);}
    if(!error){error=rtw_8814au_download_fw(&c,firmware.data(),firmware.size());std::printf("FIRMWARE=%d RUNNING=%u\n",error,c.fw_running);}
    if(!error){error=rtw_8814au_init_mac(&c,3,RTW_8814AU_HIGH_SPEED);std::printf("MAC=%d INITIALIZED=%u PHY_READY=%u\n",error,c.mac_initialized,c.phy_initialized);}
    std::vector<uint64_t> storage;
    if(!error&&radio){storage.resize((rtw_8814au_phy_size()+7)/8);error=rtw_8814au_phy_init(&c,storage.data(),storage.size()*8);std::printf("PHY_CONTEXT=%d\n",error);}
    if(!error&&radio){error=rtw_8814au_phy_load_tables(&c);std::printf("PHY_TABLES=%d\n",error);}
    if(!error&&radio){error=rtw_8814au_phy_set_channel(&c,high?149:five?36:1,20);std::printf("CHANNEL_%u_20MHZ=%d\n",high?149:five?36:1,error);}
    if(!error&&radio){error=rtw_8814au_phy_calibrate(&c);std::printf("RF_CALIBRATION=%d PHY_READY=%u\n",error,c.phy_initialized);}
    unsigned received=0,beacons=0,events=0,malformed=0,crc=0;
    if(!error&&rx){
        rtw_8814au_station_params station{};station.band_5g=five;station.passive_scan=five;station.basic_rates=five?0x150:0x15f;std::memcpy(station.mac,c.board.mac,6);
        error=rtw_8814au_station_initialize(&c,&station);
        if(!error)error=rtw_8814au_send_startup_info(&c);
        const uint8_t channels[]={1,2,3,4,5,6,7,8,9,10,11,36,40,44,48,149,153,157,161,165};
        for(unsigned ci=high?15:five?11:0;!error&&ci<(high?20U:five?15U:11U);ci++){
            uint8_t channel=channels[ci];
            error=five?rtw_8814au_phy_set_channel(&c,channel,20):rtw_8814au_phy_set_scan_channel(&c,channel,20);
            if(!error&&five)error=rtw_8814au_phy_calibrate(&c);
            if(!error)error=rtw_8814au_station_configure(&c,RTW_8814AU_STATION_SCAN,&station);
            if(error)break;
            auto until=std::chrono::steady_clock::now()+std::chrono::milliseconds(220);unsigned channelBeacons=0;
            while(std::chrono::steady_clock::now()<until){
                uint8_t bytes[16384];UInt32 n=sizeof(bytes);auto status=(*b.interface)->ReadPipeTO(b.interface,b.in,bytes,&n,100,200);
                if(status==kIOUSBTransactionTimeout||status==kIOReturnTimeout)continue;
                if(status!=kIOReturnSuccess){std::printf("RX_ERROR=0x%x\n",status);error=EIO;break;}
                rtw_rx_iterator iterator;rtw_rx_iterator_init(&iterator,bytes,n);rtw_8814au_rx_frame f{};rtw_packet_error result;
                while((result=rtw_8814au_rx_next(&iterator,&f))==RTW_PACKETS_OK){
                    if(f.is_c2h){++events;continue;}++received;
                    if(f.packet.crc_error||f.packet.icv_error){++crc;continue;}
                    if(f.packet.mpdu_length>=28&&(f.packet.mpdu[0]&0xfc)==0x80){++beacons;++channelBeacons;}
                }
                if(result!=RTW_PACKETS_DONE)++malformed;
            }
            std::printf("PASSIVE_CHANNEL=%u BEACONS=%u\n",channel,channelBeacons);
        }
        std::printf("RX_MPDU=%u BEACONS=%u C2H=%u MALFORMED=%u CRC_ICV=%u\n",received,beacons,events,malformed,crc);
    }
    if(c.phy){int stopped=rtw_8814au_phy_stop(&c);std::printf("PHY_STOP=%d\n",stopped);if(!error)error=stopped;}
    if(attempted){int stopped=rtw_8814au_stop(&c);std::printf("STOP=%d POWERED=%u FIRMWARE_RUNNING=%u\n",stopped,c.powered,c.fw_running);if(!error)error=stopped;}
    if(b.interface){(*b.interface)->USBInterfaceClose(b.interface);(*b.interface)->Release(b.interface);}
    std::printf("USB=2357:0106 RESULT=%d READS=%u WRITES=%u BULKS=%u\n",error,b.reads,b.writes,b.bulks);
    (*device)->USBDeviceClose(device);(*device)->Release(device);return error?5:0;
}
