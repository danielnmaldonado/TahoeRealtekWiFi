/* SPDX-License-Identifier: GPL-2.0-only
 * Differential goldens use the unmodified pinned Realtek field macros and
 * checksum implementation; no register callbacks or hardware are involved. */
#include "rtw/RTW8814BHAL.h"
#include "../src/rtl8814b/halmac/RTW8814BPlatform.h"
#include "../src/rtl8814b/halmac/halmac_api.h"
#include "../src/rtl8814b/halmac/halmac_88xx_v1/halmac_common_88xx_v1.h"
#include <assert.h>
#include <string.h>
void rtwb14_test_vendor_goldens(void){
 struct halmac_adapter a;unsigned kind,size;memset(&a,0,sizeof(a));a.tx_desc_transfer=1;a.tx_desc_checksum=1;
 for(kind=0;kind<2;kind++)for(size=1;size<=31744;size+=127){
  u32 storage[64]={0};u8*out=(u8*)storage;u8 actual[48];u8*ie0=out+16,*ie1=out+24,*ie2=out+32,*ie3=out+40;
  SET_TX_DESC_TXPKTSIZE(out,size);SET_TX_DESC_OFFSET(out,48);SET_TX_DESC_PKT_OFFSET_V1(out,4);
  SET_TX_DESC_QSEL(out,kind?HALMAC_QSEL_CMD:HALMAC_QSEL_BCN);SET_TX_DESC_CHANNEL_DMA(out,kind?HALMAC_DMA_CH_H2C:HALMAC_DMA_CH_BCN);
  IE0_SET_TX_DESC_IE_NUM(ie0,0);IE1_SET_TX_DESC_IE_NUM(ie1,1);IE2_SET_TX_DESC_IE_NUM(ie2,2);IE3_SET_TX_DESC_IE_NUM(ie3,3);IE3_SET_TX_DESC_IE_END(ie3,1U);
  assert(fill_txdesc_check_sum_88xx_v1(&a,out)==HALMAC_RET_SUCCESS);
  assert(!rtwb14_control_descriptor(kind,size,actual,sizeof(actual)));assert(!memcmp(actual,out,48));
 }
 for(size=1;size<16384;size+=113){
  u32 storage[64]={0};u8*b=(u8*)storage;rtwb14_rx_info got;
  storage[0]=size|(1U<<14)|(1U<<15)|(1U<<26)|(1U<<27);storage[2]=1U<<28;storage[3]=75;
  /* Parser requires a complete packet; allocate an aligned model for it. */
  u8 packet[24+16384];memcpy(packet,b,24);memset(packet+24,0,size);
  assert(!rtwb14_rx_inspect(packet,24+size,&got));
  assert(got.payload_length==GET_RX_DESC_PKT_LEN(b));assert(got.crc_error==GET_RX_DESC_CRC32(b));assert(got.icv_error==GET_RX_DESC_ICV_ERR(b));assert(got.phy_status==GET_RX_DESC_PHYST(b));assert(got.software_decrypted==GET_RX_DESC_SWDEC(b));assert(got.c2h==GET_RX_DESC_C2H(b));assert(got.rate==GET_RX_DESC_RX_RATE(b));
 }
}
