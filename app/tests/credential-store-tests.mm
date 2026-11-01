// SPDX-License-Identifier: GPL-2.0-only
#import "../CredentialStore.h"
#include <cstdio>
#include <cstdlib>
static unsigned checks=0,updates=0,adds=0,deletes=0,reads=0;
static NSMutableDictionary<NSString*,NSData*>* items;
static OSStatus forcedUpdate=errSecSuccess,forcedAdd=errSecSuccess,forcedRead=errSecSuccess,forcedDelete=errSecSuccess;
static BOOL creationRace=NO;
static BOOL lastReadNoninteractive=NO;
static void check(bool condition,const char* message) {
    ++checks;if(!condition){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
static NSDictionary* dictionary(CFDictionaryRef value) {return (__bridge NSDictionary*)value;}
static NSString* key(CFDictionaryRef value) {
    NSDictionary* q=dictionary(value);
    check([q[(__bridge id)kSecAttrService] isEqual:@"test.TahoeUSBWiFi"],"service scoped");
    check([q[(__bridge id)kSecClass] isEqual:(__bridge id)kSecClassGenericPassword],"generic password");
    check(!q[(__bridge id)kSecAttrAccessGroup]&&!q[(__bridge id)kSecAttrAccess],"no ACL weakening");
    return q[(__bridge id)kSecAttrAccount];
}
static OSStatus copyItem(CFDictionaryRef query,CFTypeRef* result) {
    ++reads;NSString* account=key(query);NSDictionary* q=dictionary(query);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    lastReadNoninteractive=[q[(__bridge id)kSecUseAuthenticationUI] isEqual:(__bridge id)kSecUseAuthenticationUIFail];
#pragma clang diagnostic pop
    if(forcedRead!=errSecSuccess)return forcedRead;
    if([q[(__bridge id)kSecReturnAttributes] boolValue]) {
        check(!q[(__bridge id)kSecReturnData],"list never requests secret data");
        NSMutableArray* records=[NSMutableArray array];
        for(NSString* name in items)[records addObject:@{(__bridge id)kSecAttrAccount:name}];
        if(!records.count)return errSecItemNotFound;
        if(result)*result=CFBridgingRetain(records);
        return errSecSuccess;
    }
    check([q[(__bridge id)kSecReturnData] boolValue],"read requests data");
    NSData* value=items[account];if(!value)return errSecItemNotFound;
    if(result)*result=CFBridgingRetain(value);return errSecSuccess;
}
static OSStatus updateItem(CFDictionaryRef query,CFDictionaryRef changes) {
    ++updates;NSString* account=key(query);
    check(!dictionary(query)[(__bridge id)kSecValueData],"secret not in search query");
    if(forcedUpdate!=errSecSuccess)return forcedUpdate;
    if(!items[account])return errSecItemNotFound;
    items[account]=[dictionary(changes)[(__bridge id)kSecValueData] copy];return errSecSuccess;
}
static OSStatus addItem(CFDictionaryRef attributes,CFTypeRef*) {
    ++adds;NSString* account=key(attributes);
    if(forcedAdd!=errSecSuccess)return forcedAdd;
    if(creationRace) {items[account]=[@"racepass" dataUsingEncoding:NSASCIIStringEncoding];creationRace=NO;return errSecDuplicateItem;}
    if(items[account])return errSecDuplicateItem;
    items[account]=[dictionary(attributes)[(__bridge id)kSecValueData] copy];return errSecSuccess;
}
static OSStatus removeItem(CFDictionaryRef query) {
    ++deletes;NSString* account=key(query);
    check(!dictionary(query)[(__bridge id)kSecValueData],"delete is identity-only");
    if(forcedDelete!=errSecSuccess)return forcedDelete;
    if(!items[account])return errSecItemNotFound;
    [items removeObjectForKey:account];return errSecSuccess;
}
int main() {
    @autoreleasepool {
        items=[NSMutableDictionary dictionary];
        RTWKeychainAPI api{copyItem,addItem,updateItem,removeItem};
        RTWCredentialStore* store=[[RTWCredentialStore alloc]initWithService:@"test.TahoeUSBWiFi" api:api];
        NSData* ssid=[@"Lab" dataUsingEncoding:NSUTF8StringEncoding];
        NSData* password=[@"password1" dataUsingEncoding:NSASCIIStringEncoding];
        NSData* replacement=[@"password2" dataUsingEncoding:NSASCIIStringEncoding];
        NSError* error=nil;
        check(![store passwordForSSID:ssid error:&error]&&!error,"missing is not access denied");
        check([store savePassword:password forSSID:ssid error:&error]&&!error,"add and verify");
        check(adds==1&&updates==1,"update-then-add sequence");
        NSMutableData* read=[store passwordForSSID:ssid error:&error];
        check([read isEqualToData:password],"read persisted secret");
        RTWWipePassword(read);check(read.length==0,"wipe clears caller buffer");
        read=[store passwordForSSID:ssid allowInteraction:NO error:&error];
        check([read isEqualToData:password]&&lastReadNoninteractive,"background read forbids authentication UI");
        RTWWipePassword(read);
        read=[store passwordForSSID:ssid error:&error];
        check([read isEqualToData:password]&&!lastReadNoninteractive,"explicit user action permits Keychain approval");
        RTWWipePassword(read);
        check([store savePassword:replacement forSSID:ssid error:&error],"update existing");
        check(adds==1&&deletes==0,"no delete/recreate on update");
        RTWCredentialStore* reopened=[[RTWCredentialStore alloc]initWithService:@"test.TahoeUSBWiFi" api:api];
        read=[reopened passwordForSSID:ssid error:&error];
        check([read isEqualToData:replacement],"new store instance reads persisted secret");RTWWipePassword(read);
        forcedUpdate=errSecInteractionNotAllowed;
        unsigned oldAdds=adds;
        check(![store savePassword:password forSSID:ssid error:&error],"locked keychain rejects save");
        check(error.code==errSecInteractionNotAllowed&&adds==oldAdds&&deletes==0,"locked error, no fallback");
        forcedUpdate=errSecSuccess;forcedRead=errSecAuthFailed;
        check(![store passwordForSSID:ssid error:&error]&&error.code==errSecAuthFailed,"denied not treated as missing");
        forcedRead=errSecUserCanceled;
        check(![store passwordForSSID:ssid error:&error]&&error.code==errSecUserCanceled,"cancel preserved");
        forcedRead=errSecSuccess;
        const uint8_t binaryBytes[]={0xff,0,'x'};
        NSData* binary=[NSData dataWithBytes:binaryBytes length:3];
        creationRace=YES;
        check([store savePassword:password forSSID:binary error:&error],"binary SSID and duplicate race");
        NSArray* saved=[store savedSSIDsAndReturnError:&error];
        check(saved.count==2&&[saved containsObject:ssid]&&[saved containsObject:binary],"list raw SSIDs");
        items[@"unrelated"]=[@"secret" dataUsingEncoding:NSASCIIStringEncoding];
        check([store savedSSIDsAndReturnError:&error].count==2,"ignore unknown account format");
        unsigned oldUpdates=updates;
        check(![store savePassword:[NSData data] forSSID:ssid error:&error],"invalid password rejected");
        check(updates==oldUpdates,"invalid password never reaches API");
        check(![store savePassword:password forSSID:[NSData data] error:&error],"empty SSID rejected");
        forcedDelete=errSecAuthFailed;
        check(![store deletePasswordForSSID:ssid error:&error],"deletion error visible");
        check([store savedSSIDsAndReturnError:&error].count==2,"failed deletion preserves record");
        forcedDelete=errSecSuccess;
        check([store deletePasswordForSSID:ssid error:&error],"delete one SSID");
        check([store deletePasswordForSSID:ssid error:&error],"delete idempotent");
        check([store savedSSIDsAndReturnError:&error].count==1,"delete scoped to one network");
        items[@"wpa2:ff0078"]=[@"bad" dataUsingEncoding:NSASCIIStringEncoding];
        check(![store passwordForSSID:binary error:&error]&&error.code==errSecDecode,"corrupt stored password rejected");
        [items removeAllObjects];forcedAdd=errSecAuthFailed;
        check(![store savePassword:password forSSID:ssid error:&error],"add denial propagated");
        forcedAdd=errSecSuccess;forcedRead=errSecInteractionNotAllowed;
        check(![store savePassword:password forSSID:ssid error:&error],"unreadable save is not reported successful");
        check(error.code==errSecInteractionNotAllowed,"readback error retained");
        forcedRead=errSecSuccess;
        items=nil;
        std::printf("credential-store backend: %u checks passed (mock, no user Keychain changes)\n",checks);
    }
}
