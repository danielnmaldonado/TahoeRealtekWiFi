// SPDX-License-Identifier: GPL-2.0-only
#import "LoginItem.h"
#include "LoginPolicy.hpp"
#import <ServiceManagement/ServiceManagement.h>
#import <CoreServices/CoreServices.h>

static NSError* loginError(NSInteger code,NSString* detail) {
    return [NSError errorWithDomain:@"org.research.TahoeUSBWiFi.LoginItem"
        code:code userInfo:@{NSLocalizedDescriptionKey:detail}];
}

@interface RTWSystemLoginItemBackend : NSObject <RTWLoginItemBackend>
@end

@implementation RTWSystemLoginItemBackend
- (RTWLoginItemState)state {
    switch(SMAppService.mainAppService.status) {
    case SMAppServiceStatusEnabled:return RTWLoginItemStateEnabled;
    case SMAppServiceStatusRequiresApproval:return RTWLoginItemStateRequiresApproval;
    case SMAppServiceStatusNotRegistered:return RTWLoginItemStateNotRegistered;
    case SMAppServiceStatusNotFound:return RTWLoginItemStateNotFound;
    }
    return RTWLoginItemStateNotFound;
}
- (BOOL)prepareBundleAndReturnError:(NSError**)error {
    if(error)*error=nil;
    NSBundle* bundle=NSBundle.mainBundle;
    NSString* path=bundle.bundleURL.URLByResolvingSymlinksInPath.path;
    NSFileManager* files=NSFileManager.defaultManager;
    const BOOL installed=[path isEqualToString:@"/Applications/Tahoe Wi-Fi USB.app"];
    if(!installed||![path.pathExtension isEqualToString:@"app"]||
       ![bundle.bundleIdentifier isEqualToString:@"org.research.TahoeUSBWiFi"]||
       ![files isExecutableFileAtPath:bundle.executableURL.path]||
       ![files isExecutableFileAtPath:[path stringByAppendingPathComponent:
            @"Contents/Helpers/TahoeWiFiHelper"]]) {
        if(error)*error=loginError(1,
            @"Install the complete app bundle using bash ./app/install.command, "
             "quit the copy in Release/Downloads and open /Applications/Tahoe Wi-Fi USB.app. "
             "Registration must not point to a temporary build directory.");
        return NO;
    }
    // Refresh Launch Services with the installed location before asking
    // ServiceManagement to resolve the app (especially after a rebuild/move).
    OSStatus status=LSRegisterURL((__bridge CFURLRef)bundle.bundleURL,true);
    if(status!=noErr) {
        if(error)*error=[NSError errorWithDomain:NSOSStatusErrorDomain code:status
            userInfo:@{NSLocalizedDescriptionKey:
                [NSString stringWithFormat:@"Could not register the installed app with macOS (code %d).",(int)status]}];
        return NO;
    }
    return YES;
}
- (BOOL)registerAndReturnError:(NSError**)error {
    return [SMAppService.mainAppService registerAndReturnError:error];
}
- (BOOL)unregisterAndReturnError:(NSError**)error {
    return [SMAppService.mainAppService unregisterAndReturnError:error];
}
- (void)openSystemSettings {[SMAppService openSystemSettingsLoginItems];}
@end

@implementation RTWLoginItemManager {
    id<RTWLoginItemBackend> _backend;
}
- (instancetype)init {return [self initWithBackend:[RTWSystemLoginItemBackend new]];}
- (instancetype)initWithBackend:(id<RTWLoginItemBackend>)backend {
    self=[super init];if(self)_backend=backend;return self;
}
- (RTWLoginItemState)state {return _backend.state;}
- (BOOL)registerPreparedBundleAndReturnError:(NSError**)error {
    // NotFound is recoverable. The old code refused registration in exactly
    // this state, making the menu permanently unusable for a moved/new bundle.
    NSError* operationError=nil;
    BOOL accepted=[_backend registerAndReturnError:&operationError];
    const auto actual=_backend.state;
    if(rtwui::loginRegistrationConfirmed(actual==RTWLoginItemStateEnabled,
        actual==RTWLoginItemStateRequiresApproval))return YES;
    if(error)*error=operationError?:loginError(2,accepted?
        @"macOS received the request but has not confirmed registration. Open Login Items and check approval.":
        @"macOS denied registration. Check the installation, code signature and Login Items.");
    return NO;
}
- (BOOL)setEnabled:(BOOL)enabled error:(NSError**)error {
    if(error)*error=nil;
    if(enabled) {
        if(![_backend prepareBundleAndReturnError:error])return NO;
        const auto actual=_backend.state;
        if(rtwui::loginRegistrationConfirmed(actual==RTWLoginItemStateEnabled,
        actual==RTWLoginItemStateRequiresApproval))return YES;
        return [self registerPreparedBundleAndReturnError:error];
    }
    const auto actual=_backend.state;
    if(actual==RTWLoginItemStateNotRegistered)return YES;
    // Removal does not require the app to remain in Applications.
    if(![_backend unregisterAndReturnError:error])return NO;
    if(_backend.state==RTWLoginItemStateEnabled) {
        if(error)*error=loginError(3,@"macOS still reports login startup as enabled. Check Login Items.");
        return NO;
    }
    return YES;
}
- (BOOL)repairRegistrationAndReturnError:(NSError**)error {
    if(error)*error=nil;
    if(![_backend prepareBundleAndReturnError:error])return NO;
    const auto actual=_backend.state;
    // A missing permission is not a broken registration. Never circumvent a
    // user's denial by silently switching to a daemon or a second login item.
    if(actual==RTWLoginItemStateRequiresApproval) {
        [_backend openSystemSettings];return YES;
    }
    if(actual!=RTWLoginItemStateNotRegistered) {
        NSError* removalError=nil;
        if(![_backend unregisterAndReturnError:&removalError] &&
           _backend.state!=RTWLoginItemStateNotRegistered &&
           _backend.state!=RTWLoginItemStateNotFound) {
            if(error)*error=removalError;return NO;
        }
    }
    return [self registerPreparedBundleAndReturnError:error];
}
- (void)openSystemSettings {[_backend openSystemSettings];}
@end
