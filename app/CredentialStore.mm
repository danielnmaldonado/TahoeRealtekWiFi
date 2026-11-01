// SPDX-License-Identifier: GPL-2.0-only
#import "CredentialStore.h"
#include "Model.hpp"
#include "CredentialPolicy.hpp"
#import "EnglishErrors.h"

static NSString* const RTWPasswordService=@"org.research.TahoeUSBWiFi.WPA2";

void RTWWipePassword(NSMutableData* password) {
    if(password.length)rtwui::wipe(password.mutableBytes,password.length);
    [password setLength:0];
}

static BOOL reportKeychainError(OSStatus status,NSError** error) {
    if(error) {
        NSString* detail=RTWEnglishError([NSError errorWithDomain:NSOSStatusErrorDomain
            code:status userInfo:nil]);
        *error=[NSError errorWithDomain:NSOSStatusErrorDomain code:status
            userInfo:@{NSLocalizedDescriptionKey:
                [NSString stringWithFormat:@"macOS Keychain: %@ (code %d).",
                    detail?:@"The operation did not complete.",(int)status]}];
    }
    return NO;
}

static NSString* accountForSSID(NSData* ssid) {
    const auto account=rtwui::credentialAccount(static_cast<const uint8_t*>(ssid.bytes),ssid.length);
    return account.empty()?nil:[NSString stringWithUTF8String:account.c_str()];
}
static NSData* ssidForAccount(id account) {
    if(![account isKindOfClass:NSString.class])return nil;
    NSData* encoded=[account dataUsingEncoding:NSASCIIStringEncoding allowLossyConversion:NO];
    if(!encoded.length)return nil;
    const auto ssid=rtwui::credentialSSID(std::string(static_cast<const char*>(encoded.bytes),encoded.length));
    return ssid.empty()?nil:[NSData dataWithBytes:ssid.data() length:ssid.size()];
}

