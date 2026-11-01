// SPDX-License-Identifier: GPL-2.0-only
#include "rtw/DiagnosticPublishPolicy.hpp"
#include <assert.h>
#include <stdint.h>
int main() {
    using rtw::diagnosticCounterNeedsWrite;
    assert(diagnosticCounterNeedsWrite(false,0,0,0));
    assert(diagnosticCounterNeedsWrite(true,32,7,7));
    assert(!diagnosticCounterNeedsWrite(true,64,0,0));
    assert(!diagnosticCounterNeedsWrite(true,64,UINT64_MAX,UINT64_MAX));
    assert(diagnosticCounterNeedsWrite(true,64,UINT64_MAX,0));
    assert(diagnosticCounterNeedsWrite(true,64,UINT64_MAX-1,UINT64_MAX));
    // A stable counter remains unchanged across arbitrarily many status polls.
    uint64_t writes=0, cached=0;bool present=false;
    for(unsigned i=0;i<100000;++i) {
        const uint64_t next=i/10000;
        if(diagnosticCounterNeedsWrite(present,64,cached,next)) {
            cached=next;present=true;++writes;
        }
    }
    assert(writes==10 && cached==9);
}
