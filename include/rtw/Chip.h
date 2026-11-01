#ifndef RTW_CHIP_H
#define RTW_CHIP_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Hardware identification from RTL8812AU REG_SYS_CFG, independent of IOKit. */
typedef struct {
    uint32_t sys_cfg;
    uint8_t cut;           /* 0=A, 1=B, ... */
    uint8_t is_test_chip;
    uint8_t is_umc;
    uint8_t is_usb;
} rtw_chip_info;

/* A SYS_CFG read alone is NOT proof of a complete RTL8812AU identity. */
int rtw_chip_decode_syscfg(uint32_t sys_cfg, rtw_chip_info* result);

#ifdef __cplusplus
}
#endif
#endif
