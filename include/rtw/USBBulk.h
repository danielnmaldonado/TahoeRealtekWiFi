#ifndef RTW_USB_BULK_H
#define RTW_USB_BULK_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Synchronous, caller-owned USB bulk transport HAL. Pure C for host/kernel
   adapter implementations. NOT invoked from the research kext. In particular,
   'radio_ready' is only a caller-provided precondition: it is NOT proof of
   firmware readiness or association. No Ethernet data TX API exists here. */
typedef enum {
    RTW_BULK_OK=0,
    RTW_BULK_BAD_ARGUMENT,
    RTW_BULK_RADIO_NOT_READY,
    RTW_BULK_IO_ERROR,
    RTW_BULK_SHORT_TRANSFER,
    RTW_BULK_NO_SPACE,
    RTW_BULK_BAD_MPDU
} rtw_bulk_error;

typedef int (*rtw_bulk_read_callback)(
    void* context, uint8_t endpoint, uint8_t* buffer,
    size_t capacity, size_t* received);
typedef int (*rtw_bulk_write_callback)(
    void* context, uint8_t endpoint, const uint8_t* buffer,
    size_t length, size_t* transmitted);

typedef struct {
    void* context;
    rtw_bulk_read_callback read;
    rtw_bulk_write_callback write;
    uint8_t endpoint_in;
    uint8_t endpoint_out;
    uint16_t max_packet_size;
    uint8_t radio_ready;
} rtw_bulk_transport;

/* The callback MUST respect capacity and return received<=capacity.
   The caller owns the RX buffer until all descriptors are consumed. */
rtw_bulk_error rtw_bulk_receive(
    const rtw_bulk_transport* transport, uint8_t* buffer,
    size_t capacity, size_t* received);

/* TX supports management-type MPDUs only; never host Ethernet data. Caller
   supplies staging buffer and length; no heap allocations occur. The queue
   and rate have to match the tested RF/channel calibration on actual hardware. */
rtw_bulk_error rtw_bulk_send_management(
    const rtw_bulk_transport* transport,
    const uint8_t* mpdu, size_t mpdu_length,
    uint8_t rate_index, uint8_t* staging, size_t staging_capacity,
    size_t* sent);
const char* rtw_bulk_error_string(rtw_bulk_error error);
#ifdef __cplusplus
}
#endif
#endif
