#ifndef RTW_USB_PACKETS_H
#define RTW_USB_PACKETS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* RTL8812AU Jaguar RX descriptor v1 (24 bytes), aggregated bulk IN units. */
#define RTW_RX_DESC_BYTES 24U
#define RTW_TX_DESC_BYTES 40U
#define RTW_MAX_RX_FRAMES 4096U

typedef enum {
    RTW_PACKETS_OK=0,
    RTW_PACKETS_DONE,
    RTW_PACKETS_BAD_ARGUMENT,
    RTW_PACKETS_TRUNCATED,
    RTW_PACKETS_MALFORMED,
    RTW_PACKETS_TOO_MANY
} rtw_packet_error;

typedef struct {
    const uint8_t* buffer;
    size_t length;
    size_t cursor;
    size_t frames;
} rtw_rx_iterator;

typedef struct {
    const uint8_t* mpdu;
    size_t mpdu_length;
    const uint8_t* phy_info;
    size_t phy_info_length;
    uint8_t rate_index;
    uint8_t bandwidth_code;
    uint8_t crc_error;
    uint8_t icv_error;
    uint8_t phy_status_present;
    uint32_t timestamp;
} rtw_rx_packet;

void rtw_rx_iterator_init(rtw_rx_iterator* it, const uint8_t* transfer,
                          size_t length);
/* Returns DONE at end of aggregated transfer; zero MPDU length accepted only
   when all remaining bytes are zero. Caller MUST reject CRC/ICV-bad frames
   and enforce association/RSN security before delivery to macOS. */
rtw_packet_error rtw_rx_next(rtw_rx_iterator* it, rtw_rx_packet* packet);

/* A decoded prefix is always an exact boundary of fully validated frames.
 * If a later descriptor is broken, callers may process only valid_bytes;
 * never pass an invalid suffix to the network stack. Decode callback uses
 * the real chipset-specific RX parser (including report/FCS validation).
 * Returning OK without exactly one frame and cursor progress is an error. */
typedef rtw_packet_error (*rtw_rx_preflight_decoder)(rtw_rx_iterator*, void*);
typedef struct {
    size_t valid_bytes;
    size_t valid_frames;
    size_t error_offset;
    size_t error_frames;
} rtw_rx_preflight_result;
rtw_packet_error rtw_rx_preflight_transfer(const uint8_t* transfer, size_t length,
    rtw_rx_preflight_decoder decode, void* context, rtw_rx_preflight_result* out);

/* Pure descriptor encoder, NOT an interface for transmitting on the air.
   mgmt frame fixed-rate option is preparation for real radio HAL; firmware
   and RF/queue mapping have not yet been verified on Tahoe. */
rtw_packet_error rtw_tx_make_descriptor(
    size_t mpdu_length, uint8_t rate_index, uint8_t queue_id,
    int multicast, uint8_t descriptor[RTW_TX_DESC_BYTES]);
/* Return total transfer size with mandatory extra pad byte if aligned to MPS.
   Caller is responsible for owning and sending that buffer. */
#define RTW_FLEXIBLE_TX_DESCRIPTOR 1
rtw_packet_error rtw_tx_total_transfer_size_for_descriptor(size_t descriptor_bytes,
    size_t payload_len, uint16_t max_packet_size, size_t *total_size);
rtw_packet_error rtw_tx_total_transfer_size(size_t mpdu_length,
                                             uint16_t max_packet_size,
                                             size_t* total_size);
const char* rtw_packet_error_string(rtw_packet_error error);
#ifdef __cplusplus
}
#endif
#endif
