/* SPDX-License-Identifier: GPL-2.0-only
 * DNMTechLabs read-only RTL8822B/C discovery. Hardware codes and pre-power USB read:
 * Realtek HALMAC, RinCat/RTL88x2BU-Linux-Driver@465ae7cb522a382adc85ea5c31f228af8d172148,
 * hal/halmac/halmac_api.c65,72,518-534. Provenance: ChipIdentity-Upstream.json.
 * No power, firmware, RF, register-write or generic unverified-ID fallback.
 */
#include "rtw/ChipIdentity.h"
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
#include <string.h>
#define ID_TAG UINT64_C(0x5254574348495031)
struct rtw_chip_identity {
 uint64_t tag;
 const struct rtw_chip_identity *self;
 const void *bound_context;
 const rtw_usb_wifi_profile *origin;
 rtw_usb_wifi_profile profile;
 rtw_chip_identity_descriptor descriptor;
 rtw88_fw_bus bus;
 uint16_t origin_index;
 uint8_t code,ready;
};
_Static_assert(sizeof(struct rtw_chip_identity)<=RTW_CHIP_IDENTITY_STORAGE_BYTES,"identity caller storage");
static int overlap(const void*a,size_t an,const void*b,size_t bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return an&&bn&&(x<=y?y-x<an:x-y<bn);}
static int positive_errno(int e){return e<0?(e==INT32_MIN?EIO:-e):e;}
static rtw_wifi_family family(uint32_t code){return code==0x0a?RTW_WIFI_RTL8822BU:code==0x13?RTW_WIFI_RTL8822CU:RTW_WIFI_UNKNOWN;}
static int immutable(const rtw_usb_wifi_profile*p){size_t i;for(i=0;i<rtw_usb_wifi_profile_count();++i)if(p==rtw_usb_wifi_profile_at(i))return 1;return 0;}
static int in_scope(const rtw_usb_wifi_profile*p,const rtw_chip_identity_descriptor*d){
 if(!p||!d)return 0;
 if(p->vendor!=0x13b1||p->product!=0x0043||p->primary_verified||p->configuration!=1||p->interface_number!=0)return 0;
 if(d->vendor!=p->vendor||d->product!=p->product||d->configuration!=1||d->interface_number!=0||d->interface_class!=0xff||d->interface_subclass!=0xff||d->interface_protocol!=0xff)return 0;
 return rtw_usb_wifi_lookup_descriptor(d->vendor,d->product,d->bcd_device,d->configuration,d->interface_number,d->interface_class,d->interface_subclass,d->interface_protocol)==p;
}
int rtw_chip_identity_candidate(const rtw_usb_wifi_profile*p,const rtw_chip_identity_descriptor*d){return p&&immutable(p)&&in_scope(p,d);}
size_t rtw_chip_identity_size(void){return sizeof(struct rtw_chip_identity);}
static int same_bus(const rtw88_fw_bus*a,const rtw88_fw_bus*b){return a&&b&&a->cookie==b->cookie&&a->read==b->read&&a->write==b->write&&a->bulk_write==b->bulk_write&&a->delay_us==b->delay_us&&a->coex_read==b->coex_read&&a->coex_write==b->coex_write;}
static int same_profile(const rtw_usb_wifi_profile*a,const rtw_usb_wifi_profile*b){
 return a->vendor==b->vendor&&a->product==b->product&&a->bcd_device==b->bcd_device&&a->match_bcd==b->match_bcd&&a->configuration==b->configuration&&a->interface_number==b->interface_number&&a->primary_verified==b->primary_verified&&a->name==b->name&&a->interface_match_flags==b->interface_match_flags&&a->interface_class==b->interface_class&&a->interface_subclass==b->interface_subclass&&a->interface_protocol==b->interface_protocol&&a->band_restriction==b->band_restriction;
}
static int intact(const rtw_chip_identity*c){
 return c&&c->tag==ID_TAG&&c->self==c&&c->ready&&family(c->code)!=RTW_WIFI_UNKNOWN&&c->profile.family==family(c->code)&&c->origin==rtw_usb_wifi_profile_at(c->origin_index)&&in_scope(c->origin,&c->descriptor)&&same_profile(&c->profile,c->origin);
}
int rtw_chip_identity_valid(const rtw_chip_identity*c,const rtw_usb_wifi_profile*p,const rtw88_fw_bus*b){return intact(c)&&p==&c->profile&&same_bus(&c->bus,b);}
const rtw_usb_wifi_profile*rtw_chip_identity_profile(const rtw_chip_identity*c){return intact(c)?&c->profile:NULL;}
uint8_t rtw_chip_identity_code(const rtw_chip_identity*c){return intact(c)?c->code:0;}
int rtw_chip_identity_bind_context(rtw_chip_identity*c,const void*owner){
 if(!intact(c)||!owner)return EINVAL;
 if(c->bound_context&&c->bound_context!=owner)return EBUSY;
 c->bound_context=owner;return 0;
}
int rtw_chip_identity_bound_to(const rtw_chip_identity*c,const void*owner){return intact(c)&&owner&&c->bound_context==owner;}
static int same_descriptor(const rtw_chip_identity_descriptor*a,const rtw_chip_identity_descriptor*d){
 if(!a||!d)return 0;
 return a->vendor==d->vendor&&a->product==d->product&&a->bcd_device==d->bcd_device&&a->configuration==d->configuration&&a->interface_number==d->interface_number&&a->interface_class==d->interface_class&&a->interface_subclass==d->interface_subclass&&a->interface_protocol==d->interface_protocol&&a->alternate_setting==d->alternate_setting;
}
int rtw_chip_identity_descriptor_matches(const rtw_chip_identity*c,const rtw_chip_identity_descriptor*d){return intact(c)&&same_descriptor(&c->descriptor,d);}
void rtw_chip_identity_forget(rtw_chip_identity*c){if(c)memset(c,0,sizeof(*c));}
int rtw_chip_identity_init(void*storage,size_t n,const rtw_usb_wifi_profile*p,const rtw_chip_identity_descriptor*d,const rtw88_fw_bus*b,rtw_chip_identity**out){
 struct rtw_chip_identity next;uint32_t code=0;int e;
 if(!out||!storage||n<sizeof(next)||(uintptr_t)storage%_Alignof(struct rtw_chip_identity)||!p||!d||!b||!b->read)return EINVAL;
 if(overlap(storage,n,out,sizeof(*out))||overlap(storage,n,p,sizeof(*p))||overlap(storage,n,d,sizeof(*d))||overlap(storage,n,b,sizeof(*b))||overlap(out,sizeof(*out),p,sizeof(*p))||overlap(out,sizeof(*out),d,sizeof(*d))||overlap(out,sizeof(*out),b,sizeof(*b)))return EINVAL;
 *out=NULL;if(!rtw_chip_identity_candidate(p,d))return ENODEV;
 memset(storage,0,sizeof(next));memset(&next,0,sizeof(next));
 next.origin=p;next.profile=*p;next.descriptor=*d;next.bus=*b;
 {size_t i;for(i=0;i<rtw_usb_wifi_profile_count();++i)if(p==rtw_usb_wifi_profile_at(i)){next.origin_index=(uint16_t)i;break;}}
 /* Checked value-return transport, unlike HALMAC's legacy value-only read API.
  * An error cannot masquerade as a successful0A/13 byte. */
 e=positive_errno(next.bus.read(next.bus.cookie,0xfc,1,&code));if(e)return e;
 if(!same_bus(&next.bus,b)||!same_descriptor(&next.descriptor,d))return EINVAL;
 if(code>255U)return EIO;if(family(code)==RTW_WIFI_UNKNOWN)return ENODEV;
 next.tag=ID_TAG;next.self=storage;next.profile.family=family(code);next.code=(uint8_t)code;next.ready=1;
 memcpy(storage,&next,sizeof(next));*out=storage;return 0;
}
int rtw_chip_identity_revalidate(rtw_chip_identity*c,const rtw88_fw_bus*b){uint32_t code=0;rtw88_fw_bus snapshot;int e;
 if(!c)return EINVAL;
 if(!rtw_chip_identity_valid(c,&c->profile,b)){rtw_chip_identity_forget(c);return EINVAL;}
 snapshot=c->bus;e=positive_errno(snapshot.read(snapshot.cookie,0xfc,1,&code));
 if(!e&&(!same_bus(&snapshot,b)||!same_bus(&snapshot,&c->bus)||!intact(c)))e=EINVAL;
 if(!e&&code>255U)e=EIO;if(!e&&code!=c->code)e=ENODEV;
 if(e)rtw_chip_identity_forget(c);return e;
}
