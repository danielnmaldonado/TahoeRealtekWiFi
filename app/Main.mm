// SPDX-License-Identifier: GPL-2.0-only
#import <AppKit/AppKit.h>
#include <IOKit/IOReturn.h>
#include "Model.hpp"
#include "SignalHistory.hpp"
#include "PollPolicy.hpp"
#include "NetworkRefreshPolicy.hpp"
#include "WorkEpoch.hpp"
#import "LoginItem.h"
#include "LoginPolicy.hpp"
#import "CredentialStore.h"
#import "DriverConnection.h"
#import "CodeIdentity.h"
#include "ReconnectPolicy.hpp"
#import "AdapterPreference.h"
#include "InstanceLock.hpp"
#include "Transport.hpp"
#import "AdapterDiagnostics.h"
#import "DiagnosticCLI.h"
#import "EnglishErrors.h"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if_dl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <unistd.h>
#include <memory>
#include <map>

static NSString* text(const std::string& value) {
    return [[NSString alloc] initWithBytes:value.data() length:value.size() encoding:NSUTF8StringEncoding] ?: @"[SSID]";
}
static dispatch_queue_t serialWorkQueue(const char* label) {
    dispatch_queue_attr_t attributes=dispatch_queue_attr_make_with_autorelease_frequency(
        DISPATCH_QUEUE_SERIAL,DISPATCH_AUTORELEASE_FREQUENCY_WORK_ITEM);
    return dispatch_queue_create(label,attributes);
}
static NSString* boundedMenuTitle(NSString* title) {
    NSString* singleLine=[[title componentsSeparatedByCharactersInSet:
        NSCharacterSet.newlineCharacterSet] componentsJoinedByString:@" "];
    if(singleLine.length<=90)return singleLine;
    NSRange prefix=[singleLine rangeOfComposedCharacterSequencesForRange:NSMakeRange(0,89)];
    return [[singleLine substringWithRange:prefix] stringByAppendingString:@"…"];
}
static NSMenu* menuForControl() {
    NSMenu* menu=[[NSMenu alloc]initWithTitle:@"Tahoe Wi-Fi USB"];
    menu.autoenablesItems=NO;menu.minimumWidth=300;return menu;
}
static NSString* ssidText(const uint8_t* bytes,size_t length) {return text(rtwui::displaySSID(bytes,length));}
// SF Symbols are immutable menu-bar assets; do not allocate a new NSImage per poll.
static unsigned wifiImageIndex(int rssi,BOOL slash) {
    return slash ? 0U : rssi>=0 ? 1U : 1U+rtwui::signalLevel(rssi);
}
static NSImage* wifiImageForLevel(unsigned level,BOOL slash=NO) {
    static NSImage* images[5]{}; // slash, unavailable, weak, medium, strong
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        images[0]=[NSImage imageWithSystemSymbolName:@"wifi.slash" accessibilityDescription:@"USB Wi-Fi disconnected"];
        images[1]=[NSImage imageWithSystemSymbolName:@"wifi.exclamationmark" accessibilityDescription:@"USB Wi-Fi status or signal unavailable"];
        for (unsigned level=1;level<=3;++level)
            images[level+1]=[NSImage imageWithSystemSymbolName:@"wifi" variableValue:level/3.0 accessibilityDescription:@"USB Wi-Fi signal"];
        for(unsigned i=0;i<5;++i) {
            if(!images[i])images[i]=[NSImage imageWithSystemSymbolName:@"wifi" accessibilityDescription:@"Wi-Fi USB"];
            images[i].size=NSMakeSize(18,18);
            [images[i] setTemplate:YES];
        }
    });
    return images[slash?0U:1U+std::min(level,3U)];
}
static NSImage* wifiImage(int rssi,BOOL slash=NO) {
    const unsigned index=wifiImageIndex(rssi,slash);
    return wifiImageForLevel(index>1?index-1:0,slash);
}
static NSString* errorText(uint32_t code) {
    switch(code) {
    case uint32_t(kIOReturnNotFound):case uint32_t(kIOReturnNoDevice):return @"The kernel extension is unavailable. Check the USB adapter and driver loading.";
    case uint32_t(kIOReturnNotAttached):return @"The adapter was disconnected.";
    case uint32_t(kIOReturnNotReady):return @"The radio is unavailable. Check the USB adapter or restart the driver.";
    case uint32_t(kIOReturnExclusiveAccess):return @"The selected adapter is busy.";
    case uint32_t(kIOReturnNoResources):return @"More than 16 adapters were found. No adapter was selected automatically.";
    case uint32_t(kIOReturnNotPrivileged):return @"Administrative access to the driver was denied.";
    case rtwui::protocolError: return @"Incompatible response. Update the app and helper (IPC v2; kernel ABI v1).";
    case rtwui::transportError: return @"The service is unavailable. Reconnecting automatically.";
    default:return [NSString stringWithFormat:@"The driver returned error 0x%08x.",code];
    }
}
static NSString* interfaceInfo(const uint8_t* mac) {
    ifaddrs* addresses=nullptr;if(getifaddrs(&addresses))return @"USB interface has no address";
    std::string name,ip;
    for (auto* p=addresses;p;p=p->ifa_next) {
        if(!p->ifa_addr || p->ifa_addr->sa_family!=AF_LINK)continue;
        const auto* link=reinterpret_cast<const sockaddr_dl*>(p->ifa_addr);
        if(link->sdl_alen==6 && !memcmp(LLADDR(link),mac,6)) {name=p->ifa_name;break;}
    }
    if(!name.empty()) for(auto* p=addresses;p;p=p->ifa_next) {
        if(!p->ifa_addr || p->ifa_addr->sa_family!=AF_INET || name!=p->ifa_name)continue;
        auto* in=reinterpret_cast<sockaddr_in*>(p->ifa_addr);char buffer[INET_ADDRSTRLEN]{};
        const auto host=ntohl(in->sin_addr.s_addr);
        if(host && (host>>16)!=0xa9fe && inet_ntop(AF_INET,&in->sin_addr,buffer,sizeof(buffer)))ip=buffer;
    }
    freeifaddrs(addresses);
    if(name.empty())return @"USB interface not yet available";
    return ip.empty()?[NSString stringWithFormat:@"%@ · waiting for DHCP",text(name)]:
        [NSString stringWithFormat:@"%@ · %@",text(name),text(ip)];
}
// Older app versions did not save the chosen USB. Adopt an already confirmed
// sole link once, rather than infer user intent from enumeration or route order.
// Called only on the control queue, and every listed adapter must be readable.
static NSDictionary* existingAdapterLink(RTWDriverConnection* driver,const rtwui::DeviceList& list,NSData* preferredSSID) {
    if(!rtwui::validDevices(list)||!list.count)return nil;
    NSDictionary* result=nil;
    for(unsigned i=0;i<list.count;++i) {
        RTWControlRequest request{};request.version=RTW_CONTROL_VERSION;RTWControlStatus status{};
        if([driver exchangeRegistryID:list.devices[i].registryID selector:RTW_CONTROL_STATUS
            input:&request length:sizeof(request) output:&status capacity:sizeof(status)]||
           status.version!=RTW_CONTROL_VERSION||status.ssidLength>32||status.state>4)return nil;
        if(rtwui::phase(status)!=rtwui::Phase::Connected)continue;
        if(result||!status.ssidLength)return nil;
        NSData* ssid=[NSData dataWithBytes:status.ssid length:status.ssidLength];
        result=@{@"registryID":@(list.devices[i].registryID),@"ssid":ssid};
    }
    return result&&(!preferredSSID||[result[@"ssid"] isEqual:preferredSSID])?result:nil;
}

@interface WiFiApp : NSObject <NSApplicationDelegate,NSMenuDelegate> {
    NSStatusItem* _item;
    int _instanceLock;
    NSMenu* _menu;
    NSMenuItem* _header;
    NSTimer* _timer;
    id<RTWLoginItemManaging> _loginItems;
    NSUserDefaults* _preferences;
    id<RTWCredentialStoring> _credentials;
    dispatch_queue_t _credentialQueue;
    BOOL _credentialBusy,_terminating,_adapterReportBusy;
    dispatch_queue_t _queue;
    RTWDriverConnection* _driver;
    RTWDriverServiceManager* _serviceManager;
    rtwui::RetryGate _serviceRetry;
    rtwui::AutoJoinPolicy _autoJoin;
    BOOL _sleeping,_inactiveSession,_autoJoinEnabled;
    BOOL _savedNetworksLoaded,_loadingSavedNetworks,_reloadSavedAgain;
    NSArray<NSData*>* _savedNetworks;
    NSData* _preferredSSID;
    NSData* _legacyPreferredSSID;
    NSDictionary* _preferredAdapter;
    NSArray<NSDictionary*>* _adapterProfiles;
    NSDictionary<NSNumber*,NSDictionary*>* _adapterIdentities;
    NSDictionary* _existingAdapterLink;
    BOOL _connectionPreferencesLoaded;
    uint64_t _preferredAdapterPendingID;
    NSData* _pendingNetworkSSID;
    uint64_t _pendingNetworkRegistryID;
    double _lastLinkedAt,_lastAutoScanAt,_lastSavedReloadAt;
    BOOL _rememberCurrentJoin,_soleAdapterRecovery;
    BOOL _automaticCurrentJoin;
    std::atomic<uint64_t> _autoJoinEpoch;
    BOOL _authorizedSession,_authorizing,_busy,_hasStatus,_menuOpen,_joining,_joinAccepted,_diagnostic,_diagnosticPrinted;
    uint64_t _joinGeneration;
    rtwui::DeviceSelection _selection;
    rtwui::ChannelMapState _channelMap;
    std::vector<rtwui::DeviceRecord> _adapters;
    BOOL _selectedPresent,_statusReadDelayed,_forceDeviceList,_forceNetworkCache,_pollNotice;
    double _lastStatusSuccess,_lastDeviceListAt,_lastNetworkCacheAt,_lastInterfaceAt;
    double _joinStartedAt;
    uint64_t _pollToken;
    std::atomic<uint64_t> _adapterEpoch;
    rtwui::WorkEpoch _workEpoch;
    BOOL _commandPending,_refreshingNetworks;
    std::map<uint64_t,rtwui::NetworkRefreshPolicy> _networkRefresh;
    double _refreshStartedAt;
    NSInteger _displayedIconKind;
    rtwui::SignalMemory _signal;
    rtwui::SignalHistory _signalHistory;
    NSMenuItem* _signalRow;
    RTWControlStatus _status;
    RTWControlScanResults _scan;
    std::vector<RTWControlScanRecord> _networks;
    NSString* _notice;
    NSString* _interface;
    NSData* _joinSSID;
}
@end

