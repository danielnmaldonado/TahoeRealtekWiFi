// SPDX-License-Identifier: GPL-2.0-only
// Hardware/helper/Keychain-free tests of the app's exact-adapter diagnostic mode.
#import "../DiagnosticCLI.h"
#import "../ControlDiagnostics.h"
#import "../DriverConnection.h"
#include "../Transport.hpp"
#include <cassert>
#include <initializer_list>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

static rtwui::DiagnosticArguments arguments(std::initializer_list<const char*> words) {
    std::vector<const char*> values(words);
    return rtwui::parseDiagnosticArguments(static_cast<int>(values.size()),values.data());
}
@interface DiagnosticDriver : RTWDriverConnection
@property(nonatomic) uint64_t expectedID;
@property(nonatomic) BOOL missing;
@property(nonatomic) uint32_t listError;
@property(nonatomic) uint32_t scanError;
@property(nonatomic) NSMutableArray<NSNumber*>* selectors;
@end
@implementation DiagnosticDriver
- (uint32_t)exchangeRegistryID:(uint64_t)registryID selector:(uint32_t)selector
                        input:(const void*)input length:(size_t)length
                       output:(void*)output capacity:(size_t)capacity {
    assert(!NSThread.isMainThread);
    [self.selectors addObject:@(selector)];
    assert(capacity==rtwui::outputSize(selector));
    memset(output,0,capacity);
    if(selector==rtwui::listSelector) {
        assert(registryID==0&&!input&&!length);
        if(self.listError)return self.listError;
        auto& list=*static_cast<rtwui::DeviceList*>(output);list.version=rtwui::ipcVersion;
        list.count=self.missing?1:2;
        list.devices[0].registryID=999;strcpy(list.devices[0].family,"RTL8192EU");
        if(!self.missing){list.devices[1].registryID=self.expectedID;strcpy(list.devices[1].family,"RTL8814AU");}
        return 0;
    }
    assert(registryID==self.expectedID);
    assert(rtw_control_validate_request(selector,input,length)==RTW_CONTROL_VALID);
    assert(selector==RTW_CONTROL_STATUS||selector==RTW_CONTROL_CHANNEL_MAP||selector==RTW_CONTROL_SCAN);
    if(selector==RTW_CONTROL_STATUS){
        auto& status=*static_cast<RTWControlStatus*>(output);status.version=RTW_CONTROL_VERSION;
        status.state=2;status.channel=6;status.usbError=0xe00002cd;
        memset(status.mac,0xab,sizeof(status.mac));memset(status.bssid,0xcd,sizeof(status.bssid));
        strcpy(reinterpret_cast<char*>(status.ssid),"DO-NOT-LOG-SSID");status.ssidLength=14;
        status.txPackets=123;status.rxPackets=456;
    } else if(selector==RTW_CONTROL_CHANNEL_MAP){
        auto& map=*static_cast<RTWControlChannelMap*>(output);map.version=RTW_CONTROL_VERSION;
        map.channels[6/8]|=1u<<(6%8);
    } else {
        if(self.scanError)return self.scanError;
        static_cast<RTWControlReply*>(output)->version=RTW_CONTROL_VERSION;
    }
    return 0;
}
@end
static NSArray<NSDictionary*>* run(DiagnosticDriver* driver,bool scan,int expectedExit) {
    NSMutableArray<NSDictionary*>* records=[NSMutableArray array];
    __block int actual=-1;__block unsigned snapshots=0;
    dispatch_semaphore_t ready=dispatch_semaphore_create(0);
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY,0),^{
        actual=RTWRunAdapterDiagnosis({rtwui::DiagnosticMode::Valid,driver.expectedID,scan},driver,
            ^NSDictionary*(uint64_t id){assert(id==driver.expectedID);++snapshots;return @{ @"registry_id":@(id),@"RTWDiagPublicationGeneration":@1 };},
            ^(NSDictionary* event){[records addObject:event];});
        dispatch_semaphore_signal(ready);
    });
    assert(dispatch_semaphore_wait(ready,dispatch_time(DISPATCH_TIME_NOW,2*NSEC_PER_SEC))==0);
    assert(actual==expectedExit);
    assert(snapshots==(driver.listError||driver.missing?0:2));
    return records;
}
int main() {
    @autoreleasepool {
        assert(arguments({"app"}).mode==rtwui::DiagnosticMode::NotRequested);
        auto parsed=arguments({"app","--diagnose-adapter","4294968492","--scan"});
        assert(parsed.mode==rtwui::DiagnosticMode::Valid&&parsed.registryID==4294968492ULL&&parsed.scan);
        assert(arguments({"app","--diagnose-adapter","0x1000004ac"}).registryID==4294968492ULL);
        assert(arguments({"app","--diagnose-adapter","18446744073709551615"}).registryID==UINT64_MAX);
        for(const char* bad:{"0","-1","+1"," 1","1 ","0x","0x0","1x","18446744073709551616","0x10000000000000000",""})
            assert(arguments({"app","--diagnose-adapter",bad}).mode==rtwui::DiagnosticMode::Invalid);
        assert(arguments({"app","--diagnose-adapter"}).mode==rtwui::DiagnosticMode::Invalid);
        assert(arguments({"app","--diagnose-adapter","1","--join"}).mode==rtwui::DiagnosticMode::Invalid);
        assert(arguments({"app","--prepare-update","--diagnose-adapter","1"}).mode==rtwui::DiagnosticMode::Invalid);
        DiagnosticDriver* driver=[DiagnosticDriver new];driver.expectedID=4294968492ULL;driver.selectors=[NSMutableArray array];
        auto records=run(driver,true,0);
        assert(([driver.selectors isEqualToArray:@[@(rtwui::listSelector),@(RTW_CONTROL_STATUS),@(RTW_CONTROL_CHANNEL_MAP),@(RTW_CONTROL_SCAN),@(RTW_CONTROL_STATUS)]]));
        NSData* json=[NSJSONSerialization dataWithJSONObject:records options:0 error:nil];
        NSString* rendered=[[NSString alloc]initWithData:json encoding:NSUTF8StringEncoding];
        assert(![rendered containsString:@"DO-NOT-LOG-SSID"]&&![rendered containsString:@"bssid"]&&![rendered containsString:@"mac\""]);
        assert([rendered containsString:@"0xe00002cd"]&&[rendered containsString:@"duration_ms"]);
        driver.selectors=[NSMutableArray array];run(driver,false,0);
        assert(([driver.selectors isEqualToArray:@[@(rtwui::listSelector),@(RTW_CONTROL_STATUS),@(RTW_CONTROL_CHANNEL_MAP)]]));
        driver.scanError=0xe00002c0;driver.selectors=[NSMutableArray array];records=run(driver,true,1);
        assert(driver.selectors.count==5&&[driver.selectors.lastObject unsignedIntValue]==RTW_CONTROL_STATUS);
        assert(([[[records filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"result_hex == %@",@"0xe00002c0"]] firstObject] count]>0));
        driver.missing=YES;driver.selectors=[NSMutableArray array];run(driver,true,1);
        assert(driver.selectors.count==1);
        driver.missing=NO;driver.listError=rtwui::transportError;driver.selectors=[NSMutableArray array];run(driver,true,1);
        assert(driver.selectors.count==1);
        // Real DriverConnection's local validation must stay in front of XPC,
        // clear previous outputs, and log a numeric error without input bytes.
        RTWDriverConnection* connection=[RTWDriverConnection new];RTWControlStatus stale{};
        memset(&stale,0xff,sizeof(stale));RTWControlRequest invalid{};
        assert([connection exchangeRegistryID:driver.expectedID selector:RTW_CONTROL_STATUS input:&invalid length:sizeof(invalid) output:&stale capacity:sizeof(stale)]==rtwui::protocolError);
        for(auto byte:std::vector<uint8_t>(reinterpret_cast<uint8_t*>(&stale),reinterpret_cast<uint8_t*>(&stale)+sizeof(stale)))assert(byte==0);
        // Private JSONL logging rejects symlink destinations and records no
        // caller-supplied packet/request data.
        NSString* dir=[NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString];
        NSDictionary* event=RTWControlExchangeRecord(driver.expectedID,RTW_CONTROL_JOIN,0xe00002c0,12.5,NO);
        assert(RTWAppendControlDiagnosticRecord(event,dir));
        NSString* path=[dir stringByAppendingPathComponent:@"control.jsonl"];
        struct stat st{};assert(!stat(path.fileSystemRepresentation,&st)&&(st.st_mode&0777)==0600);
        NSString* line=[NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
        assert([line containsString:@"JOIN"]&&[line containsString:@"0xe00002c0"]);
        [NSFileManager.defaultManager removeItemAtPath:path error:nil];
        assert(!symlink("/dev/null",path.fileSystemRepresentation));
        assert(!RTWAppendControlDiagnosticRecord(event,dir));
        [NSFileManager.defaultManager removeItemAtPath:dir error:nil];
        puts("App diagnostic CLI: exact device, safe selectors, errors and secret-free logging passed");
    }
}