@implementation RTWCredentialStore {
    NSString* _service;
    RTWKeychainAPI _api;
}
- (instancetype)init {
    return [self initWithService:RTWPasswordService
        api:RTWKeychainAPI{SecItemCopyMatching,SecItemAdd,SecItemUpdate,SecItemDelete}];
}
- (instancetype)initWithService:(NSString*)service api:(RTWKeychainAPI)api {
    self=[super init];
    if(self) {
        if(!service.length||!api.copyMatching||!api.add||!api.update||!api.remove)return nil;
        _service=[service copy];_api=api;
    }
    return self;
}
- (NSMutableDictionary*)baseQuery {
    // The macOS file-based user keychain supports locally/ad-hoc signed apps
    // without an application-identifier or a shared access-group entitlement.
    // Do not weaken its default ACL and do not request iCloud synchronization.
    return [@{(__bridge id)kSecClass:(__bridge id)kSecClassGenericPassword,
              (__bridge id)kSecAttrService:_service} mutableCopy];
}
- (NSMutableDictionary*)queryForSSID:(NSData*)ssid error:(NSError**)error {
    if(error)*error=nil;
    NSString* account=accountForSSID(ssid);
    if(!account){reportKeychainError(errSecParam,error);return nil;}
    NSMutableDictionary* query=[self baseQuery];
    query[(__bridge id)kSecAttrAccount]=account;
    return query;
}
- (NSMutableData*)passwordForSSID:(NSData*)ssid error:(NSError**)error {
    return [self passwordForSSID:ssid allowInteraction:YES error:error];
}
- (NSMutableData*)passwordForSSID:(NSData*)ssid allowInteraction:(BOOL)allowInteraction
                           error:(NSError**)error {
    NSMutableDictionary* query=[self queryForSSID:ssid error:error];
    if(!query)return nil;
    // Keep the same file-based keychain as 0.2.3 (including ad-hoc builds).
    // The legacy no-UI flag is scoped to this query, never process-global.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    if(!allowInteraction)query[(__bridge id)kSecUseAuthenticationUI]=(__bridge id)kSecUseAuthenticationUIFail;
#pragma clang diagnostic pop
    query[(__bridge id)kSecReturnData]=@YES;
    query[(__bridge id)kSecMatchLimit]=(__bridge id)kSecMatchLimitOne;
    CFTypeRef result=nullptr;
    const OSStatus status=_api.copyMatching((__bridge CFDictionaryRef)query,&result);
    id value=CFBridgingRelease(result);
    if(status==errSecItemNotFound)return nil;
    if(status!=errSecSuccess){reportKeychainError(status,error);return nil;}
    if(![value isKindOfClass:NSData.class]||
       !rtwui::validPassword(static_cast<const char*>([value bytes]),[value length])) {
        reportKeychainError(errSecDecode,error);return nil;
    }
    return [value mutableCopy];
}
- (BOOL)savePassword:(NSData*)password forSSID:(NSData*)ssid error:(NSError**)error {
    NSMutableDictionary* query=[self queryForSSID:ssid error:error];
    if(!query)return NO;
    if(!rtwui::validPassword(static_cast<const char*>(password.bytes),password.length))
        return reportKeychainError(errSecParam,error);
    NSDictionary* changes=@{(__bridge id)kSecValueData:password};
    OSStatus status=rtwui::upsertCredential<OSStatus>(errSecItemNotFound,errSecDuplicateItem,
        [&] {return _api.update((__bridge CFDictionaryRef)query,(__bridge CFDictionaryRef)changes);},
        [&] {
            NSMutableDictionary* item=[query mutableCopy];
            [item addEntriesFromDictionary:changes];
            item[(__bridge id)kSecAttrLabel]=@"Tahoe Wi-Fi USB · WPA2";
            return _api.add((__bridge CFDictionaryRef)item,nullptr);
        });
    if(status!=errSecSuccess)return reportKeychainError(status,error);
    // Never report "saved" unless the stored value can actually be read back.
    NSMutableData* verified=[self passwordForSSID:ssid error:error];
    if(!verified){if(error&&!*error)reportKeychainError(errSecItemNotFound,error);return NO;}
    const BOOL matches=[verified isEqualToData:password];
    RTWWipePassword(verified);
    return matches?YES:reportKeychainError(errSecDecode,error);
}
- (BOOL)deletePasswordForSSID:(NSData*)ssid error:(NSError**)error {
    NSMutableDictionary* query=[self queryForSSID:ssid error:error];
    if(!query)return NO;
    const OSStatus status=_api.remove((__bridge CFDictionaryRef)query);
    return status==errSecSuccess||status==errSecItemNotFound?YES:reportKeychainError(status,error);
}
- (NSArray<NSData*>*)savedSSIDsAndReturnError:(NSError**)error {
    if(error)*error=nil;
    NSMutableDictionary* query=[self baseQuery];
    query[(__bridge id)kSecReturnAttributes]=@YES;
    query[(__bridge id)kSecMatchLimit]=(__bridge id)kSecMatchLimitAll;
    CFTypeRef result=nullptr;
    const OSStatus status=_api.copyMatching((__bridge CFDictionaryRef)query,&result);
    id value=CFBridgingRelease(result);
    if(status==errSecItemNotFound)return @[];
    if(status!=errSecSuccess){reportKeychainError(status,error);return nil;}
    NSArray* items=[value isKindOfClass:NSArray.class]?value:
        [value isKindOfClass:NSDictionary.class]?@[value]:nil;
    if(!items){reportKeychainError(errSecDecode,error);return nil;}
    NSMutableArray<NSData*>* networks=[NSMutableArray array];
    NSMutableSet<NSData*>* seen=[NSMutableSet set];
    for(id item in items) {
        if(![item isKindOfClass:NSDictionary.class])continue;
        NSData* ssid=ssidForAccount(item[(__bridge id)kSecAttrAccount]);
        if(ssid&&![seen containsObject:ssid]) {
            [seen addObject:ssid];[networks addObject:ssid];
        }
    }
    return [networks copy];
}
@end
