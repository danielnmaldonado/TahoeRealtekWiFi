// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#import <Foundation/Foundation.h>
#include <cstdint>

// Read-only, on-demand registry snapshot. Call off the AppKit main thread.
FOUNDATION_EXPORT NSString* RTWDriverDiagnosticsReport(uint64_t registryID);