@implementation WiFiApp
- (instancetype)init {
    self=[super init];
    if(self)_instanceLock=-1;
    return self;
}
- (NSUserDefaults*)preferences {
    if(!_preferences)_preferences=NSUserDefaults.standardUserDefaults;
    return _preferences;
}
- (void)loadConnectionPreferences {
    if(_connectionPreferencesLoaded)return;
    _connectionPreferencesLoaded=YES;
    NSUserDefaults* defaults=[self preferences];
    [defaults registerDefaults:@{@"RTWAutoJoinEnabled":@YES}];
    _autoJoinEnabled=[defaults boolForKey:@"RTWAutoJoinEnabled"];
    _autoJoin.paused=[defaults boolForKey:@"RTWAutoJoinPaused"];
    id ssid=[defaults objectForKey:@"RTWLastConnectedSSID"];
    if([ssid isKindOfClass:NSData.class]&&[ssid length]>0&&[ssid length]<=32)
        _preferredSSID=_legacyPreferredSSID=[ssid copy];
    _preferredAdapter=rtwui::adapterIdentity([defaults objectForKey:@"RTWPreferredAdapter"]);
    NSMutableArray* profiles=[NSMutableArray array];
    id saved=[defaults objectForKey:@"RTWAdapterNetworkPreferences"];
    if([saved isKindOfClass:NSArray.class]&&[saved count]<=rtwui::maxDevices)
        for(id profile in saved) {
            if(![profile isKindOfClass:NSDictionary.class])continue;
            NSDictionary* identity=rtwui::adapterIdentity(profile[@"identity"]);id network=profile[@"ssid"];
            if(identity&&[network isKindOfClass:NSData.class]&&[network length]>0&&[network length]<=32)
                [profiles addObject:@{@"identity":identity,@"ssid":network}];
        }
    _adapterProfiles=[profiles copy];
}
- (NSDictionary*)adapterIdentityForRegistryID:(uint64_t)registryID {
    return _adapterIdentities[@(registryID)];
}
- (void)restoreAdapterNetworkPreference {
    NSDictionary* identity=rtwui::adapterIdentity([self adapterIdentityForRegistryID:_selection.registryID]);
    unsigned best=0,matches=0;NSData* ssid=nil;
    for(NSDictionary* profile in _adapterProfiles) {
        const unsigned score=rtwui::adapterIdentityMatch(profile[@"identity"],identity);
        if(score>best){best=score;matches=1;ssid=profile[@"ssid"];}
        else if(score&&score==best)++matches;
    }
    _preferredSSID=matches==1?ssid:!_adapterProfiles.count?_legacyPreferredSSID:nil;
}
- (void)rememberAdapter:(uint64_t)registryID {
    NSDictionary* identity=rtwui::adapterIdentity([self adapterIdentityForRegistryID:registryID]);
    if(!identity&&registryID==_selection.registryID&&_selectedPresent)identity=_preferredAdapter;
    if(registryID==_selection.registryID&&_selectedPresent&&
       rtwui::adapterIdentityMatch(identity,_preferredAdapter)) {
        NSMutableDictionary* strongest=[_preferredAdapter mutableCopy];
        [strongest addEntriesFromDictionary:identity];identity=[strongest copy];
    }
    _preferredAdapterPendingID=identity?0:registryID;_preferredAdapter=identity;
    if(identity)[[self preferences] setObject:identity forKey:@"RTWPreferredAdapter"];
    else [[self preferences] removeObjectForKey:@"RTWPreferredAdapter"];
}
- (void)rememberConnectedNetwork:(NSData*)ssid {
    if(!ssid.length||ssid.length>32)return;
    _preferredSSID=[ssid copy];
    [[self preferences] setObject:_preferredSSID forKey:@"RTWLastConnectedSSID"];
    NSDictionary* identity=rtwui::adapterIdentity([self adapterIdentityForRegistryID:_selection.registryID]);
    if(!identity){_pendingNetworkSSID=_preferredSSID;_pendingNetworkRegistryID=_selection.registryID;return;}
    _pendingNetworkSSID=nil;_pendingNetworkRegistryID=0;
    [self rememberAdapter:_selection.registryID];
    identity=_preferredAdapter;
    NSMutableArray* profiles=[NSMutableArray array];
    for(NSDictionary* profile in _adapterProfiles)
        if(!rtwui::adapterIdentityMatch(profile[@"identity"],identity))[profiles addObject:profile];
    while(profiles.count>=rtwui::maxDevices)[profiles removeObjectAtIndex:0];
    [profiles addObject:@{@"identity":identity,@"ssid":_preferredSSID}];
    _adapterProfiles=[profiles copy];
    [[self preferences] setObject:_adapterProfiles forKey:@"RTWAdapterNetworkPreferences"];
}
- (void)strengthenSelectedAdapterIdentity {
    NSDictionary* current=rtwui::adapterIdentity([self adapterIdentityForRegistryID:_selection.registryID]);
    if(!rtwui::adapterIdentityMatch(_preferredAdapter,current))return;
    NSMutableDictionary* strongest=[_preferredAdapter mutableCopy];[strongest addEntriesFromDictionary:current];
    if([strongest isEqual:_preferredAdapter])return;
    // Metadata can appear after the controller itself. Upgrade only this
    // still-selected instance and never discard an already recorded ID.
    unsigned best=0,matches=0;NSUInteger index=NSNotFound;
    for(NSUInteger i=0;i<_adapterProfiles.count;++i) {
        const unsigned score=rtwui::adapterIdentityMatch(_adapterProfiles[i][@"identity"],_preferredAdapter);
        if(score>best){best=score;matches=1;index=i;}else if(score&&score==best)++matches;
    }
    _preferredAdapter=[strongest copy];
    [[self preferences] setObject:_preferredAdapter forKey:@"RTWPreferredAdapter"];
    if(matches==1) {
        NSMutableArray* profiles=[_adapterProfiles mutableCopy];
        profiles[index]=@{@"identity":_preferredAdapter,@"ssid":_adapterProfiles[index][@"ssid"]};
        _adapterProfiles=[profiles copy];
        [[self preferences] setObject:_adapterProfiles forKey:@"RTWAdapterNetworkPreferences"];
    }
}
- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    (void)notification;
    _instanceLock=-1;_lastSavedReloadAt=-1;_lastLinkedAt=-1;_lastAutoScanAt=-1;_lastStatusSuccess=-1;_lastDeviceListAt=-1;_lastNetworkCacheAt=-1;
    _lastInterfaceAt=-1;_joinStartedAt=-1;_displayedIconKind=-1;
    _forceDeviceList=YES;_forceNetworkCache=YES;
    _queue=serialWorkQueue("org.research.TahoeUSBWiFi.control");
    _driver=[RTWDriverConnection new];
    _serviceManager=[RTWDriverServiceManager new];
    _loginItems=[RTWLoginItemManager new];
    _credentials=[RTWCredentialStore new];
    _credentialQueue=serialWorkQueue("org.research.TahoeUSBWiFi.credentials");
    if(geteuid()==0){fprintf(stderr,"Run this app as a normal user.\n");[NSApp terminate:nil];return;}
    NSURL* support=[NSFileManager.defaultManager URLForDirectory:NSApplicationSupportDirectory
        inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:nil];
    NSString* lockDirectory=[support.path stringByAppendingPathComponent:@"org.research.TahoeUSBWiFi"];
    _instanceLock=rtwui::acquireInstanceLock(lockDirectory.fileSystemRepresentation);
    if(_instanceLock<0) {
        fprintf(stderr,"Another instance is running, or the private instance lock is unavailable.\n");
        [NSApp terminate:nil];return;
    }
    _diagnostic=[NSProcessInfo.processInfo.arguments containsObject:@"--diagnose"];
    _item=[NSStatusBar.systemStatusBar statusItemWithLength:NSVariableStatusItemLength];
    _item.autosaveName=@"TahoeUSBWiFi";_item.visible=YES;_item.button.image=wifiImage(0);
    _item.button.toolTip=@"Tahoe Wi-Fi USB · connecting to service";
    _menu=menuForControl();_menu.delegate=self;_item.menu=_menu;
    NSUserDefaults* defaults=[self preferences];
    [self loadConnectionPreferences];
    _notice=@"Connecting to the driver service automatically…";
    [self rebuildMenu];
    [self refreshPollingTimer];
    NSNotificationCenter* workspace=NSWorkspace.sharedWorkspace.notificationCenter;
    [workspace addObserver:self selector:@selector(workspaceWillSleep:)
        name:NSWorkspaceWillSleepNotification object:nil];
    [workspace addObserver:self selector:@selector(workspaceDidWake:)
        name:NSWorkspaceDidWakeNotification object:nil];
    [workspace addObserver:self selector:@selector(workspaceBecameInactive:)
        name:NSWorkspaceSessionDidResignActiveNotification object:nil];
    [workspace addObserver:self selector:@selector(workspaceBecameActive:)
        name:NSWorkspaceSessionDidBecomeActiveNotification object:nil];
    [self reloadSavedNetworks];
    // A .pkg installed without a GUI session cannot pass --complete-install.
    // Complete the same setup on the first normal launch of the protected copy.
    // Existing explicit startup/service choices remain in configureInstallation.
    const BOOL pendingInstallation=![defaults boolForKey:@"RTWServiceSetupComplete"] ||
        [defaults boolForKey:@"RTWResumeServiceAfterInstall"] ||
        [defaults boolForKey:@"RTWResumeLoginAfterInstall"];
    if([NSProcessInfo.processInfo.arguments containsObject:@"--complete-install"] ||
       (pendingInstallation && RTWProtectedBundle(NSBundle.mainBundle.bundlePath)))
        [self configureInstallation];
    [self connectDriverAutomatically];
}
- (NSMenuItem*)add:(NSString*)title action:(SEL)action enabled:(BOOL)enabled {
    NSString* visible=boundedMenuTitle(title);
    NSMenuItem* entry=[[NSMenuItem alloc]initWithTitle:visible action:action keyEquivalent:@""];
    if(![visible isEqualToString:title])entry.toolTip=title;
    entry.target=self;entry.enabled=enabled;[_menu addItem:entry];return entry;
}
- (id<RTWLoginItemManaging>)loginItems {
    if(!_loginItems)_loginItems=[RTWLoginItemManager new];
    return _loginItems;
}
- (id<RTWCredentialStoring>)credentials {
    if(!_credentials)_credentials=[RTWCredentialStore new];
    return _credentials;
}
- (dispatch_queue_t)credentialQueue {
    if(!_credentialQueue)_credentialQueue=serialWorkQueue(
        "org.research.TahoeUSBWiFi.credentials");
    return _credentialQueue;
}
- (double)uptime {return NSProcessInfo.processInfo.systemUptime;}
- (void)refreshPollingTimer {
    if(!_item)return;
    const double interval=rtwui::pollInterval(_terminating||_sleeping||_inactiveSession,
        _menuOpen,_joining,_hasStatus&&rtwui::phase(_status)==rtwui::Phase::Connected,
        _selectedPresent,_authorizedSession);
    if(interval>0&&_timer.valid&&_timer.timeInterval==interval)return;
    [_timer invalidate];_timer=nil;
    if(interval<=0)return;
    __weak WiFiApp* weakSelf=self;
    _timer=[NSTimer timerWithTimeInterval:interval repeats:YES block:^(NSTimer* timer) {
        [weakSelf poll:timer];
    }];
    _timer.tolerance=rtwui::pollTolerance(interval);
    [NSRunLoop.mainRunLoop addTimer:_timer forMode:NSRunLoopCommonModes];
    [NSRunLoop.mainRunLoop addTimer:_timer forMode:NSModalPanelRunLoopMode];
}
- (void)invalidatePendingWork {
    _workEpoch.invalidate();++_pollToken;
    _channelMap.clear();
    _busy=NO;_authorizing=NO;_commandPending=NO;_refreshingNetworks=NO;
    _refreshStartedAt=-1;
}
- (rtwui::LinkIndicator)linkIndicator {
    return rtwui::linkIndicator(_authorizedSession,_selection.registryID!=0,_selectedPresent,
        _hasStatus,_statusReadDelayed,_joining,_status,[self uptime],_lastStatusSuccess);
}
- (NSString*)phaseTitle {
    if(!_authorizedSession)return _authorizing?@"Connecting to service…":@"Driver service unavailable";
    if(!_selection.registryID)return _adapters.empty()?@"No adapter available":@"Select an adapter";
    if(!_selectedPresent)return @"Selected adapter disconnected";
    if(!_hasStatus)return @"Checking adapter…";
    if(_statusReadDelayed)return [self linkIndicator]==rtwui::LinkIndicator::Connected ?
        @"Last confirmed connection · checking driver…" : @"Driver status unavailable · checking…";
    if(_refreshingNetworks)return @"Refreshing networks…";
    if(_joining&&!_joinAccepted)return @"Preparing connection…";
    switch(rtwui::phase(_status)) {
    case rtwui::Phase::Absent:return @"USB adapter disconnected";
    case rtwui::Phase::RadioOff:return @"Radio off";
    case rtwui::Phase::Scanning:return _joining?@"Looking for the network…":@"Looking for networks…";
    case rtwui::Phase::Associating:return @"Associating with network…";
    case rtwui::Phase::Authenticating:return @"Authenticating WPA2…";
    case rtwui::Phase::Connected:return _joining?@"Connecting…":@"Connected via WPA2";
    case rtwui::Phase::Error:return @"USB communication error";
    case rtwui::Phase::Idle:return _joining?@"Connecting…":@"Ready to connect";
    }
}
- (NSString*)signalSummary {
    const double now=[self uptime];
    const int raw=_signal.rawValue(now),filtered=_signal.value(now);
    const BOOL held=_statusReadDelayed||!rtwui::validSignal(_status.rssi)||
        (_status.flags&RTW_CONTROL_SIGNAL_HELD);
    NSString* signal=raw?[NSString stringWithFormat:@"%d dBm %@ · %d dBm filtered · %u/3 bars",
        raw,held?@"last valid":@"raw",filtered,_signal.level(now)]:@"signal unavailable";
    return [NSString stringWithFormat:@"%s · channel %u · %@",
        rtwui::bandDescription(_status.channel),_status.channel,signal];
}
- (uint32_t)stationFlags {return _hasStatus?_status.flags:0;}
- (const RTWControlChannelMap*)stationChannelMap {return _channelMap.current(_selection);}
- (void)rebuildMenu {
    if(_menuOpen)return;
    _signalRow=nil;
    [_menu removeAllItems];
    NSMenuItem* title=[self add:@"Tahoe Wi-Fi USB" action:nil enabled:NO];
    title.attributedTitle=[[NSAttributedString alloc]initWithString:title.title attributes:@{NSFontAttributeName:[NSFont boldSystemFontOfSize:13]}];
    _header=[self add:[self phaseTitle] action:nil enabled:NO];
    BOOL canControl=_authorizedSession&&_selectedPresent&&_selection.registryID;
    NSMenuItem* adapterMenu=[self add:@"USB adapter" action:nil enabled:_authorizedSession];
    NSMenu* choices=[[NSMenu alloc]initWithTitle:@"USB adapters"];choices.autoenablesItems=NO;
    for(const auto& adapter:_adapters){
        NSString* family=text(std::string(adapter.family));
        NSMenuItem* choice=[[NSMenuItem alloc]initWithTitle:[NSString stringWithFormat:@"%@ · ID %llu",family,(unsigned long long)adapter.registryID] action:@selector(selectAdapter:) keyEquivalent:@""];
        choice.target=self;choice.enabled=_authorizedSession;choice.representedObject=@(adapter.registryID);
        choice.state=adapter.registryID==_selection.registryID?NSControlStateValueOn:NSControlStateValueOff;[choices addItem:choice];
    }
    if(_selection.registryID&&!_selectedPresent){NSMenuItem* missing=[[NSMenuItem alloc]initWithTitle:[NSString stringWithFormat:@"ID %llu · disconnected",(unsigned long long)_selection.registryID] action:nil keyEquivalent:@""];missing.enabled=NO;missing.state=NSControlStateValueOn;[choices addItem:missing];}
    if(!_adapters.size()){NSMenuItem* empty=[[NSMenuItem alloc]initWithTitle:_authorizedSession?@"No compatible adapter":@"Enable the service to list adapters" action:nil keyEquivalent:@""];empty.enabled=NO;[choices addItem:empty];}
    adapterMenu.submenu=choices;
    BOOL connected=[self linkIndicator]==rtwui::LinkIndicator::Connected;
    if(connected){
        [self add:ssidText(_status.ssid,_status.ssidLength) action:nil enabled:NO];
        if(_interface)[self add:_interface action:nil enabled:NO];
        _signalRow=[self add:[self signalSummary] action:nil enabled:NO];
    }
    if(_hasStatus&&_status.usbError==0xe00002edU)
        [self add:@"The selected USB adapter stopped responding. Unplug and reconnect it." action:nil enabled:NO];
    else if(_notice.length)[self add:_notice action:nil enabled:NO];
    [_menu addItem:NSMenuItem.separatorItem];
    if(!_authorizedSession)[self add:_authorizing?@"Connecting…":@"Enable driver service…" action:@selector(authorize:) enabled:!_authorizing];
    NSMenuItem* refresh=[self add:_refreshingNetworks?@"Refreshing networks…":@"Refresh networks"
        action:@selector(refresh:) enabled:canControl&&!_joining&&!_credentialBusy&&
            !_commandPending&&!_refreshingNetworks&&!(_hasStatus&&(_status.flags&RTW_CONTROL_SCANNING))];
    refresh.toolTip=@"Scan supported Wi-Fi channels. The active network may briefly reconnect.";
    [self add:_joining?@"Cancel connection":@"Disconnect" action:@selector(disconnect:) enabled:canControl&&(_joining||(_hasStatus&&(_status.flags&RTW_CONTROL_RUNNING)))];
    [_menu addItem:NSMenuItem.separatorItem];
    [self add:[NSString stringWithFormat:@"Nearby networks · %lu",(unsigned long)_networks.size()] action:nil enabled:NO];
    for(NSUInteger i=0;i<_networks.size();++i){
        const auto& r=_networks[i];BOOL available=rtwui::supported(r,[self stationFlags],[self stationChannelMap]);
        const unsigned bands=rtwui::observedNetworkBands(_scan,r,[self stationFlags],[self stationChannelMap]);
        NSString* bandsText=[NSString stringWithUTF8String:rtwui::observedNetworkBandsText(bands)];
        NSString* location=bands==3?bandsText:[NSString stringWithFormat:@"%s · ch %u",rtwui::bandDescription(r.channel),r.channel];
        NSString* label=[NSString stringWithFormat:@"%@  · %@ · %@",
            ssidText(r.ssid,r.ssidLength),location,available?@"WPA2":@"unsupported"];
        NSMenuItem* entry=[self add:label action:@selector(selectNetwork:) enabled:canControl&&available&&!_joining&&!_credentialBusy];
        entry.tag=available?1:0;entry.image=wifiImage(r.rssi);
        entry.representedObject=@{@"ssid":[NSData dataWithBytes:r.ssid length:r.ssidLength],
            @"registryID":@(_selection.registryID),@"generation":@(_selection.generation),
            @"channel":@(r.channel),@"security":@(r.securityFlags)};
        NSString* strength=rtwui::validSignal(r.rssi)?
            [NSString stringWithFormat:@"%d dBm",r.rssi]:@"Signal unavailable";
        const unsigned frequency=rtwui::channelFrequencyMHz(r.channel);
        NSString* frequencyText=frequency?[NSString stringWithFormat:@"%u MHz",frequency]:@"frequency unavailable";
        entry.toolTip=[NSString stringWithFormat:
            @"Observed usable bands: %@\nRepresentative BSS: %s · channel %u · %@ · %@%@\nThe driver selects the access point for this SSID automatically.",
            bandsText,rtwui::bandDescription(r.channel),r.channel,frequencyText,strength,
            available?@"":@" · Requires WPA2-Personal/CCMP and a channel enabled by this driver"];
        if(connected&&r.ssidLength==_status.ssidLength&&!memcmp(r.ssid,_status.ssid,r.ssidLength))entry.state=NSControlStateValueOn;
    }
    if(_networks.empty())[self add:_authorizedSession?@"No cached networks. Choose Refresh networks.":@"Enable the service to list networks." action:nil enabled:NO];
    if(_scan.truncated)[self add:@"The driver limits the list to 64 access points." action:nil enabled:NO];
    [self add:@"Other network (hidden SSID)…" action:@selector(otherNetwork:) enabled:canControl&&!_joining&&!_credentialBusy];
    [self add:[NSString stringWithFormat:@"WPA2-Personal / CCMP · %@",
        text(rtwui::stationBandDescription([self stationFlags],[self stationChannelMap]))] action:nil enabled:NO];
    [_menu addItem:NSMenuItem.separatorItem];
    [self add:@"Save a network password…" action:@selector(saveNetworkPassword:) enabled:!_credentialBusy&&!_joining];
    [self add:@"Saved passwords…" action:@selector(manageSavedPasswords:) enabled:!_credentialBusy&&!_joining];
    NSMenuItem* automatic=[self add:@"Reconnect automatically" action:@selector(toggleAutoJoin:) enabled:YES];
    automatic.state=_autoJoinEnabled?NSControlStateValueOn:NSControlStateValueOff;
    if(_autoJoin.paused)[self add:@"Automatic reconnection paused · select a network to resume." action:nil enabled:NO];
    if(_credentialBusy)[self add:@"Accessing macOS Keychain…" action:nil enabled:NO];
    [self add:@"Driver details…" action:@selector(details:) enabled:YES];
    [self add:@"Signal diagnostics…" action:@selector(signalDiagnostics:) enabled:YES];
    [self add:@"Adapter catalog…" action:@selector(adapterCatalog:) enabled:YES];
    [self add:@"USB adapters and support…" action:@selector(adapterSupport:) enabled:!_adapterReportBusy];
    [_menu addItem:NSMenuItem.separatorItem];
    RTWLoginItemState loginState=[self loginItems].state;
    NSMenuItem* login=[self add:@"Open at login" action:@selector(toggleLoginAtStartup:) enabled:YES];
    login.state=loginState==RTWLoginItemStateEnabled?NSControlStateValueOn:
        loginState==RTWLoginItemStateRequiresApproval?NSControlStateValueMixed:NSControlStateValueOff;
    if(loginState==RTWLoginItemStateRequiresApproval) {
        [self add:@"Allow startup in System Settings…" action:@selector(openLoginSettings:) enabled:YES];
        [self add:@"Cancel pending startup" action:@selector(disableLoginAtStartup:) enabled:YES];
    }
    if(loginState==RTWLoginItemStateNotFound)
        [self add:@"Registration not found · click to register or repair." action:nil enabled:NO];
    [self add:@"Repair login startup…" action:@selector(repairLoginAtStartup:) enabled:YES];
    if(_serviceManager.state==RTWLoginItemStateEnabled||_serviceManager.state==RTWLoginItemStateRequiresApproval)
        [self add:@"Disable driver service…" action:@selector(disableDriverService:) enabled:!_authorizing];
    [_menu addItem:NSMenuItem.separatorItem];
    [self add:@"About Tahoe Wi-Fi USB…" action:@selector(about:) enabled:YES];
    NSMenuItem* quit=[self add:@"Quit" action:@selector(quit:) enabled:YES];quit.keyEquivalent=@"q";
}
- (void)menuNeedsUpdate:(NSMenu*)menu {
    (void)menu;
    // AppKit owns the visible menu while it is tracking a click. Rebuild only
    // when it is about to open, not during the status timer callback.
    [self rebuildMenu];
    [self poll:nil];
}
- (void)menuWillOpen:(NSMenu*)menu {(void)menu;_menuOpen=YES;[self refreshPollingTimer];}
- (void)menuDidClose:(NSMenu*)menu {(void)menu;_menuOpen=NO;[self refreshPollingTimer];}
- (void)updateAppearance {
    if(_terminating)return;
    [self refreshPollingTimer];
    const rtwui::LinkIndicator indicator=[self linkIndicator];
    const unsigned level=indicator==rtwui::LinkIndicator::Connected?_signal.level([self uptime]):0;
    const BOOL disconnected=indicator==rtwui::LinkIndicator::Disconnected;
    const NSInteger kind=disconnected?0U:1U+level;
    if(kind!=_displayedIconKind) {
        _displayedIconKind=kind;
        _item.button.image=wifiImageForLevel(level,disconnected);
    }
    NSString* phase=[self phaseTitle];
    NSString* tooltip=[NSString stringWithFormat:@"Tahoe Wi-Fi USB · %@",phase];
    if(indicator==rtwui::LinkIndicator::Connected)
        tooltip=[tooltip stringByAppendingFormat:@" · %@",[self signalSummary]];
    if(![_item.button.toolTip isEqualToString:tooltip]) {
        _item.button.toolTip=tooltip;
        [_item.button setAccessibilityLabel:tooltip];
    }
    if(![_header.title isEqualToString:phase])_header.title=phase;
    if(_signalRow) {
        NSString* summary=indicator==rtwui::LinkIndicator::Connected?
            [self signalSummary]:@"Signal unavailable · connection not confirmed";
        if(![_signalRow.title isEqualToString:summary])_signalRow.title=summary;
    }
    // The native menu refreshes through menuNeedsUpdate. Avoid allocating and
    // discarding up to 64 NSMenuItems every two seconds while it is closed.
}
- (void)configureInstallation {
    NSUserDefaults* defaults=[self preferences];
    BOOL firstSetup=![defaults boolForKey:@"RTWServiceSetupComplete"];
    BOOL resumeLogin=[defaults boolForKey:@"RTWResumeLoginAfterInstall"];
    if(resumeLogin||![defaults boolForKey:@"RTWStartupChoiceMade"]) {
        NSError* error=nil;
        if([[self loginItems] setEnabled:YES error:&error]) {
            [defaults setBool:YES forKey:@"RTWStartupChoiceMade"];
            [defaults setBool:YES forKey:@"RTWStartupRequested"];
            [defaults removeObjectForKey:@"RTWResumeLoginAfterInstall"];
        }else {
            [self showMessage:@"Login startup" detail:RTWEnglishError(error)?:
                @"macOS did not confirm app registration."];
        }
    }
    if(firstSetup||[defaults boolForKey:@"RTWResumeServiceAfterInstall"]) {
        [self authorize:nil];
        if(_serviceManager.state==RTWLoginItemStateEnabled||
           _serviceManager.state==RTWLoginItemStateRequiresApproval) {
            [defaults setBool:YES forKey:@"RTWServiceSetupComplete"];
            [defaults removeObjectForKey:@"RTWResumeServiceAfterInstall"];
        }
    }
}
- (void)authorize:(id)sender {
    (void)sender;
    if(_terminating||_authorizing)return;
    NSError* error=nil;
    if(![_serviceManager enableAndReturnError:&error]) {
        _notice=RTWEnglishError(error)?:@"Could not register the driver service.";
        [self showMessage:@"Driver service" detail:_notice];
        [self updateAppearance];return;
    }
    if(_serviceManager.state==RTWLoginItemStateRequiresApproval) {
        _notice=@"Allow Tahoe Wi-Fi USB in Login Items & Extensions.";
        [_serviceManager openSystemSettings];
    }else {
        _serviceRetry.reset();[self connectDriverAutomatically];
    }
    [self updateAppearance];
}
- (void)connectDriverAutomatically {
    if(_terminating||_authorizing||_authorizedSession||_sleeping||_inactiveSession||!_queue)return;
    const auto state=_serviceManager.state;
    if(state!=RTWLoginItemStateEnabled) {
        _notice=state==RTWLoginItemStateRequiresApproval?
            @"Service approval is pending in System Settings.":
            @"Enable the driver service once from the menu.";
        [self updateAppearance];return;
    }
    if(!_serviceRetry.due([self uptime]))return;
    _authorizing=YES;[self updateAppearance];
    const auto epoch=_workEpoch.snapshot();
    const BOOL discoverExisting=!_preferredAdapter&&!_preferredAdapterPendingID&&!_selection.everSelected;
    NSData* legacySSID=_legacyPreferredSSID;
    dispatch_async(_queue,^{
        if(!self->_workEpoch.matches(epoch))return;
        rtwui::DeviceList devices{};
        uint32_t result=[self->_driver exchangeRegistryID:0 selector:rtwui::listSelector
            input:nullptr length:0 output:&devices capacity:sizeof(devices)];
        NSDictionary* identities=!result?rtwui::readAdapterIdentities(devices):nil;
        NSDictionary* existing=!result&&discoverExisting?existingAdapterLink(self->_driver,devices,legacySSID):nil;
        dispatch_async(dispatch_get_main_queue(),^{
            if(self->_terminating||!self->_workEpoch.matches(epoch))return;
            self->_authorizing=NO;
            self->_authorizedSession=result==0;
            if(result) {
                self->_serviceRetry.fail([self uptime]);
                self->_notice=@"Service unavailable; retrying automatically. Check approval in System Settings.";
            }else {
                self->_serviceRetry.reset();self->_notice=nil;
                self->_forceDeviceList=YES;self->_forceNetworkCache=YES;
                self->_adapterIdentities=identities;
                self->_existingAdapterLink=existing;
                [self applyAdapters:devices];[self poll:nil];
            }
            [self updateAppearance];
        });
    });
}
- (void)disableDriverService:(id)sender {
    (void)sender;
    NSAlert* alert=[NSAlert new];alert.messageText=@"Disable the driver service?";
    alert.informativeText=@"The app will not control or reconnect USB Wi-Fi until you enable the service again. No passwords will be deleted.";
    [alert addButtonWithTitle:@"Disable"];[alert addButtonWithTitle:@"Cancel"];
    if([alert runModal]!=NSAlertFirstButtonReturn)return;
    NSError* error=nil;
    if(![_serviceManager disableAndReturnError:&error]) {
        [self showMessage:@"Could not disable" detail:RTWEnglishError(error)?:@"macOS denied the operation."];return;
    }
    _authorizedSession=NO;_selection.invalidate();_adapterEpoch.store(_selection.generation);
    [self clearAdapterState];
    dispatch_async(_queue,^{[self->_driver close];});
    _notice=@"You disabled the service.";[self updateAppearance];
}
- (void)suspendActivity {
    [self invalidatePendingWork];
    _signal.clear();_signalHistory.clear();
    ++_joinGeneration;_joining=NO;_joinAccepted=NO;_joinSSID=nil;
    _joinStartedAt=-1;_rememberCurrentJoin=NO;
    [self refreshPollingTimer];
}
- (void)resumeActivity {
    if(_terminating||_sleeping||_inactiveSession)return;
    _serviceRetry.reset();_autoJoin.reset();
    _signal.clear();_signalHistory.clear();
    _forceDeviceList=YES;_forceNetworkCache=YES;_hasStatus=NO;
    _lastLinkedAt=-1;_lastAutoScanAt=-1;
    for(auto& entry:_networkRefresh)entry.second.lastFinishedAt=-1;
    [self refreshPollingTimer];[self reloadSavedNetworks];[self poll:nil];
}
- (void)workspaceWillSleep:(NSNotification*)notification {
    (void)notification;_sleeping=YES;[self suspendActivity];
}
- (void)workspaceBecameInactive:(NSNotification*)notification {
    (void)notification;_inactiveSession=YES;[self suspendActivity];
}
- (void)workspaceDidWake:(NSNotification*)notification {
    (void)notification;_sleeping=NO;[self resumeActivity];
}
- (void)workspaceBecameActive:(NSNotification*)notification {
    (void)notification;_inactiveSession=NO;[self resumeActivity];
}
- (void)toggleAutoJoin:(id)sender {
_autoJoinEnabled=!_autoJoinEnabled;
    _autoJoinEpoch.fetch_add(1);
    if(_joining&&_automaticCurrentJoin) {
        ++_joinGeneration;_joining=NO;_joinAccepted=NO;_joinSSID=nil;
        _joinStartedAt=-1;_rememberCurrentJoin=NO;_automaticCurrentJoin=NO;
    }
    [[self preferences] setBool:_autoJoinEnabled forKey:@"RTWAutoJoinEnabled"];
    if(_autoJoinEnabled) {
        _autoJoin.paused=false;_autoJoin.reset();
        [[self preferences] setBool:NO forKey:@"RTWAutoJoinPaused"];
        [self reloadSavedNetworks];[self poll:nil];
    }
    // AppKit keeps this menu open while handling the action; rebuildMenu
    // intentionally does nothing while tracking, so update its checkmark now.
    if([sender isKindOfClass:NSMenuItem.class])
        ((NSMenuItem*)sender).state=_autoJoinEnabled?NSControlStateValueOn:NSControlStateValueOff;
    [self rebuildMenu];
}
- (void)reloadSavedNetworks {
    if(_terminating)return;
    if(_loadingSavedNetworks){_reloadSavedAgain=YES;return;}
    _reloadSavedAgain=NO;_loadingSavedNetworks=YES;_lastSavedReloadAt=[self uptime];
    id<RTWCredentialStoring> store=[self credentials];
    dispatch_async([self credentialQueue],^{
        NSError* error=nil;NSArray<NSData*>* networks=[store savedSSIDsAndReturnError:&error];
        dispatch_async(dispatch_get_main_queue(),^{
            self->_loadingSavedNetworks=NO;if(self->_terminating)return;
            if(networks){self->_savedNetworks=networks;self->_savedNetworksLoaded=YES;}
            else {self->_notice=RTWEnglishError(error)?:@"Keychain unavailable.";self->_savedNetworksLoaded=NO;}
            if(self->_reloadSavedAgain)[self reloadSavedNetworks];
        });
    });
}
- (void)considerAutoJoin {
    if(_refreshingNetworks||!_autoJoinEnabled||!_savedNetworksLoaded||!_savedNetworks.count)return;
    const double now=[self uptime];
    const bool connected=_hasStatus&&(_status.flags&RTW_CONTROL_AUTHORIZED);
    if(connected){_lastLinkedAt=now;return;}
    if(!_autoJoin.mayAttempt(now,_authorizedSession&&_selectedPresent&&_selection.registryID,
        _hasStatus&&!_statusReadDelayed&&!_status.usbError,connected,_joining,_credentialBusy,
        _sleeping||_inactiveSession))return;
    // Give the in-kernel link recovery a chance before sending another JOIN.
    if(_lastLinkedAt>=0&&now>=_lastLinkedAt&&now-_lastLinkedAt<15.0)return;
    NSData* candidate=nil;
    auto reconnectAllowed=[&](NSData* ssid){return rtwui::savedProfileReconnectAllowed(_scan,
        static_cast<const uint8_t*>(ssid.bytes),ssid.length,[self stationFlags],[self stationChannelMap]);};
    if(_preferredSSID&&[_savedNetworks containsObject:_preferredSSID]&&reconnectAllowed(_preferredSSID))candidate=_preferredSSID;
    else if(_savedNetworks.count==1&&reconnectAllowed(_savedNetworks.firstObject))candidate=_savedNetworks.firstObject;
    else for(const auto& network:_networks) {
        if(!rtwui::supported(network,[self stationFlags],[self stationChannelMap]))continue;
        NSData* ssid=[NSData dataWithBytes:network.ssid length:network.ssidLength];
        if([_savedNetworks containsObject:ssid]){candidate=ssid;break;}
    }
    if(!candidate) {
        if((_lastAutoScanAt<0||now-_lastAutoScanAt>=30.0)&&
           _networkRefresh[_selection.registryID].due(now)) {
            _lastAutoScanAt=now;[self command:RTW_CONTROL_SCAN];
        }
        return;
    }
    [self loadPasswordForSSID:candidate connect:YES automatic:YES];
}
- (uint64_t)selectedAdapterID {return _selection.registryID;}
- (uint64_t)adapterGeneration {return _selection.generation;}
- (void)clearAdapterState {
    [self invalidatePendingWork];
    ++_joinGeneration;_joining=NO;_joinAccepted=NO;_joinSSID=nil;_joinStartedAt=-1;
    _rememberCurrentJoin=NO;
    _busy=NO;_hasStatus=NO;_statusReadDelayed=NO;_lastStatusSuccess=-1;
    _lastNetworkCacheAt=-1;_lastInterfaceAt=-1;_forceNetworkCache=YES;_pollNotice=NO;
    _signal.clear();_signalHistory.clear();_status={};_scan={};_networks.clear();_interface=nil;_channelMap.clear();
}
- (void)useAdapter:(uint64_t)registryID {
    if(!_selection.select(registryID))return;
    if(registryID!=_pendingNetworkRegistryID){_pendingNetworkSSID=nil;_pendingNetworkRegistryID=0;}
    _adapterEpoch.store(_selection.generation);
    [self clearAdapterState];_selectedPresent=NO;_autoJoin.reset();_lastLinkedAt=-1;
    for(const auto& adapter:_adapters)if(adapter.registryID==registryID)_selectedPresent=YES;
    [self restoreAdapterNetworkPreference];
    if(!_preferredAdapter&&_selectedPresent&&[self adapterIdentityForRegistryID:registryID])
        [self rememberAdapter:registryID];
    _notice=_selectedPresent?nil:@"The selected adapter was disconnected. Choose another from the menu.";
    [self updateAppearance];if(_queue)[self poll:nil];
}
- (void)selectAdapter:(NSMenuItem*)item {
    if(!_authorizedSession||![item.representedObject isKindOfClass:NSNumber.class])return;
    const uint64_t id=[item.representedObject unsignedLongLongValue];
    for(const auto& adapter:_adapters)if(adapter.registryID==id){[self rememberAdapter:id];[self useAdapter:id];return;}
}
- (void)applyAdapters:(const rtwui::DeviceList&)list {
    if(!rtwui::validDevices(list))return;
    [self loadConnectionPreferences];
    const unsigned previousCount=static_cast<unsigned>(_adapters.size());
    if(!list.count)_soleAdapterRecovery=YES;
    _adapters.assign(list.devices,list.devices+list.count);
    for(auto entry=_networkRefresh.begin();entry!=_networkRefresh.end();) {
        const bool present=std::any_of(_adapters.begin(),_adapters.end(),
            [&](const auto& adapter){return adapter.registryID==entry->first;});
        if(!present)entry=_networkRefresh.erase(entry);else ++entry;
    }
    BOOL present=NO;for(const auto& adapter:_adapters)if(adapter.registryID==_selection.registryID)present=YES;
    if(_selectedPresent&&!present){_soleAdapterRecovery=previousCount==1||list.count==0;_selection.invalidate();_adapterEpoch.store(_selection.generation);[self clearAdapterState];_notice=@"The selected adapter was disconnected. Choose another from the menu.";}
    _selectedPresent=present;
    if(present)[self strengthenSelectedAdapterIdentity];
    if(present&&_pendingNetworkRegistryID==_selection.registryID&&_pendingNetworkSSID&&
       [self adapterIdentityForRegistryID:_selection.registryID])
        [self rememberConnectedNetwork:_pendingNetworkSSID];
    if(present&&!_preferredAdapter&&
       (!_preferredAdapterPendingID||_preferredAdapterPendingID==_selection.registryID)&&
       [self adapterIdentityForRegistryID:_selection.registryID]) {
        [self rememberAdapter:_selection.registryID];
        [self restoreAdapterNetworkPreference];
    }
    if(present&&rtwui::adapterIdentityMatch(_preferredAdapter,
       rtwui::adapterIdentity([self adapterIdentityForRegistryID:_selection.registryID])))
        [self restoreAdapterNetworkPreference];
    if(present&&_selection.invalidated) {
        // LIST can confirm the same controller after a temporary absence or
        // service restart. Renew its token before polling; marking it present
        // alone would leave every STATUS result invalidated forever.
        _soleAdapterRecovery=NO;[self useAdapter:_selection.registryID];return;
    }
    if(_preferredAdapter) {
        unsigned best=0,matches=0;uint64_t preferredID=0;
        for(const auto& adapter:_adapters) {
            const unsigned score=rtwui::adapterIdentityMatch(_preferredAdapter,
                rtwui::adapterIdentity([self adapterIdentityForRegistryID:adapter.registryID]));
            if(score>best){best=score;matches=1;preferredID=adapter.registryID;}
            else if(score&&score==best)++matches;
        }
        if(!present&&matches==1){_soleAdapterRecovery=NO;[self useAdapter:preferredID];}
        // Keep the preferred USB through delayed boot enumeration or an
        // ambiguous duplicate. A different sole adapter cannot replace it.
        return;
    }
    if(_preferredAdapterPendingID)return;
    if(!_selection.everSelected&&_existingAdapterLink) {
        const uint64_t linkedID=[_existingAdapterLink[@"registryID"] unsignedLongLongValue];
        for(const auto& adapter:_adapters)if(adapter.registryID==linkedID) {
            [self rememberAdapter:linkedID];[self useAdapter:linkedID];
            [self rememberConnectedNetwork:_existingAdapterLink[@"ssid"]];return;
        }
    }
    if((!_selection.everSelected&&list.count==1)||
       (_soleAdapterRecovery&&rtwui::recoverSoleAdapter(present,1,list.count))) {
        _soleAdapterRecovery=NO;
        [self useAdapter:list.devices[0].registryID];
    }
}
- (void)considerNetworkRefresh {
    if(_terminating||_sleeping||_inactiveSession||!_authorizedSession||!_selectedPresent||
       !_selection.registryID||!_queue||!_hasStatus||_statusReadDelayed||_status.usbError||
       _joining||_credentialBusy||_commandPending||_refreshingNetworks)return;
    if(!(_status.flags&RTW_CONTROL_DEVICE)||
       (_status.flags&RTW_CONTROL_AUTHORIZED))return;
    auto& policy=_networkRefresh[_selection.registryID];
    const double now=[self uptime];
    if(!policy.due(now))return;
    if(_status.flags&RTW_CONTROL_SCANNING) {
        // Observe an existing driver scan instead of starting a duplicate.
        _refreshingNetworks=YES;_refreshStartedAt=now;_forceNetworkCache=YES;
        return;
    }
    // RUNNING means the interface is initialized, not that it has a link.
    // Association/authentication states still belong to kernel recovery.
    if(_status.state>=2)return;
    const auto* map=[self stationChannelMap];
    bool channels=false;
    for(unsigned channel=1;channel<256&&!channels;++channel)
        channels=map?rtw_control_channel_map_contains(map,channel):
            rtw_control_channel_supported(channel,[self stationFlags]);
    if(!channels)return;
    // An attached idle adapter can have RADIO clear until SCAN enables it.
    // Freshness is measured from completed RF scans, never cache reads.
    [self refresh:nil];
}
- (void)poll:(id)sender {
    (void)sender;
    if(_terminating||_sleeping||_inactiveSession)return;
    if(!_savedNetworksLoaded&&!_loadingSavedNetworks&&
       rtwui::refreshDue([self uptime],_lastSavedReloadAt,30.0))
        [self reloadSavedNetworks];
    if(!_authorizedSession){[self connectDriverAutomatically];return;}
    if(_busy||!_queue)return;
    _busy=YES;
    const double now=[self uptime];
    const auto generation=_joinGeneration,registryID=_selection.registryID,adapterGeneration=_selection.generation;
    const auto pollToken=++_pollToken,epoch=_workEpoch.snapshot();
    const bool listDue=_forceDeviceList||!_selectedPresent||rtwui::refreshDue(now,_lastDeviceListAt,8.0);
    const bool scanDue=registryID&&(_forceNetworkCache||_refreshingNetworks||
        rtwui::refreshDue(now,_lastNetworkCacheAt,_menuOpen?6.0:15.0));
    const bool interfaceDue=registryID&&(!_interface||
        rtwui::refreshDue(now,_lastInterfaceAt,_menuOpen?8.0:20.0));
    if(listDue){_forceDeviceList=NO;_lastDeviceListAt=now;}
    if(scanDue){_forceNetworkCache=NO;_lastNetworkCacheAt=now;}
    if(interfaceDue)_lastInterfaceAt=now;
    const BOOL discoverExisting=!_preferredAdapter&&!_preferredAdapterPendingID&&!_selection.everSelected;
    NSData* legacySSID=_legacyPreferredSSID;
    dispatch_async(_queue,^{
        if(!self->_workEpoch.matches(epoch))return;
        rtwui::DeviceList devices{};
        const uint32_t listCode=listDue?[self->_driver exchangeRegistryID:0 selector:rtwui::listSelector input:nullptr length:0 output:&devices capacity:sizeof(devices)]:0;
        NSDictionary* identities=listDue&&!listCode?rtwui::readAdapterIdentities(devices):nil;
        NSDictionary* existing=listDue&&!listCode&&discoverExisting?existingAdapterLink(self->_driver,devices,legacySSID):nil;
        RTWControlRequest request{};request.version=1;
        RTWControlStatus status{};RTWControlScanResults scan{};RTWControlChannelMap channels{};
        const bool listBroken=listDue&&(listCode==rtwui::transportError||listCode==rtwui::protocolError);
        uint32_t code=registryID?(listBroken?listCode:[self->_driver exchangeRegistryID:registryID selector:RTW_CONTROL_STATUS input:&request length:sizeof(request) output:&status capacity:sizeof(status)]):0;
        if(registryID&&!code&&(status.ssidLength>32||status.state>4))code=rtwui::protocolError;
        const bool requestMap=registryID&&!code;
        uint32_t mapCode=requestMap?[self->_driver exchangeRegistryID:registryID selector:RTW_CONTROL_CHANNEL_MAP input:&request length:sizeof(request) output:&channels capacity:sizeof(channels)]:0;
        const bool legacyMap=requestMap&&mapCode==static_cast<uint32_t>(kIOReturnUnsupported);
        if(requestMap&&!mapCode&&!rtw_control_channel_map_valid(&channels))mapCode=rtwui::protocolError;
        const uint32_t mapError=legacyMap?0:mapCode;
        const bool requestScan=scanDue&&registryID&&!code&&!mapError;
        uint32_t scanCode=requestScan?[self->_driver exchangeRegistryID:registryID selector:RTW_CONTROL_SCAN_RESULTS input:&request length:sizeof(request) output:&scan capacity:sizeof(scan)]:0;
        if(requestScan&&!scanCode&&(scan.count>64||scan.truncated>1))scanCode=rtwui::protocolError;
        if(requestScan&&!scanCode)for(unsigned i=0;i<scan.count;++i)if(scan.records[i].ssidLength>32)scanCode=rtwui::protocolError;
        const BOOL broken=(listDue&&listBroken)||code==rtwui::transportError||code==rtwui::protocolError||
            scanCode==rtwui::transportError||scanCode==rtwui::protocolError||
            mapError==rtwui::transportError||mapError==rtwui::protocolError;
        if(broken)[self->_driver close];
        NSString* iface=registryID&&!code&&interfaceDue?interfaceInfo(status.mac):nil;
        dispatch_async(dispatch_get_main_queue(),^{
            // Adapter switches can schedule a replacement poll while an older
            // result is in flight. Never apply old LIST/STATUS to the new UI.
            if(self->_terminating||self->_sleeping||self->_inactiveSession||
               pollToken!=self->_pollToken||!self->_workEpoch.matches(epoch))return;
            self->_busy=NO;
            if(broken){self->_authorizedSession=NO;self->_serviceRetry.fail([self uptime]);}
            if(listDue&&listCode)self->_forceDeviceList=YES;
            if(scanDue&&(code||scanCode||mapError))self->_forceNetworkCache=YES;
            if(listDue&&!listCode){self->_adapterIdentities=identities;self->_existingAdapterLink=existing;[self applyAdapters:devices];}
            if(!self->_selection.matches(registryID,adapterGeneration)){[self updateAppearance];return;}
            if(!registryID){
                if(listCode){self->_notice=errorText(listCode);self->_pollNotice=YES;}
                else if(listDue&&self->_pollNotice){self->_notice=nil;self->_pollNotice=NO;}
                [self updateAppearance];return;
            }
            const bool absent=!code&&!(status.flags&RTW_CONTROL_DEVICE);
            if(absent){
                self->_selection.invalidate();self->_adapterEpoch.store(self->_selection.generation);
                [self clearAdapterState];self->_selectedPresent=NO;self->_forceDeviceList=YES;
                self->_notice=@"The selected adapter was disconnected. Choose another from the menu.";
                [self updateAppearance];return;
            }
            if(requestMap&&self->_selectedPresent) {
                if(legacyMap)self->_channelMap.setLegacy(self->_selection,registryID,adapterGeneration);
                else if(!mapError)self->_channelMap.setExact(self->_selection,registryID,adapterGeneration,channels);
                else self->_channelMap.clear();
            }
            if(!code&&self->_selectedPresent){
                const bool changedBands=(self->_status.flags^status.flags)&(RTW_CONTROL_BAND_5GHZ|RTW_CONTROL_BAND_5GHZ_HIGH|RTW_CONTROL_BAND_5GHZ_DFS_LOW|RTW_CONTROL_BAND_5GHZ_DFS_MID|RTW_CONTROL_BAND_2GHZ|RTW_CONTROL_BANDS_EXPLICIT);
                self->_hasStatus=YES;self->_statusReadDelayed=NO;
                self->_lastStatusSuccess=[self uptime];self->_status=status;
                if(changedBands||requestMap) {
                    self->_networks=rtwui::networks(self->_scan,status.flags,[self stationChannelMap]);
                    if(changedBands)self->_forceNetworkCache=YES;
                }
                if(interfaceDue)self->_interface=iface;
                self->_signal.observe(status,self->_lastStatusSuccess);
            }else if(code){
                self->_statusReadDelayed=YES;
                self->_forceDeviceList=YES;
            }
            self->_signalHistory.record(status,self->_signal,[self uptime],code);
            if(requestScan&&!scanCode&&self->_selectedPresent){
                self->_scan=scan;self->_networks=rtwui::networks(scan,status.flags,[self stationChannelMap]);
            }
            if(listCode||code||scanCode||mapError){
                if(self->_refreshingNetworks)
                    self->_networkRefresh[registryID].failed([self uptime]);
                self->_refreshingNetworks=NO;
                self->_refreshStartedAt=-1;
                self->_notice=errorText(listCode?:code?:scanCode?:mapError);self->_pollNotice=YES;
            }else if(self->_refreshingNetworks&&!self->_commandPending&&requestScan&&
                     !(status.flags&RTW_CONTROL_SCANNING)){
                self->_refreshingNetworks=NO;
                self->_refreshStartedAt=-1;
                self->_networkRefresh[registryID].finished([self uptime]);
                self->_lastAutoScanAt=[self uptime];
                self->_notice=@"Network refresh finished.";self->_pollNotice=NO;
            }else if(self->_refreshingNetworks&&!self->_commandPending&&
                     rtwui::refreshDue([self uptime],self->_refreshStartedAt,30.0)){
                self->_refreshingNetworks=NO;self->_refreshStartedAt=-1;
                self->_networkRefresh[registryID].failed([self uptime]);
                self->_notice=@"Network refresh timed out. Retrying automatically.";self->_pollNotice=YES;
            }else if(self->_pollNotice){
                self->_notice=nil;self->_pollNotice=NO;
            }
            if(self->_joining&&!code){
                BOOL matched=rtwui::confirmsJoin(status,static_cast<const uint8_t*>(self->_joinSSID.bytes),self->_joinSSID.length,generation,self->_joinGeneration,self->_joinAccepted);
                if(matched){
                    if(self->_rememberCurrentJoin&&self->_joinSSID) {
                        [self rememberConnectedNetwork:self->_joinSSID];
                    }
                    self->_autoJoin.reset();
                    self->_joining=NO;self->_joinSSID=nil;self->_joinStartedAt=-1;
                    self->_rememberCurrentJoin=NO;self->_notice=nil;self->_pollNotice=NO;
                }
                else if(rtwui::joinTimedOut([self uptime],self->_joinStartedAt)){
                    self->_joining=NO;self->_joinSSID=nil;self->_joinStartedAt=-1;
                    self->_autoJoin.retry.fail([self uptime]);self->_rememberCurrentJoin=NO;
                    self->_notice=@"Connection not confirmed. Your saved password was kept; check signal/WPA2 or edit it in Saved passwords.";
                    self->_pollNotice=NO;
                }
            }
            if(self->_joining&&rtwui::joinTimedOut([self uptime],self->_joinStartedAt)){
                self->_joining=NO;self->_joinAccepted=NO;self->_joinSSID=nil;
                self->_joinStartedAt=-1;self->_rememberCurrentJoin=NO;
                self->_autoJoin.retry.fail([self uptime]);
                self->_notice=@"The connection timed out while the driver was unavailable.";
            }
            [self considerNetworkRefresh];[self considerAutoJoin];
            if(self->_diagnostic&&!self->_diagnosticPrinted){
                self->_diagnosticPrinted=YES;
                printf("{\"menuBar\":%s,\"helper\":true,\"registryID\":%llu,\"statusError\":%u,\"scanError\":%u,\"flags\":%u,\"state\":%u,\"channel\":%u,\"usbError\":%u,\"accessPoints\":%u,\"networks\":%zu}\n",self->_item.visible?"true":"false",(unsigned long long)registryID,code,scanCode,status.flags,status.state,status.channel,status.usbError,self->_scan.count,self->_networks.size());fflush(stdout);
            }
            [self updateAppearance];
        });
    });
}
- (void)command:(uint32_t)selector {
    if(_terminating||_sleeping||_inactiveSession||_commandPending||!_queue||
       !_authorizedSession||!_selectedPresent||!_selection.registryID)return;
    _commandPending=YES;
    const auto registryID=_selection.registryID,adapterGeneration=_selection.generation;
    if(selector==RTW_CONTROL_SCAN) {
        _refreshingNetworks=YES;_refreshStartedAt=[self uptime];
        _lastAutoScanAt=_refreshStartedAt;
    }
    const auto epoch=_workEpoch.snapshot();
    dispatch_async(_queue,^{
        if(self->_adapterEpoch.load()!=adapterGeneration||!self->_workEpoch.matches(epoch))return;
        RTWControlRequest in{};in.version=1;RTWControlReply out{};
        uint32_t result=[self->_driver exchangeRegistryID:registryID selector:selector input:&in length:sizeof(in) output:&out capacity:sizeof(out)];
        dispatch_async(dispatch_get_main_queue(),^{
            if(self->_terminating||!self->_workEpoch.matches(epoch)||
               !self->_selection.matches(registryID,adapterGeneration))return;
            self->_commandPending=NO;
            if(result){
                if(selector==RTW_CONTROL_SCAN) {
                    self->_refreshingNetworks=NO;self->_refreshStartedAt=-1;
                    self->_networkRefresh[registryID].failed([self uptime]);
                }
                self->_notice=errorText(result);self->_pollNotice=NO;
                if(result==rtwui::transportError||result==rtwui::protocolError) {
                    self->_authorizedSession=NO;self->_serviceRetry.fail([self uptime]);
                }
            }
            else if(selector==RTW_CONTROL_SCAN)self->_forceNetworkCache=YES;
            [self poll:nil];[self updateAppearance];
        });
    });
}
- (void)refresh:(id)sender {
    (void)sender;
    if(_terminating||_sleeping||_inactiveSession||!_authorizedSession||!_selectedPresent||
       !_selection.registryID||!_queue||_joining||_credentialBusy||_commandPending||
       _refreshingNetworks||(_hasStatus&&(_status.flags&RTW_CONTROL_SCANNING)))return;
    _refreshingNetworks=YES;_notice=@"Refreshing networks…";_pollNotice=NO;
    [self command:RTW_CONTROL_SCAN];[self updateAppearance];
}
- (void)disconnect:(id)sender {
    (void)sender;[self invalidatePendingWork];_autoJoin.paused=true;
    [[self preferences] setBool:YES forKey:@"RTWAutoJoinPaused"];
    ++_joinGeneration;_joining=NO;_joinAccepted=NO;_joinSSID=nil;_rememberCurrentJoin=NO;
    _joinStartedAt=-1;_notice=nil;_pollNotice=NO;
    _signal.clear();[self command:RTW_CONTROL_LEAVE];
}
- (void)selectNetwork:(NSMenuItem*)sender {
    // Keep the raw SSID bound to the displayed item while polling updates the cache.
    if(!_authorizedSession||!_selectedPresent||_joining||_credentialBusy||sender.tag!=1||![sender.representedObject isKindOfClass:NSDictionary.class])return;
    NSDictionary* record=sender.representedObject;
    if(!_selection.matches([record[@"registryID"] unsignedLongLongValue],[record[@"generation"] unsignedLongLongValue]))return;
    id channel=record[@"channel"],security=record[@"security"];
    if(![channel isKindOfClass:NSNumber.class]||![security isKindOfClass:NSNumber.class]||
       [channel unsignedLongLongValue]>UINT8_MAX||[security unsignedLongLongValue]>UINT32_MAX)return;
    RTWControlScanRecord observed{};observed.channel=[channel unsignedCharValue];
    observed.securityFlags=[security unsignedIntValue];
    if(!rtwui::supported(observed,[self stationFlags],[self stationChannelMap])) {
        _notice=@"This network's observed channel is not enabled by the current driver. Refresh networks.";
        [self updateAppearance];return;
    }
    NSData* ssid=record[@"ssid"];
    if([ssid isKindOfClass:NSData.class]&&ssid.length&&ssid.length<=32)[self connectSSID:ssid];
}
- (void)otherNetwork:(id)sender {
    (void)sender;if(!_authorizedSession||!_selectedPresent||_joining||_credentialBusy)return;
    const auto registryID=_selection.registryID,adapterGeneration=_selection.generation;
    NSAlert* alert=[NSAlert new];alert.messageText=@"Other WPA2 network";
    alert.informativeText=[NSString stringWithFormat:
        @"Enter the exact SSID of a WPA2-Personal/CCMP network on %@. The driver selects the access point.",
        text(rtwui::stationChannelDescription([self stationFlags],[self stationChannelMap]))];
    [alert addButtonWithTitle:@"Continue"];[alert addButtonWithTitle:@"Cancel"];
    NSTextField* field=[[NSTextField alloc]initWithFrame:NSMakeRect(0,0,340,24)];field.placeholderString=@"Network name (SSID)";[field setAccessibilityLabel:@"Network name"];alert.accessoryView=field;
    [NSApp activateIgnoringOtherApps:YES];alert.window.initialFirstResponder=field;
    // Polling continues in modal run-loop mode. Reserve this interaction so
    // a timer cannot begin an automatic JOIN while the SSID is being edited.
    _credentialBusy=YES;
    const NSModalResponse answer=[alert runModal];
    _credentialBusy=NO;
    if(answer!=NSAlertFirstButtonReturn)return;
    if(!_selection.matches(registryID,adapterGeneration)||!_selectedPresent){
        _notice=@"The adapter changed during connection. Select the network again.";[self updateAppearance];return;
    }
    NSData* ssid=[field.stringValue dataUsingEncoding:NSUTF8StringEncoding];
    if(!ssid.length||ssid.length>32){[self showMessage:@"Invalid SSID" detail:@"The SSID must contain 1 to 32 UTF-8 bytes."];return;}
    [self connectSSID:ssid];
}
- (void)connectSSID:(NSData*)ssid {
    if(!_authorizedSession||!_selectedPresent||_joining||_credentialBusy)return;
    _autoJoin.paused=false;_autoJoin.reset();
    [[self preferences] setBool:NO forKey:@"RTWAutoJoinPaused"];
    [self requestPasswordForSSID:ssid connect:YES];
}
- (void)requestPasswordForSSID:(NSData*)ssid connect:(BOOL)connect {
    [self loadPasswordForSSID:ssid connect:connect automatic:NO];
}
- (void)loadPasswordForSSID:(NSData*)ssid connect:(BOOL)connect automatic:(BOOL)automatic {
    if(_credentialBusy||_terminating||!ssid.length||ssid.length>32)return;
    const auto registryID=_selection.registryID,adapterGeneration=_selection.generation;
    const auto connectionGeneration=_joinGeneration;
    const auto automaticEpoch=_autoJoinEpoch.load();
    NSData* network=[ssid copy];
    id<RTWCredentialStoring> store=[self credentials];
    _credentialBusy=YES;[self updateAppearance];
    dispatch_async([self credentialQueue],^{
        NSError* error=nil;
        NSMutableData* saved=nil;
        if(automatic&&[store respondsToSelector:@selector(passwordForSSID:allowInteraction:error:)])
            saved=[store passwordForSSID:network allowInteraction:NO error:&error];
        else if(!automatic)
            saved=[store passwordForSSID:network error:&error];
        auto prepared=std::shared_ptr<RTWControlJoin>(new RTWControlJoin{},
            [](auto* p){rtwui::wipe(p,sizeof(*p));delete p;});
        const BOOL useSaved=connect&&saved&&!error;
        const BOOL validSaved=useSaved&&rtwui::makeJoin(static_cast<const uint8_t*>(network.bytes),
            network.length,static_cast<const char*>(saved.bytes),saved.length,*prepared);
        if(connect)RTWWipePassword(saved);
        dispatch_async(dispatch_get_main_queue(),^{
            if(self->_terminating){RTWWipePassword(saved);self->_credentialBusy=NO;return;}
            // A Keychain lookup can finish after the user turned automatic
            // reconnection off. Do not launch a stale automatic JOIN.
            if((automatic&&automaticEpoch!=self->_autoJoinEpoch.load())||
               !rtwui::credentialJoinStillAllowed(automatic,self->_autoJoinEnabled,
                                                  self->_autoJoin.paused)) {
                RTWWipePassword(saved);self->_credentialBusy=NO;
                [self updateAppearance];return;
            }
            if(connect&&(!self->_authorizedSession||!self->_selectedPresent||
               !self->_selection.matches(registryID,adapterGeneration)||
               connectionGeneration!=self->_joinGeneration)) {
                RTWWipePassword(saved);self->_credentialBusy=NO;
                self->_notice=@"The adapter changed. Select the network again.";[self updateAppearance];return;
            }
            if(automatic&&!rtwui::savedProfileReconnectAllowed(self->_scan,
               static_cast<const uint8_t*>(network.bytes),network.length,[self stationFlags],[self stationChannelMap])) {
                RTWWipePassword(saved);self->_credentialBusy=NO;
                [self updateAppearance];return;
            }
            if(useSaved&&validSaved) {
                self->_credentialBusy=NO;self->_rememberCurrentJoin=!automatic;
                self->_automaticCurrentJoin=automatic;
                [self startJoin:prepared ssid:network registryID:registryID adapterGeneration:adapterGeneration];
                return;
            }
            if(automatic) {
                RTWWipePassword(saved);self->_credentialBusy=NO;
                self->_autoJoin.retry.fail([self uptime]);
                self->_notice=error?@"Keychain is locked or access is pending. Select the network to authorize access.":
                    @"Saved password unavailable. Select the network to configure it.";
                [self updateAppearance];return;
            }
            if(connect&&error) {
                RTWWipePassword(saved);self->_credentialBusy=NO;
                [self showMessage:@"Could not access the saved password"
                    detail:RTWEnglishError(error)?:@"Unlock Keychain and try again. The password was not deleted."];
                return;
            }
            [self promptPasswordForSSID:network saved:saved readError:error connect:connect
                registryID:registryID adapterGeneration:adapterGeneration
                connectionGeneration:connectionGeneration];
        });
    });
}
- (void)promptPasswordForSSID:(NSData*)ssid saved:(NSMutableData*)saved
                   readError:(NSError*)readError connect:(BOOL)connect
                  registryID:(uint64_t)registryID adapterGeneration:(uint64_t)adapterGeneration
        connectionGeneration:(uint64_t)connectionGeneration {
    NSAlert* alert=[NSAlert new];
    alert.messageText=[NSString stringWithFormat:connect?@"Connect to %@":@"Save password for %@",
        ssidText(static_cast<const uint8_t*>(ssid.bytes),ssid.length)];
    NSString* explanation=connect?
        @"WPA2 password (8 to 63 ASCII characters). When selected, the password is saved to Keychain before connecting. Clearing this option uses the password only for this attempt; it does not delete a previously saved password.":
        @"Save to your user Keychain without connecting or authorizing the driver. The password can be reused when you select this network.";
    if(readError)explanation=[explanation stringByAppendingFormat:
        @"\n\nCould not read the saved password: %@\nEnter the password, or cancel and unlock Keychain.",RTWEnglishError(readError)];
    alert.informativeText=explanation;
    [alert addButtonWithTitle:connect?@"Connect":@"Save"];
    [alert addButtonWithTitle:@"Cancel"];
    NSView* accessory=[[NSView alloc]initWithFrame:NSMakeRect(0,0,360,76)];
    NSSecureTextField* field=[[NSSecureTextField alloc]initWithFrame:NSMakeRect(0,43,360,26)];
    field.placeholderString=@"Network password";
    [field setAccessibilityLabel:@"Network password"];
    if(saved)field.stringValue=[[NSString alloc]initWithData:saved encoding:NSASCIIStringEncoding]?:@"";
    RTWWipePassword(saved);
    [accessory addSubview:field];
    NSButton* remember=[NSButton checkboxWithTitle:@"Save password in macOS Keychain" target:nil action:nil];
    remember.frame=NSMakeRect(0,5,360,26);remember.state=NSControlStateValueOn;
    remember.enabled=connect;[accessory addSubview:remember];alert.accessoryView=accessory;
    [NSApp activateIgnoringOtherApps:YES];alert.window.initialFirstResponder=field;
    if([alert runModal]!=NSAlertFirstButtonReturn) {
        field.stringValue=@"";_credentialBusy=NO;[self updateAppearance];return;
    }
    char bytes[64]{};
    NSString* entered=field.stringValue;
    const NSUInteger length=[entered lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    const BOOL encoded=length<=63&&[entered getCString:bytes maxLength:sizeof(bytes) encoding:NSUTF8StringEncoding];
    field.stringValue=@"";entered=nil;
    if(!encoded||!rtwui::validPassword(bytes,length)) {
        rtwui::wipe(bytes,sizeof(bytes));_credentialBusy=NO;
        [self showMessage:@"Invalid password" detail:@"Use 8 to 63 printable ASCII characters. A 64-character hexadecimal key is not supported."];
        [self updateAppearance];return;
    }
    NSMutableData* password=[NSMutableData dataWithBytes:bytes length:length];
    rtwui::wipe(bytes,sizeof(bytes));
    if(connect&&(!_authorizedSession||!_selectedPresent||
       !_selection.matches(registryID,adapterGeneration)||connectionGeneration!=_joinGeneration)) {
        RTWWipePassword(password);_credentialBusy=NO;
        _notice=@"The adapter changed during connection. Select the network again.";[self updateAppearance];return;
    }
    const BOOL save=!connect||remember.state==NSControlStateValueOn;
    id<RTWCredentialStoring> store=[self credentials];
    _notice=save?@"Saving password to Keychain…":@"Preparing connection…";_pollNotice=NO;
    [self updateAppearance];
    dispatch_async([self credentialQueue],^{
        NSError* error=nil;
        const BOOL persisted=!save||[store savePassword:password forSSID:ssid error:&error];
        auto join=std::shared_ptr<RTWControlJoin>(new RTWControlJoin{},
            [](auto* p){rtwui::wipe(p,sizeof(*p));delete p;});
        const BOOL valid=!connect||rtwui::makeJoin(static_cast<const uint8_t*>(ssid.bytes),ssid.length,
            static_cast<const char*>(password.bytes),password.length,*join);
        RTWWipePassword(password);
        dispatch_async(dispatch_get_main_queue(),^{
            self->_credentialBusy=NO;
            if(self->_terminating)return;
            if(!persisted) {
                self->_notice=@"The password could not be saved. No connection was started.";self->_pollNotice=NO;
                [self showMessage:@"Could not save the password"
                    detail:[NSString stringWithFormat:@"%@\n\nUnlock Keychain and authorize access. To connect without saving, try again with the save option cleared.",
                        RTWEnglishError(error)?:@"Keychain did not confirm the save."]];
            } else if(!valid) {
                self->_notice=@"Could not prepare the connection.";self->_pollNotice=NO;
            } else if(!connect) {
                self->_notice=@"Password saved to macOS Keychain.";self->_pollNotice=NO;
                [self showMessage:@"Password saved" detail:@"The password was saved and read back from Keychain. When you select this network, the app will reuse it without opening another password prompt."];
            } else if(!self->_authorizedSession||!self->_selectedPresent||
                      !self->_selection.matches(registryID,adapterGeneration)||
                      connectionGeneration!=self->_joinGeneration) {
                self->_notice=save?@"Password saved. The adapter changed; select the network again.":
                    @"The adapter changed; select the network again.";self->_pollNotice=NO;
            } else {
                self->_rememberCurrentJoin=YES;
                self->_automaticCurrentJoin=NO;
                [self startJoin:join ssid:ssid registryID:registryID adapterGeneration:adapterGeneration];
            }
            if(save&&persisted)[self reloadSavedNetworks];
            [self updateAppearance];
        });
    });
}
- (void)startJoin:(std::shared_ptr<RTWControlJoin>)join ssid:(NSData*)ssid
      registryID:(uint64_t)registryID adapterGeneration:(uint64_t)adapterGeneration {
    if(_terminating||_sleeping||_inactiveSession||!_queue||
       !_authorizedSession||!_selectedPresent||_joining||
       !_selection.matches(registryID,adapterGeneration))return;
    const auto generation=++_joinGeneration,epoch=_workEpoch.snapshot();
    const bool automatic=_automaticCurrentJoin;
    const auto automaticEpoch=_autoJoinEpoch.load();
    _signal.clear();_joining=YES;_joinAccepted=NO;_joinStartedAt=[self uptime];
    _joinSSID=[ssid copy];_notice=nil;_pollNotice=NO;[self updateAppearance];
    dispatch_async(_queue,^{
        if(self->_adapterEpoch.load()!=adapterGeneration||!self->_workEpoch.matches(epoch)||
           (automatic&&automaticEpoch!=self->_autoJoinEpoch.load())){
            rtwui::wipe(join.get(),sizeof(*join));return;
        }
        RTWControlReply reply{};
        uint32_t code=[self->_driver exchangeRegistryID:registryID selector:RTW_CONTROL_JOIN input:join.get() length:sizeof(*join) output:&reply capacity:sizeof(reply)];
        rtwui::wipe(join.get(),sizeof(*join));
        dispatch_async(dispatch_get_main_queue(),^{
            if(self->_terminating||!self->_workEpoch.matches(epoch)||generation!=self->_joinGeneration||
               !self->_selection.matches(registryID,adapterGeneration))return;
            self->_joinAccepted=!code;
            if(code){self->_joining=NO;self->_joinSSID=nil;self->_joinStartedAt=-1;
                self->_rememberCurrentJoin=NO;self->_autoJoin.retry.fail([self uptime]);
                self->_notice=errorText(code);self->_pollNotice=NO;
                if(code==rtwui::transportError||code==rtwui::protocolError) {
                    self->_authorizedSession=NO;self->_serviceRetry.fail([self uptime]);
                }
            }
            [self poll:nil];[self updateAppearance];
        });
    });
}
- (void)saveNetworkPassword:(id)sender {
    (void)sender;if(_credentialBusy||_joining)return;
    NSAlert* alert=[NSAlert new];alert.messageText=@"Save a WPA2 network password";
    alert.informativeText=@"Enter the exact network name (SSID). An adapter does not need to be connected to save a password.";
    [alert addButtonWithTitle:@"Continue"];[alert addButtonWithTitle:@"Cancel"];
    NSTextField* field=[[NSTextField alloc]initWithFrame:NSMakeRect(0,0,360,26)];
    field.placeholderString=@"Network name (SSID)";[field setAccessibilityLabel:@"Network name"];alert.accessoryView=field;
    [NSApp activateIgnoringOtherApps:YES];alert.window.initialFirstResponder=field;
    _credentialBusy=YES;
    const NSModalResponse response=[alert runModal];
    _credentialBusy=NO;
    if(response!=NSAlertFirstButtonReturn)return;
    NSData* ssid=[field.stringValue dataUsingEncoding:NSUTF8StringEncoding];
    if(!ssid.length||ssid.length>32) {
        [self showMessage:@"Invalid SSID" detail:@"The SSID must contain 1 to 32 UTF-8 bytes."];return;
    }
    [self requestPasswordForSSID:ssid connect:NO];
}
- (void)manageSavedPasswords:(id)sender {
    (void)sender;if(_credentialBusy||_joining||_terminating)return;
    id<RTWCredentialStoring> store=[self credentials];
    _credentialBusy=YES;[self updateAppearance];
    dispatch_async([self credentialQueue],^{
        NSError* error=nil;NSArray<NSData*>* networks=[store savedSSIDsAndReturnError:&error];
        dispatch_async(dispatch_get_main_queue(),^{
            self->_credentialBusy=NO;
            if(self->_terminating)return;
            if(!networks) {
                [self showMessage:@"Could not list saved passwords" detail:RTWEnglishError(error)?:@"Unlock Keychain and try again."];
                return;
            }
            if(!networks.count) {
                [self showMessage:@"No saved passwords" detail:@"Choose 'Save a network password…' or select 'Save password in macOS Keychain' when connecting."];
                return;
            }
            NSAlert* alert=[NSAlert new];alert.messageText=@"Saved passwords";
            alert.informativeText=@"Only networks saved by this app are listed. Editing does not start a connection. Forget removes only the selected network password.";
            [alert addButtonWithTitle:@"Edit password"];[alert addButtonWithTitle:@"Cancel"];[alert addButtonWithTitle:@"Forget"];
            NSPopUpButton* chooser=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(0,0,360,28) pullsDown:NO];
            for(NSData* ssid in networks) {
                // Explicit NSMenuItem keeps distinct binary SSIDs even if their
                // display labels are visually identical.
                NSMenuItem* item=[[NSMenuItem alloc]initWithTitle:
                    ssidText(static_cast<const uint8_t*>(ssid.bytes),ssid.length) action:nil keyEquivalent:@""];
                item.representedObject=ssid;[chooser.menu addItem:item];
            }
            [chooser selectItemAtIndex:0];alert.accessoryView=chooser;[NSApp activateIgnoringOtherApps:YES];
            self->_credentialBusy=YES;
            const NSModalResponse response=[alert runModal];
            self->_credentialBusy=NO;
            NSData* ssid=chooser.selectedItem.representedObject;
            if(response==NSAlertFirstButtonReturn) {
                [self requestPasswordForSSID:ssid connect:NO];
            } else if(response==NSAlertThirdButtonReturn&&ssid) {
                [self forgetPasswordForSSID:ssid];
            }
        });
    });
}
- (void)forgetPasswordForSSID:(NSData*)ssid {
    NSAlert* confirm=[NSAlert new];confirm.messageText=@"Forget this network password?";
    confirm.informativeText=ssidText(static_cast<const uint8_t*>(ssid.bytes),ssid.length);
    [confirm addButtonWithTitle:@"Forget"];[confirm addButtonWithTitle:@"Cancel"];
    _credentialBusy=YES;
    if([confirm runModal]!=NSAlertFirstButtonReturn){_credentialBusy=NO;return;}
    id<RTWCredentialStoring> store=[self credentials];
    dispatch_async([self credentialQueue],^{
        NSError* error=nil;BOOL removed=[store deletePasswordForSSID:ssid error:&error];
        dispatch_async(dispatch_get_main_queue(),^{
            self->_credentialBusy=NO;if(self->_terminating)return;
            if(removed) {
                if([self->_preferredSSID isEqualToData:ssid]) {
                    self->_preferredSSID=nil;
                    [[self preferences] removeObjectForKey:@"RTWLastConnectedSSID"];
                }
                [self reloadSavedNetworks];
            }
            self->_notice=removed?@"Password removed from Keychain.":@"Could not remove the password.";
            self->_pollNotice=NO;[self updateAppearance];
            if(!removed)[self showMessage:@"Could not forget the password"
                detail:RTWEnglishError(error)?:@"Unlock Keychain and try again."];
        });
    });
}
- (void)showMessage:(NSString*)title detail:(NSString*)detail {
    NSAlert* alert=[NSAlert new];alert.messageText=title;alert.informativeText=detail;[alert addButtonWithTitle:@"OK"];
    [NSApp activateIgnoringOtherApps:YES];[alert runModal];
}
- (void)showReport:(NSString*)report title:(NSString*)title explanation:(NSString*)explanation {
    if(_terminating||_sleeping||_inactiveSession)return;
    NSAlert* alert=[NSAlert new];
    alert.messageText=title;
    alert.informativeText=explanation;
    NSScreen* screen=NSApp.keyWindow.screen?:NSScreen.mainScreen;
    const NSRect available=screen?screen.visibleFrame:NSMakeRect(0,0,1024,768);
    const CGFloat width=std::max<CGFloat>(300,std::min<CGFloat>(640,available.size.width-120));
    const CGFloat height=std::max<CGFloat>(160,std::min<CGFloat>(360,available.size.height-260));
    NSScrollView* scroll=[[NSScrollView alloc]initWithFrame:NSMakeRect(0,0,width,height)];
    scroll.hasVerticalScroller=YES;
    scroll.borderType=NSBezelBorder;
    NSTextView* view=[[NSTextView alloc]initWithFrame:NSMakeRect(0,0,width-20,height)];
    view.textColor=NSColor.textColor;view.backgroundColor=NSColor.textBackgroundColor;
    view.textContainerInset=NSMakeSize(8,8);
    [view setAccessibilityLabel:title];
    view.editable=NO;view.selectable=YES;
    view.font=[NSFont monospacedSystemFontOfSize:11 weight:NSFontWeightRegular];
    view.verticallyResizable=YES;view.horizontallyResizable=NO;
    view.autoresizingMask=NSViewWidthSizable;
    view.textContainer.widthTracksTextView=YES;
    view.textContainer.containerSize=NSMakeSize(width-36,CGFLOAT_MAX);
    view.maxSize=NSMakeSize(CGFLOAT_MAX,CGFLOAT_MAX);
    view.string=report?:@"Could not generate the report.";
    scroll.documentView=view;alert.accessoryView=scroll;
    [alert addButtonWithTitle:@"Close"];
    [alert addButtonWithTitle:@"Copy report"];
    [NSApp activateIgnoringOtherApps:YES];
    if([alert runModal]==NSAlertSecondButtonReturn){
        [NSPasteboard.generalPasteboard clearContents];
        [NSPasteboard.generalPasteboard setString:view.string forType:NSPasteboardTypeString];
    }
}
- (void)signalDiagnostics:(id)sender {
    (void)sender;
    [self showReport:text(_signalHistory.report()) title:@"Signal diagnostics"
        explanation:@"Raw readings, filtered readings and connection/error counters. Copy the report after an oscillation."];
}
- (void)adapterCatalog:(id)sender {
    (void)sender;
    [self showReport:RTWAdapterCatalogReport() title:@"Adapter catalog"
        explanation:@"All catalog profiles are visible. Only verified identities with implemented station backends can attach."];
}
- (void)adapterSupport:(id)sender {
    (void)sender;
    if(_terminating||_adapterReportBusy)return;
    _adapterReportBusy=YES;
    const auto epoch=_workEpoch.snapshot();
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY,0),^{
        NSString* report=RTWAdapterSupportReport();
        dispatch_async(dispatch_get_main_queue(),^{
            if(self->_terminating||!self->_workEpoch.matches(epoch)){
                self->_adapterReportBusy=NO;return;
            }
            [self showReport:report title:@"USB adapters and support"
                explanation:@"Read-only diagnostics. An enabled catalog ID does not certify physical hardware operation."];
            self->_adapterReportBusy=NO;
        });
    });
}
- (void)details:(id)sender {
    (void)sender;
    NSString* detail=_hasStatus?[NSString stringWithFormat:@"%@\n%@\nnet80211 state: %u · flags: 0x%x\nUSB: 0x%08x\nTX: %llu · RX: %llu\nTX errors: %llu · RX errors: %llu\n\nWPA2 authorization indicates a link. An IP address does not prove internet access.",[self phaseTitle],_interface?:@"",_status.state,_status.flags,_status.usbError,(unsigned long long)_status.txPackets,(unsigned long long)_status.rxPackets,(unsigned long long)_status.txErrors,(unsigned long long)_status.rxErrors]:_notice?:@"Authorize driver access from the menu.";
    [self showMessage:@"USB Wi-Fi diagnostics" detail:detail];
}
- (void)toggleLoginAtStartup:(id)sender {
    id<RTWLoginItemManaging> manager=[self loginItems];
    RTWLoginItemState current=manager.state;
    const auto action=rtwui::loginToggleAction(current==RTWLoginItemStateEnabled,
        current==RTWLoginItemStateRequiresApproval);
    if(action==rtwui::LoginAction::OpenSettings) {
        [manager openSystemSettings];return;
    }
    const BOOL enable=action==rtwui::LoginAction::Enable;
    NSError* error=nil;
    if(![manager setEnabled:enable error:&error]) {
        [self showMessage:@"Could not change login startup"
            detail:[NSString stringWithFormat:@"%@\n\nRun the installed copy in Applications. Choose 'Repair login startup…' to replace an old registration.",
                RTWEnglishError(error)?:@"macOS denied the operation."]];
    } else {
        [[self preferences] setBool:YES forKey:@"RTWStartupChoiceMade"];
        [[self preferences] setBool:enable forKey:@"RTWStartupRequested"];
        if(enable&&manager.state==RTWLoginItemStateRequiresApproval) {
            [manager openSystemSettings];
            [self showMessage:@"Approval required"
                detail:@"Allow the app in macOS Login Items. A mixed checkmark means approval is still pending, not that startup is enabled."];
        }
    }
    // Show the state actually confirmed by macOS, never merely the request.
    if([sender isKindOfClass:NSMenuItem.class]) {
        const auto confirmed=manager.state;
        ((NSMenuItem*)sender).state=confirmed==RTWLoginItemStateEnabled?NSControlStateValueOn:
            confirmed==RTWLoginItemStateRequiresApproval?NSControlStateValueMixed:NSControlStateValueOff;
    }
    [self updateAppearance];
}
- (void)disableLoginAtStartup:(id)sender {
    (void)sender;NSError* error=nil;
    if(![[self loginItems] setEnabled:NO error:&error])
        [self showMessage:@"Could not disable login startup"
            detail:RTWEnglishError(error)?:@"Check macOS Login Items."];
    else {
        [[self preferences] setBool:YES forKey:@"RTWStartupChoiceMade"];
        [[self preferences] setBool:NO forKey:@"RTWStartupRequested"];
    }
    [self updateAppearance];
}
- (void)repairLoginAtStartup:(id)sender {
    (void)sender;
    NSAlert* alert=[NSAlert new];alert.messageText=@"Repair and enable login startup?";
    alert.informativeText=@"Remove this app's old registration and register the installed copy again. This does not change other login items or save your administrator password.";
    [alert addButtonWithTitle:@"Repair and enable"];[alert addButtonWithTitle:@"Cancel"];
    [NSApp activateIgnoringOtherApps:YES];if([alert runModal]!=NSAlertFirstButtonReturn)return;
    NSError* error=nil;id<RTWLoginItemManaging> manager=[self loginItems];
    if(![manager repairRegistrationAndReturnError:&error])
        [self showMessage:@"Could not repair login startup"
            detail:RTWEnglishError(error)?:@"macOS denied registration."];
    else if(manager.state==RTWLoginItemStateRequiresApproval)
        [self showMessage:@"Approval required" detail:@"Allow the app in Login Items in System Settings."];
    else
        [self showMessage:@"Registration updated" detail:@"macOS reports that the app is enabled to open the next time you log in."];
    if(manager.state==RTWLoginItemStateEnabled||manager.state==RTWLoginItemStateRequiresApproval) {
        [[self preferences] setBool:YES forKey:@"RTWStartupChoiceMade"];
        [[self preferences] setBool:YES forKey:@"RTWStartupRequested"];
    }
    [self updateAppearance];
}
- (void)openLoginSettings:(id)sender {(void)sender;[[self loginItems] openSystemSettings];}
- (void)about:(id)sender {
    (void)sender;[self showMessage:@"Tahoe Wi-Fi USB 0.3.11" detail:[NSString stringWithFormat:@"Menu-bar controller for TahoeRealtekWiFi (ABI v1).\nRealtek USB adapters · macOS Ventura 13 / Sonoma 14 / Sequoia 15 / Tahoe 26 · Intel\n\nWPA2-Personal/CCMP. Selected driver channels: %@. Low, DFS (52–64 and 100–140) and upper 5 GHz each require their own live station capability. DFS is managed-station HT20, with passive discovery and AP channel-switch recovery; channel 144 is not enabled. Other bands or catalog entries do not imply connection support. Experimental driver. WPA2 passwords can be saved in macOS Keychain. Automatic reconnection after login, sleep and USB reconnection. The background helper requires initial approval in macOS Login Items; the app never saves your administrator password.\n\nProject and macOS development: MaLd0n.\nApple AppKit and SF Symbols.\nApp source: GPL-2.0-only.",text(rtwui::stationChannelDescription([self stationFlags],[self stationChannelMap]))]];
}
- (void)quit:(id)sender {(void)sender;[NSApp terminate:nil];}
- (void)applicationWillTerminate:(NSNotification*)notification {
    (void)notification;_terminating=YES;[self invalidatePendingWork];
    [_timer invalidate];_timer=nil;
    [NSWorkspace.sharedWorkspace.notificationCenter removeObserver:self];
    // Never wait for an XPC deadline on AppKit's main thread. Queued requests
    // are cancelled by epoch; process exit also destroys the XPC connection.
    // Capture the connection, not the app, and preserve the active radio link.
    RTWDriverConnection* driver=_driver;
    if(_queue)dispatch_async(_queue,^{[driver close];});
    if(_item)[NSStatusBar.systemStatusBar removeStatusItem:_item];
    if(_instanceLock>=0){close(_instanceLock);_instanceLock=-1;}
}

