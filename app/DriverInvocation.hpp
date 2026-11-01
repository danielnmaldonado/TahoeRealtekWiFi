// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "Devices.hpp"
#include "Transport.hpp"

namespace rtwui {
inline uint32_t invokeDriver(uint64_t registryID,uint32_t selector,
                             const void* input,size_t length,void* output) {
    if(selector==listSelector)return listDevices(*static_cast<DeviceList*>(output));
    io_connect_t handle=IO_OBJECT_NULL;
    uint32_t error=openDevice(registryID,handle);
    if(error)return error;
    size_t outputLength=rtw_control_output_size(selector);
    error=IOConnectCallStructMethod(handle,selector,input,length,output,&outputLength);
    IOServiceClose(handle);
    if(!error&&(outputLength!=rtw_control_output_size(selector)||!validPayload(selector,output)))
        return protocolError;
    return error;
}
}
