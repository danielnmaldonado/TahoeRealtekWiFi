/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
 * Copyright(c) 2018-2019, 2025 Realtek Corporation.
 * Adapted from pinned Linux rtw88 mac.c, usb.c, rtw8814a.c and its MAC table.
 * Unlike upstream's later sec-enable step, this MAC path keeps hardware
 * crypto disabled for the native driver's software CCMP implementation. */
#include "RTL8814AURegs.h"
static const uint32_t mac_table[] = {
	0x010, 0x0000007C,
	0x014, 0x000000DB,
	0x016, 0x00000002,
	0x073, 0x00000010,
	0x420, 0x00000080,
	0x421, 0x0000000F,
	0x428, 0x0000000A,
	0x429, 0x00000010,
	0x430, 0x00000000,
	0x431, 0x00000000,
	0x432, 0x00000000,
	0x433, 0x00000001,
	0x434, 0x00000004,
	0x435, 0x00000005,
	0x436, 0x00000007,
	0x437, 0x00000008,
	0x43C, 0x00000004,
	0x43D, 0x00000005,
	0x43E, 0x00000007,
	0x43F, 0x00000008,
	0x440, 0x0000005D,
	0x441, 0x00000001,
	0x442, 0x00000000,
	0x444, 0x00000010,
	0x445, 0x000000F0,
	0x446, 0x00000001,
	0x447, 0x000000FE,
	0x448, 0x00000000,
	0x449, 0x00000000,
	0x44A, 0x00000000,
	0x44B, 0x00000040,
	0x44C, 0x00000010,
	0x44D, 0x000000F0,
	0x44E, 0x0000003F,
	0x44F, 0x00000000,
	0x450, 0x00000000,
	0x451, 0x00000000,
	0x452, 0x00000000,
	0x453, 0x00000040,
	0x45E, 0x00000004,
	0x49C, 0x00000010,
	0x49D, 0x000000F0,
	0x49E, 0x00000000,
	0x49F, 0x00000006,
	0x4A0, 0x000000E0,
	0x4A1, 0x00000003,
	0x4A2, 0x00000000,
	0x4A3, 0x00000040,
	0x4A4, 0x00000015,
	0x4A5, 0x000000F0,
	0x4A6, 0x00000000,
	0x4A7, 0x00000006,
	0x4A8, 0x000000E0,
	0x4A9, 0x00000000,
	0x4AA, 0x00000000,
	0x4AB, 0x00000000,
	0x7DA, 0x00000008,
	0x1448, 0x00000006,
	0x144A, 0x00000006,
	0x144C, 0x00000006,
	0x144E, 0x00000006,
	0x4C8, 0x000000FF,
	0x4C9, 0x00000008,
	0x4CA, 0x0000003C,
	0x4CB, 0x0000003C,
	0x4CC, 0x000000FF,
	0x4CD, 0x000000FF,
	0x4CE, 0x00000001,
	0x4CF, 0x00000008,
	0x500, 0x00000026,
	0x501, 0x000000A2,
	0x502, 0x0000002F,
	0x503, 0x00000000,
	0x504, 0x00000028,
	0x505, 0x000000A3,
	0x506, 0x0000005E,
	0x507, 0x00000000,
	0x508, 0x0000002B,
	0x509, 0x000000A4,
	0x50A, 0x0000005E,
	0x50B, 0x00000000,
	0x50C, 0x0000004F,
	0x50D, 0x000000A4,
	0x50E, 0x00000000,
	0x50F, 0x00000000,
	0x512, 0x0000001C,
	0x514, 0x0000000A,
	0x516, 0x0000000A,
	0x521, 0x0000002F,
	0x525, 0x00000047,
	0x550, 0x00000010,
	0x551, 0x00000010,
	0x559, 0x00000002,
	0x55C, 0x00000064,
	0x55D, 0x000000FF,
	0x577, 0x00000003,
	0x5BE, 0x00000064,
	0x604, 0x00000001,
	0x605, 0x00000030,
	0x607, 0x00000001,
	0x608, 0x0000000E,
	0x609, 0x0000002A,
	0x60A, 0x00000000,
	0x60C, 0x00000018,
	0x60D, 0x00000050,
	0x6A0, 0x000000FF,
	0x6A1, 0x000000FF,
	0x6A2, 0x000000FF,
	0x6A3, 0x000000FF,
	0x6A4, 0x000000FF,
	0x6A5, 0x000000FF,
	0x6DE, 0x00000084,
	0x620, 0x000000FF,
	0x621, 0x000000FF,
	0x622, 0x000000FF,
	0x623, 0x000000FF,
	0x624, 0x000000FF,
	0x625, 0x000000FF,
	0x626, 0x000000FF,
	0x627, 0x000000FF,
	0x638, 0x00000064,
	0x63C, 0x0000000A,
	0x63D, 0x0000000A,
	0x63E, 0x0000000E,
	0x63F, 0x0000000E,
	0x640, 0x00000040,
	0x642, 0x00000040,
	0x643, 0x00000000,
	0x652, 0x000000C8,
	0x66E, 0x00000005,
	0x700, 0x00000021,
	0x701, 0x00000043,
	0x702, 0x00000065,
	0x703, 0x00000087,
	0x708, 0x00000021,
	0x709, 0x00000043,
	0x70A, 0x00000065,
	0x70B, 0x00000087,
	0x718, 0x00000040,
	0x7D5, 0x000000BC,
	0x7D8, 0x00000028,
	0x7D9, 0x00000000,
	0x7DA, 0x0000000B,
};

