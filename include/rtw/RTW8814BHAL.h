/* SPDX-License-Identifier: GPL-2.0-only
 * RTL8814B USB HALMAC boundary. Realtek algorithms retain their own notices.
 * This bootstrap does not authorize station traffic or substitute RTL8814AU.
 */
#ifndef RTW_RTW8814B_HAL_H
#define RTW_RTW8814B_HAL_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define RTWB14_OTP_BYTES 1024U
#define RTWB14_OTP_PROTECT_BYTES 96U
#define RTWB14_TX_BODY_BYTES 16U
#define RTWB14_TX_IE_BYTES 8U
#define RTWB14_TX_DESC_BYTES 48U
#define RTWB14_RX_DESC_BYTES 24U
/* The owner serializes all calls and callbacks. No operation is IRQ-safe.
 * Bus callbacks return a positive or negative errno; zero means success.
 * allocate/release must be paired, nonblocking driver allocations. send_payload
 * receives bytes WITHOUT a TX descriptor; use rtwb14_control_descriptor and a
 * bounded synchronous transfer. kind0=reserved/beacon, kind1=H2C. */
typedef struct {
 void *cookie;
 int (*read)(void*,uint16_t,unsigned,uint32_t*);
 int (*write)(void*,uint16_t,unsigned,uint32_t);
 int (*send_payload)(void*,unsigned,const uint8_t*,size_t);
 void (*delay_us)(void*,uint32_t);
 void *(*allocate)(void*,size_t);
 void (*release)(void*,void*,size_t);
} rtwb14_bus;
typedef enum {RTWB14_UNBOUND=0,RTWB14_IDENTIFIED,RTWB14_MAC_READY,
 RTWB14_FIRMWARE_READY,RTWB14_FAULT} rtwb14_state;
typedef struct {
 rtwb14_bus bus;
 void *adapter,*api,*private_owner;
 uint32_t operations_left,delay_left_us;
 int last_error;
 uint32_t vendor_status;
 uint8_t state,cut,bulkout_count,power_owned;
} rtwb14_context;
/* Context must start zeroed. Only SYS_CFG2=0x11 is accepted; all other known
 * HALMAC families fail before allocation/writes. Exactly3/4/5 bulk-out pipes.
 * No device endpoint discovery, power or firmware operation occurs in open. */
int rtwb14_open(rtwb14_context*,const rtwb14_bus*,unsigned);
int rtwb14_mac_boot(rtwb14_context*);
/* Explicit power-off requires live chip identity. A transport fault suppresses
 * later I/O and clears authority. close only releases allocations, so it is
 * also safe after detach. Neither path authorizes transmission. */
int rtwb14_power_off(rtwb14_context*);
/* Live register authority for the separate B14 PHY/RFK port. Every access
 * requires owned firmware state and fresh chip/MCU identity; no cached guard. */
/* Pure, serialized PHY lifetime lease. One active client per firmware session;
 * power transitions, faults and close invalidate it. No USB operation. */
int rtwb14_phy_claim(rtwb14_context*,const void*);
int rtwb14_phy_owned(rtwb14_context*,const void*);
void rtwb14_phy_release(rtwb14_context*,const void*);
int rtwb14_live_firmware(rtwb14_context*);
int rtwb14_register_read(rtwb14_context*,uint16_t,unsigned,uint32_t*);
int rtwb14_register_write(rtwb14_context*,uint16_t,unsigned,uint32_t);
int rtwb14_delay_us(rtwb14_context*,uint32_t);
void *rtwb14_allocate(rtwb14_context*,size_t);
void rtwb14_release(rtwb14_context*,void*,size_t);
void rtwb14_close(rtwb14_context*);
/* Physical OTP read is read-only: no programming API is exposed. The bounded
 * decoder preserves extended headers, disabled words and last-write wins. */
int rtwb14_read_otp(rtwb14_context*,uint16_t,uint8_t*,size_t);
int rtwb14_decode_otp(const uint8_t*,size_t,uint8_t*,size_t);
typedef struct {size_t offset,length;uint32_t address;} rtwb14_fw_segment;
typedef struct {
 uint16_t signature,version,h2c_version;
 uint8_t subversion,subindex;
 rtwb14_fw_segment dmem,imem,emem;
} rtwb14_fw_image;
/* Structural parser only: no authenticity, licensing or NIC compatibility
 * assertion. Signature8814 is shared with AU; IMEM0x30000 is mandatory here.
 * ABI11 NIC and ABI15 router images stay distinct. */
int rtwb14_firmware_inspect(const uint8_t*,size_t,uint16_t,rtwb14_fw_image*);
/* Caller supplies an independently validated/licensed USB NIC image. Only
 * ABI11 is accepted by this USB bootstrap. It runs the actual B14 reserved-page
 * +DDMA+MCU flow, then verifies exact MCUFW_CTRL=0xC078. No PHY or station init. */
int rtwb14_firmware_download(rtwb14_context*,const uint8_t*,size_t);
/* Pure codecs, no USB. Output descriptor has body16 + IE0..IE3 (8 each), IE3
 * terminates the chain, and an XOR checksum at body offset8. Reserved/H2C DMA
 * channels follow the B14 mapping, not flat AU/B/C descriptors. */
int rtwb14_control_descriptor(unsigned,size_t,uint8_t*,size_t);
int rtwb14_descriptor_check(const uint8_t*,size_t);
/* Build a complete control transfer; MPS512/1024 only. A zero dummy payload
 * byte avoids a full-MPS terminal packet. The DDMA copy length stays the
 * original firmware segment length. Buffers/output length must not overlap. */
int rtwb14_control_packet(unsigned,const uint8_t*,size_t,uint16_t,uint8_t*,size_t,size_t*);
typedef struct {
 size_t payload_offset,payload_length,record_length;
 uint8_t crc_error,icv_error,phy_status,software_decrypted,c2h,rate;
} rtwb14_rx_info;
int rtwb14_rx_inspect(const uint8_t*,size_t,rtwb14_rx_info*);
#ifdef __cplusplus
}
#endif
#endif
