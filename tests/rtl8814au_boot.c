/* Host tests for the actual portable boot functions. Register side effects
 * model only hardware handshakes; assertions inspect emitted bus traffic. */
#include "rtw/RTL8814AU.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct event { unsigned addr,width,value; };
struct model { uint8_t reg[65536], physical[1024]; struct event writes[10000];
 size_t nw, reads, fail_read_n, bulk_count, bulk_bytes; unsigned efuse_addr;
 unsigned fail_addr,fail_write,fail_bulk,timeout_efuse,timeout_power,timeout_llt,timeout_ddma,bad_checksum,timeout_fw;
 uint64_t delayed; uint8_t bulk[4200], bulk_headers[32][80]; size_t bulk_lengths[32]; size_t last_bulk; uint8_t qsel; };
static struct model m;
static uint32_t get(unsigned a,unsigned w) { uint32_t v=0;for(unsigned i=0;i<w;i++)v|=(uint32_t)m.reg[a+i]<<(8*i);return v; }
static void put(unsigned a,unsigned w,uint32_t v) {for(unsigned i=0;i<w;i++)m.reg[a+i]=(uint8_t)(v>>(8*i));}
static int rd(void *cookie,uint16_t a,unsigned w,uint32_t *v) { assert(cookie==&m);m.reads++;
 if(m.fail_addr==a+1||(m.fail_read_n&&m.reads==m.fail_read_n))return EIO;
 *v=get(a,w);
 if(a==0x30&&w==4&&!m.timeout_efuse)*v=(*v&~255u)|m.physical[m.efuse_addr]|0x80000000u;
 if(a==0x5&&!m.timeout_power)*v&=~3u;
 if(a==0x6&&!m.timeout_power)*v|=2;
 if(a==0x208&&!m.timeout_llt)*v&=~1u;
 if(a==0x1208&&w==4&&!m.timeout_ddma)*v&=~0x80000000u;
 if(a==0x1208&&m.bad_checksum)*v|=1u<<27;
 return 0; }
static int wr(void *cookie,uint16_t a,unsigned w,uint32_t v) { assert(cookie==&m);
 if(m.fail_write==a+1)return EIO;
 assert(m.nw<10000);m.writes[m.nw++]=(struct event){a,w,v};put(a,w,v);
 if(a==0x30&&w==4)m.efuse_addr=(v>>8)&1023;
 if(a==0x3&&(v&4)&&(get(0x80,2)&0x4000)&&!m.timeout_fw)put(0x80,2,0xc078);
 return 0; }
static int bulk(void *cookie,uint8_t qsel,const uint8_t *b,size_t n) { assert(cookie==&m);if(m.fail_bulk)return EIO;
 assert(n<=sizeof(m.bulk));memcpy(m.bulk,b,n);m.last_bulk=n;m.qsel=qsel;assert(m.bulk_count<32);m.bulk_lengths[m.bulk_count]=n;memcpy(m.bulk_headers[m.bulk_count],b,n<80?n:80);m.bulk_count++;m.bulk_bytes+=n;
 put(0x205,1,get(0x205,1)|0x80);return 0; }
static void delay(void *cookie,uint32_t u) {assert(cookie==&m);m.delayed+=u;}
static void reset(struct rtw_8814au *c) {memset(&m,0,sizeof(m));memset(m.physical,255,sizeof(m.physical));
 
 struct rtw_8814au_bus_ops o={&m,rd,wr,bulk,delay};rtw_8814au_init(c,&o);put(0x100,1,0xea);put(0xf0,4,0x044411b5);put(0xfc,4,0x80000008);put(4,4,0x30453f15); }
