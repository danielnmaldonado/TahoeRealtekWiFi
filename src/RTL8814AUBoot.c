/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
 * Copyright(c) 2018-2019, 2025 Realtek Corporation.
 * Portable adaptation of Linux rtw88 mac.c/fw.c/usb.c and the actual 8814A
 * power tables. USB only; checked errno replaces Linux's unchecked I/O. */
#include "RTL8814AURegs.h"

int rtw14_fail(struct rtw_8814au *c,int e)
{ if(e<0)e=-e;if(c&&!c->io_error)c->io_error=e;return c?c->io_error:e; }
void rtw_8814au_init(struct rtw_8814au *c,const struct rtw_8814au_bus_ops *ops)
{ if(!c)return;memset(c,0,sizeof(*c));if(ops)c->bus=*ops;else c->io_error=EINVAL; }
int rtw_8814au_read(struct rtw_8814au *c,uint16_t a,unsigned w,uint32_t *v)
{
    if(!c||!v)return EINVAL;
    *v=0;if(c->io_error)return c->io_error;
    if((w!=1&&w!=2&&w!=4)||a>65536u-w||!c->bus.read)return rtw14_fail(c,EINVAL);
    int e=c->bus.read(c->bus.cookie,a,w,v);if(e){*v=0;return rtw14_fail(c,e);}return 0;
}
int rtw_8814au_write(struct rtw_8814au *c,uint16_t a,unsigned w,uint32_t v)
{
    if(!c)return EINVAL;if(c->io_error)return c->io_error;
    if((w!=1&&w!=2&&w!=4)||a>65536u-w||!c->bus.write)return rtw14_fail(c,EINVAL);
    int e=c->bus.write(c->bus.cookie,a,w,v);return e?rtw14_fail(c,e):0;
}
int rtw_8814au_update(struct rtw_8814au *c,uint16_t a,unsigned w,uint32_t mask,uint32_t value)
{ uint32_t v;int e=rtw_8814au_read(c,a,w,&v);return e?e:rtw_8814au_write(c,a,w,(v&~mask)|(value&mask)); }
void rtw_8814au_delay(struct rtw_8814au *c,uint32_t u)
{ if(c&&c->bus.delay_us)c->bus.delay_us(c->bus.cookie,u); }
int rtw_8814au_poll(struct rtw_8814au *c,uint16_t a,unsigned w,uint32_t mask,uint32_t expected,uint32_t us,unsigned attempts)
{
    if(!c||!attempts)return EINVAL;
    for(unsigned i=0;i<attempts;i++){uint32_t v;int e=rtw_8814au_read(c,a,w,&v);if(e)return e;
        if((v&mask)==(expected&mask))return 0;
        if(i+1<attempts)rtw_8814au_delay(c,us);
    }return rtw14_fail(c,ETIMEDOUT);
}
void rtw14_restore(struct rtw_8814au *c,uint16_t a,unsigned w,uint32_t v)
{ if(!c||!c->bus.write)return;int e=c->bus.write(c->bus.cookie,a,w,v);if(e)rtw14_fail(c,e); }
struct power_cmd {uint16_t offset;uint8_t cmd,mask,value;};
static const struct power_cmd init_power_on_8814a[] = {
    {0x10c2, 0, BIT(1), BIT(1)},
    {0xFFFF, 3, 0, 0},
};
static const struct power_cmd trans_carddis_to_cardemu_8814a[] = {
    {0x0012, 0, BIT(6), BIT(6)},
    {0x0015, 0, BIT(5), 0},
    {0x0015, 0, BIT(6), 0},
    {0x0023, 0, BIT(4), 0},
    {0x0046, 0, 0xFF, 0x00},
    {0x0062, 0, 0xFF, 0x00},
    {0x0005, 0, BIT(3), 0},
    {0xFFFF, 3, 0, 0},
};
static const struct power_cmd trans_cardemu_to_act_8814a[] = {
    {0x0005, 0, BIT(2), 0},
    {0x0006, 1, BIT(1), BIT(1)},
    {0x0005, 0, BIT(3), 0},
    {0x00F0, 0, BIT(7), 0},
    {0x0081, 0, 0x30, 0x20},
    {0x0005, 0, BIT(0), BIT(0)},
    {0x0005, 1, BIT(0), 0},
    {0xFFFF, 3, 0, 0},
};
static const struct power_cmd trans_act_to_cardemu_8814a[] = {
    {0x0c00, 0, 0xFF, 0x04},
    {0x0e00, 0, 0xFF, 0x04},
    {0x1002, 0, BIT(0), 0},
    {0x0002, 2, 1, 0},
    {0x001F, 0, 0xFF, 0},
    {0x0007, 0, 0xFF, 0x28},
    {0x0008, 0, 0x02, 0},
    {0x0066, 0, BIT(7), 0},
    {0x0041, 0, BIT(4), 0},
    {0x0042, 0, BIT(1), 0},
    {0x004e, 0, BIT(5), BIT(5)},
    {0x0041, 0, BIT(0), 0},
    {0x0005, 0, BIT(1), BIT(1)},
    {0x0005, 1, BIT(1), 0},
    {0xFFFF, 3, 0, 0},
};
static const struct power_cmd trans_cardemu_to_carddis_8814a[] = {
    {0x0003, 0, BIT(2), 0},
    {0x0080, 0, 0xFF, 0x01},
    {0x0081, 0, 0xFF, 0x30},
    {0x0045, 0, 0xFF, 0x00},
    {0x0046, 0, 0xFF, 0xff},
    {0x0047, 0, 0xFF, 0},
    {0x0015, 0, BIT(6), BIT(6)},
    {0x0015, 0, BIT(5), BIT(5)},
    {0x0012, 0, BIT(6), 0},
    {0x0023, 0, BIT(4), BIT(4)},
    {0x0008, 0, BIT(1), 0},
    {0x0007, 0, 0xFF, 0x20},
    {0x001f, 0, BIT(1), 0},
    {0x0020, 0, BIT(1), 0},
    {0x0021, 0, BIT(1), 0},
    {0x0076, 0, BIT(1), 0},
    {0x0091, 0, 0xA0, 0xA0},
    {0x0070, 0, BIT(3), BIT(3)},
    {0x0005, 0, BIT(3), BIT(3)},
    {0xFFFF, 3, 0, 0},
};
static int power_sequence(struct rtw_8814au *c,const struct power_cmd *seq,size_t count)
{
    for(size_t i=0;i<count;i++) {
        const struct power_cmd *p=&seq[i];
        switch(p->cmd) {
        case 0: U8(c,p->offset,p->mask,p->value);break;
        case 1: if(rtw_8814au_poll(c,p->offset,1,p->mask,p->value,50,20000))return c->io_error;break;
        case 2: rtw_8814au_delay(c,p->offset);break;
        case 3: return c->io_error;
        default: return rtw14_fail(c,EINVAL);
        }if(c->io_error)return c->io_error;
    }return rtw14_fail(c,EINVAL);
}
#define SEQ(c,s) power_sequence(c,s,sizeof(s)/sizeof((s)[0]))
static int pre_system(struct rtw_8814au *c)
{
    W8(c,REG_RSV_CTRL,0);
    U32(c,REG_PAD_CTRL1,BIT_PAPE_WLBT_SEL|BIT_LNAON_WLBT_SEL,BIT_PAPE_WLBT_SEL|BIT_LNAON_WLBT_SEL);
    U32(c,REG_LED_CFG,BIT_PAPE_SEL_EN|BIT_LNAON_SEL_EN,0);
    U32(c,REG_GPIO_MUXCFG,BIT_WLRFE_4_5_EN,BIT_WLRFE_4_5_EN);
    U8(c,REG_SYS_FUNC_EN,BIT_FEN_BB_RSTB|BIT_FEN_BB_GLB_RST,0);
    U8(c,REG_RF_CTRL,BIT_RF_SDM_RSTB|BIT_RF_RSTB|BIT_RF_EN,0);
    U32(c,REG_WLRF1,BIT_WLRF1_BBRF_EN,0);
    return c->io_error;
}
static int power_switch(struct rtw_8814au *c,int on)
{
    uint8_t rpwm=(uint8_t)R8(c,0xfe58);
    if(R16(c,REG_MCUFW_CTRL)==0xc078)W8(c,0xfe58,(rpwm^BIT_RPWM_TOGGLE)&BIT_RPWM_TOGGLE);
    if(c->io_error)return c->io_error;
    int current=R8(c,REG_CR)!=0xea;
    if(c->io_error)return c->io_error;
    if(current==on)return EALREADY;
    if(on) {
        if(SEQ(c,init_power_on_8814a)||SEQ(c,trans_carddis_to_cardemu_8814a)||SEQ(c,trans_cardemu_to_act_8814a))return c->io_error;
    }else {
        if(SEQ(c,trans_act_to_cardemu_8814a)||SEQ(c,trans_cardemu_to_carddis_8814a))return c->io_error;
    }return 0;
}
int rtw_8814au_power_on(struct rtw_8814au *c)
{
    if(!c)return EINVAL;if(c->io_error)return c->io_error;
    c->powered=c->fw_running=c->mac_initialized=c->phy_initialized=0;
    if(pre_system(c))return c->io_error;
    int e=power_switch(c,1);
    if(e==EALREADY){e=power_switch(c,0);if(e&&e!=EALREADY)return e;if(pre_system(c))return c->io_error;e=power_switch(c,1);}
    if(e)return e;
    U32(c,REG_CPU_DMEM_CON,BIT_WL_PLATFORM_RST|BIT_DDMA_EN,BIT_WL_PLATFORM_RST|BIT_DDMA_EN);
    U8(c,REG_SYS_FUNC_EN+1,0xdc,0xdc);
    U8(c,REG_CR_EXT+3,15,12);
    if(R32(c,REG_MCUFW_CTRL)&BIT_BOOT_FSPI_EN){U32(c,REG_MCUFW_CTRL,BIT_BOOT_FSPI_EN,0);U32(c,REG_GPIO_MUXCFG,BIT_FSPI_EN,0);}
    if(!c->io_error)c->powered=1;return c->io_error;
}
int rtw_8814au_stop(struct rtw_8814au *c)
{
    if(!c)return EINVAL;
    /* A stop is also a recovery operation, preserving any earlier error. */
    int original=c->io_error;c->io_error=0;
    int e=power_switch(c,0);if(e==EALREADY)e=0;
    if(original)c->io_error=original;else if(e)c->io_error=e;
    c->powered=c->fw_running=c->mac_initialized=c->phy_initialized=0;
    return c->io_error;
}

