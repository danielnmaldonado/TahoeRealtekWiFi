#ifndef RTW_POWER_SEQUENCE_H
#define RTW_POWER_SEQUENCE_H
#include "rtw/USBRegisterIO.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* B-cut/TSMC/USB RTL8812AU vendor flow. Not appropriate for other cuts/RFEs.
   All steps execute synchronously with bounded polls; no kernel allocations. */
typedef enum {
    RTW_POWER_OK=0, RTW_POWER_BAD_ARGUMENT, RTW_POWER_UNSUPPORTED,
    RTW_POWER_USB_ERROR, RTW_POWER_POLL_TIMEOUT, RTW_POWER_DELAY_FAILED
} rtw_power_error;
typedef enum { RTW_PWR_WRITE8=1, RTW_PWR_POLL8=2,
               RTW_PWR_DELAY_US=3, RTW_PWR_END=4 } rtw_power_opcode;
typedef struct {
    uint16_t address_or_delay;
    uint8_t opcode;
    uint8_t mask;
    uint8_t value;
} rtw_power_step;
/* Delay is required (including between polls) to avoid unbounded USB I/O.
   Callback returns 1 on success; it is responsible for sleep without spinning. */
typedef int (*rtw_wait_us_fn)(void* context, uint32_t microseconds);
typedef struct { void* context; rtw_wait_us_fn wait_us; } rtw_waiter;

rtw_power_error rtw_power_execute(const rtw_usb_rw_transport* transport,
                                  const rtw_power_step* steps, size_t count,
                                  uint32_t max_polls, const rtw_waiter* waiter,
                                  size_t* executed);
/* No action when either chip cut or foundry/interface does not match.
   The USB VID/PID must be checked by the outer transport before using this. */
rtw_power_error rtw_power_on_8812au_bcut_tsmc_usb(
    const rtw_usb_rw_transport* transport, const rtw_waiter* waiter,
    uint32_t sys_cfg, size_t* executed);
const char* rtw_power_error_string(rtw_power_error err);
#ifdef __cplusplus
}
#endif
#endif
