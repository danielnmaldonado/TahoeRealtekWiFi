/* SPDX-License-Identifier: GPL-2.0-only
 * MaLd0n portable RTL8814B USB boundary around Realtek HALMAC1.6.6.6.
 * Vendor algorithms and notices: rtl8814b/halmac; immutable provenance:
 * RTW8814BHAL-Upstream.json. Every transport error revokes authority.
 */
#include "rtw/RTW8814BHAL.h"
#include "rtl8814b/halmac/RTW8814BPlatform.h"
#include "rtl8814b/halmac/halmac_api.h"
#include "rtl8814b/halmac/halmac_88xx_v1/halmac_efuse_88xx_v1.h"
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
#include <string.h>
#define AD(c) ((struct halmac_adapter *)(c)->adapter)
#define API(c) ((struct halmac_api *)(c)->api)
typedef struct {
 rtwb14_context *identity;
 const void *phy_client;
 rtwb14_bus bus;
 struct halmac_adapter *adapter;
 struct halmac_api *api;
 uint8_t state,power_owned,cut,bulkout_count;
} rtwb14_owner;
static rtwb14_owner *owner(rtwb14_context*c){return (rtwb14_owner*)c->private_owner;}
static void state(rtwb14_context*c,uint8_t s,uint8_t p){
 c->state=s;c->power_owned=p;
 if(owner(c)&&owner(c)->identity==c){owner(c)->state=s;owner(c)->power_owned=p;if(s!=RTWB14_FIRMWARE_READY)owner(c)->phy_client=NULL;}
}
static int fault(rtwb14_context*c,int e){
 if(e<0)e=e==INT32_MIN?EIO:-e;
 if(e&&!c->last_error)c->last_error=e;
 if(e)state(c,RTWB14_FAULT,0);
 return c->last_error;
}
static int bound(rtwb14_context*c){rtwb14_owner*o;
 if(!c||!(o=owner(c)))return EINVAL;
 if(o->identity!=c||o->adapter!=c->adapter||o->api!=c->api||
    o->state!=c->state||o->power_owned!=c->power_owned||o->cut!=c->cut||o->bulkout_count!=c->bulkout_count||
    o->bus.cookie!=c->bus.cookie||o->bus.read!=c->bus.read||
    o->bus.write!=c->bus.write||o->bus.send_payload!=c->bus.send_payload||
    o->bus.delay_us!=c->bus.delay_us||o->bus.allocate!=c->bus.allocate||
    o->bus.release!=c->bus.release)return fault(c,EINVAL);
 return c->last_error;
}
static const rtwb14_bus *bus(rtwb14_context*c){return owner(c)?&owner(c)->bus:&c->bus;}
static int operation(rtwb14_context*c,uint32_t a,unsigned w){
 if(c->last_error)return c->last_error;
 if(owner(c)&&bound(c))return c->last_error;
 if((w!=1&&w!=2&&w!=4)||a>65536U-w)return fault(c,EINVAL);
 if(!c->operations_left)return fault(c,ETIMEDOUT);
 --c->operations_left;return 0;
}
static u32 read_reg(rtwb14_context*c,u32 a,unsigned w){
 u32 v=UINT32_MAX;if(operation(c,a,w))return v;
 if(fault(c,bus(c)->read(bus(c)->cookie,(uint16_t)a,w,&v)))return UINT32_MAX;
 if(owner(c)&&bound(c))return UINT32_MAX;
 return v;
}
static void write_reg(rtwb14_context*c,u32 a,unsigned w,u32 v){
 if(!operation(c,a,w)){fault(c,bus(c)->write(bus(c)->cookie,(uint16_t)a,w,v));if(owner(c))bound(c);}
}
static u8 r8(void*p,u32 a){return (u8)read_reg(p,a,1);}
static u16 r16(void*p,u32 a){return (u16)read_reg(p,a,2);}
static u32 r32(void*p,u32 a){return read_reg(p,a,4);}
static void w8(void*p,u32 a,u8 v){write_reg(p,a,1,v);}
static void w16(void*p,u32 a,u16 v){write_reg(p,a,2,v);}
static void w32(void*p,u32 a,u32 v){write_reg(p,a,4,v);}
static void *allocate(void*p,u32 n){rtwb14_context*c=p;void*b;
 if(c->last_error||!n||n>1048576U||(owner(c)&&bound(c)))return NULL;
 b=bus(c)->allocate(bus(c)->cookie,n);if(!b)fault(c,ENOMEM);return b;
}
static u8 release(void*p,void*b,u32 n){rtwb14_context*c=p;if(b)bus(c)->release(bus(c)->cookie,b,n);return 1;}
static u8 copy(void*p,void*d,void*s,u32 n){(void)p;memcpy(d,s,n);return 1;}
static u8 clear(void*p,void*d,u8 v,u32 n){(void)p;memset(d,v,n);return 1;}
static void delay(void*p,u32 us){rtwb14_context*c=p;if(c->last_error)return;
 if(us>c->delay_left_us){fault(c,ETIMEDOUT);return;}
 c->delay_left_us-=us;bus(c)->delay_us(bus(c)->cookie,us);
}
/* Public calls and callbacks are serialized by the owner; these vendor locks
 * never cross a public call boundary or provide IRQ synchronization. */
