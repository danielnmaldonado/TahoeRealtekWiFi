// SPDX-License-Identifier: GPL-2.0-only
// Restoring per-STATUS or duplicate MAP+SCAN rebuilds must fail this regression.
// Map-only revocation and generation changes must still re-evaluate the cache.
@interface NetworkRebuildApp : WiFiApp
- (void)prepare:(BandDriverConnection*)driver;
- (void)renewGeneration;
@end
@implementation NetworkRebuildApp
- (double)uptime {return 100;}
- (void)updateAppearance {}
- (void)considerNetworkRefresh {}
- (void)considerAutoJoin {}
- (void)prepare:(BandDriverConnection*)driver {
    _driver=driver;_selection.select(3003);_adapterEpoch.store(_selection.generation);
    _selectedPresent=_authorizedSession=_savedNetworksLoaded=_connectionPreferencesLoaded=YES;
    rtwui::DeviceRecord record{};record.registryID=3003;_adapters={record};
    _preferredAdapterPendingID=3003;_forceDeviceList=_forceNetworkCache=YES;
    _lastDeviceListAt=_lastNetworkCacheAt=-1;_lastInterfaceAt=100;_interface=@"Fixture";
    _queue=serialWorkQueue("org.research.TahoeUSBWiFi.test.model-rebuild");_menu=menuForControl();
}
- (void)renewGeneration {
    _selection.invalidate();_selection.select(3003);_adapterEpoch.store(_selection.generation);
}
@end
static unsigned testNetworkRebuildCoalescing() {
    BandDriverConnection* driver=[BandDriverConnection new];driver.sharedSSID=YES;
    NetworkRebuildApp* app=[NetworkRebuildApp new];[app prepare:driver];
    NSMenu* menu=[app valueForKey:@"menu"];unsigned failures=0;
    auto poll=[&](bool scan,unsigned expected,const char* reason) {
        networkModelRebuilds=0;const auto maps=driver.mapReads;
        completeBandPoll(app,scan);
        std::printf("Network rebuild: %s: %u (expected %u)\n",reason,networkModelRebuilds,expected);
        failures+=networkModelRebuilds!=expected;
        assert(driver.mapReads==maps+1&&"every STATUS must re-read the map for immediate revocation");
    };
    poll(true,1,"initial legacy MAP and SCAN coalesce");
    assert([networkItem(menu,"SharedLab").representedObject[@"channel"] unsignedIntValue]==6);
    poll(false,0,"unchanged legacy STATUS preserves the cached vector");
    driver.fiveGHz=YES;poll(false,1,"legacy band change reselects cached BSS");
    assert([networkItem(menu,"SharedLab").representedObject[@"channel"] unsignedIntValue]==36);
    driver.channelMapMode=1;poll(true,1,"exact MAP transition and new SCAN coalesce");
    assert([networkItem(menu,"SharedLab").representedObject[@"channel"] unsignedIntValue]==100);
    poll(false,0,"unchanged exact STATUS preserves the cached vector");
    driver.channelMapMode=2;poll(false,1,"map-only revocation reselects cached BSS");
    assert([networkItem(menu,"SharedLab").representedObject[@"channel"] unsignedIntValue]==132);
    poll(false,0,"unchanged revoked map needs no rebuild");
    [app renewGeneration];poll(false,1,"a new adapter generation renews map ownership");
    driver.channelMapMode=0;poll(false,1,"explicit Unsupported restores legacy band rules");
    assert([networkItem(menu,"SharedLab").representedObject[@"channel"] unsignedIntValue]==120);
    assert(!networkItem(menu,"SharedLab").enabled); // cached DFS-only BSSs are forbidden by this legacy driver
    driver.fiveGHz=NO;poll(true,1,"band change and new SCAN coalesce");
    assert([networkItem(menu,"SharedLab").representedObject[@"channel"] unsignedIntValue]==6);
    std::puts("Actual Main model work: unchanged polls avoid sorting; new cache/map/band/generation changes rebuild once");
    return failures;
}
