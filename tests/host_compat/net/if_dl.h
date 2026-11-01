// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#ifdef __APPLE__
#include_next <net/if_dl.h>
#else
// Crypto/BA host tests do not use Darwin sockaddr_dl or link-layer ioctls.
#include <netpacket/packet.h>
#endif
