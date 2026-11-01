#ifndef RTW_STATION_PORT_H
#define RTW_STATION_PORT_H

// Conservative 802.11 station controlled-port policy (IEEE 802.1X concept).
// This gate must never substitute for a WPA supplicant or CCMP verification.
// The future HAL invokes authorize() only after real association / key checks.
#include "rtw/FrameWire.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { RTW_PORT_OPEN, RTW_PORT_RSN } rtw_port_security;
typedef enum {
    RTW_PORT_OK = 0,
    RTW_PORT_BAD_ARGUMENT,
    RTW_PORT_NOT_ASSOCIATED,
    RTW_PORT_UNAUTHORIZED,
    RTW_PORT_NEEDS_ENCRYPTION,
    RTW_PORT_FRAME_REJECTED
} rtw_port_error;

typedef struct {
    uint8_t station[6];
    uint8_t bssid[6];
    rtw_port_security security;
    uint8_t configured;
    uint8_t associated;
    uint8_t authorized;
} rtw_station_port;

void rtw_port_reset(rtw_station_port* port);
rtw_port_error rtw_port_configure(rtw_station_port* port,
                                  const uint8_t station[6],
                                  const uint8_t bssid[6],
                                  rtw_port_security security);
rtw_port_error rtw_port_associated(rtw_station_port* port);
// For RSN, 'keys_installed_and_verified' MUST reflect actual MIC/replay
// verification, key installation and associated AP state, not a simulation.
rtw_port_error rtw_port_authorize(rtw_station_port* port,
                                  int keys_installed_and_verified);
int rtw_port_carrier(const rtw_station_port* port);

// Converts host Ethernet frames to 802.11, if policy permits.
// Open-link data or EAPOL during RSN association may be emitted unprotected.
// After RSN authorization, all host TX is REFUSED pending a separate
// fully-implemented hardware/software CCMP TX path (NEEDS_ENCRYPTION).
rtw_port_error rtw_port_tx(rtw_station_port* port,
                          const uint8_t* ethernet, size_t length,
                          uint16_t sequence, uint8_t* output,
                          size_t capacity, size_t* written,
                          rtw_frame_error* frame_error);

// RX is only accepted if headers are valid and controlled-port policy permits.
// 'verified_decrypted' is trusted ONLY after hardware/software MIC+replay
// checks and correct key selection; this function performs none of them.
rtw_port_error rtw_port_rx(const rtw_station_port* port,
                          const uint8_t* mpdu, size_t length,
                          int verified_decrypted,
                          uint8_t* output, size_t capacity,
                          size_t* written, rtw_frame_error* frame_error);

const char* rtw_port_error_string(rtw_port_error status);
#ifdef __cplusplus
}
#endif
#endif
