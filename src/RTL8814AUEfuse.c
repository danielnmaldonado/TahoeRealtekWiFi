/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
 * Copyright(c) 2018-2019, 2025 Realtek Corporation.
 * Adapted from Linux rtw88 efuse.c and rtw8814a.c. The portable decoder adds
 * strict bounds before every header/pair access; hardware errors restore OTP
 * access registers and never become erased OTP bytes. */
#include "RTL8814AURegs.h"

int rtw_8814au_decode_efuse(const uint8_t *physical,size_t physical_size,
    uint8_t *logical,size_t logical_size)
{
    size_t p=0;
    if(!physical||!logical||!physical_size||!logical_size)return EINVAL;
    memset(logical,255,logical_size);
    while(p<physical_size) {
        uint8_t h=physical[p++],mask; unsigned block;
        if(h==255)return 0;
        if((h&31)==15) {
            if(p==physical_size)return EINVAL;
            uint8_t e=physical[p++]; if(e==255)return 0;
            block=((e&0xf0)>>1)|((h>>5)&7); mask=e&15;
        } else {block=h>>4;mask=h&15;}
        for(unsigned word=0;word<4;word++)if(!(mask&(1u<<word))) {
            size_t l=(size_t)block*8+word*2;
            if(physical_size-p<2||l>=logical_size||logical_size-l<2)return EINVAL;
            logical[l]=physical[p++];logical[l+1]=physical[p++];
        }
    }
    /* A fully used map with no terminator is valid if its final pair fits. */
    return 0;
}

int rtw_8814au_parse_board(struct rtw_8814au *c,const uint8_t b[512],enum rtw_8814au_speed speed)
{
    struct rtw_8814au_board board;
    if(!c||!b)return EINVAL;
    c->board_valid=0;
    if(speed<RTW_8814AU_FULL_SPEED||speed>RTW_8814AU_SUPER_SPEED)return EINVAL;
    memset(&board,0,sizeof(board));
    board.sys_cfg=c->board.sys_cfg;board.sys_cfg2=c->board.sys_cfg2;board.hpon=c->board.hpon;
    board.cut=(uint8_t)((board.sys_cfg>>12)&15);
    board.vendor=(uint8_t)((board.sys_cfg>>16)&15); /* raw vendor field, not a foundry label */
    board.mp_chip=(board.sys_cfg&BIT_RTL_ID)?0:1;
    memcpy(board.logical,b,512);memcpy(board.mac,b+0xd8,6);
    unsigned any=0,allff=1;for(unsigned i=0;i<6;i++){any|=board.mac[i];if(board.mac[i]!=255)allff=0;}
    if(!any||allff||(board.mac[0]&1))return EINVAL;
    board.rfe_option=(b[0xca]&128)?1:b[0xca];
    /* These are the only actual RFE power/limit tables in this pinned source. */
    if(board.rfe_option>1)return ENODEV;
    board.crystal_cap=b[0xb9]==255?0:b[0xb9];board.thermal_meter=b[0xba];
    board.channel_plan=b[0xb8]==255?0x7f:b[0xb8];
    board.country_code[0]=b[0xcb];board.country_code[1]=b[0xcc];
    board.rf_board_option=b[0xc1]==255?0:b[0xc1];board.bt_setting=b[0xc3];
    board.share_ant=(board.bt_setting&1)!=0;board.btcoex=(board.rf_board_option&0xe0)==0x20;
    board.regd=board.rf_board_option&7;
    board.tx_bb_swing_setting_2g=b[0xc6]==255?0:b[0xc6];
    board.tx_bb_swing_setting_5g=b[0xc7]==255?0:b[0xc7];
    if(board.rfe_option==1){board.pa_type_5g=1;board.lna_type_5g=8;}
    board.ext_pa_2g=(board.pa_type_2g&16)!=0;board.ext_pa_5g=(board.pa_type_5g&1)!=0;
    board.ext_lna_2g=(board.lna_type_2g&8)!=0;board.ext_lna_5g=(board.lna_type_5g&8)!=0;
    board.physical_rf_paths=4;
    board.tx_streams=board.rx_streams=((b[0xc9]==255||b[0xc9]==0xee)&&speed==RTW_8814AU_SUPER_SPEED)?3:2;
    board.antenna_tx=board.antenna_rx=board.tx_streams==3?7:3;
    memcpy(board.txpwr_idx_table,b+0x10,sizeof(board.txpwr_idx_table));
    c->board=board;c->speed=speed;c->board_valid=1;return 0;
}

int rtw_8814au_identify_read_board(struct rtw_8814au *c,enum rtw_8814au_speed speed)
{
    uint8_t physical[1024],logical[512];
    uint32_t access,func,clock,ldo;
    if(!c)return EINVAL;
    c->identified=c->board_valid=0;
    if(c->io_error)return c->io_error;
    c->board.sys_cfg=R32(c,REG_SYS_CFG1);
    c->board.sys_cfg2=R32(c,REG_SYS_CFG2);
    c->board.hpon=R32(c,REG_SYS_PW_CTRL);
    if(c->io_error)return c->io_error;
    if(c->board.sys_cfg==UINT32_MAX||c->board.sys_cfg2==UINT32_MAX||c->board.hpon==UINT32_MAX)return ENODEV;
    c->identified=1;
    access=R8(c,REG_EFUSE_ACCESS);func=R16(c,REG_SYS_FUNC_EN);
    clock=R16(c,REG_SYS_CLKR);ldo=R32(c,REG_LDO_EFUSE_CTRL);
    if(c->io_error)return c->io_error;
    W8(c,REG_EFUSE_ACCESS,EFUSE_ACCESS_ON);
    W16(c,REG_SYS_FUNC_EN,func|BIT_FEN_ELDR);
    W16(c,REG_SYS_CLKR,clock|BIT_LOADER_CLK_EN|BIT_ANA8M);
    W32(c,REG_LDO_EFUSE_CTRL,ldo&~BIT_MASK_EFUSE_BANK_SEL);
    size_t bytes_read=0;
    int read_error=0;
    for(unsigned a=0;a<sizeof(physical)&&!c->io_error;a++) {
        uint32_t v=R32(c,REG_EFUSE_CTRL);
        v=(v&~(BIT_MASK_EF_DATA|BITS_EF_ADDR|BIT_EF_FLAG))|(a<<BIT_SHIFT_EF_ADDR);
        W32(c,REG_EFUSE_CTRL,v);
        read_error=rtw_8814au_poll(c,REG_EFUSE_CTRL,4,BIT_EF_FLAG,BIT_EF_FLAG,10,1000);
        if(read_error)break;
        physical[bytes_read]=(uint8_t)R32(c,REG_EFUSE_CTRL);
        if(c->io_error)break;
        ++bytes_read;
    }
    rtw14_restore(c,REG_LDO_EFUSE_CTRL,4,ldo);
    rtw14_restore(c,REG_SYS_CLKR,2,clock);
    rtw14_restore(c,REG_SYS_FUNC_EN,2,func);
    rtw14_restore(c,REG_EFUSE_ACCESS,1,access);
    if(c->io_error)return c->io_error;
    if(read_error)return rtw14_fail(c,read_error);
    /* Decoding requires a complete successful read. Do not rely solely on an
     * external poll function having latched its error into the controller. */
    if(bytes_read!=sizeof(physical))return rtw14_fail(c,EIO);
    int error=rtw_8814au_decode_efuse(physical,bytes_read,logical,sizeof(logical));
    if(error)return error;
    return rtw_8814au_parse_board(c,logical,speed);
}
