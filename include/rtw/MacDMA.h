#ifndef RTW_MAC_DMA_H
#define RTW_MAC_DMA_H
#include "rtw/USBRegisterIO.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { RTW_MAC_OK=0, RTW_MAC_BAD_ARGUMENT,
               RTW_MAC_USB_ERROR } rtw_mac_error;
typedef struct {
    uint8_t out_endpoints; /* ONLY verified 3-endpoint profile */
    uint8_t drvinfo_words;  /* 4 = 32 bytes */
    uint8_t enable_receive; /* must be 0; enabling RX belongs AFTER RF init */
} rtw_mac_options;
/* Prepares TX/RX FIFO boundaries, queue map and DMA; uses RTL8812AU vendor
   register values. Does not mean the RF is calibrated. No network carrier. */
rtw_mac_error rtw_mac_dma_prepare_8812au(
    const rtw_usb_rw_transport* transport,
    const rtw_mac_options* options,size_t* operations);
const char* rtw_mac_error_string(rtw_mac_error err);
#ifdef __cplusplus
}
#endif
#endif
