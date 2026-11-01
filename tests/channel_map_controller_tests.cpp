/* SPDX-License-Identifier: GPL-2.0-only */
#include "RTWControl.h"
#include "rtw/DFSRegulatory.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <array>
using IOReturn=int;
enum {kIOReturnSuccess=0,kIOReturnBadArgument=-1,kIOReturnUnsupported=-2,
    IEEE80211_CHAN_2GHZ=0x80,IEEE80211_CHAN_5GHZ=0x100};
class RTWUSBController {
public:
    bool attached_=true;
    struct {struct {struct {unsigned ic_flags=0;} ic_channels[256];} sc_ic;} core_{};
    struct ControlCall {uint32_t selector;const void* input;size_t length;void* output;size_t* outputLength;};
    void channelMap(RTWControlChannelMap*)const;
    IOReturn handleControl(ControlCall*);
    void status(RTWControlStatus*){}
    void scanResults(RTWControlScanResults*){}
};
/* ACTUAL_CONTROLLER_METHODS */
int main() {
    for(const char* cc:{"AU","RU"}) {
        RTWUSBController c;
        for(unsigned ch=1;ch<=11;++ch)c.core_.sc_ic.ic_channels[ch].ic_flags=IEEE80211_CHAN_2GHZ;
        for(unsigned ch=52;ch<=140;++ch)if(rtw_dfs_regulatory_channel(reinterpret_cast<const uint8_t*>(cc),uint8_t(ch)))
            c.core_.sc_ic.ic_channels[ch].ic_flags=IEEE80211_CHAN_5GHZ;
        const auto before=c.core_;
        RTWControlRequest request{};request.version=1;
        RTWControlChannelMap out;std::memset(&out,0xa5,sizeof(out));size_t n=sizeof(out);
        RTWUSBController::ControlCall call{5,&request,sizeof(request),&out,&n};
        assert(c.handleControl(&call)==0&&n==sizeof(out)&&rtw_control_channel_map_valid(&out));
        assert(!std::memcmp(&before,&c.core_,sizeof(before)));
        for(unsigned ch=1;ch<256;++ch)assert(rtw_control_channel_map_contains(&out,ch)==
            bool(c.core_.sc_ic.ic_channels[ch].ic_flags&(ch<=14?IEEE80211_CHAN_2GHZ:IEEE80211_CHAN_5GHZ)));
        assert(!rtw_control_channel_map_contains(&out,144)&&!rtw_control_channel_map_contains(&out,0));
        assert(out.reserved[0]==0&&out.reserved[1]==0&&out.reserved[2]==0);
        request.reserved[2]=1;assert(c.handleControl(&call)==kIOReturnBadArgument);request.reserved[2]=0;
        n=sizeof(out)-1;assert(c.handleControl(&call)==kIOReturnBadArgument);n=sizeof(out);
        c.attached_=false;std::memset(&out,0xa5,sizeof(out));assert(c.handleControl(&call)==0);
        assert(rtw_control_channel_map_valid(&out));for(auto byte:out.channels)assert(!byte);
    }
    // A5GHz-only registration must not inherit legacy2.4GHz permission.
    RTWUSBController five;five.core_.sc_ic.ic_channels[132].ic_flags=IEEE80211_CHAN_5GHZ;
    RTWControlChannelMap out{};five.channelMap(&out);
    assert(rtw_control_channel_map_contains(&out,132)&&!rtw_control_channel_map_contains(&out,6));
    std::puts("Actual controller channel-map query: AU8/RU3 registration, OEM5GHz, zeroreserved, bounded read-only dispatch passed");
}
