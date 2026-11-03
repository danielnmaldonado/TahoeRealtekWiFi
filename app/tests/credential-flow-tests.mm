// SPDX-License-Identifier: GPL-2.0-only
// Native tests of the real controller flow. No actual Keychain, USB, XPC,
// login registration or production UserDefaults domain is touched.
#define main rtwui_application_main
#include "../Main.mm"
#undef main
#include <cassert>

@interface FlowCredentialStore : NSObject <RTWCredentialStoring>
@property(nonatomic,strong) NSData* password;
@property(nonatomic,strong) NSError* error;
@property(nonatomic,strong) NSMutableData* lastRead;
@property(nonatomic,assign) NSUInteger reads;
@property(nonatomic,assign) BOOL interaction;
@end
@implementation FlowCredentialStore
- (NSMutableData*)passwordForSSID:(NSData*)ssid error:(NSError**)error {
    return [self passwordForSSID:ssid allowInteraction:YES error:error];
}
- (NSMutableData*)passwordForSSID:(NSData*)ssid allowInteraction:(BOOL)interaction error:(NSError**)error {
    assert(ssid.length);self.reads++;self.interaction=interaction;
    if(error)*error=self.error;
    self.lastRead=self.error?nil:[self.password mutableCopy];
    return self.lastRead;
}
- (BOOL)savePassword:(NSData*)password forSSID:(NSData*)ssid error:(NSError**)error {
    assert(ssid.length);if(error)*error=nil;self.password=[password copy];return YES;
}
- (BOOL)deletePasswordForSSID:(NSData*)ssid error:(NSError**)error {
    assert(ssid.length);if(error)*error=nil;self.password=nil;return YES;
}
- (NSArray<NSData*>*)savedSSIDsAndReturnError:(NSError**)error {
    if(error)*error=nil;return @[];
}
@end