static void fifo_layout(struct rtw_8814au *c)
{
    struct rtw_8814au_fifo *f=&c->fifo;
    f->rsvd_drv_pg_num=8;f->txff_pg_num=2048-10;f->rsvd_pg_num=8+24+8+8+0+4;
    f->acq_pg_num=f->rsvd_boundary=f->txff_pg_num-f->rsvd_pg_num;
    uint16_t at=f->txff_pg_num;
    at-=4;f->rsvd_fw_txbuf_addr=at;f->rsvd_cpu_instr_addr=at;
    at-=8;f->rsvd_h2cq_addr=at;at-=8;f->rsvd_h2c_sta_info_addr=at;
    at-=24;f->rsvd_h2c_info_addr=at;at-=8;f->rsvd_drv_addr=at;
}
static int queues(struct rtw_8814au *c,unsigned endpoints)
{
    /* DMA mapping HIGH=3, NORMAL=2, LOW=1, EXTRA=0. */
    unsigned vo=3,vi=endpoints==2?3:2,be=endpoints==2?2:1,bk=be,mg=3,hi=3;
    uint16_t mapping=(uint16_t)(vo<<4|vi<<6|be<<8|bk<<10|mg<<12|hi<<14);
    W16(c,REG_TXDMA_PQ_MAP,mapping);W8(c,REG_CR,0);W8(c,REG_CR,MAC_TRX_ENABLE);
    W32(c,REG_H2CQ_CSR,BIT_H2CQ_FULL);U8(c,REG_TXDMA_PQ_MAP,BIT_RXDMA_ARBBW_EN,BIT_RXDMA_ARBBW_EN);
    fifo_layout(c);
    W16(c,REG_FIFOPAGE_INFO_1,32);W16(c,REG_FIFOPAGE_INFO_2,32);
    W16(c,REG_FIFOPAGE_INFO_3,32);W16(c,REG_FIFOPAGE_INFO_4,32);
    W16(c,REG_FIFOPAGE_INFO_5,c->fifo.acq_pg_num-128);
    U32(c,REG_RQPN_CTRL_2,BIT_LD_RQPN,BIT_LD_RQPN);
    W16(c,REG_FIFOPAGE_CTRL_2,c->fifo.rsvd_boundary);
    U8(c,REG_FWHW_TXQ_CTRL+2,BIT_EN_WR_FREE_TAIL>>16,BIT_EN_WR_FREE_TAIL>>16);
    W16(c,REG_BCNQ_BDNY_V1,c->fifo.rsvd_boundary);
    W16(c,REG_FIFOPAGE_CTRL_2+2,c->fifo.rsvd_boundary);
    W16(c,REG_BCNQ1_BDNY_V1,c->fifo.rsvd_boundary);
    W32(c,REG_RXFF_BNDY,23552-256-1);
    /* Source uses mask-value (unshifted3); portable update is preshifted. */
    U8(c,REG_AUTO_LLT_V1,BIT_MASK_BLK_DESC_NUM,3u<<4);
    W8(c,REG_AUTO_LLT_V1+3,3);U8(c,REG_TXDMA_OFFSET_CHK+1,BIT(1),BIT(1));
    U8(c,REG_AUTO_LLT_V1,BIT_AUTO_INIT_LLT_V1,BIT_AUTO_INIT_LLT_V1);
    if(rtw_8814au_poll(c,REG_AUTO_LLT_V1,1,BIT_AUTO_INIT_LLT_V1,0,10,1000))return c->io_error;
    W8(c,REG_CR+3,0);
    uint32_t addr=(uint32_t)c->fifo.rsvd_h2cq_addr<<7,size=8u<<7;
    U32(c,REG_H2C_HEAD,0x3ffff,addr);U32(c,REG_H2C_READ_ADDR,0x3ffff,addr);
    U32(c,REG_H2C_TAIL,0x3ffff,addr+size);
    U8(c,REG_H2C_INFO,3,1);U8(c,REG_H2C_INFO,4,4);U8(c,REG_TXDMA_OFFSET_CHK+1,0x80,0x80);
    uint32_t wp=R32(c,REG_H2C_PKT_WRITEADDR)&0x3ffff,rp=R32(c,REG_H2C_PKT_READADDR)&0x3ffff;
    if(c->io_error)return c->io_error;
    if(wp!=rp)return rtw14_fail(c,EINVAL);
    return 0;
}
int rtw_8814au_init_mac(struct rtw_8814au *c,unsigned endpoints,enum rtw_8814au_speed speed)
{
    if(!c)return EINVAL;if(c->io_error)return c->io_error;
    c->mac_initialized=0;
    if(endpoints<2||endpoints>4||speed<RTW_8814AU_FULL_SPEED||speed>RTW_8814AU_SUPER_SPEED)return EINVAL;
    if(!c->powered||!c->fw_running)return ENXIO;
    c->output_endpoints=(uint8_t)endpoints;c->speed=speed;
    if(queues(c,endpoints))return c->io_error;
    W16(c,REG_CR,MAC_TRX_ENABLE|BIT_32K_CAL_TMR_EN);
    for(size_t i=0;i<sizeof(mac_table)/sizeof(mac_table[0]);i+=2)W8(c,(uint16_t)mac_table[i],mac_table[i+1]);
    W8(c,REG_AUTO_LLT_V1+3,3<<1);W32(c,REG_HIMR0,0);W32(c,REG_HIMR1,0);
    U32(c,REG_RRSR,0xfffff,0xfffff);W16(c,REG_RETRY_LIMIT,0x3030);
    W16(c,REG_RXFLTMAP0,0xffff);W16(c,REG_RXFLTMAP1,0x0400);W16(c,REG_RXFLTMAP2,0xffff);
    W8(c,REG_MAX_AGGR_NUM,0x36);W8(c,REG_MAX_AGGR_NUM+1,0x36);
    W16(c,REG_SPEC_SIFS,0x100a);W16(c,REG_MAC_SPEC_SIFS,0x100a);W16(c,REG_SIFS,0x100a);W16(c,REG_SIFS+2,0x100a);
    W32(c,REG_EDCA_BE_PARAM,0x005ea42b);W32(c,REG_EDCA_BK_PARAM,0x0000a44f);
    W32(c,REG_EDCA_VI_PARAM,0x005ea324);W32(c,REG_EDCA_VO_PARAM,0x002fa226);
    U8(c,REG_FWHW_TXQ_CTRL,BIT(7),BIT(7));W8(c,REG_ACKTO,0x80);
    W16(c,REG_BCN_CTRL,BIT_DIS_TSF_UDT|(BIT_DIS_TSF_UDT<<8));
    U32(c,REG_TBTT_PROHIBIT,0xfffff,0x6404);W8(c,REG_DRVERLYINT,5);W8(c,REG_BCNDMATIM,2);
    W16(c,REG_BCNTCFG,0x4413);W8(c,REG_BCN_MAX_ERR,255);
    W32(c,REG_FAST_EDCA_VOVI_SETTING,0x08070807);W32(c,REG_FAST_EDCA_BEBK_SETTING,0x08070807);
    if(speed==RTW_8814AU_SUPER_SPEED){U8(c,REG_USB_MOD,BIT(4)|BIT(3),0);W16(c,0xf002,0);U8(c,REG_SW_AMPDU_BURST_MODE_CTRL,BIT_PRE_TX_CMD,0);}
    W8(c,REG_RX_DRVINFO_SZ,4);U8(c,REG_TRXFF_BNDY+1,15,15);U32(c,REG_RCR,BIT_APP_PHYSTS,BIT_APP_PHYSTS);
    U32(c,REG_WMAC_OPTION_FUNCTION+4,BIT(8)|BIT(9),0);
    uint8_t burst=speed==RTW_8814AU_SUPER_SPEED?0:speed==RTW_8814AU_HIGH_SPEED?1:2;
    W8(c,REG_RXDMA_MODE,BIT_DMA_BURST_CNT|BIT_DMA_MODE|((uint32_t)burst<<4));
    U16(c,REG_TXDMA_OFFSET_CHK,BIT_DROP_DATA_EN,BIT_DROP_DATA_EN);
    U8(c,REG_TXDMA_PQ_MAP,BIT_RXDMA_AGG_EN,BIT_RXDMA_AGG_EN);
    U8(c,REG_RXDMA_AGG_PG_TH+3,BIT(7),0);W16(c,REG_RXDMA_AGG_PG_TH,0x2005);
    if(!c->io_error)c->mac_initialized=1;return c->io_error;
}
