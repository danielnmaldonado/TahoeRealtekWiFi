// SPDX-License-Identifier: GPL-2.0-only
// Real Main reconnect/credential callbacks. No driver, service or Keychain I/O.
#define main rtwui_application_main
#include "../Main.mm"
#undef main
#include <cassert>
@interface SavedProfileStore : NSObject <RTWCredentialStoring>
@end
@implementation SavedProfileStore
- (NSMutableData*)passwordForSSID:(NSData*)ssid error:(NSError**)error {
    return [self passwordForSSID:ssid allowInteraction:NO error:error];
}
- (NSMutableData*)passwordForSSID:(NSData*)ssid allowInteraction:(BOOL)interaction error:(NSError**)error {
    assert(ssid.length&&!interaction);if(error)*error=nil;
    return [[@"password123" dataUsingEncoding:NSASCIIStringEncoding] mutableCopy];
}
- (BOOL)savePassword:(NSData*)password forSSID:(NSData*)ssid error:(NSError**)error {
    (void)password;(void)ssid;(void)error;assert(false);return NO;
}
- (BOOL)deletePasswordForSSID:(NSData*)ssid error:(NSError**)error {
    (void)ssid;(void)error;assert(false);return NO;
}
- (NSArray<NSData*>*)savedSSIDsAndReturnError:(NSError**)error {(void)error;return @[];}
@end
@interface SavedProfileProbe : WiFiApp
@property(nonatomic,assign) unsigned requests;
@property(nonatomic,assign) unsigned joins;
@property(nonatomic,assign) unsigned scans;
- (void)prepare:(unsigned)mode preferred:(BOOL)preferred row:(unsigned)row;
- (void)policy:(unsigned)mode;
- (void)lookup;
@end
@implementation SavedProfileProbe
- (double)uptime {return 100;}
- (void)updateAppearance {}
- (void)policy:(unsigned)mode {
    _status.flags=RTW_CONTROL_DEVICE|RTW_CONTROL_RADIO;_channelMap.clear();
    RTWControlChannelMap map{};map.version=1;
    if(mode==0||mode==2)map.channels[6/8]|=1U<<(6%8);
    if(mode==4)map.channels[132/8]|=1U<<(132%8);
    if(mode==3)map.reserved[0]=1;
    if(mode==5||mode==6||mode==7) {
        if(mode==6)_status.flags|=RTW_CONTROL_BANDS_EXPLICIT|RTW_CONTROL_BAND_5GHZ;
        if(mode==7)_status.flags|=RTW_CONTROL_BANDS_EXPLICIT|RTW_CONTROL_BAND_2GHZ;
        assert(_channelMap.setLegacy(_selection,42,_selection.generation));
    } else if(mode!=8) {
        const bool accepted=_channelMap.setExact(_selection,42,_selection.generation,map);
        assert(accepted==(mode!=3));
    }
    if(mode==2){_selection.invalidate();_selection.select(42);}
}
- (void)prepare:(unsigned)mode preferred:(BOOL)preferred row:(unsigned)row {
    _selection.select(42);_selectedPresent=YES;_authorizedSession=YES;
    _hasStatus=YES;_statusReadDelayed=NO;_status={};_status.version=1;
    _autoJoinEnabled=YES;_autoJoin.paused=false;_savedNetworksLoaded=YES;
    NSData* profile=[@"HiddenLab" dataUsingEncoding:NSUTF8StringEncoding];
    _preferredSSID=preferred?profile:nil;_savedNetworks=@[profile];
    _lastLinkedAt=-1;_lastAutoScanAt=-1;
    _scan={};_scan.version=1;_scan.count=1;
    auto& record=_scan.records[0];record.channel=6;record.securityFlags=7;
    if(row) {
        record.ssidLength=static_cast<uint8_t>(profile.length);
        std::memcpy(record.ssid,profile.bytes,profile.length);
        if(row==1)record.channel=100; // visible sameSSID, forbidden by every test map
        if(row==2)record.securityFlags|=RTW_CONTROL_SECURITY_UNSUPPORTED;
        if(row==3)record.ssid[0]='O'; // unrelated visible AP does not identify saved hidden SSID
        if(row==4)record.channel=132;
    }
    [self policy:mode];
}
- (void)loadPasswordForSSID:(NSData*)ssid connect:(BOOL)connect automatic:(BOOL)automatic {
    assert(ssid.length&&connect&&automatic);self.requests++;
}
- (void)command:(uint32_t)selector {assert(selector==RTW_CONTROL_SCAN);self.scans++;}
- (id<RTWCredentialStoring>)credentials {return [SavedProfileStore new];}
- (void)lookup {
    [super loadPasswordForSSID:_savedNetworks.firstObject connect:YES automatic:YES];
}
- (void)startJoin:(std::shared_ptr<RTWControlJoin>)join ssid:(NSData*)ssid
      registryID:(uint64_t)registryID adapterGeneration:(uint64_t)generation {
    assert(join&&ssid.length&&registryID==42&&_selection.matches(registryID,generation));self.joins++;
}
@end
static void drain(SavedProfileProbe* probe) {
    NSDate* deadline=[NSDate dateWithTimeIntervalSinceNow:5];
    while([[probe valueForKey:@"credentialBusy"] boolValue]&&deadline.timeIntervalSinceNow>0)
        [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
    assert(![[probe valueForKey:@"credentialBusy"] boolValue]);
}
int main() {@autoreleasepool {
    unsigned cases=0;
    for(BOOL preferred:{NO,YES})for(unsigned mode=0;mode<9;++mode)for(unsigned row=0;row<6;++row) {
        SavedProfileProbe* probe=[SavedProfileProbe new];[probe prepare:mode preferred:preferred row:row];
        [probe considerAutoJoin];
        const bool activeTwo=mode==0||mode==5||mode==7;
        const bool allowed=(activeTwo&&(row==0||row==3||row==5))||(mode==4&&row==4);
        assert(probe.requests==unsigned(allowed)&&probe.scans==unsigned(!allowed));++cases;
    }
    SavedProfileProbe* lookup=[SavedProfileProbe new];[lookup prepare:0 preferred:YES row:0];
    [lookup lookup];drain(lookup);assert(lookup.joins==1);
    [lookup lookup];[lookup policy:4];drain(lookup);assert(lookup.joins==1);
    [lookup policy:0];[lookup lookup];[lookup policy:2];drain(lookup);assert(lookup.joins==1);
    [lookup policy:5];[lookup lookup];drain(lookup);assert(lookup.joins==2);
    [lookup policy:6];[lookup lookup];drain(lookup);assert(lookup.joins==2);
    std::printf("Actual Main saved-profile reconnect: %u preferred/sole/hidden/visible/map cases and delayed-Keychain fences passed\n",cases);
}}