int rtw_8814au_inspect_fw(const uint8_t *data,size_t size,struct rtw_8814au_fw_info *out)
{
    struct rtw_8814au_fw_info f;
    if(!data||!out||size<64||rtw14_le16(data)!=0x8814)return EINVAL;
    memset(&f,0,sizeof(f));f.version=rtw14_le16(data+4);f.subversion=data[6];
    uint32_t d=rtw14_le32(data+36),i=rtw14_le32(data+48),e=(data[24]&16)?rtw14_le32(data+52):0;
    if(!d||!i||d>0x20000||i>0x40000||e>0x20000)return EINVAL;
    f.dmem=(struct rtw_8814au_fw_segment){64,(size_t)d+8,rtw14_le32(data+32)&0x7fffffffu};
    f.imem=(struct rtw_8814au_fw_segment){64+(size_t)d+8,(size_t)i+8,rtw14_le32(data+60)&0x7fffffffu};
    f.emem=(struct rtw_8814au_fw_segment){64+(size_t)d+8+(size_t)i+8,e?(size_t)e+8:0,rtw14_le32(data+56)&0x7fffffffu};
    /* Restrict destinations to the actual 3081 memory windows. */
    if(f.dmem.size>0x20000||f.imem.size>0x40000||f.emem.size>0x20000||
       f.dmem.address<0x200000||f.dmem.address>0x220000-f.dmem.size||f.imem.address>0x40000-f.imem.size||
       (f.emem.size&&(f.emem.address<0x100000||f.emem.address>0x120000-f.emem.size)))return EINVAL;
    if(f.emem.offset+f.emem.size!=size)return EINVAL;
    *out=f;return 0;
}
static void cpu_enable(struct rtw_8814au *c,int en)
{
    if(en){U8(c,REG_RSV_CTRL+1,BIT_WLMCU_IOIF,BIT_WLMCU_IOIF);U8(c,REG_SYS_FUNC_EN+1,BIT_FEN_CPUEN,BIT_FEN_CPUEN);}
    else{U8(c,REG_SYS_FUNC_EN+1,BIT_FEN_CPUEN,0);U8(c,REG_RSV_CTRL+1,BIT_WLMCU_IOIF,0);}
}
/* Restricted boot packet encoder: the pinned USB implementation sets offset40
 * for reserved pages and offset0 for H2C packets. No data-frame policy here. */