@interface FlowWiFiApp : WiFiApp
@property(nonatomic,assign) NSUInteger joins;
@property(nonatomic,assign) NSUInteger prompts;
@property(nonatomic,assign) NSUInteger messages;
- (void)prepareObservedSSID:(NSData*)ssid;
@end
@implementation FlowWiFiApp
- (void)prepareObservedSSID:(NSData*)ssid {
    _hasStatus=YES;_statusReadDelayed=NO;_status.version=1;
    _status.flags=RTW_CONTROL_DEVICE|RTW_CONTROL_RADIO;
    _channelMap.setLegacy(_selection,_selection.registryID,_selection.generation);
    _scan={};_scan.version=1;_scan.count=1;
    auto& row=_scan.records[0];row.channel=6;row.securityFlags=7;
    row.ssidLength=static_cast<uint8_t>(ssid.length);std::memcpy(row.ssid,ssid.bytes,ssid.length);
}
- (void)startJoin:(std::shared_ptr<RTWControlJoin>)join ssid:(NSData*)ssid
      registryID:(uint64_t)registryID adapterGeneration:(uint64_t)generation {
    assert(join&&ssid.length&&registryID==[self selectedAdapterID]);
    assert(generation==[self adapterGeneration]);
    assert(rtw_control_validate_request(RTW_CONTROL_JOIN,join.get(),sizeof(*join))==RTW_CONTROL_VALID);
    self.joins++;
}
- (void)promptPasswordForSSID:(NSData*)ssid saved:(NSMutableData*)saved
                    readError:(NSError*)error connect:(BOOL)connect
                   registryID:(uint64_t)registryID adapterGeneration:(uint64_t)generation
         connectionGeneration:(uint64_t)connectionGeneration {
    (void)ssid;(void)error;(void)connect;(void)registryID;(void)generation;(void)connectionGeneration;
    RTWWipePassword(saved);self.prompts++;[self setValue:@NO forKey:@"credentialBusy"];
}
- (void)showMessage:(NSString*)title detail:(NSString*)detail {
    (void)title;(void)detail;self.messages++;
}
@end
static void drainCredentials(FlowWiFiApp* app) {
    NSDate* deadline=[NSDate dateWithTimeIntervalSinceNow:5.0];
    while([[app valueForKey:@"credentialBusy"] boolValue]&&deadline.timeIntervalSinceNow>0)
        [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
    assert(![[app valueForKey:@"credentialBusy"] boolValue]);
}
int main() {
    @autoreleasepool {
        [NSApplication sharedApplication];
        NSString* suite=[@"org.research.TahoeUSBWiFi.test." stringByAppendingString:NSUUID.UUID.UUIDString];
        NSUserDefaults* preferences=[[NSUserDefaults alloc]initWithSuiteName:suite];
        FlowCredentialStore* store=[FlowCredentialStore new];
        FlowWiFiApp* app=[FlowWiFiApp new];
        [app setValue:preferences forKey:@"preferences"];
        [app setValue:store forKey:@"credentials"];
        [app setValue:@YES forKey:@"authorizedSession"];
        // Match applicationDidFinishLaunching's default. This fixture does
        // not run launch setup because it must not touch the real service.
        [app setValue:@YES forKey:@"autoJoinEnabled"];
        rtwui::DeviceList list{};list.version=rtwui::ipcVersion;list.count=1;
        list.devices[0].registryID=101;
        [app applyAdapters:list];
        NSData* ssid=[@"Lab" dataUsingEncoding:NSUTF8StringEncoding];
        [app prepareObservedSSID:ssid];
        store.password=[@"password123" dataUsingEncoding:NSASCIIStringEncoding];

        [app connectSSID:ssid];drainCredentials(app);
        assert(app.joins==1&&app.prompts==0&&app.messages==0);
        assert(store.interaction&&store.lastRead.length==0);

        // Repeated selection reads the saved item, never asks for the password.
        [app connectSSID:ssid];drainCredentials(app);
        assert(app.joins==2&&app.prompts==0&&store.reads==2);

        [app loadPasswordForSSID:ssid connect:YES automatic:YES];drainCredentials(app);
        assert(app.joins==3&&!store.interaction&&app.prompts==0);

        // A lookup started before the user disables automatic reconnection
        // must not deliver a late automatic JOIN on the main queue.
        [app loadPasswordForSSID:ssid connect:YES automatic:YES];
        [app setValue:@NO forKey:@"autoJoinEnabled"];drainCredentials(app);
        assert(app.joins==3&&app.prompts==0&&store.lastRead.length==0);
        [app setValue:@YES forKey:@"autoJoinEnabled"];

        store.error=[NSError errorWithDomain:NSOSStatusErrorDomain
            code:errSecInteractionNotAllowed userInfo:nil];
        [app loadPasswordForSSID:ssid connect:YES automatic:YES];drainCredentials(app);
        assert(app.joins==3&&app.prompts==0&&app.messages==0&&store.password.length>0);
        [app connectSSID:ssid];drainCredentials(app);
        assert(app.joins==3&&app.prompts==0&&app.messages==1);

        store.error=nil;store.password=nil;
        [app connectSSID:ssid];drainCredentials(app);
        assert(app.joins==3&&app.prompts==1);
        [app loadPasswordForSSID:ssid connect:YES automatic:YES];drainCredentials(app);
        assert(app.joins==3&&app.prompts==1); // No automatic modal fallback.

        store.password=[@"password123" dataUsingEncoding:NSASCIIStringEncoding];
        [app loadPasswordForSSID:ssid connect:YES automatic:YES];
        // An adapter change invalidates a Keychain result already in flight.
        list.devices[0].registryID=202;[app applyAdapters:list];drainCredentials(app);
        assert([app selectedAdapterID]==202&&app.joins==3&&app.prompts==1);
        assert(store.lastRead.length==0);

        [app connectSSID:ssid];[app connectSSID:ssid];drainCredentials(app);
        assert(app.joins==4); // Only one outstanding credential operation.
        [preferences removePersistentDomainForName:suite];
        puts("credential-flow: saved reuse, noninteractive autojoin, locked/missing Keychain, hotplug and duplicate-action guards passed");
    }
}