static u8 lock(void*p,HALMAC_MUTEX*m){(void)p;(void)m;return 1;}
static u8 message(void*p,u32 t,u8 l,s8*f,...){(void)p;(void)t;(void)l;(void)f;return 1;}
static u8 event(void*p,enum halmac_feature_id id,enum halmac_cmd_process_status s,u8*b,u32 n){(void)p;(void)id;(void)s;(void)b;(void)n;return 1;}
static u8 packet(rtwb14_context*c,unsigned kind,u8*b,u32 n){
 if(c->last_error)return 0;
 if(!b||!n||n>31744U||!c->operations_left){fault(c,EINVAL);return 0;}
 --c->operations_left;
 return fault(c,bus(c)->send_payload(bus(c)->cookie,kind,b,n))?0:1;
}
static u8 reserved(void*p,u8*b,u32 n){return packet(p,0,b,n);}
static u8 h2c(void*p,u8*b,u32 n){return packet(p,1,b,n);}
static struct halmac_platform_api platform={
 .REG_READ_8=r8,.REG_READ_16=r16,.REG_READ_32=r32,
 .REG_WRITE_8=w8,.REG_WRITE_16=w16,.REG_WRITE_32=w32,
 .SEND_RSVD_PAGE=reserved,.SEND_H2C_PKT=h2c,.RTL_FREE=release,
 .RTL_MALLOC=allocate,.RTL_MEMCPY=copy,.RTL_MEMSET=clear,.RTL_DELAY_US=delay,
 .MUTEX_INIT=lock,.MUTEX_DEINIT=lock,.MUTEX_LOCK=lock,.MUTEX_UNLOCK=lock,
 .MSG_PRINT=message,.EVENT_INDICATION=event
};
static int finish(rtwb14_context*c,enum halmac_ret_status s){
 c->vendor_status=(uint32_t)s;
 if(owner(c)&&bound(c))return c->last_error;
 if(c->last_error)return c->last_error;
 if(s==HALMAC_RET_SUCCESS)return 0;
 return fault(c,s==HALMAC_RET_MALLOC_FAIL?ENOMEM:
  s==HALMAC_RET_PWR_UNCHANGE?EBUSY:ETIMEDOUT);
}
static int begin(rtwb14_context*c,int firmware){
 if(!c||!owner(c))return EINVAL;
 if(bound(c))return c->last_error;
 if(!c->adapter||!c->api)return fault(c,EINVAL);
 if(c->last_error)return c->last_error;
 if(c->state==RTWB14_UNBOUND||c->state==RTWB14_FAULT)return ENXIO;
 c->operations_left=32768U;c->delay_left_us=5000000U;
 if(r8(c,0xfc)!=0x11)return fault(c,c->last_error?c->last_error:ENODEV);
 if(firmware&&r16(c,0x80)!=0xc078U)return fault(c,c->last_error?c->last_error:ENXIO);
 return c->last_error;
}
int rtwb14_open(rtwb14_context*c,const rtwb14_bus*b,unsigned pipes){
 struct halmac_adapter*a=NULL;struct halmac_api*api=NULL;enum halmac_ret_status s;
 rtwb14_bus cfg;rtwb14_owner preflight,*o;int e;
 if(!c||!b||!b->read||!b->write||!b->send_payload||!b->delay_us||!b->allocate||!b->release||pipes<3||pipes>5)return EINVAL;
 if(c->adapter||c->api||c->private_owner)return EBUSY;
 cfg=*b;memset(c,0,sizeof(*c));c->bus=cfg;c->operations_left=32768U;c->delay_left_us=5000000U;c->bulkout_count=(u8)pipes;
 /* Stack snapshot protects even the first identity reads without allocating
  * for a foreign chip. Never retain this temporary owner after return. */
 memset(&preflight,0,sizeof(preflight));preflight.identity=c;preflight.bus=cfg;preflight.bulkout_count=c->bulkout_count;c->private_owner=&preflight;
 if(r8(c,0xfc)!=0x11){e=fault(c,c->last_error?c->last_error:ENODEV);c->private_owner=NULL;return e;}
 c->cut=(u8)(r8(c,0xf1)>>4);preflight.cut=c->cut;
 if(c->last_error){c->private_owner=NULL;return c->last_error;}
 o=cfg.allocate(cfg.cookie,sizeof(*o));
 if(!o){e=fault(c,ENOMEM);c->private_owner=NULL;return e;}
 if(bound(c)){cfg.release(cfg.cookie,o,sizeof(*o));c->private_owner=NULL;return c->last_error;}
 *o=preflight;c->private_owner=o;
 s=halmac_init_adapter(c,&platform,HALMAC_INTERFACE_USB,&a,&api);
 c->adapter=a;c->api=api;o->adapter=a;o->api=api;
 if(!s&&a&&api&&a->chip_id!=HALMAC_CHIP_ID_8814B)fault(c,ENODEV);
 if(!s&&a&&api&&(a->hw_cfg_info.efuse_size!=RTWB14_OTP_BYTES||a->hw_cfg_info.txdesc_size!=RTWB14_TX_DESC_BYTES||a->hw_cfg_info.rxdesc_size!=RTWB14_RX_DESC_BYTES))fault(c,ENODEV);
 if(finish(c,s)||!a||!api){e=c->last_error?c->last_error:fault(c,EIO);rtwb14_close(c);c->last_error=e;c->state=RTWB14_FAULT;return e;}
 state(c,RTWB14_IDENTIFIED,0);return 0;
}
int rtwb14_mac_boot(rtwb14_context*c){int e;if(!c||!owner(c))return EINVAL;
 if(bound(c))return c->last_error;
 if(c->last_error)return c->last_error;
 if(c->state!=RTWB14_IDENTIFIED)return EBUSY;
 if((e=begin(c,0)))return e;
 /* Reject an already-running MAC: this boundary owns its cold MAC transition.
  * Firmware ownership is established only by the later validated download. */
 if(r8(c,0x100)!=0xeaU)return fault(c,c->last_error?c->last_error:EBUSY);
 if(c->last_error)return c->last_error;
 if((e=finish(c,API(c)->halmac_init_usb_cfg(AD(c))))||
    (e=finish(c,API(c)->halmac_set_bulkout_num(AD(c),c->bulkout_count)))||
    (e=finish(c,API(c)->halmac_mac_power_switch(AD(c),HALMAC_MAC_POWER_ON))))return e;
 state(c,RTWB14_IDENTIFIED,1);
 if((e=finish(c,API(c)->halmac_init_mac_cfg(AD(c),HALMAC_TRX_MODE_NORMAL))))return e;
 state(c,RTWB14_MAC_READY,1);return 0;
}
int rtwb14_power_off(rtwb14_context*c){int e;if(!c||!owner(c))return EINVAL;
 if(bound(c))return c->last_error;
 if(!c->adapter)return fault(c,EINVAL);
 if(c->last_error)return c->last_error;
 if(!c->power_owned)return 0;
 if((e=begin(c,c->state==RTWB14_FIRMWARE_READY)))return e;
 e=finish(c,API(c)->halmac_mac_power_switch(AD(c),HALMAC_MAC_POWER_OFF));
 if(!e)state(c,RTWB14_IDENTIFIED,0);return e;
}
void rtwb14_close(rtwb14_context*c){rtwb14_owner*o;if(!c)return;o=owner(c);
 if(o&&o->identity==c){
  /* Use the private adapter and bus even if public diagnostics were changed. */
  if(o->adapter)halmac_deinit_adapter(o->adapter);
  o->bus.release(o->bus.cookie,o,sizeof(*o));
 }
 memset(c,0,sizeof(*c));
}
int rtwb14_read_otp(rtwb14_context*c,uint16_t off,uint8_t*out,size_t n){int e;
 if(!c||!out||!n||off>=1024U||n>1024U-off)return EINVAL;
 if((e=begin(c,c->state==RTWB14_FIRMWARE_READY)))return e;
 if((e=finish(c,switch_efuse_bank_88xx_v1(AD(c),HALMAC_EFUSE_BANK_WIFI))))return e;
 /* The vendor multi-byte convenience wrapper rejects the final byte. The
  * checked low-level read accepts the full physical1024-byte region. */
 e=finish(c,read_efuse_88xx_v1(AD(c),off,(u32)n,out));
 if(e)memset(out,0,n);return e;
}
int rtwb14_firmware_download(rtwb14_context*c,const uint8_t*b,size_t n){
 rtwb14_fw_image im;u8*aligned;int e;
 if(!c||c->state!=RTWB14_MAC_READY)return EINVAL;
 if((e=rtwb14_firmware_inspect(b,n,11,&im)))return e;
 if((e=begin(c,0)))return e;
 /* Upstream accesses header words directly: provide aligned owned storage,
  * and never let the non-const vendor ABI mutate the caller's image. */
 aligned=allocate(c,(u32)n);if(!aligned)return c->last_error?c->last_error:ENOMEM;
 memcpy(aligned,b,n);e=finish(c,API(c)->halmac_download_firmware(AD(c),aligned,(u32)n));
 release(c,aligned,(u32)n);
 if(!e&&r16(c,0x80)!=0xc078U)e=fault(c,c->last_error?c->last_error:ENXIO);
 if(!e)state(c,RTWB14_FIRMWARE_READY,1);return e;
}

