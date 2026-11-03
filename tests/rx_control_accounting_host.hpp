/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RX_CONTROL_ACCOUNTING_HOST_HPP
#define RX_CONTROL_ACCOUNTING_HOST_HPP
#include "rtw/RTL8192EUDataPath.h"
#include "rtw/USBWiFiProfiles.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

struct RXControlPacket {
    const uint8_t *mpdu=nullptr;
    size_t mpdu_length=0;
    uint8_t rate_index=8, bandwidth_code=0, crc_error=0, icv_error=0;
    uint8_t phy_status_present=1;
    size_t phy_info_length=32;
};
struct RXControlFrame {
    RXControlPacket payload{};
    uint8_t security_code=0, sw_decrypt_required=0;
};
/* Only association/HT and silicon-rate decisions are host boundaries. Their
 * family dispatch remains distinct; the extracted production block decides
 * all priority, admission, consume-only and error accounting behavior. */
struct RXControlBoundary {
    struct Node { uint8_t ni_bssid[6]{}; } node_;
    struct {
        struct { uint8_t ic_myaddr[6]{}; Node *ic_bss=nullptr;
                 unsigned ic_state=4; } sc_ic;
        unsigned nrxchains=2;
    } core_{};
    struct { unsigned mask=0xff000; } r23Rates_{};
    struct { bool ht=true; } acRateConfig_{};
    struct { struct { bool ht=true; } rates; } duMAC_{};
    struct { uint64_t inputErrors=0; } statsStorage_, *stats_=&statsStorage_;
    rtw_wifi_family family_;
    bool auth_=true, ht_=true, rateAllowed_=true;
    explicit RXControlBoundary(rtw_wifi_family family) : family_(family) {
        core_.sc_ic.ic_myaddr[0]=node_.ni_bssid[0]=2;
        core_.sc_ic.ic_myaddr[5]=1;node_.ni_bssid[5]=2;
        core_.sc_ic.ic_bss=&node_;
    }
    bool authorized() const { return auth_; }
    bool is8723Family() const { return family_==RTW_WIFI_RTL8723BU; }
    bool is8710BUFamily() const { return family_==RTW_WIFI_RTL8710BU; }
    bool is8192FUFamily() const { return family_==RTW_WIFI_RTL8192FU; }
    bool is8812Family() const { return family_==RTW_WIFI_RTL8812AU || family_==RTW_WIFI_RTL8821AU; }
    bool is88Family() const { return family_==RTW_WIFI_RTL8821CU || family_==RTW_WIFI_RTL8822BU || family_==RTW_WIFI_RTL8822CU; }
    bool usesFirmwareRate() const { return ht_ && family_==RTW_WIFI_RTL8192EU; }
    bool usesLegacyHighThroughput() const { return ht_ && (family_==RTW_WIFI_RTL8192CU || family_==RTW_WIFI_RTL8188EU); }
    bool uses8814FirmwareRate() const { return ht_ && family_==RTW_WIFI_RTL8814AU; }
    bool uses88HighThroughput() const { return ht_ && is88Family(); }
    bool uses8812HighThroughput() const { return ht_ && is8812Family(); }
    bool uses8192FUHighThroughput() const { return ht_ && is8192FUFamily(); }
    bool uses8710BUHighThroughput() const { return ht_ && is8710BUFamily(); }
    bool uses8723FirmwareRate() const { return ht_ && is8723Family(); }
    bool uses8192DUFirmwareRate() const { return ht_ && family_==RTW_WIFI_RTL8192DU; }
    bool acceptsLegacyReceiveRate(const RXControlPacket&) const { return rateAllowed_; }
    bool accepts8710BUReceiveRate(const RXControlPacket&) const { return rateAllowed_; }
    bool accepts8192FUReceiveRate(const RXControlPacket&) const { return rateAllowed_; }
    bool accepts8812ReceiveRate(const RXControlPacket&) const { return rateAllowed_; }
    bool accepts88ReceiveRate(const RXControlPacket&) const { return rateAllowed_; }
    static uint64_t nowMS() { return 100; }
};
#endif
