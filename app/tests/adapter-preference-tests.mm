// SPDX-License-Identifier: GPL-2.0-only
// Real Main selection/persistence. Only the read-only registry boundary is replaced.
#define main rtwui_application_main
#include "../Main.mm"
#undef main
#include <cassert>
@interface PreferenceApp : WiFiApp
@property(nonatomic,copy) NSDictionary<NSNumber*,NSDictionary*>* identities;
@property(nonatomic,assign) BOOL holdPolls;
- (void)prepareDriver:(RTWDriverConnection*)driver store:(id<RTWCredentialStoring>)store;
- (void)prepareDisconnectedLookup;
@end
@implementation PreferenceApp
- (NSDictionary*)adapterIdentityForRegistryID:(uint64_t)registryID {return self.identities[@(registryID)];}
- (void)updateAppearance {}
- (void)considerNetworkRefresh {}
- (void)poll:(id)sender {if(!self.holdPolls)[super poll:sender];}
- (double)uptime {return 100;}
- (void)prepareDisconnectedLookup {
    _hasStatus=YES;_statusReadDelayed=NO;_status.version=1;_status.state=0;
    _status.flags=RTW_CONTROL_DEVICE|RTW_CONTROL_RADIO;
}
- (void)prepareDriver:(RTWDriverConnection*)driver store:(id<RTWCredentialStoring>)store {
    _driver=driver;_credentials=store;_queue=serialWorkQueue("org.research.TahoeUSBWiFi.test.preference");
    _savedNetworksLoaded=YES;_savedNetworks=@[[@"HomeA" dataUsingEncoding:NSUTF8StringEncoding],[@"HomeB" dataUsingEncoding:NSUTF8StringEncoding]];
    _lastLinkedAt=_lastAutoScanAt=_lastDeviceListAt=_lastNetworkCacheAt=-1;
}
@end
static NSDictionary* identity(NSString* serial,unsigned port,NSData* mac=nil) {
    NSMutableDictionary* value=[@{@"vendor":@0x0bda,@"product":@0x8814,@"location":@(port)} mutableCopy];
    if(serial)value[@"serial"]=serial;if(mac)value[@"mac"]=mac;return value;
}
static rtwui::DeviceList devices(std::initializer_list<uint64_t> ids) {
    rtwui::DeviceList list{};list.version=rtwui::ipcVersion;
    for(auto id:ids){auto& d=list.devices[list.count++];d.registryID=id;std::strcpy(d.family,"RTL8814AU");}
    return list;
}
static PreferenceApp* app(NSUserDefaults* preferences,NSDictionary* identities) {
    PreferenceApp* value=[PreferenceApp new];value.identities=identities;
    [value setValue:preferences forKey:@"preferences"];
    [value setValue:@YES forKey:@"authorizedSession"];return value;
}
static void select(PreferenceApp* value,uint64_t id) {
    NSMenuItem* item=[[NSMenuItem alloc]initWithTitle:@"USB" action:nil keyEquivalent:@""];
    item.representedObject=@(id);[value selectAdapter:item];
}
@interface PreferenceStore : NSObject <RTWCredentialStoring>
@property(nonatomic,assign) unsigned reads;
@property(nonatomic,assign) BOOL interaction;
@end
@implementation PreferenceStore
- (NSMutableData*)passwordForSSID:(NSData*)ssid allowInteraction:(BOOL)interaction error:(NSError**)error {
    (void)error;assert(ssid.length==5);++self.reads;self.interaction=interaction;
    return [[@"password123" dataUsingEncoding:NSASCIIStringEncoding] mutableCopy];
}
- (NSMutableData*)passwordForSSID:(NSData*)ssid error:(NSError**)error {return [self passwordForSSID:ssid allowInteraction:YES error:error];}
- (BOOL)savePassword:(NSData*)password forSSID:(NSData*)ssid error:(NSError**)error {(void)password;(void)ssid;(void)error;assert(false);return NO;}
- (NSArray<NSData*>*)savedSSIDsAndReturnError:(NSError**)error {(void)error;return @[];}
- (BOOL)deletePasswordForSSID:(NSData*)ssid error:(NSError**)error {(void)ssid;(void)error;assert(false);return NO;}
@end
@interface PreferenceDriver : RTWDriverConnection
@property(nonatomic,assign) rtwui::DeviceList list;
@property(nonatomic,strong) NSMutableDictionary<NSNumber*,NSData*>* links;
@property(nonatomic,assign) unsigned joins;
@property(nonatomic,assign) uint64_t lastJoinID;
@end
@implementation PreferenceDriver
- (instancetype)init {self=[super init];if(self)_links=[NSMutableDictionary dictionary];return self;}
- (uint32_t)exchangeRegistryID:(uint64_t)registryID selector:(uint32_t)selector
                        input:(const void*)input length:(size_t)length output:(void*)output capacity:(size_t)capacity {
    assert(capacity==rtwui::outputSize(selector));
    if(selector==rtwui::listSelector){assert(!registryID&&!input&&!length);*static_cast<rtwui::DeviceList*>(output)=self.list;return 0;}
    assert(rtw_control_validate_request(selector,input,length)==RTW_CONTROL_VALID);
    if(selector==RTW_CONTROL_CHANNEL_MAP){auto* map=static_cast<RTWControlChannelMap*>(output);*map={};map->version=1;map->channels[0]=1U<<6;return 0;}
    if(selector==RTW_CONTROL_STATUS) {
        auto* status=static_cast<RTWControlStatus*>(output);*status={};status->version=1;
        status->flags=RTW_CONTROL_DEVICE|RTW_CONTROL_RADIO;NSData* ssid=self.links[@(registryID)];
        if(ssid){status->flags|=RTW_CONTROL_AUTHORIZED|RTW_CONTROL_RUNNING;status->state=4;
            status->ssidLength=ssid.length;std::memcpy(status->ssid,ssid.bytes,ssid.length);}
        return 0;
    }
    if(selector==RTW_CONTROL_SCAN_RESULTS) {
        auto* scan=static_cast<RTWControlScanResults*>(output);*scan={};scan->version=1;scan->count=2;
        for(unsigned i=0;i<2;++i){auto& row=scan->records[i];row.ssidLength=5;std::memcpy(row.ssid,i?"HomeB":"HomeA",5);row.rssi=-40;row.channel=6;row.securityFlags=7;}return 0;
    }
    assert(selector==RTW_CONTROL_JOIN);++self.joins;self.lastJoinID=registryID;
    const auto* join=static_cast<const RTWControlJoin*>(input);
    self.links[@(registryID)]=[NSData dataWithBytes:join->ssid length:join->ssidLength];
    auto* reply=static_cast<RTWControlReply*>(output);*reply={};reply->version=1;return 0;
}
@end
static void drain(PreferenceApp* value) {
    NSDate* deadline=[NSDate dateWithTimeIntervalSinceNow:5];
    for(unsigned stable=0;stable<3&&deadline.timeIntervalSinceNow>0;) {
        [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.005]];
        if([[value valueForKey:@"busy"] boolValue]||[[value valueForKey:@"credentialBusy"] boolValue]||
           [[value valueForKey:@"joining"] boolValue])stable=0;else ++stable;
    }
    assert(![[value valueForKey:@"busy"] boolValue]&&![[value valueForKey:@"credentialBusy"] boolValue]&&![[value valueForKey:@"joining"] boolValue]);
}
static void testNetworkPreferences(NSUserDefaults* preferences,NSDictionary* a,NSDictionary* b) {
    [preferences setBool:NO forKey:@"RTWAutoJoinEnabled"];
    PreferenceStore* store=[PreferenceStore new];PreferenceDriver* driver=[PreferenceDriver new];driver.list=devices({11,22});
    PreferenceApp* first=app(preferences,@{@11:a,@22:b});[first prepareDriver:driver store:store];[first applyAdapters:driver.list];
    select(first,11);[first connectSSID:[@"HomeA" dataUsingEncoding:NSUTF8StringEncoding]];drain(first);
    assert(driver.joins==1&&[[preferences objectForKey:@"RTWLastConnectedSSID"] isEqual:[@"HomeA" dataUsingEncoding:NSUTF8StringEncoding]]);
    select(first,22);[first connectSSID:[@"HomeB" dataUsingEncoding:NSUTF8StringEncoding]];drain(first);
    assert(driver.joins==2&&[[preferences arrayForKey:@"RTWAdapterNetworkPreferences"] count]==2);
    [preferences setBool:YES forKey:@"RTWAutoJoinEnabled"];
    PreferenceDriver* bootDriver=[PreferenceDriver new];bootDriver.list=devices({101,202});
    PreferenceApp* boot=app(preferences,@{@101:a,@202:b});[boot prepareDriver:bootDriver store:store];[boot applyAdapters:bootDriver.list];drain(boot);
    assert([boot selectedAdapterID]==202&&bootDriver.joins==1&&bootDriver.lastJoinID==202&&!store.interaction);
    assert([[boot valueForKey:@"preferredSSID"] isEqual:[@"HomeB" dataUsingEncoding:NSUTF8StringEncoding]]);
    select(boot,101);drain(boot);assert(bootDriver.joins==2&&bootDriver.lastJoinID==101);
    assert([[boot valueForKey:@"preferredSSID"] isEqual:[@"HomeA" dataUsingEncoding:NSUTF8StringEncoding]]);
    // Off/on during a delayed Keychain read cancels the old automatic request.
    boot.holdPolls=YES;
    [boot prepareDisconnectedLookup]; // the previous successful link must first be lost
    [boot loadPasswordForSSID:[@"HomeA" dataUsingEncoding:NSUTF8StringEncoding] connect:YES automatic:YES];
    [boot toggleAutoJoin:nil];[boot toggleAutoJoin:nil];
    NSDate* deadline=[NSDate dateWithTimeIntervalSinceNow:5];
    while([[boot valueForKey:@"credentialBusy"] boolValue]&&deadline.timeIntervalSinceNow>0)
        [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.005]];
    dispatch_sync([boot valueForKey:@"queue"],^{});
    assert(bootDriver.joins==2&&"an off/on toggle must invalidate an earlier automatic Keychain lookup");
    dispatch_queue_t queue=[boot valueForKey:@"queue"];
    dispatch_semaphore_t gate=dispatch_semaphore_create(0);
    dispatch_async(queue,^{assert(!dispatch_semaphore_wait(gate,dispatch_time(DISPATCH_TIME_NOW,5LL*NSEC_PER_SEC)));});
    [boot loadPasswordForSSID:[@"HomeA" dataUsingEncoding:NSUTF8StringEncoding] connect:YES automatic:YES];
    deadline=[NSDate dateWithTimeIntervalSinceNow:5];
    while(![[boot valueForKey:@"joining"] boolValue]&&deadline.timeIntervalSinceNow>0)
        [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.005]];
    assert([[boot valueForKey:@"joining"] boolValue]);
    [boot toggleAutoJoin:nil];dispatch_semaphore_signal(gate);dispatch_sync(queue,^{});
    assert(bootDriver.joins==2&&![[boot valueForKey:@"joining"] boolValue]);
    [preferences setBool:YES forKey:@"RTWAutoJoinEnabled"];
    [preferences setBool:YES forKey:@"RTWAutoJoinPaused"];
    PreferenceDriver* guardedDriver=[PreferenceDriver new];guardedDriver.list=devices({101,202});
    PreferenceApp* paused=app(preferences,@{@101:a,@202:b});[paused prepareDriver:guardedDriver store:store];
    [paused applyAdapters:guardedDriver.list];drain(paused);assert(guardedDriver.joins==0);
    [preferences setBool:NO forKey:@"RTWAutoJoinPaused"];[preferences setBool:NO forKey:@"RTWAutoJoinEnabled"];
    PreferenceApp* disabled=app(preferences,@{@101:a,@202:b});[disabled prepareDriver:guardedDriver store:store];
    [disabled applyAdapters:guardedDriver.list];drain(disabled);assert(guardedDriver.joins==0);
    puts("Actual Main network preference: confirmed manual per-USB SSIDs, restart noninteractive saved-Keychain reconnect and stale lookup cancellation passed");
}
static void testExistingLinkMigration(NSUserDefaults* preferences,NSDictionary* a,NSDictionary* b) {
    [preferences setBool:NO forKey:@"RTWAutoJoinEnabled"];
    [preferences setObject:[@"HomeB" dataUsingEncoding:NSUTF8StringEncoding] forKey:@"RTWLastConnectedSSID"];
    PreferenceDriver* driver=[PreferenceDriver new];driver.list=devices({11,22});
    driver.links[@22]=[@"HomeB" dataUsingEncoding:NSUTF8StringEncoding];
    PreferenceApp* upgrade=app(preferences,@{@11:a,@22:b});
    [upgrade prepareDriver:driver store:[PreferenceStore new]];[upgrade applyAdapters:driver.list];
    [upgrade poll:nil];drain(upgrade);
    assert([upgrade selectedAdapterID]==22&&driver.joins==0&&"upgrade should adopt the sole confirmed existing link without a JOIN");
    [preferences removeObjectForKey:@"RTWPreferredAdapter"];[preferences removeObjectForKey:@"RTWAdapterNetworkPreferences"];
    driver.links[@11]=[@"HomeA" dataUsingEncoding:NSUTF8StringEncoding];
    PreferenceApp* two=app(preferences,@{@11:a,@22:b});[two prepareDriver:driver store:[PreferenceStore new]];
    [two applyAdapters:driver.list];[two poll:nil];drain(two);assert([two selectedAdapterID]==0&&driver.joins==0);
    [driver.links removeObjectForKey:@11];[preferences setObject:[@"Other" dataUsingEncoding:NSUTF8StringEncoding] forKey:@"RTWLastConnectedSSID"];
    PreferenceApp* mismatch=app(preferences,@{@11:a,@22:b});[mismatch prepareDriver:driver store:[PreferenceStore new]];
    [mismatch applyAdapters:driver.list];[mismatch poll:nil];drain(mismatch);assert([mismatch selectedAdapterID]==0&&driver.joins==0);
    puts("Actual Main migration: unique current authorized USB is adopted without network mutation");
}
static void testDelayedIdentity(NSUserDefaults* preferences,NSDictionary* a,NSDictionary* b) {
    PreferenceApp* late=app(preferences,@{});[late applyAdapters:devices({11})];assert([late selectedAdapterID]==11);
    late.identities=@{@11:a};[late applyAdapters:devices({11})];
    PreferenceApp* reboot=app(preferences,@{@101:a,@202:b});[reboot applyAdapters:devices({101,202})];
    assert([reboot selectedAdapterID]==101&&"metadata published after first enumeration must still persist the selected USB");
}
static void testIdentityStrengthening(NSUserDefaults* preferences) {
    const uint8_t bytesA[]={2,1,2,3,4,5},bytesB[]={2,1,2,3,4,6};
    NSData* macA=[NSData dataWithBytes:bytesA length:6],*macB=[NSData dataWithBytes:bytesB length:6];
    [preferences setBool:NO forKey:@"RTWAutoJoinEnabled"];
    PreferenceDriver* driver=[PreferenceDriver new];driver.list=devices({11});
    PreferenceApp* partial=app(preferences,@{@11:identity(nil,200)});
    [partial prepareDriver:driver store:[PreferenceStore new]];[partial applyAdapters:driver.list];
    [partial connectSSID:[@"HomeA" dataUsingEncoding:NSUTF8StringEncoding]];drain(partial);
    partial.identities=@{@11:identity(@"original",200,macA)};[partial applyAdapters:driver.list];
    partial.identities=@{@11:identity(nil,200)};select(partial,11);
    assert([[preferences dictionaryForKey:@"RTWPreferredAdapter"][@"mac"] isEqual:macA]&&"reselecting the same instance must not downgrade its known durable identity");
    partial.identities=@{};select(partial,11);
    assert([[preferences dictionaryForKey:@"RTWPreferredAdapter"][@"mac"] isEqual:macA]&&"missing metadata must not erase the same present instance's known identity");
    partial.identities=@{@11:identity(@"original",200,macA)};[partial applyAdapters:driver.list];
    PreferenceApp* reboot=app(preferences,@{@101:identity(@"replacement",200,macB),@202:identity(@"original",300,macA)});
    [reboot applyAdapters:devices({101,202})];
    assert([reboot selectedAdapterID]==202&&"late durable metadata must strengthen an early port-only preference");
    assert([[reboot valueForKey:@"preferredSSID"] isEqual:[@"HomeA" dataUsingEncoding:NSUTF8StringEncoding]]);
}
static void testNetworkBeforeIdentity(NSUserDefaults* preferences,NSDictionary* a,NSDictionary* b) {
    [preferences setBool:NO forKey:@"RTWAutoJoinEnabled"];
    PreferenceDriver* driver=[PreferenceDriver new];driver.list=devices({22});
    PreferenceApp* late=app(preferences,@{});[late prepareDriver:driver store:[PreferenceStore new]];
    [late applyAdapters:driver.list];[late connectSSID:[@"HomeB" dataUsingEncoding:NSUTF8StringEncoding]];drain(late);
    late.identities=@{@22:b};[late applyAdapters:driver.list];
    assert([[late valueForKey:@"preferredSSID"] isEqual:[@"HomeB" dataUsingEncoding:NSUTF8StringEncoding]]&&"late identity must preserve the network already confirmed on this USB");
    PreferenceApp* reboot=app(preferences,@{@101:a,@202:b});[reboot applyAdapters:devices({101,202})];
    assert([reboot selectedAdapterID]==202&&[[reboot valueForKey:@"preferredSSID"] isEqual:[@"HomeB" dataUsingEncoding:NSUTF8StringEncoding]]);
}
static void testIncompleteDurableIdentity(NSUserDefaults* preferences) {
    const uint8_t bytes[]={2,1,2,3,4,5};
    NSData* mac=[NSData dataWithBytes:bytes length:6];
    [preferences setObject:identity(@"shared-serial",200,mac) forKey:@"RTWPreferredAdapter"];
    PreferenceApp* incomplete=app(preferences,@{@11:identity(@"shared-serial",200)});
    [incomplete applyAdapters:devices({11})];
    assert([incomplete selectedAdapterID]==0&&"missing recorded MAC must not let an early duplicate serial occupy the preference");
}
int main() {@autoreleasepool {
    NSString* suite=[@"org.research.TahoeUSBWiFi.preference-test." stringByAppendingString:NSUUID.UUID.UUIDString];
    NSUserDefaults* preferences=[[NSUserDefaults alloc]initWithSuiteName:suite];
    NSDictionary* a=identity(@"adapter-A",100),*b=identity(@"adapter-B",200);
    PreferenceApp* first=app(preferences,@{@11:a,@22:b});
    [first applyAdapters:devices({11,22})];assert([first selectedAdapterID]==0);
    select(first,22);assert([first selectedAdapterID]==22);
    // Removing persistent selection must fail this restart behavior.
    PreferenceApp* restarted=app(preferences,@{@101:a,@202:b});
    [restarted applyAdapters:devices({202,101})];
    assert([restarted selectedAdapterID]==202&&"preferred physical USB must survive RegistryID changes and enumeration order");
    // An early sole fallback must not overwrite or occupy the preferred choice.
    PreferenceApp* delayed=app(preferences,@{@101:a,@202:b});
    [delayed applyAdapters:devices({101})];assert([delayed selectedAdapterID]==0);
    [delayed applyAdapters:devices({})];[delayed applyAdapters:devices({101,202})];
    assert([delayed selectedAdapterID]==202);
    const auto token=[delayed adapterGeneration];
    [delayed applyAdapters:devices({101})];assert([delayed selectedAdapterID]==202);
    [delayed applyAdapters:devices({})];
    [delayed applyAdapters:devices({101})];assert([delayed selectedAdapterID]==202);
    delayed.identities=@{@101:a,@303:identity(@"adapter-B",999)};
    [delayed applyAdapters:devices({101,303})];
    assert([delayed selectedAdapterID]==303&&[delayed adapterGeneration]>token);
    // Duplicate serials cannot lend another physical adapter the preference.
    PreferenceApp* ambiguous=app(preferences,@{@301:identity(@"adapter-B",300),@302:identity(@"adapter-B",400)});
    [ambiguous applyAdapters:devices({301,302})];assert([ambiguous selectedAdapterID]==0);
    PreferenceApp* duplicated=app(preferences,@{@301:identity(@"adapter-B",200),@302:identity(@"adapter-B",400)});
    [duplicated applyAdapters:devices({301,302})];assert([duplicated selectedAdapterID]==0&&"a duplicate serial cannot become unique merely by reusing the saved port");
    // Explicit user selection replaces the choice even while the old USB is absent.
    select(delayed,101);
    PreferenceApp* changed=app(preferences,@{@801:a,@802:b});
    [changed applyAdapters:devices({801,802})];assert([changed selectedAdapterID]==801);
    [preferences removePersistentDomainForName:suite];
    testNetworkPreferences(preferences,a,b);
    [preferences removePersistentDomainForName:suite];
    testExistingLinkMigration(preferences,a,b);
    [preferences removePersistentDomainForName:suite];
    testDelayedIdentity(preferences,a,b);
    [preferences removePersistentDomainForName:suite];
    testIdentityStrengthening(preferences);
    [preferences removePersistentDomainForName:suite];
    testNetworkBeforeIdentity(preferences,a,b);
    [preferences removePersistentDomainForName:suite];
    testIncompleteDurableIdentity(preferences);
    [preferences removePersistentDomainForName:suite];
    puts("Actual Main adapter preference: reboot IDs/order, delayed enumeration, unplug/rebind, ambiguity and explicit override passed");
}}
