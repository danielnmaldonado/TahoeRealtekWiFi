#include "rtw/USBWiFiProfiles.h"
#include "rtw/RTL8192EU.h"
#if __has_include("rtw/USBWiFiFirmware.h")
#include "rtw/USBWiFiFirmware.h"
#else
static int rtw_usb_wifi_inspect_firmware(rtw_wifi_family,const uint8_t*b,size_t n,rtw_8192eu_fw_info*out){return rtw_8192eu_inspect_firmware(b,n,out)==RTW_8192EU_OK;}
#endif
#ifndef RTW_FLEXIBLE_TX_DESCRIPTOR
static rtw_packet_error rtw_tx_total_transfer_size_for_descriptor(size_t,size_t n,uint16_t mps,size_t*out){return rtw_tx_total_transfer_size(n,mps,out);}
#endif
#include <array>
#include <cstdio>
#include <cstdlib>
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(){
 const rtw_wifi_family families[]={RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8188EU,RTW_WIFI_RTL8192EU};
 const uint16_t signatures[]={0x88c1,0x88e1,0x92e1};
 for(unsigned i=0;i<3;++i){std::array<uint8_t,96>b{};b[0]=uint8_t(signatures[i]);b[1]=uint8_t(signatures[i]>>8);b[4]=7;rtw_8192eu_fw_info out{};
  CHECK(rtw_usb_wifi_inspect_firmware(families[i],b.data(),b.size(),&out));CHECK(out.payload==b.data()+32&&out.payload_length==64&&out.major_version==7);
  for(unsigned j=0;j<3;++j)if(i!=j)CHECK(!rtw_usb_wifi_inspect_firmware(families[j],b.data(),b.size(),&out));
  for(size_t n=0;n<=32;++n)CHECK(!rtw_usb_wifi_inspect_firmware(families[i],b.data(),n,&out));
  CHECK(!rtw_usb_wifi_inspect_firmware(RTW_WIFI_UNKNOWN,b.data(),b.size(),&out));
  CHECK(!rtw_usb_wifi_inspect_firmware(families[i],nullptr,b.size(),&out));CHECK(!rtw_usb_wifi_inspect_firmware(families[i],b.data(),b.size(),nullptr));
  b[12]=0xff;b[13]=0xff;CHECK(!rtw_usb_wifi_inspect_firmware(families[i],b.data(),b.size(),&out));CHECK(out.payload==nullptr&&out.payload_length==0);
 }
 size_t total=0;
 CHECK(rtw_tx_total_transfer_size_for_descriptor(32,32,64,&total)==RTW_PACKETS_OK&&total==65);
 CHECK(rtw_tx_total_transfer_size_for_descriptor(40,32,64,&total)==RTW_PACKETS_OK&&total==72);
 for(size_t d:{size_t(32),size_t(40)})for(uint16_t m:{uint16_t(64),uint16_t(512),uint16_t(1024)})for(size_t n=1;n<=4096;++n){
  CHECK(rtw_tx_total_transfer_size_for_descriptor(d,n,m,&total)==RTW_PACKETS_OK);CHECK(total==d+n+((d+n)%m==0?1:0));
 }
 CHECK(rtw_tx_total_transfer_size_for_descriptor(31,1,64,&total)==RTW_PACKETS_BAD_ARGUMENT&&total==0);
 CHECK(rtw_tx_total_transfer_size_for_descriptor(0,1,64,&total)==RTW_PACKETS_BAD_ARGUMENT);
 CHECK(rtw_tx_total_transfer_size_for_descriptor(32,0,64,&total)==RTW_PACKETS_BAD_ARGUMENT);
 CHECK(rtw_tx_total_transfer_size_for_descriptor(32,1,63,&total)==RTW_PACKETS_BAD_ARGUMENT);
 std::printf("Cross-family firmware: %u checks passed\n",checks);
}
