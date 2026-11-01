/* SPDX-License-Identifier: BSD-3-Clause
 * Compile the actual native adapter in this translation unit so the imported
 * policy's LNA decision can be exercised without adding a production test API.
 * Only the physical register/USB/RF boundary is modeled; MP scheduling,
 * mailbox packing, authority checks and periodic entry points remain real. */
#ifdef COEX_MP_AU
#include "../src/RTL8821AUCoex.c"
#else
#include "../src/RTW88Coex.c"
#endif
#include <assert.h>
#include <stdio.h>

struct mp_bus {
 u8 reg[65536],commands[256][8];
 unsigned calls,count,fail;
 bool fail_lna;
};
static int mp_read(struct mp_bus*b,u16 a,unsigned n,u32*out){
 *out=0;if(++b->calls==b->fail)return EIO;
 for(unsigned j=0;j<n;++j)*out|=(u32)b->reg[a+j]<<(8*j);
 return 0;
}
static int mp_write(struct mp_bus*b,u16 a,unsigned n,u32 value){
 if(++b->calls==b->fail)return EIO;
 if(n==4&&a>=0x1d0&&a<=0x1dc&&b->fail_lna&&
    (value&255U)==0x67&&((value>>16)&255U)==0x32)return EIO;
 for(unsigned j=0;j<n;++j)b->reg[a+j]=(u8)(value>>(8*j));
 if(n==4&&a>=0x1d0&&a<=0x1dc){
  assert(b->count<ARRAY_SIZE(b->commands));
  memcpy(b->commands[b->count],b->reg+a,4);
  memcpy(b->commands[b->count]+4,b->reg+0x1f0+a-0x1d0,4);++b->count;
 }
 return 0;
}
#ifdef COEX_MP_AU
static rtw_usb_error mp_in(void*p,const rtw_usb_setup*s,u8*bytes,u16*n){
 struct mp_bus*b=p;u32 value;int e=mp_read(b,s->w_value,s->w_length,&value);
 if(e)return RTW_USB_IO_ERROR;
 for(unsigned j=0;j<s->w_length;++j)bytes[j]=(u8)(value>>(8*j));
 *n=s->w_length;return RTW_USB_OK;
}
static rtw_usb_error mp_out(void*p,const rtw_usb_setup*s,const u8*bytes,u16*n){
 u32 value=0;for(unsigned j=0;j<s->w_length;++j)value|=(u32)bytes[j]<<(8*j);
 if(mp_write(p,s->w_value,s->w_length,value))return RTW_USB_IO_ERROR;
 *n=s->w_length;return RTW_USB_OK;
}
static int mp_wait(void*p,u32 us){(void)p;(void)us;return 1;}
typedef rtw_r21au_coex_input mp_input;
typedef rtw_r12au_boot mp_boot;
#define MP_START rtw_r21au_coex_start
#define MP_FORGET rtw_r21au_coex_forget
#define MP_STOP rtw_r21au_coex_stop
#else
static int mp_bus_read(void*p,u16 a,unsigned n,u32*out){return mp_read(p,a,n,out);}
static int mp_bus_write(void*p,u16 a,unsigned n,u32 value){return mp_write(p,a,n,value);}
static int mp_indirect_read(void*p,u16 a,u32*out){return mp_read(p,a,4,out);}
static int mp_indirect_write(void*p,u16 a,u32 v){return mp_write(p,a,4,v);}
static void mp_delay(void*p,u32 us){(void)p;(void)us;}
static int mp_mimo(void*p,int enabled){(void)p;(void)enabled;return 0;}
/* These tests do not calibrate RF; the existing PHY/coex suites cover it. */
int rtw88_phy_write_rf(rtw88_phy_context*c,u8 path,u8 a,u32 mask,u32 value){
 (void)a;(void)value;assert(path<c->boot->board.rf_paths&&mask);u32 out;
 return mp_bus_read(c->boot->firmware.bus.cookie,0,4,&out);
}
typedef rtw88_coex_input mp_input;
typedef rtw88_boot_context mp_boot;
#define MP_START rtw88_coex_start
#define MP_FORGET rtw88_coex_forget
#define MP_STOP rtw88_coex_stop
#endif
struct mp_fixture {
 struct mp_bus bus;mp_boot boot;mp_input input;
#ifndef COEX_MP_AU
 rtw88_mac_context mac;rtw88_phy_context phy;
#endif
 _Alignas(max_align_t) u8 storage[8192];struct rtw_dev*c;
};
static void mp_fixture_init(struct mp_fixture*f,unsigned family){
 memset(f,0,sizeof(*f));
#ifdef COEX_MP_AU
 (void)family;f->boot.bus=(rtw_usb_rw_transport){&f->bus,mp_in,mp_out};
 f->boot.waiter=(rtw_waiter){&f->bus,mp_wait};
 f->boot.chip=f->boot.board.phy.chip=RTW_R12AU_CHIP_8821AU;
 f->boot.board.phy.rf_paths=f->boot.board.phy.btcoex=1;
 f->boot.identified=f->boot.powered=f->boot.firmware_running=f->boot.mac_initialized=1;
 f->bus.reg[0x80]=0xc6;f->bus.reg[0x770]=1;
 assert(!rtw_r21au_coex_init(f->storage,sizeof(f->storage),&f->boot,&f->c));
#else
 rtw_wifi_family chip=family==0?RTW_WIFI_RTL8821CU:family==1?RTW_WIFI_RTL8822BU:RTW_WIFI_RTL8822CU;
 f->boot.firmware.family=chip;
 f->boot.firmware.identified=f->boot.firmware.powered=f->boot.firmware.firmware_running=1;
 f->boot.board_valid=f->boot.physical_valid=f->boot.board.features_valid=1;
 f->boot.board.rf_paths=f->boot.board.active_rf_paths=f->boot.board.tx_streams=family?2:1;
 f->boot.board.rfe_option=family==1?3:0;
 f->boot.board.btcoex=1;f->boot.board.rf_board_option=0x20;
 f->boot.firmware.bus=(rtw88_fw_bus){&f->bus,mp_bus_read,mp_bus_write,NULL,mp_delay,mp_indirect_read,mp_indirect_write};
 f->mac.device=&f->boot.firmware;f->mac.mac_ready=1;
 f->phy.boot=&f->boot;f->phy.mac=&f->mac;f->phy.tables_loaded=1;
 f->bus.reg[0x80]=0x78;f->bus.reg[0x81]=0xc0;f->bus.reg[0xaa]=2;f->bus.reg[0x770]=1;
 assert(!rtw88_coex_init(f->storage,sizeof(f->storage),&f->phy,mp_mimo,&f->bus,&f->c));
#endif
 f->input.now_ms=1000;f->input.primary=f->input.center=6;f->input.width_mhz=20;
 f->input.beacon_interval=100;f->input.rssi_percent=60;f->input.rx_rate=f->input.tx_rate=4;
 assert(!MP_START(f->c,&f->input));rtw_coex_set_bt_rx_gain(f->c,7);
 assert(f->c->coex.dm.cur_bt_lna_lvl==7);
 /* Keep a selected policy decision steady during a response. The imported
  * setter, periodic scheduler and public C2H handling are still unmodified. */
 f->c->coex.manual_control=true;
}
static unsigned mp_count(const struct mp_fixture*f,u8 op){
 unsigned count=0;for(unsigned j=0;j<f->bus.count;++j)
  count+=f->bus.commands[j][0]==0x67&&f->bus.commands[j][2]==op;
 return count;
}
static u8 mp_last_lna(const struct mp_fixture*f){
 for(unsigned j=f->bus.count;j;--j)if(f->bus.commands[j-1][0]==0x67&&f->bus.commands[j-1][2]==0x32)
  return f->bus.commands[j-1][3];
 return 255;
}
static void mp_ble(struct mp_fixture*f){
 u8 scan=0;assert(!rtw_coex_get_bt_scan_type(f->c,&scan));
 assert(f->c->mp.pending&&mp_count(f,0x2d)==1&&!mp_count(f,0x32));
}
static int mp_ack(struct mp_fixture*f){
 const u8 response[10]={11,0,1,0,0,1,0,0,0,0};++f->input.now_ms;
#ifdef COEX_MP_AU
 return rtw_r21au_coex_c2h(f->c,response,sizeof(response),f->input.now_ms);
#else
 return rtw88_coex_c2h(f->c,response,sizeof(response),&f->input);
#endif
}
static int mp_tick(struct mp_fixture*f,u64 delta){
 f->input.now_ms+=delta;
#ifdef COEX_MP_AU
 f->input.scanning=1;return rtw_r21au_coex_watchdog(f->c,&f->input);
#else
 return rtw88_coex_poll(f->c,f->input.now_ms);
#endif
}
static void mp_busy_and_coalesce(unsigned family){
 struct mp_fixture f;mp_fixture_init(&f,family);mp_ble(&f);
 rtw_coex_set_bt_rx_gain(f.c,4);rtw_coex_set_bt_rx_gain(f.c,5);rtw_coex_set_bt_rx_gain(f.c,2);
 /* A busy slot cannot claim a hardware change that was never submitted. */
 assert(f.c->coex.dm.cur_bt_lna_lvl==7&&!mp_count(&f,0x32));
 assert(!mp_ack(&f));assert(mp_count(&f,0x32)==1&&mp_last_lna(&f)==2);
 assert(f.c->coex.dm.cur_bt_lna_lvl==2&&f.c->coex.stat.bt_ble_scan_type==1);
 rtw_coex_set_bt_rx_gain(f.c,1);rtw_coex_set_bt_rx_gain(f.c,4);
 assert(f.c->coex.dm.cur_bt_lna_lvl==2&&mp_count(&f,0x32)==1);
 assert(!mp_ack(&f));assert(mp_count(&f,0x32)==2&&mp_last_lna(&f)==4);
 assert(f.c->coex.dm.cur_bt_lna_lvl==4);assert(!mp_ack(&f));
 assert(!mp_tick(&f,2000)&&mp_count(&f,0x32)==2);assert(!MP_STOP(f.c));
}
static void mp_timeout(unsigned family){
 struct mp_fixture f;mp_fixture_init(&f,family);mp_ble(&f);rtw_coex_set_bt_rx_gain(f.c,5);
 assert(!mp_tick(&f,9)&&!mp_count(&f,0x32));
 assert(!mp_tick(&f,1)&&f.c->mp_timeouts==1&&mp_count(&f,0x32)==1&&mp_last_lna(&f)==5);
 assert(!mp_tick(&f,9)&&f.c->mp_timeouts==1&&mp_count(&f,0x32)==1);
 assert(!mp_tick(&f,1)&&f.c->mp_timeouts==2&&!f.c->mp.pending&&mp_count(&f,0x32)==1);
 assert(!mp_tick(&f,2000)&&mp_count(&f,0x32)==1);assert(!MP_STOP(f.c));
}
static void mp_late_ack_recovery(unsigned family){
 for(unsigned late=0;late<2;++late){
  struct mp_fixture f;mp_fixture_init(&f,family);mp_ble(&f);rtw_coex_set_bt_rx_gain(f.c,5);
  /* mp_ack advances one millisecond: exercise ACK exactly at expiry and
   * one millisecond after it, with no intervening periodic timeout. */
  f.input.now_ms=f.c->mp.deadline-1+late;
  unsigned calls=f.bus.calls;int result=mp_ack(&f);
#ifdef COEX_MP_AU
  assert(result==EAGAIN&&!f.c->mp.pending&&f.c->mp_timeouts==1);
#else
  assert(result==EINVAL&&f.c->mp.pending&&!f.c->mp_timeouts);
  assert(rtw88_coex_next_deadline(f.c)&&
         rtw88_coex_next_deadline(f.c)<=f.c->mp.deadline);
#endif
  assert(f.bus.calls==calls&&f.c->active&&!f.c->error&&f.c->mp.lna_pending);
  assert(f.c->coex.dm.cur_bt_lna_lvl==7&&!mp_count(&f,0x32));
  /* AU's watchdog must recover even though its late-ACK path cleared the
   * MP slot. B/C's scheduled poll expires that slot before retrying. */
  assert(!mp_tick(&f,2000)&&f.c->mp_timeouts==1);
  assert(mp_count(&f,0x32)==1&&mp_last_lna(&f)==5);
  assert(f.c->coex.dm.cur_bt_lna_lvl==5&&!f.c->mp.lna_pending&&f.c->mp.pending);
#ifndef COEX_MP_AU
  assert(rtw88_coex_next_deadline(f.c)&&
         rtw88_coex_next_deadline(f.c)<=f.c->mp.deadline);
#endif
  assert(!mp_tick(&f,10)&&f.c->mp_timeouts==2&&!f.c->mp.pending);
  assert(!mp_tick(&f,2000)&&mp_count(&f,0x32)==1);assert(!MP_STOP(f.c));
 }
}
static void mp_cancel(unsigned family){
 struct mp_fixture f;mp_fixture_init(&f,family);mp_ble(&f);
 rtw_coex_set_bt_rx_gain(f.c,4);rtw_coex_set_bt_rx_gain(f.c,7);
 assert(!mp_ack(&f)&&!mp_count(&f,0x32)&&f.c->coex.dm.cur_bt_lna_lvl==7);
 assert(!MP_STOP(f.c));
 mp_fixture_init(&f,family);rtw_coex_set_bt_rx_gain(f.c,2);
 rtw_coex_set_bt_rx_gain(f.c,4);rtw_coex_set_bt_rx_gain(f.c,2);
 assert(!mp_ack(&f)&&mp_count(&f,0x32)==1&&mp_last_lna(&f)==2&&!f.c->mp.lna_pending);
 assert(f.c->coex.dm.cur_bt_lna_lvl==2);assert(!MP_STOP(f.c));
 for(unsigned action=0;action<4;++action){
  mp_fixture_init(&f,family);mp_ble(&f);rtw_coex_set_bt_rx_gain(f.c,4);
  unsigned before=f.bus.count;
  if(action==0)MP_FORGET(f.c);
  else if(action==1)assert(!MP_STOP(f.c));
  else if(action==2){
#ifdef COEX_MP_AU
   f.boot.firmware_running=0;
#else
   f.boot.firmware.firmware_running=0;
#endif
   assert(mp_tick(&f,10)==ENXIO);
  }else{
#ifdef COEX_MP_AU
   f.boot.board.country[0]^=1;
#else
   ++f.boot.power_epoch;
#endif
   assert(mp_tick(&f,10)==ENXIO);
  }
  assert(!f.c->active&&!f.c->mp.pending&&!f.c->mp.lna_pending);
  for(unsigned j=before;j<f.bus.count;++j)assert(f.bus.commands[j][0]!=0x67||f.bus.commands[j][2]!=0x32);
  unsigned calls=f.bus.calls;assert(mp_tick(&f,2000)!=0&&f.bus.calls==calls);
  mp_fixture_init(&f,family);assert(!mp_tick(&f,2000)&&!mp_count(&f,0x32));assert(!MP_STOP(f.c));
 }
}
static void mp_errors(unsigned family){
 {
  struct mp_fixture f;mp_fixture_init(&f,family);f.bus.fail=f.bus.calls+1;
  const struct coex_rf_para para={1,2,true,4};rtw_coex_set_rf_para(f.c,para);
  /* Imported void callbacks continue after a prior failure; their final LNA
   * decision must not reintroduce pending work into the failed context. */
  assert(f.c->error==EIO&&!f.c->active&&!f.c->mp.pending&&!f.c->mp.lna_pending&&!mp_count(&f,0x32));
  assert(f.c->coex.dm.cur_bt_lna_lvl==7);
 }
 for(unsigned failure=0;failure<2;++failure){
  struct mp_fixture f;mp_fixture_init(&f,family);mp_ble(&f);rtw_coex_set_bt_rx_gain(f.c,4);
  if(failure)f.bus.fail_lna=true;else f.bus.fail=f.bus.calls+1;
  assert(mp_ack(&f)==EIO&&!f.c->active&&!f.c->mp.pending&&!f.c->mp.lna_pending);
  assert(!mp_count(&f,0x32)&&f.c->coex.dm.cur_bt_lna_lvl==7);
  unsigned before=f.bus.calls;assert(mp_tick(&f,2000)==EIO&&before==f.bus.calls);
  MP_FORGET(f.c);assert(f.bus.calls==before);
 }
}
static void mp_contexts(unsigned family){
 struct mp_fixture a,b;mp_fixture_init(&a,family);mp_fixture_init(&b,family);
 mp_ble(&a);rtw_coex_set_bt_rx_gain(a.c,4);unsigned before=a.bus.calls;
 assert(!mp_tick(&b,100)&&!mp_count(&b,0x32)&&a.bus.calls==before);
 assert(!mp_ack(&a)&&mp_count(&a,0x32)==1&&!mp_count(&b,0x32));
 MP_FORGET(a.c);assert(!mp_tick(&b,100)&&!mp_count(&b,0x32));assert(!MP_STOP(b.c));
}
int main(void){
#ifdef COEX_MP_AU
 unsigned families=1;
#else
 unsigned families=3;
#endif
 for(unsigned family=0;family<families;++family){
  mp_busy_and_coalesce(family);mp_timeout(family);mp_late_ack_recovery(family);mp_cancel(family);mp_errors(family);mp_contexts(family);
 }
 printf("Coex MP: %u family contexts; busy BLE/LNA admission,latest-level coalescing,ACK/exact-expiry/late-ACK recovery,periodic timeout,error/cancel/reset and context isolation passed\n",families);
 return 0;
}