@end

int main(int argc,const char** argv) {
    rlimit core{0,0};setrlimit(RLIMIT_CORE,&core);
    @autoreleasepool {
        const int diagnosticExit=RTWHandleAdapterDiagnosticCLI(argc,argv);
        if(diagnosticExit>=0)return diagnosticExit;
        if([NSProcessInfo.processInfo.arguments containsObject:@"--prepare-update"]) {
            if(geteuid()==0)return 77;
            RTWDriverServiceManager* service=[RTWDriverServiceManager new];
            RTWLoginItemManager* login=[RTWLoginItemManager new];
            NSUserDefaults* defaults=NSUserDefaults.standardUserDefaults;
            NSError* error=nil;
            if(service.state==RTWLoginItemStateEnabled) {
                [defaults setBool:YES forKey:@"RTWResumeServiceAfterInstall"];
                if(![service disableAndReturnError:&error])return 1;
            }
            if(login.state==RTWLoginItemStateEnabled) {
                [defaults setBool:YES forKey:@"RTWResumeLoginAfterInstall"];
                if(![login setEnabled:NO error:&error])return 1;
            }
            // This short-lived process must flush migration flags before exit.
            [defaults synchronize];
            return 0;
        }
        NSApplication* app=NSApplication.sharedApplication;[app setActivationPolicy:NSApplicationActivationPolicyAccessory];
        __attribute__((objc_precise_lifetime)) WiFiApp* delegate=[WiFiApp new];
        app.delegate=delegate;[app run];
    }
    return 0;
}
