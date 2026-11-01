#ifndef RTW_NET80211_PREFIX_HPP
#define RTW_NET80211_PREFIX_HPP

/* Keep the imported stack's compatibility singletons and OS class private to
 * this kext. Every imported kernel translation unit must include this first. */
#define CTimeout RTWNet80211Timeout
#define _fWorkloop RTWNet80211WorkLoop
#define _fCommandGate RTWNet80211CommandGate

#include <libkern/libkern.h>
#include <IOKit/IOTypes.h>
#include <machine/types.h>

#ifdef __PRIVATE_SPI__
typedef u_int32_t ifnet_ctl_cmd_t;
#endif

#endif
