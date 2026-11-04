// SPDX-License-Identifier: GPL-2.0-only
// Removing idle admission or the delayed status recheck must fail this real-Main regression.
// The credential and USB command boundaries are mocked; no user data is read.
@interface AutomaticAdmissionStore : NSObject <RTWCredentialStoring>
@end
@implementation AutomaticAdmissionStore
- (NSMutableData*)passwordForSSID:(NSData*)ssid error:(NSError**)error {return [self passwordForSSID:ssid allowInteraction:NO error:error];}
- (NSMutableData*)passwordForSSID:(NSData*)ssid allowInteraction:(BOOL)interaction error:(NSError**)error {
    assert(ssid.length==3);(void)interaction;if(error)*error=nil;
    return [[@"password123" dataUsingEncoding:NSASCIIStringEncoding] mutableCopy];
}
- (BOOL)savePassword:(NSData*)password forSSID:(NSData*)ssid error:(NSError**)error {(void)password;(void)ssid;(void)error;assert(false);return NO;}
- (BOOL)deletePasswordForSSID:(NSData*)ssid error:(NSError**)error {(void)ssid;(void)error;assert(false);return NO;}
- (NSArray<NSData*>*)savedSSIDsAndReturnError:(NSError**)error {(void)error;return @[];}
@end
@interface AutomaticAdmissionApp : WiFiApp
@property(nonatomic,assign) unsigned lookups;
@property(nonatomic,assign) unsigned scans;
@property(nonatomic,assign) unsigned joins;
- (void)beginRealLookup;
- (void)beginManualLookup;
- (void)changePolicy:(unsigned)mode;
- (void)markLocalManualOff;
- (void)prepareState:(unsigned)state scanning:(BOOL)scanning visible:(BOOL)visible;
@end
@implementation AutomaticAdmissionApp
- (void)updateAppearance {}
- (double)uptime {return 100;}
- (void)prepareState:(unsigned)state scanning:(BOOL)scanning visible:(BOOL)visible {
    _selection.select(101);_selectedPresent=YES;_authorizedSession=YES;
    _hasStatus=YES;_status.version=1;_status.state=state;
    _status.flags=RTW_CONTROL_DEVICE|RTW_CONTROL_RADIO|RTW_CONTROL_RUNNING;
    if(scanning)_status.flags|=RTW_CONTROL_SCANNING;
    _autoJoinEnabled=YES;_savedNetworksLoaded=YES;
    _preferredSSID=[@"Lab" dataUsingEncoding:NSUTF8StringEncoding];
    _savedNetworks=@[_preferredSSID];_lastLinkedAt=-1;_lastAutoScanAt=-1;
    _scan.version=1;
    RTWControlChannelMap map{};map.version=1;
    map.channels[6/8]|=1U<<(6%8);
    if(!visible){map={};map.version=1;map.channels[36/8]|=1U<<(36%8);}
    _channelMap.setExact(_selection,101,_selection.generation,map);
    if(visible){_scan.count=1;auto&r=_scan.records[0];r.ssidLength=3;std::memcpy(r.ssid,"Lab",3);r.channel=6;r.securityFlags=7;}
}
- (void)loadPasswordForSSID:(NSData*)ssid connect:(BOOL)connect automatic:(BOOL)automatic {
    assert(ssid.length==3&&connect&&automatic);++self.lookups;
}
- (void)command:(uint32_t)selector {assert(selector==RTW_CONTROL_SCAN);++self.scans;}
- (id<RTWCredentialStoring>)credentials {return [AutomaticAdmissionStore new];}
- (void)beginRealLookup {[super loadPasswordForSSID:_preferredSSID connect:YES automatic:YES];}
- (void)beginManualLookup {[super loadPasswordForSSID:_preferredSSID connect:YES automatic:NO];}
- (void)changePolicy:(unsigned)mode {
    if(mode<=4)_status.state=mode;
    if(mode==1)_status.flags|=RTW_CONTROL_SCANNING;
    if(mode==5)_status.flags|=RTW_CONTROL_AUTHORIZED;
    if(mode==6)_statusReadDelayed=YES;
    if(mode==7)_status.usbError=uint32_t(kIOReturnIOError);
    if(mode==8)_status.flags|=RTW_CONTROL_USER_RADIO_DISABLED;
}
- (void)markLocalManualOff {_manualRadioOffAdapters.insert(_selection.registryID);}
- (void)startJoin:(std::shared_ptr<RTWControlJoin>)join ssid:(NSData*)ssid registryID:(uint64_t)registryID adapterGeneration:(uint64_t)generation {
    assert(join&&ssid.length==3&&_selection.matches(registryID,generation));++self.joins;
}
@end
static unsigned testAutomaticConnectionAdmission() {
    unsigned failures=0;
    for(BOOL visible:{NO,YES})for(unsigned state:{1U,2U,3U,4U}){
        AutomaticAdmissionApp* app=[AutomaticAdmissionApp new];
        [app prepareState:state scanning:state==1 visible:visible];
        [app considerAutoJoin];
        const bool violated=app.lookups||app.scans;
        std::printf("{\"state\":%u,\"scanning\":%s,\"visibleSavedSSID\":%s,\"automaticCredentialLookups\":%u,\"automaticScans\":%u,\"kernelRecoveryPreempted\":%s}\n",
            state,state==1?"true":"false",visible?"true":"false",app.lookups,app.scans,violated?"true":"false");
        failures+=violated;
    }
    for(unsigned mode=1;mode<=8;++mode){
        AutomaticAdmissionApp* app=[AutomaticAdmissionApp new];
        [app prepareState:0 scanning:NO visible:YES];[app beginRealLookup];[app changePolicy:mode];
        NSDate* deadline=[NSDate dateWithTimeIntervalSinceNow:5];
        while([[app valueForKey:@"credentialBusy"] boolValue]&&deadline.timeIntervalSinceNow>0)
            [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
        assert(![[app valueForKey:@"credentialBusy"] boolValue]);
        std::printf("{\"delayedCredentialMode\":%u,\"automaticJoins\":%u}\n",mode,app.joins);
        failures+=app.joins;
    }
    // A current idle request must still connect; explicit manual requests
    // retain their authority when driver recovery starts during Keychain I/O.
    for(unsigned mode=0;mode<=8;++mode){
        AutomaticAdmissionApp* app=[AutomaticAdmissionApp new];
        [app prepareState:0 scanning:NO visible:YES];
        if(mode)[app beginManualLookup];else [app beginRealLookup];
        [app changePolicy:mode];
        NSDate* deadline=[NSDate dateWithTimeIntervalSinceNow:5];
        while([[app valueForKey:@"credentialBusy"] boolValue]&&deadline.timeIntervalSinceNow>0)
            [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
        assert(![[app valueForKey:@"credentialBusy"] boolValue]);
        failures+=app.joins!=1;
    }
    AutomaticAdmissionApp* idle=[AutomaticAdmissionApp new];
    [idle prepareState:0 scanning:NO visible:YES];[idle considerAutoJoin];
    failures+=idle.lookups!=1||idle.scans!=0;
    AutomaticAdmissionApp* discovery=[AutomaticAdmissionApp new];
    [discovery prepareState:0 scanning:NO visible:NO];[discovery considerAutoJoin];
    failures+=discovery.lookups!=0||discovery.scans!=1;
    AutomaticAdmissionApp* disabled=[AutomaticAdmissionApp new];
    [disabled prepareState:0 scanning:NO visible:YES];[disabled changePolicy:8];[disabled considerAutoJoin];
    failures+=disabled.lookups||disabled.scans;
    AutomaticAdmissionApp* legacyDisabled=[AutomaticAdmissionApp new];
    [legacyDisabled prepareState:0 scanning:NO visible:YES];[legacyDisabled markLocalManualOff];[legacyDisabled considerAutoJoin];
    failures+=legacyDisabled.lookups||legacyDisabled.scans;
    std::puts("Actual Main automatic admission: recovery/manual-radio-off/delayed-Keychain cases and idle/manual controls");
    return failures;
}
