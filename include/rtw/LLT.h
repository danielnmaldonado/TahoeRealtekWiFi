#ifndef RTW_LLT_H
#define RTW_LLT_H
#include "rtw/PowerSequence.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define RTW_LLT_ENTRIES 256U
#define RTW_LLT_BOUNDARY_8812AU 0xF7U
typedef enum { RTW_LLT_OK=0, RTW_LLT_BAD_ARGUMENT,
               RTW_LLT_USB_ERROR, RTW_LLT_TIMEOUT, RTW_LLT_DELAY_FAILED } rtw_llt_error;
/* Programs 256 LLT entries. STOP immediately on any fault; never reports
   success for a partial chain. Must be called only after successful power-on. */
rtw_llt_error rtw_llt_initialize_8812au(const rtw_usb_rw_transport* transport,
                                          const rtw_waiter* waiter,
                                          uint32_t polls_per_entry,
                                          size_t* entries_done);
const char* rtw_llt_error_string(rtw_llt_error err);
#ifdef __cplusplus
}
#endif
#endif
