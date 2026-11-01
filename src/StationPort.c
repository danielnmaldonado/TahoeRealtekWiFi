#include "rtw/StationPort.h"
#include <string.h>

static int valid_station_mac(const uint8_t a[6]) {
    unsigned i;
    int nonzero = 0;
    if (a[0] & 1U) return 0;
    for (i=0; i<6; ++i) nonzero |= a[i];
    return nonzero != 0;
}

static int eth_is_eapol(const uint8_t* eth, size_t len) {
    return len >= 14 && eth[12] == 0x88U && eth[13] == 0x8EU;
}

void rtw_port_reset(rtw_station_port* port) {
    if (port) memset(port, 0, sizeof(*port));
}

rtw_port_error rtw_port_configure(rtw_station_port* port,
                                  const uint8_t station[6],
                                  const uint8_t bssid[6],
                                  rtw_port_security security) {
    rtw_station_port next;
    if (!port) return RTW_PORT_BAD_ARGUMENT;
    if (!station || !bssid ||
        (security != RTW_PORT_OPEN && security != RTW_PORT_RSN)) {
        // On errors, do not accidentally retain previous authorized AP.
        rtw_port_reset(port);
        return RTW_PORT_BAD_ARGUMENT;
    }
    if (!valid_station_mac(station) || !valid_station_mac(bssid) ||
        memcmp(station, bssid, 6) == 0) {
        rtw_port_reset(port);
        return RTW_PORT_BAD_ARGUMENT;
    }
    // Copy before replacing port so station/BSSID may alias its prior fields.
    memset(&next, 0, sizeof(next));
    memcpy(next.station, station, 6);
    memcpy(next.bssid, bssid, 6);
    next.security = security;
    next.configured = 1;
    *port = next;
    return RTW_PORT_OK;
}

rtw_port_error rtw_port_associated(rtw_station_port* port) {
    if (!port || !port->configured) return RTW_PORT_BAD_ARGUMENT;
    // Association never automatically authorizes a data port.
    port->associated = 1;
    port->authorized = 0;
    return RTW_PORT_OK;
}

rtw_port_error rtw_port_authorize(rtw_station_port* port,
                                  int keys_installed_and_verified) {
    if (!port || !port->configured || !port->associated)
        return RTW_PORT_NOT_ASSOCIATED;
    if (port->security == RTW_PORT_RSN && !keys_installed_and_verified)
        return RTW_PORT_UNAUTHORIZED;
    port->authorized = 1;
    return RTW_PORT_OK;
}

int rtw_port_carrier(const rtw_station_port* port) {
    return port && port->configured && port->associated && port->authorized;
}

rtw_port_error rtw_port_tx(rtw_station_port* port,
                          const uint8_t* eth, size_t length, uint16_t sequence,
                          uint8_t* output, size_t capacity, size_t* written,
                          rtw_frame_error* frame_error) {
    rtw_frame_error err;
    if (written) *written = 0;
    if (frame_error) *frame_error = RTW_FRAME_OK;
    if (!port || !eth || !output || !written || !frame_error)
        return RTW_PORT_BAD_ARGUMENT;
    if (!port->configured || !port->associated) return RTW_PORT_NOT_ASSOCIATED;
    if (port->security == RTW_PORT_RSN) {
        if (port->authorized) return RTW_PORT_NEEDS_ENCRYPTION;
        if (!eth_is_eapol(eth, length)) return RTW_PORT_UNAUTHORIZED;
    } else if (!port->authorized) {
        return RTW_PORT_UNAUTHORIZED;
    }
    err = rtw_ethernet_to_sta_mpdu(eth, length, port->station,
                                   port->bssid, sequence, output,
                                   capacity, written);
    *frame_error = err;
    return err == RTW_FRAME_OK ? RTW_PORT_OK : RTW_PORT_FRAME_REJECTED;
}

rtw_port_error rtw_port_rx(const rtw_station_port* port,
                          const uint8_t* mpdu, size_t length,
                          int verified_decrypted,
                          uint8_t* output, size_t capacity,
                          size_t* written, rtw_frame_error* frame_error) {
    int protected_frame;
    rtw_frame_error err;
    if (written) *written = 0;
    if (frame_error) *frame_error = RTW_FRAME_OK;
    if (!port || !mpdu || !output || !written || !frame_error)
        return RTW_PORT_BAD_ARGUMENT;
    if (!port->configured || !port->associated) return RTW_PORT_NOT_ASSOCIATED;
    protected_frame = length >= 2 && (mpdu[1] & 0x40U);
    // Even a valid protected MPDU must never be accepted on an open link.
    if (port->security == RTW_PORT_OPEN && protected_frame)
        return RTW_PORT_UNAUTHORIZED;
    // No protected data can be accepted before keys have been verified.
    if (port->security == RTW_PORT_RSN && !port->authorized && protected_frame)
        return RTW_PORT_UNAUTHORIZED;
    err = rtw_sta_mpdu_to_ethernet(mpdu, length, port->station,
                                   port->bssid, verified_decrypted,
                                   output, capacity, written);
    *frame_error = err;
    if (err != RTW_FRAME_OK) return RTW_PORT_FRAME_REJECTED;
    if (port->security == RTW_PORT_RSN) {
        if (!port->authorized || !protected_frame) {
            if (!eth_is_eapol(output, *written)) {
                // Do not leave unauthorized RX plaintext in the caller buffer.
                memset(output, 0, *written);
                *written = 0;
                return RTW_PORT_UNAUTHORIZED;
            }
        }
    } else if (!port->authorized) {
        memset(output, 0, *written);
        *written = 0;
        return RTW_PORT_UNAUTHORIZED;
    }
    return RTW_PORT_OK;
}

const char* rtw_port_error_string(rtw_port_error status) {
    switch (status) {
        case RTW_PORT_OK: return "ok";
        case RTW_PORT_BAD_ARGUMENT: return "invalid argument";
        case RTW_PORT_NOT_ASSOCIATED: return "not associated";
        case RTW_PORT_UNAUTHORIZED: return "unauthorized";
        case RTW_PORT_NEEDS_ENCRYPTION: return "CCMP TX not implemented";
        case RTW_PORT_FRAME_REJECTED: return "malformed or unsupported frame";
        default: return "unknown";
    }
}