static int send_packet(struct rtw_8814au *c,uint8_t qsel,const uint8_t *data,size_t size,int pad)
{
    uint8_t bytes[40+4096+1];
    size_t wire_size=size+(pad?1:0);
    if(size>4096||!data||!c->bus.bulk_write)return rtw14_fail(c,EINVAL);
    if(c->io_error)return c->io_error;
    memset(bytes,0,40);memcpy(bytes+40,data,size);if(pad)bytes[40+size]=0;
    rtw14_put32(bytes,(uint32_t)wire_size|(qsel==RTW_8814AU_QSEL_BEACON?(40u<<16)|BIT(26):0));
    rtw14_put32(bytes+4,(uint32_t)qsel<<8);
    uint16_t sum=0;for(unsigned j=0;j<32;j+=2)sum^=rtw14_le16(bytes+j);
    bytes[28]=(uint8_t)sum;bytes[29]=(uint8_t)(sum>>8);
    int e=c->bus.bulk_write(c->bus.cookie,qsel,bytes,40+wire_size);return e?rtw14_fail(c,e):0;
}
static int reserved_page(struct rtw_8814au *c,const uint8_t *data,size_t size)
{
    uint32_t bcn=R8(c,REG_BCN_CTRL),cr=R8(c,REG_CR+1);
    if(c->io_error)return c->io_error;
    W16(c,REG_FIFOPAGE_CTRL_2,BIT_BCN_VALID_V1);
    W8(c,REG_CR+1,cr|(BIT_ENSWBCN>>8));
    W8(c,REG_BCN_CTRL,(bcn&~BIT_EN_BCN_FUNCTION)|BIT_DIS_TSF_UDT);
    /* Preserve the pinned 3081 firmware workaround, but pad with an explicit
     * zero rather than reading one byte past the supplied final segment. */
    int e=send_packet(c,RTW_8814AU_QSEL_BEACON,data,size,((size+48)&511)==0);
    if(!e)e=rtw_8814au_poll(c,REG_FIFOPAGE_CTRL_2,4,BIT_BCN_VALID_V1,BIT_BCN_VALID_V1,10,1000);
    rtw14_restore(c,REG_FIFOPAGE_CTRL_2,2,c->fifo.rsvd_boundary|BIT_BCN_VALID_V1);
    rtw14_restore(c,REG_BCN_CTRL,1,bcn);rtw14_restore(c,REG_CR+1,1,cr);
    return e?e:c->io_error;
}
static int download_segment(struct rtw_8814au *c,const uint8_t *data,const struct rtw_8814au_fw_segment *s)
{
    if(!s->size)return 0;
    U32(c,REG_DDMA_CH0CTRL,BIT_DDMACH0_RESET_CHKSUM_STS,BIT_DDMACH0_RESET_CHKSUM_STS);
    for(size_t off=0;off<s->size;){size_t n=s->size-off;if(n>4096)n=4096;
        if(reserved_page(c,data+s->offset+off,n))return c->io_error;
        if(rtw_8814au_poll(c,REG_DDMA_CH0CTRL,4,BIT_DDMACH0_OWN,0,10,1000))return c->io_error;
        W32(c,REG_DDMA_CH0SA,0x18780000+40);
        W32(c,REG_DDMA_CH0DA,s->address+(uint32_t)off);
        W32(c,REG_DDMA_CH0CTRL,BIT_DDMACH0_CHKSUM_EN|BIT_DDMACH0_OWN|(off?BIT_DDMACH0_CHKSUM_CONT:0)|(uint32_t)n);
        if(rtw_8814au_poll(c,REG_DDMA_CH0CTRL,4,BIT_DDMACH0_OWN,0,10,1000))return c->io_error;
        off+=n;
    }
    uint32_t v=R32(c,REG_DDMA_CH0CTRL);if(c->io_error)return c->io_error;
    uint8_t done=s->address<0x200000?BIT_IMEM_DW_OK:BIT_DMEM_DW_OK;
    uint8_t check=s->address<0x200000?BIT_IMEM_CHKSUM_OK:BIT_DMEM_CHKSUM_OK;
    U8(c,REG_MCUFW_CTRL,done|check,done|((v&BIT_DDMACH0_CHKSUM_STS)?0:check));
    if(v&BIT_DDMACH0_CHKSUM_STS)return rtw14_fail(c,EINVAL);
    return c->io_error;
}
struct backup {uint16_t addr;uint8_t width;uint32_t value;};
int rtw_8814au_download_fw(struct rtw_8814au *c,const uint8_t *data,size_t size)
{
    struct rtw_8814au_fw_info f;
    if(!c)return EINVAL;if(c->io_error)return c->io_error;
    int e=rtw_8814au_inspect_fw(data,size,&f);if(e)return e;
    if(!c->powered)return ENXIO;
    c->fw_running=c->mac_initialized=c->phy_initialized=0;
    struct backup b[]={{REG_TXDMA_PQ_MAP+1,1,0},{REG_CR,1,0},{REG_H2CQ_CSR,4,BIT_H2CQ_FULL},
        {REG_FIFOPAGE_INFO_1,2,0},{REG_RQPN_CTRL_2,4,0},{REG_BCN_CTRL,1,0}};
    unsigned saved=0;
    cpu_enable(c,0);
    /* Read every saved register before mutating any of the six. */
    for(unsigned n=0;n<6&&!c->io_error;n++){if(n!=2)b[n].value=rtw14_r(c,b[n].addr,b[n].width);if(!c->io_error)saved=n+1;}
    if(c->io_error)goto fail;
    b[4].value|=BIT_LD_RQPN;
    W8(c,REG_TXDMA_PQ_MAP+1,3u<<6);W8(c,REG_CR,BIT_HCI_TXDMA_EN|BIT_TXDMA_EN);
    W32(c,REG_H2CQ_CSR,BIT_H2CQ_FULL);W16(c,REG_FIFOPAGE_INFO_1,0x200);W32(c,REG_RQPN_CTRL_2,b[4].value);
    W8(c,REG_BCN_CTRL,(b[5].value&~BIT_EN_BCN_FUNCTION)|BIT_DIS_TSF_UDT);
    U8(c,REG_CPU_DMEM_CON+2,BIT_WL_PLATFORM_RST>>16,0);
    U8(c,REG_SYS_CLK_CTRL+1,BIT_CPU_CLK_EN>>8,0);
    U8(c,REG_CPU_DMEM_CON+2,BIT_WL_PLATFORM_RST>>16,BIT_WL_PLATFORM_RST>>16);
    U8(c,REG_SYS_CLK_CTRL+1,BIT_CPU_CLK_EN>>8,BIT_CPU_CLK_EN>>8);
    W16(c,REG_MCUFW_CTRL,(R16(c,REG_MCUFW_CTRL)&0x3800)|BIT_MCUFWDL_EN);
    if(download_segment(c,data,&f.dmem)||download_segment(c,data,&f.imem)||download_segment(c,data,&f.emem))goto fail;
    for(unsigned n=0;n<saved;n++)rtw14_restore(c,b[n].addr,b[n].width,b[n].value);
    saved=0;
    W32(c,REG_TXDMA_STATUS,BTI_PAGE_OVF);
    uint32_t fw=R16(c,REG_MCUFW_CTRL);
    if((fw&BIT_CHECK_SUM_OK)!=BIT_CHECK_SUM_OK){rtw14_fail(c,EINVAL);goto fail;}
    W16(c,REG_MCUFW_CTRL,(fw|BIT_FW_DW_RDY)&~BIT_MCUFWDL_EN);
    cpu_enable(c,1);
    if(rtw_8814au_poll(c,REG_MCUFW_CTRL,2,FW_READY_MASK,FW_READY,10,1000))goto fail;
    c->h2c_box=c->h2c_seq=0;c->fw_running=1;return 0;
fail:
    e=c->io_error?c->io_error:EIO;
    for(unsigned n=0;n<saved;n++)rtw14_restore(c,b[n].addr,b[n].width,b[n].value);
    /* Cleanup reads through the bus even after an earlier sticky error. */
    uint32_t v;if(c->bus.read&&!c->bus.read(c->bus.cookie,REG_MCUFW_CTRL,1,&v))rtw14_restore(c,REG_MCUFW_CTRL,1,v&~BIT_MCUFWDL_EN);
    if(c->bus.read&&!c->bus.read(c->bus.cookie,REG_SYS_FUNC_EN+1,1,&v))rtw14_restore(c,REG_SYS_FUNC_EN+1,1,v|BIT_FEN_CPUEN);
    return rtw14_fail(c,e);
}
int rtw_8814au_h2c_mailbox(struct rtw_8814au *c,const uint8_t command[8])
{
    if(!c||!command)return EINVAL;if(c->io_error)return c->io_error;
    if(!c->fw_running||c->h2c_box>=4)return ENXIO;
    if(rtw_8814au_poll(c,REG_HMETFR,1,BIT(c->h2c_box),0,100,31))return c->io_error;
    W32(c,REG_HMEBOX0_EX+c->h2c_box*4,rtw14_le32(command+4));
    W32(c,REG_HMEBOX0+c->h2c_box*4,rtw14_le32(command));
    if(!c->io_error)c->h2c_box=(c->h2c_box+1)&3;return c->io_error;
}
int rtw_8814au_h2c_packet(struct rtw_8814au *c,const uint8_t packet[32])
{
    if(!c||!packet)return EINVAL;if(c->io_error)return c->io_error;if(!c->fw_running)return ENXIO;
    uint8_t p[32];memcpy(p,packet,32);p[6]=(uint8_t)c->h2c_seq;p[7]=(uint8_t)(c->h2c_seq>>8);
    int e=send_packet(c,RTW_8814AU_QSEL_H2C,p,32,0);if(!e)c->h2c_seq++;return e;
}