static int wrote(unsigned a,unsigned w,unsigned v) {for(size_t i=0;i<m.nw;i++)if(m.writes[i].addr==a&&m.writes[i].width==w&&m.writes[i].value==v)return 1;return 0;}
static void logical(uint8_t b[512]) {memset(b,255,512);b[0]=0x29;b[1]=0x81;b[0xca]=1;b[0xd8]=2;b[0xd9]=0x11;b[0xda]=0x22;b[0xdb]=0x33;b[0xdc]=0x44;b[0xdd]=0x55;b[0xc9]=255;}
static size_t pack_map(const uint8_t b[512]) {size_t j=0;for(unsigned block=0;block<64;block++){if(block<16)m.physical[j++]=(uint8_t)(block<<4);else {m.physical[j++]=(uint8_t)(((block&7)<<5)|15);m.physical[j++]=(uint8_t)((block&0x78)<<1);}for(unsigned k=0;k<8;k++)m.physical[j++]=b[block*8+k];}m.physical[j++]=255;return j;}
static void efuse_tests(void) {struct rtw_8814au c;reset(&c);uint8_t b[512],out[512];logical(b);pack_map(b);
 assert(!rtw_8814au_decode_efuse(m.physical,1024,out,512));assert(!memcmp(out,b,512));
 uint8_t ext[]={15};assert(rtw_8814au_decode_efuse(ext,sizeof(ext),out,512)==EINVAL);
 uint8_t pair[]={0x0e,0x12};assert(rtw_8814au_decode_efuse(pair,sizeof(pair),out,512)==EINVAL);
 uint8_t outside[]={0xef,0xfe,1,2,255};assert(rtw_8814au_decode_efuse(outside,sizeof(outside),out,512)==EINVAL);
 assert(!rtw_8814au_parse_board(&c,b,RTW_8814AU_HIGH_SPEED));assert(c.board.physical_rf_paths==4&&c.board.tx_streams==2&&c.board.rx_streams==2);assert(c.board.mac[5]==0x55&&c.board.rfe_option==1);
 assert(!rtw_8814au_parse_board(&c,b,RTW_8814AU_SUPER_SPEED));assert(c.board.tx_streams==3);
 b[0xca]=6;assert(rtw_8814au_parse_board(&c,b,RTW_8814AU_HIGH_SPEED)==ENODEV);assert(!c.board_valid);b[0xca]=255;assert(!rtw_8814au_parse_board(&c,b,RTW_8814AU_HIGH_SPEED));assert(c.board.rfe_option==1);
 b[0xd8]=1;assert(rtw_8814au_parse_board(&c,b,RTW_8814AU_HIGH_SPEED)==EINVAL);
 reset(&c);logical(b);pack_map(b);unsigned clk=get(8,2),fn=get(2,2),bank=get(0x34,4);
 assert(!rtw_8814au_identify_read_board(&c,RTW_8814AU_HIGH_SPEED));assert(c.board.cut==1&&c.board.vendor==4&&c.board_valid);assert(get(8,2)==clk&&get(2,2)==fn&&get(0x34,4)==bank&&get(0xcf,1)==0);
 for(size_t i=0;i<m.nw;i++)assert(m.writes[i].addr==0xcf||m.writes[i].addr==2||m.writes[i].addr==8||m.writes[i].addr==0x34||m.writes[i].addr==0x30);
 reset(&c);m.timeout_efuse=1;assert(rtw_8814au_identify_read_board(&c,RTW_8814AU_HIGH_SPEED)==ETIMEDOUT);assert(!c.board_valid&&get(0xcf,1)==0);assert(m.reads<2500);
 reset(&c);m.fail_addr=0x31;assert(rtw_8814au_identify_read_board(&c,RTW_8814AU_HIGH_SPEED)==EIO);assert(!c.board_valid&&get(0xcf,1)==0&&get(2,2)==0&&get(8,2)==0);
 /* Never publish a board after a failed read of any physical byte. */
 for(unsigned byte=0;byte<1024;byte++) {
  reset(&c);logical(b);pack_map(b);
  memset(c.board.logical,0xa5,sizeof(c.board.logical));
  clk=get(8,2);fn=get(2,2);bank=get(0x34,4);
  m.fail_read_n=10u+3u*byte;
  assert(rtw_8814au_identify_read_board(&c,RTW_8814AU_HIGH_SPEED)==EIO);
  assert(!c.board_valid&&c.io_error==EIO);
  assert(get(8,2)==clk&&get(2,2)==fn&&get(0x34,4)==bank&&get(0xcf,1)==0);
  for(unsigned j=0;j<sizeof(c.board.logical);j++)assert(c.board.logical[j]==0xa5);
 }
 puts("RTL8814AU efuse: 1024 injected partial-read failures passed");
 reset(&c);m.fail_addr=0xf1;assert(rtw_8814au_identify_read_board(&c,RTW_8814AU_HIGH_SPEED)==EIO);assert(!m.nw);
 reset(&c);put(0xf0,4,~0u);assert(rtw_8814au_identify_read_board(&c,RTW_8814AU_HIGH_SPEED)==ENODEV);assert(!m.nw);
}
static void power_tests(void) {struct rtw_8814au c;reset(&c);assert(!rtw_8814au_power_on(&c));assert(c.powered&&!c.fw_running&&!c.phy_initialized);assert(wrote(0x10c2,1,2));assert(wrote(0x81,1,0x20));assert((get(0x1080,4)&0x10100)==0x10100);assert(!rtw_8814au_stop(&c));assert(!c.powered);
 reset(&c);m.timeout_power=1;assert(rtw_8814au_power_on(&c)==ETIMEDOUT);assert(!c.powered&&m.reads<21000);
 reset(&c);m.fail_write=0x10c3;assert(rtw_8814au_power_on(&c)==EIO);assert(!c.powered);
}
static void mac_tests(void) {struct rtw_8814au c;for(unsigned ep=2;ep<=4;ep++){reset(&c);c.powered=c.fw_running=1;assert(!rtw_8814au_init_mac(&c,ep,RTW_8814AU_HIGH_SPEED));assert(c.mac_initialized&&!c.phy_initialized);assert(c.fifo.txff_pg_num==2038&&c.fifo.rsvd_boundary==1986&&c.fifo.rsvd_fw_txbuf_addr==2034);assert(wrote(0x10c,2,ep==2?0xfaf0:0xf5b0));assert(wrote(0x60f,1,4));assert(wrote(0x208,1,0x30));assert(get(0x16,1)==2);assert(get(0x6a0,4)==0x0400ffff);assert(!wrote(0xf002,2,0));assert(get(0x280,2)==0x2005&&get(0x290,1)==0x1e);assert(!(get(0x100,2)&0x200));}
 reset(&c);c.powered=c.fw_running=1;assert(rtw_8814au_init_mac(&c,1,RTW_8814AU_HIGH_SPEED)==EINVAL);assert(!m.nw);
 reset(&c);c.powered=c.fw_running=1;assert(!rtw_8814au_init_mac(&c,3,RTW_8814AU_SUPER_SPEED));assert(wrote(0xf002,2,0)&&get(0x290,1)==0x0e);
 reset(&c);c.powered=c.fw_running=1;m.timeout_llt=1;assert(rtw_8814au_init_mac(&c,3,RTW_8814AU_HIGH_SPEED)==ETIMEDOUT);assert(!c.mac_initialized);
 reset(&c);c.powered=c.fw_running=1;m.fail_write=0x17;assert(rtw_8814au_init_mac(&c,3,RTW_8814AU_HIGH_SPEED)==EIO);assert(!c.mac_initialized);
}
static void set32(uint8_t *p,unsigned o,uint32_t v){for(unsigned i=0;i<4;i++)p[o+i]=(uint8_t)(v>>(8*i));}
static uint8_t image[64+9000+8+5000+8];
static void make_fw(void){memset(image,0,sizeof(image));image[0]=0x14;image[1]=0x88;image[4]=33;image[6]=6;set32(image,32,0x80200000);set32(image,36,9000);set32(image,48,5000);set32(image,60,0x80000000);}
static void fw_tests(void) {struct rtw_8814au_fw_info f;make_fw();assert(!rtw_8814au_inspect_fw(image,sizeof(image),&f));assert(f.version==33&&f.dmem.size==9008&&f.imem.offset==9072&&f.imem.size==5008&&f.dmem.address==0x200000);
 assert(rtw_8814au_inspect_fw(image,63,&f)==EINVAL);assert(rtw_8814au_inspect_fw(image,sizeof(image)-1,&f)==EINVAL);image[0]=0xe1;assert(rtw_8814au_inspect_fw(image,sizeof(image),&f)==EINVAL);make_fw();set32(image,36,~0u);assert(rtw_8814au_inspect_fw(image,sizeof(image),&f)==EINVAL);make_fw();set32(image,32,0xffffffff);assert(rtw_8814au_inspect_fw(image,sizeof(image),&f)==EINVAL);make_fw();
 size_t huge_size=64+9+0x40000+8;uint8_t *huge=calloc(1,huge_size);assert(huge);memcpy(huge,image,64);set32(huge,36,1);set32(huge,48,0x40000);assert(rtw_8814au_inspect_fw(huge,huge_size,&f)==EINVAL);free(huge);
 struct rtw_8814au c;reset(&c);c.powered=1;assert(!rtw_8814au_download_fw(&c,image,sizeof(image)));assert(c.fw_running&&!c.mac_initialized&&!c.phy_initialized);assert(m.bulk_count==5&&m.qsel==16);assert(wrote(0x1200,4,0x18780028));assert(wrote(0x1204,4,0x200000));assert(wrote(0x1204,4,0x202000));assert(wrote(0x1204,4,0x1000));assert(get(0x80,2)==0xc078);
 reset(&c);c.powered=1;put(0x10d,1,0x77);m.fail_addr=0x10e;assert(rtw_8814au_download_fw(&c,image,sizeof(image))==EIO);assert(get(0x10d,1)==0x77);
 reset(&c);c.powered=1;put(0x10d,1,0x77);put(0x550,1,0x18);m.fail_bulk=1;assert(rtw_8814au_download_fw(&c,image,sizeof(image))==EIO);assert(!c.fw_running&&!(get(0x80,1)&1));assert(get(0x10d,1)==0x77&&get(0x550,1)==0x18);
 reset(&c);c.powered=1;m.bad_checksum=1;assert(rtw_8814au_download_fw(&c,image,sizeof(image))==EINVAL);assert(!c.fw_running);
 reset(&c);c.powered=1;m.timeout_ddma=1;assert(rtw_8814au_download_fw(&c,image,sizeof(image))==ETIMEDOUT);assert(!c.fw_running);
 uint8_t padded[64+464+16];memset(padded,0xa5,sizeof(padded));memset(padded,0,64);padded[0]=0x14;padded[1]=0x88;set32(padded,32,0x80200000);set32(padded,36,456);set32(padded,48,8);set32(padded,60,0x80000000);
 reset(&c);c.powered=1;assert(!rtw_8814au_download_fw(&c,padded,sizeof(padded)));assert(m.bulk_lengths[0]==505&&m.bulk_headers[0][0]==0xd1&&m.bulk_headers[0][1]==1);assert(wrote(0x1208,4,0xa00001d0));
 reset(&c);c.powered=1;m.timeout_fw=1;assert(rtw_8814au_download_fw(&c,image,sizeof(image))==ETIMEDOUT);assert(!c.fw_running);
}
static void h2c_tests(void) {struct rtw_8814au c;reset(&c);c.fw_running=1;uint8_t cmd[8]={0x40,1,2,3,4,5,6,7};assert(!rtw_8814au_h2c_mailbox(&c,cmd));assert(m.nw==2&&m.writes[0].addr==0x1f0&&m.writes[0].value==0x07060504&&m.writes[1].addr==0x1d0&&m.writes[1].value==0x03020140&&c.h2c_box==1);
 uint8_t p[32]={0};p[4]=12;c.h2c_seq=9;assert(!rtw_8814au_h2c_packet(&c,p));assert(m.qsel==19&&m.last_bulk==72&&m.bulk[46]==9&&c.h2c_seq==10);assert(m.bulk[0]==32&&m.bulk[2]==0); /* Linux USB H2C offset intentionally0. */
 c.h2c_seq=255;assert(!rtw_8814au_h2c_packet(&c,p));assert(c.h2c_seq==256);assert(!rtw_8814au_h2c_packet(&c,p));assert(m.bulk[46]==0&&m.bulk[47]==1);
 put(0x1cc,1,2);assert(rtw_8814au_h2c_mailbox(&c,cmd)==ETIMEDOUT);assert(c.h2c_box==1);
}
static void startup_tests(void) {struct rtw_8814au c;reset(&c);c.fw_running=c.mac_initialized=c.phy_initialized=c.board_valid=1;c.board.rfe_option=1;c.board.cut=1;c.board.tx_streams=2;c.board.antenna_tx=c.board.antenna_rx=3;c.fifo.rsvd_fw_txbuf_addr=2034;c.fifo.rsvd_boundary=1986;
 assert(!rtw_8814au_send_startup_info(&c));assert(m.bulk_count==2);const uint8_t *g=m.bulk_headers[0]+40,*p=m.bulk_headers[1]+40;assert(g[0]==1&&g[1]==255&&g[2]==13&&g[4]==12&&g[10]==48);assert(p[2]==17&&p[4]==16&&p[6]==1&&p[8]==1&&p[9]==2&&p[10]==1&&p[11]==0x33);
 reset(&c);c.fw_running=c.mac_initialized=c.board_valid=1;assert(rtw_8814au_send_startup_info(&c)==ENXIO);assert(!m.bulk_count);
}
int main(int argc,char **argv) {assert(argc==2||argc==3);if(argc==3){assert(!strcmp(argv[1],"image"));FILE *file=fopen(argv[2],"rb");assert(file);assert(!fseek(file,0,SEEK_END));long length=ftell(file);assert(length>0);rewind(file);uint8_t *bytes=malloc((size_t)length);assert(bytes&&fread(bytes,1,(size_t)length,file)==(size_t)length);fclose(file);struct rtw_8814au_fw_info f;assert(!rtw_8814au_inspect_fw(bytes,(size_t)length,&f));struct rtw_8814au c;reset(&c);c.powered=1;assert(!rtw_8814au_download_fw(&c,bytes,(size_t)length));assert(c.fw_running&&!c.phy_initialized);free(bytes);puts("Canonical RTL8814AU firmware trace passed");return 0;}if(!strcmp(argv[1],"efuse"))efuse_tests();else if(!strcmp(argv[1],"power"))power_tests();else if(!strcmp(argv[1],"mac"))mac_tests();else if(!strcmp(argv[1],"fw"))fw_tests();else if(!strcmp(argv[1],"h2c"))h2c_tests();else if(!strcmp(argv[1],"startup"))startup_tests();else abort();puts("RTL8814AU boot trace passed");return 0;}
