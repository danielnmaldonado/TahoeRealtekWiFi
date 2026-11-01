#include "rtw/USBBulk.h"
#include "rtw/USBPackets.h"
#include <string.h>

static int valid_bulk_in(uint8_t ep) {
    return (ep & 0x80U) && !(ep & 0x70U) && (ep & 0x0FU);
}
static int valid_bulk_out(uint8_t ep) {
    return !(ep & 0xF0U) && (ep & 0x0FU);
}
/* The TX descriptor is built before copying the MPDU. Refuse overlap so the
   descriptor cannot destroy the caller's input frame mid-serialization. */
static int overlaps(const void* a, size_t na, const void* b, size_t nb) {
    uintptr_t aa=(uintptr_t)a, bb=(uintptr_t)b;
    return aa <= bb ? bb-aa < na : aa-bb < nb;
}

rtw_bulk_error rtw_bulk_receive(
    const rtw_bulk_transport* t, uint8_t* buffer,
    size_t capacity, size_t* received) {
    size_t actual=0;
    if (received) *received=0;
    if (!t || !received || !buffer || !capacity || capacity > 65536U ||
        !t->read || !valid_bulk_in(t->endpoint_in))
        return RTW_BULK_BAD_ARGUMENT;
    if (!t->radio_ready) return RTW_BULK_RADIO_NOT_READY;
    if (!t->read(t->context, t->endpoint_in, buffer, capacity, &actual))
        return RTW_BULK_IO_ERROR;
    if (actual > capacity) return RTW_BULK_SHORT_TRANSFER;
    *received=actual;
    return RTW_BULK_OK;
}

rtw_bulk_error rtw_bulk_send_management(
    const rtw_bulk_transport* t,
    const uint8_t* mpdu, size_t length,
    uint8_t rate, uint8_t* staging, size_t capacity, size_t* sent) {
    size_t total=0, written=0;
    if (sent) *sent=0;
    if (!t || !sent || !t->write || !staging || !mpdu ||
        !valid_bulk_out(t->endpoint_out) || length < 24U || length > 4096U)
        return RTW_BULK_BAD_ARGUMENT;
    if (!t->radio_ready) return RTW_BULK_RADIO_NOT_READY;
    /* FC type 0 = management; reserved/protected/auth data are rejected. */
    if ((mpdu[0] & 0x0FU) != 0 || (mpdu[1] & 0x43U))
        return RTW_BULK_BAD_MPDU;
    if (rtw_tx_total_transfer_size(length, t->max_packet_size, &total) !=
        RTW_PACKETS_OK) return RTW_BULK_BAD_ARGUMENT;
    if (capacity < total) return RTW_BULK_NO_SPACE;
    if (overlaps(mpdu, length, staging, total)) return RTW_BULK_BAD_ARGUMENT;
    if (rtw_tx_make_descriptor(length, rate, 0x12U, mpdu[4]&1U,
                               staging) != RTW_PACKETS_OK)
        return RTW_BULK_BAD_ARGUMENT;
    memcpy(staging+RTW_TX_DESC_BYTES,mpdu,length);
    if (total > RTW_TX_DESC_BYTES+length)
        staging[RTW_TX_DESC_BYTES+length] = 0;
    if (!t->write(t->context,t->endpoint_out,staging,total,&written))
        return RTW_BULK_IO_ERROR;
    if (written != total) return RTW_BULK_SHORT_TRANSFER;
    *sent=written;
    return RTW_BULK_OK;
}

const char* rtw_bulk_error_string(rtw_bulk_error err) {
    switch(err) {
        case RTW_BULK_OK:return "ok";
        case RTW_BULK_BAD_ARGUMENT:return "invalid argument or endpoint";
        case RTW_BULK_RADIO_NOT_READY:return "radio not ready";
        case RTW_BULK_IO_ERROR:return "USB bulk I/O failed";
        case RTW_BULK_SHORT_TRANSFER:return "USB bulk length mismatch";
        case RTW_BULK_NO_SPACE:return "staging buffer too small";
        case RTW_BULK_BAD_MPDU:return "not an unprotected management MPDU";
        default:return "unknown";
    }
}