int rtwb14_phy_claim(rtwb14_context*c,const void*client){
 if(!c||!client||!owner(c))return EINVAL;
 if(bound(c))return c->last_error;
 if(c->state!=RTWB14_FIRMWARE_READY||!c->power_owned)return ENXIO;
 if(owner(c)->phy_client)return EBUSY;
 owner(c)->phy_client=client;return 0;
}
int rtwb14_phy_owned(rtwb14_context*c,const void*client){
 return c&&client&&owner(c)&&!bound(c)&&c->state==RTWB14_FIRMWARE_READY&&
        c->power_owned&&owner(c)->phy_client==client;
}
void rtwb14_phy_release(rtwb14_context*c,const void*client){
 if(c&&client&&owner(c)&&owner(c)->identity==c&&owner(c)->phy_client==client)
  owner(c)->phy_client=NULL;
}

int rtwb14_live_firmware(rtwb14_context*c){
 if(!c||!owner(c))return EINVAL;
 if(bound(c))return c->last_error;
 if(c->last_error)return c->last_error;
 if(c->state!=RTWB14_FIRMWARE_READY||!c->power_owned)return ENXIO;
 return begin(c,1);
}
int rtwb14_register_read(rtwb14_context*c,uint16_t a,unsigned w,uint32_t*out){int e;
 if(!out||(w!=1&&w!=2&&w!=4)||a>65536U-w)return EINVAL;
 const void*client=c&&owner(c)?owner(c)->phy_client:NULL;
 *out=0;if((e=rtwb14_live_firmware(c)))return e;
 if(client&&!rtwb14_phy_owned(c,client))return ENXIO;
 *out=read_reg(c,a,w);if(c->last_error)*out=0;return c->last_error;
}
int rtwb14_register_write(rtwb14_context*c,uint16_t a,unsigned w,uint32_t v){int e;
 if((w!=1&&w!=2&&w!=4)||a>65536U-w)return EINVAL;
 const void*client=c&&owner(c)?owner(c)->phy_client:NULL;
 if((e=rtwb14_live_firmware(c)))return e;
 if(client&&!rtwb14_phy_owned(c,client))return ENXIO;
 write_reg(c,a,w,v);return c->last_error;
}

int rtwb14_delay_us(rtwb14_context*c,uint32_t us){int e;
 if(us>1000000U)return EINVAL;
 const void*client=c&&owner(c)?owner(c)->phy_client:NULL;
 if((e=rtwb14_live_firmware(c)))return e;
 if(client&&!rtwb14_phy_owned(c,client))return ENXIO;
 delay(c,us);return c->last_error;
}
void *rtwb14_allocate(rtwb14_context*c,size_t n){
 if(!c||!owner(c)||bound(c)||!n||n>1048576U)return NULL;
 return allocate(c,(u32)n);
}
void rtwb14_release(rtwb14_context*c,void*p,size_t n){
 if(c&&owner(c)&&owner(c)->identity==c&&p)bus(c)->release(bus(c)->cookie,p,n);
}