int rtw_8814au_send_startup_info(struct rtw_8814au *c)
{
    if(!c)return EINVAL;if(c->io_error)return c->io_error;
    if(!c->fw_running||!c->mac_initialized||!c->phy_initialized||!c->board_valid)return ENXIO;
    if(c->fifo.rsvd_fw_txbuf_addr<c->fifo.rsvd_boundary||
       c->fifo.rsvd_fw_txbuf_addr-c->fifo.rsvd_boundary>255)return EINVAL;
    uint8_t packet[32]={0};
    /* H2C category1, commandFF, general-info subcommand0D. */
    packet[0]=1;packet[1]=255;packet[2]=13;packet[4]=12;
    packet[10]=(uint8_t)(c->fifo.rsvd_fw_txbuf_addr-c->fifo.rsvd_boundary);
    int e=rtw_8814au_h2c_packet(c,packet);if(e)return e;
    memset(packet,0,sizeof(packet));packet[0]=1;packet[1]=255;packet[2]=17;packet[4]=16;
    packet[8]=c->board.rfe_option;
    /* Match the pinned common PHYDM helper: only1T1R/2T2R have an encoding. */
    packet[9]=c->board.tx_streams==1?4:c->board.tx_streams==2?2:0;
    packet[10]=c->board.cut;
    packet[11]=(uint8_t)((c->board.antenna_tx&15)|((c->board.antenna_rx&15)<<4));
    return rtw_8814au_h2c_packet(c,packet);
}
