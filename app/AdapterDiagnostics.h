// SPDX-License-Identifier: GPL-2.0-only
#import <Foundation/Foundation.h>
// Read-only IOKit enumeration. Does not open devices, invoke the helper,
// access Keychain, scan networks, or load/unload kernel extensions.
NSString* RTWAdapterSupportReport(void);
NSString* RTWAdapterCatalogReport(void);
