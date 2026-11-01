// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#import <Foundation/Foundation.h>
#import <Security/Security.h>

static inline NSString* RTWEnglishError(NSError* error) {
    if(!error)return nil;
    if([error.domain hasPrefix:@"org.research.TahoeUSBWiFi"]) {
        NSString* message=error.userInfo[NSLocalizedDescriptionKey];
        if([message isKindOfClass:NSString.class]&&message.length)return message;
    }
    if([error.domain isEqualToString:NSOSStatusErrorDomain]) {
        NSString* detail=nil;
        switch(error.code) {
        case errSecInteractionNotAllowed:detail=@"Keychain is locked or requires user interaction.";break;
        case errSecAuthFailed:detail=@"Keychain access was denied.";break;
        case errSecUserCanceled:detail=@"Keychain access was canceled.";break;
        case errSecItemNotFound:detail=@"The saved password was not found.";break;
        case errSecDecode:detail=@"The saved password could not be decoded.";break;
        case errSecParam:detail=@"Invalid Keychain parameters.";break;
        default:break;
        }
        if(detail)return [NSString stringWithFormat:@"%@ (code %ld)",detail,(long)error.code];
    }
    return [NSString stringWithFormat:
        @"macOS could not complete the operation (%@, code %ld). Check system permissions and try again.",
        error.domain,(long)error.code];
}
