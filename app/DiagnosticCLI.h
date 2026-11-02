// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#import <Foundation/Foundation.h>
#include "DiagnosticArguments.hpp"
@class RTWDriverConnection;
typedef NSDictionary* (^RTWDiagnosticSnapshot)(uint64_t registryID);
typedef void (^RTWDiagnosticSink)(NSDictionary* record);
// Returns -1 for the ordinary menu app, 64 for invalid CLI arguments, or the
// diagnostic exit status. XPC exchanges run off-main while CFRunLoop services it.
int RTWHandleAdapterDiagnosticCLI(int argc,const char* const* argv);
// The same production sequence with injected driver/snapshot/sink boundaries
// permits hardware-free tests. It must be invoked on a worker queue.
int RTWRunAdapterDiagnosis(const rtwui::DiagnosticArguments& options,RTWDriverConnection* driver,
    RTWDiagnosticSnapshot snapshot,RTWDiagnosticSink sink);
