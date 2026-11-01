// SPDX-License-Identifier: BSD-3-Clause
#include "rtw/RTW88Firmware.h"
#include "rtw/USBWiFiFirmware.h"
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <vector>
#include <utility>
#include "../kext/RTL8821CUFirmware.inc"
#include "../kext/RTL8822BUFirmware.inc"
#include "../kext/RTL8822CUFirmware.inc"
#include "../kext/RTL8192DUFirmware.inc"
#include "../kext/RTL8192FUFirmware.inc"
#include "../kext/RTL8723BUFirmware.inc"
#include "../kext/RTL8710BUFirmware.inc"
int main(){
    rtw88_fw_image image{};
    assert(rtw88_fw_inspect(RTW_WIFI_RTL8821CU,RTW8821CUFirmware,sizeof(RTW8821CUFirmware),&image)==0);
    assert(image.version==24&&image.subversion==11&&image.imem.address==0x30000);
    assert(rtw88_fw_inspect(RTW_WIFI_RTL8822BU,RTW8822BUFirmware,sizeof(RTW8822BUFirmware),&image)==0);
    assert(image.version==30&&image.subversion==20&&image.imem.address==0);
    assert(rtw88_fw_inspect(RTW_WIFI_RTL8822CU,RTW8822CUFirmware,sizeof(RTW8822CUFirmware),&image)==0);
    assert(image.version==9&&image.subversion==9&&image.imem.address==0x30000);
    // B/C share a signature: their distinct IMEM origins still must match.
    assert(rtw88_fw_inspect(RTW_WIFI_RTL8822BU,RTW8822CUFirmware,sizeof(RTW8822CUFirmware),&image)==EINVAL);
    assert(rtw88_fw_inspect(RTW_WIFI_RTL8822CU,RTW8822BUFirmware,sizeof(RTW8822BUFirmware),&image)==EINVAL);
    rtw_8192eu_fw_info legacy{};
    assert(rtw_usb_wifi_inspect_firmware(RTW_WIFI_RTL8192DU,RTW8192DUFirmware,sizeof(RTW8192DUFirmware),&legacy));
    assert(legacy.major_version==38&&legacy.minor_version==0);
    assert(rtw_usb_wifi_inspect_firmware(RTW_WIFI_RTL8192FU,RTW8192FUFirmware,sizeof(RTW8192FUFirmware),&legacy));
    assert(legacy.major_version==7&&legacy.minor_version==3);
    assert(rtw_usb_wifi_inspect_firmware(RTW_WIFI_RTL8723BU,RTW8723BUFirmware,sizeof(RTW8723BUFirmware),&legacy));
    assert(legacy.major_version==35&&legacy.signature==0x5301);
    std::vector<uint8_t> wrong(RTW8723BUFirmware,RTW8723BUFirmware+sizeof(RTW8723BUFirmware));
    wrong[1]=0x23; // AU family, not this BU NIC.
    assert(!rtw_usb_wifi_inspect_firmware(RTW_WIFI_RTL8723BU,wrong.data(),wrong.size(),&legacy));
    for(auto fw:{std::pair<const uint8_t*,size_t>{RTW8710BUSMICFirmware,sizeof(RTW8710BUSMICFirmware)},
                 {RTW8710BUUMCFirmware,sizeof(RTW8710BUUMCFirmware)}}) {
        assert(rtw_usb_wifi_inspect_firmware(RTW_WIFI_RTL8710BU,fw.first,fw.second,&legacy));
        assert(legacy.major_version==16&&legacy.minor_version==0);
        wrong.assign(fw.first,fw.first+fw.second);
        for(uint8_t signature:{uint8_t(0xb0),uint8_t(0xb3)}) {
            wrong[0]=signature;
            assert(!rtw_usb_wifi_inspect_firmware(RTW_WIFI_RTL8710BU,wrong.data(),wrong.size(),&legacy));
        }
    }
    std::puts("Eight actual embedded firmware images: versions, headers and family/vendor identity boundaries passed; no USB I/O");
}
