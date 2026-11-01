// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#import <Foundation/Foundation.h>
#import <Security/Security.h>
#include <sys/stat.h>

static inline NSString* RTWRequirementForURL(NSURL* url) {
    if(!url)return nil;
    SecStaticCodeRef code=nullptr;
    SecRequirementRef requirement=nullptr;
    CFStringRef expression=nullptr;
    OSStatus status=SecStaticCodeCreateWithPath((__bridge CFURLRef)url,
        kSecCSDefaultFlags,&code);
    if(status==errSecSuccess)
        status=SecStaticCodeCheckValidity(code,kSecCSStrictValidate,nullptr);
    if(status==errSecSuccess)
        status=SecCodeCopyDesignatedRequirement(code,kSecCSDefaultFlags,&requirement);
    if(status==errSecSuccess)
        status=SecRequirementCopyString(requirement,kSecCSDefaultFlags,&expression);
    NSString* result=CFBridgingRelease(expression);
    if(requirement)CFRelease(requirement);
    if(code)CFRelease(code);
    return status==errSecSuccess?result:nil;
}

// A persistent privileged service must not derive its client identity from
// an app executable which an unprivileged process can modify in place.
static inline BOOL RTWProtectedBundle(NSString* path) {
    if(![path isEqualToString:@"/Applications/Tahoe Wi-Fi USB.app"])return NO;
    NSFileManager* files=NSFileManager.defaultManager;
    NSArray<NSString*>* components=@[path,
        [path stringByAppendingPathComponent:@"Contents"],
        [path stringByAppendingPathComponent:@"Contents/MacOS"],
        [path stringByAppendingPathComponent:@"Contents/MacOS/TahoeUSBWiFi"],
        [path stringByAppendingPathComponent:@"Contents/Helpers"],
        [path stringByAppendingPathComponent:@"Contents/Helpers/TahoeWiFiHelper"],
        [path stringByAppendingPathComponent:@"Contents/Info.plist"],
        [path stringByAppendingPathComponent:@"Contents/_CodeSignature"],
        [path stringByAppendingPathComponent:@"Contents/_CodeSignature/CodeResources"]];
    for(NSString* component in components) {
        struct stat info{};
        if(lstat(component.fileSystemRepresentation,&info) ||
           S_ISLNK(info.st_mode) || info.st_uid!=0 || (info.st_mode&0022) ||
           (!S_ISDIR(info.st_mode)&&!S_ISREG(info.st_mode)))return NO;
    }
    return [files isExecutableFileAtPath:components[3]] &&
           [files isExecutableFileAtPath:components[5]];
}
