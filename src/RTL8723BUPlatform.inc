/* SPDX-License-Identifier: GPL-2.0-only */
/* Synchronous serialized I/O adaptation; Linux helpers return byte counts. */
#define rtl8xxxu_debug 0
#define dev_dbg(dev,...) ((void)(dev))
#define dev_info(dev,...) ((void)(dev))
#define dev_warn(dev,...) ((void)(dev))
#define dev_err(dev,...) ((void)(dev))
#define usb_sndbulkpipe(dev,ep) (ep)
#define le16_to_cpu(v) (v)
#define le32_to_cpu(v) (v)
#define cpu_to_le16(v) (v)
#define cpu_to_le32(v) (v)
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define fallthrough __attribute__((fallthrough))
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define abs(v) ((v)<0?-(v):(v))
static unsigned r23_shift(u32 mask){unsigned s=0;while(mask&&!(mask&1U)){s++;mask>>=1;}return s;}
#define __ffs(m) r23_shift(m)
#define FIELD_PREP(m,v) (((u32)(v)<<r23_shift(m))&(m))
#define u32_get_bits(v,m) (((v)&(m))>>r23_shift(m))
#define u8_get_bits(v,m) u32_get_bits(v,m)
static void u8p_replace_bits(u8*p,u8 v,u8 m){*p=(u8)((*p&~m)|((v<<r23_shift(m))&m));}
static void r23_delay(struct rtw_8723bu_private*p,u32 us){if(!p->owner->last_error||p->cleanup)p->owner->bus.delay_us(p->owner->bus.cookie,us);}
static u32 r23_read(struct rtw_8723bu_private*p,u16 a,unsigned w){u32 value=0;if(p->cleanup)fail(p->owner,p->owner->bus.read(p->owner->bus.cookie,a,w,&value));else(void)readreg(p->owner,a,w,&value);return value;}
static int r23_write(struct rtw_8723bu_private*p,u16 a,unsigned w,u32 v){int e;if(p->owner->last_error&&!p->cleanup)return-p->owner->last_error;e=p->owner->bus.write(p->owner->bus.cookie,a,w,v);if(e)return-fail(p->owner,e);return(int)w;}
static u8 rtl8xxxu_read8(struct rtw_8723bu_private*p,u16 a){return(u8)r23_read(p,a,1);}
static u16 rtl8xxxu_read16(struct rtw_8723bu_private*p,u16 a){return(u16)r23_read(p,a,2);}
static u32 rtl8xxxu_read32(struct rtw_8723bu_private*p,u16 a){return r23_read(p,a,4);}
static int rtl8xxxu_write8(struct rtw_8723bu_private*p,u16 a,u8 v){return r23_write(p,a,1,v);}
static int rtl8xxxu_write16(struct rtw_8723bu_private*p,u16 a,u16 v){return r23_write(p,a,2,v);}
static int rtl8xxxu_write32(struct rtw_8723bu_private*p,u16 a,u32 v){return r23_write(p,a,4,v);}
static int rtl8xxxu_writeN(struct rtw_8723bu_private*p,u16 a,const u8*b,size_t n){int e;if(p->owner->last_error)return-p->owner->last_error;if(!n||n>4096||a>65536U-n)return-EINVAL;e=p->owner->bus.write_region(p->owner->bus.cookie,a,b,n);return e?-fail(p->owner,e):(int)n;}
static int rtl8xxxu_gen2_h2c_cmd(struct rtw_8723bu_private*p,struct h2c_cmd*command,int n){return-rtw_8723bu_h2c(p->owner,(const u8*)command,(size_t)n);}
static bool rtl8xxxu_is_assoc(struct rtw_8723bu_private*p){return p->owner->associated!=0;}
