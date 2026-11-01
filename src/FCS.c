/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/FCS.h"
uint32_t rtw_fcs32(const uint8_t* bytes, size_t length) {
    uint32_t crc=0xFFFFFFFFU;
    size_t i;
    if(!bytes && length) return 0;
    for(i=0;i<length;++i) {
        unsigned bit;
        crc^=(uint32_t)bytes[i];
        for(bit=0;bit<8U;++bit)
            crc=(crc>>1U) ^ ((crc&1U) ? 0xEDB88320U : 0U);
    }
    return crc ^ 0xFFFFFFFFU;
}
int rtw_fcs_valid(const uint8_t* bytes, size_t length) {
    uint32_t actual;
    const uint8_t* end;
    if(!bytes || length<4U) return 0;
    end=bytes+length-4U;
    actual=(uint32_t)end[0]|((uint32_t)end[1]<<8U)|
          ((uint32_t)end[2]<<16U)|((uint32_t)end[3]<<24U);
    return actual==rtw_fcs32(bytes,length-4U);
}
