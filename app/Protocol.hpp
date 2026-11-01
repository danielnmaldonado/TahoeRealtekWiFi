// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "RTWControl.h"
#include <cstdint>
#include <cstring>
namespace rtwui {
constexpr uint32_t magic=0x31575452,ipcVersion=2,listSelector=0x100,maxDevices=16;
struct Frame {
    uint32_t magic,version,selector,length,result,reserved;
    uint64_t registryID;
};
static_assert(sizeof(Frame)==32,"local IPC v2 frame");
struct DeviceRecord {uint64_t registryID;char family[64];};
struct DeviceList {uint32_t version,count,reserved[2];DeviceRecord devices[maxDevices];};
static_assert(sizeof(DeviceRecord)==72 && sizeof(DeviceList)==1168,"local device list v2");
inline uint32_t inputSize(uint32_t selector) {
    return selector==listSelector?0:rtw_control_input_size(selector);
}
inline uint32_t outputSize(uint32_t selector) {
    return selector==listSelector?sizeof(DeviceList):rtw_control_output_size(selector);
}
inline Frame makeFrame(uint32_t selector,uint64_t id,uint32_t length,uint32_t result=0) {
    return {magic,ipcVersion,selector,length,result,0,id};
}
inline bool validFrame(const Frame& f,bool response) {
    if(f.magic!=magic||f.version!=ipcVersion||f.reserved)return false;
    if(f.selector==listSelector){if(f.registryID)return false;}
    else if(f.selector>=RTW_CONTROL_SELECTOR_COUNT||!f.registryID)return false;
    if(!response)return !f.result&&f.length==inputSize(f.selector);
    return f.result?f.length==0:f.length==outputSize(f.selector);
}
inline bool validDevices(const DeviceList& list) {
    if(list.version!=ipcVersion||list.count>maxDevices||list.reserved[0]||list.reserved[1])return false;
    for(uint32_t i=0;i<list.count;i++){
        if(!list.devices[i].registryID||!std::memchr(list.devices[i].family,0,sizeof(list.devices[i].family)))return false;
        for(uint32_t j=0;j<i;j++)if(list.devices[i].registryID==list.devices[j].registryID)return false;
    }
    return true;
}
inline bool validPayload(uint32_t selector,const void* output) {
    if(!output)return false;
    if(selector==listSelector)return validDevices(*static_cast<const DeviceList*>(output));
    if(selector==RTW_CONTROL_CHANNEL_MAP)return rtw_control_channel_map_valid(static_cast<const RTWControlChannelMap*>(output));
    return rtw_control_load32(static_cast<const uint8_t*>(output))==RTW_CONTROL_VERSION;
}
struct DeviceSelection {
    uint64_t registryID=0,generation=0;
    bool everSelected=false,invalidated=false;
    bool select(uint64_t id) {
        if(!id)return false;everSelected=true;
        if(id==registryID&&!invalidated)return false;
        registryID=id;++generation;invalidated=false;return true;
    }
    // Keep the last selected ID for display/recovery, but it cannot validate
    // work or prevent selecting that same device again after an interruption.
    void invalidate(){++generation;invalidated=true;}
    bool matches(uint64_t id,uint64_t token)const{
        return !invalidated&&id==registryID&&token==generation;
    }
};
// An unqueried, failed or stale map is an empty exact map. Only a driver that
// explicitly rejects the optional selector may use the legacy group fallback.
struct ChannelMapState {
    uint64_t registryID=0,generation=0;
    RTWControlChannelMap value{RTW_CONTROL_VERSION,{0,0,0},{0}};
    bool legacy=false;
    void clear(){registryID=generation=0;value={};value.version=RTW_CONTROL_VERSION;legacy=false;}
    bool setExact(const DeviceSelection& selection,uint64_t id,uint64_t token,const RTWControlChannelMap& map) {
        if(!selection.matches(id,token))return false;
        clear();
        if(!rtw_control_channel_map_valid(&map))return false;
        registryID=id;generation=token;value=map;return true;
    }
    bool setLegacy(const DeviceSelection& selection,uint64_t id,uint64_t token) {
        if(!selection.matches(id,token))return false;
        clear();registryID=id;generation=token;legacy=true;return true;
    }
    const RTWControlChannelMap* current(const DeviceSelection& selection)const {
        if(selection.matches(registryID,generation))return legacy?nullptr:&value;
        static const RTWControlChannelMap empty{RTW_CONTROL_VERSION,{0,0,0},{0}};
        return &empty;
    }
};
}
