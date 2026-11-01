#ifndef RTW_NET80211_GLOBALS_HPP
#define RTW_NET80211_GLOBALS_HPP

#include "../vendor/itl80211/RTWNet80211Prefix.hpp"
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOCommandGate.h>

/* Compatibility operations and all controller gates share this WorkLoop.
 * These globals are owned by the runtime, never by an individual controller. */
extern IOWorkLoop *RTWNet80211WorkLoop;
extern IOCommandGate *RTWNet80211CommandGate;

/* Each successful acquire owns one runtime lease and returns one retained
 * WorkLoop reference. The caller releases that reference normally. Its own
 * controller-owned command gate must be installed on the returned WorkLoop.
 * Release the lease only after net80211 detach, USB drain and removal of that
 * controller's event sources. The final release asynchronously drains systq
 * before clearing globals. Acquire returns Busy during that final drain. */
IOReturn RTWNet80211Acquire(IOWorkLoop **retainedLoop);
void RTWNet80211Release();

#endif
