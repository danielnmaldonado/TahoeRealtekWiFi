#ifndef RTW_RATE_CONTROL_HOST_HPP
#define RTW_RATE_CONTROL_HOST_HPP
/* Host dependency closure for the real three controller methods and the
 * real vendor AMRR implementation. No USB/radio completion is fabricated. */
#include <cstdint>
#include "rtw/DiagnosticPublishPolicy.hpp"
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <map>
#include <string>
#include "rtw/RTL8192EUDataPath.h"
#include "rtw/RTL8192CDataPath.h"
#include "rtw/RTL8814AUDataPath.h"
#include "rtw/RTL8814AUPHY.h"
#include "rtw/RTL8812AURuntime.h"
#include "rtw/RTW88RX.h"
#include "rtw/RTL8192DUStation.h"
#include "rtw/RTL8192DUDataPath.h"
#include "rtw/RTL8192FU.h"
#include "rtw/RTL8710BU.h"
#include "rtw/RTL8723BUDataPath.h"
#include "rtw/USBWiFiProfiles.h"
#include "rtw/RTL8192EUHT.h"
#include "rtw/RTL8192EU.h"
#include "vendor/itl80211/openbsd/net80211/ieee80211_amrr.h"
#define IEEE80211_RATE_VAL 0x7f
#define IEEE80211_S_RUN 4
#define IEEE80211_F_RSNON 0x100U
#define IEEE80211_F_PSK 0x200U
#define IEEE80211_NODE_HT 0x0400U
#define IEEE80211_BA_AGREED 2
#define IEEE80211_ADDR_COPY(a,b) std::memcpy(a,b,6)
#define IEEE80211_ADDR_EQ(a,b) (std::memcmp(a,b,6)==0)
#define nitems(a) (sizeof(a)/sizeof((a)[0]))
#define DPRINTF(x) ((void)0)
struct ieee80211_rateset { uint8_t rs_nrates = 0, rs_rates[15]{}; };
struct ieee80211_node {
    ieee80211_rateset ni_rates;
    uint8_t ni_txrate = 0, ni_bssid[6]{2,0,0,0,0,1};
    bool ni_port_valid = true;
    unsigned ni_rsn_supp_state = 0, ni_rsnprotos = 0, ni_rsncipher = 0;
    unsigned ni_flags = 0;
    struct { unsigned ba_state=0; } ni_tx_ba[16],ni_rx_ba[16];
};
struct ieee80211com {
    int ic_state = IEEE80211_S_RUN;
    ieee80211_node *ic_bss = nullptr;
    unsigned ic_flags = 0, ic_assoc_status = 0, ic_deauth_reason = 0, ic_protmode = 0;
    struct { uint32_t is_rx_eapol_key = 0, is_rx_eapol_replay = 0, is_rx_eapol_badmic = 0; } ic_stats;
};
struct host_8814_phy_trace {unsigned calls=0;uint8_t rate=0;};
inline host_8814_phy_trace acPhyTrace;
inline int rtw_8814au_phy_set_tx_rate(rtw_8814au *,uint8_t rate) {++acPhyTrace.calls;acPhyTrace.rate=rate;return 0;}
class RTWUSBController {
public:
    rtw_wifi_family family_ = RTW_WIFI_RTL8192EU;
    bool running_ = true, stopping_ = false, rateTransportFresh_ = false;
    bool joinRequested_ = false;
    struct { ieee80211com sc_ic; rtw_92e_rate_config rate_config{}; uint8_t fw_station_connected=0; } core_;
    // Other hardware backends are isolated scheduling/readiness boundaries.
    rtw_r12au_boot auCore_{};
    rtw_r12au_rate_config auRateConfig_{};
    rtw88_rx_phy_config r88RX_{};
    rtw88_station_rate r88Rates_{};
    rtw_92du_rom duROM_{};rtw_92du_mac duMAC_{};bool duFirmwareReady_=false;
    bool uses8192DUFirmwareRate()const{return family_==RTW_WIFI_RTL8192DU&&duFirmwareReady_;}
    rtw_packet_error decode8192DUUSBFrame(rtw_rx_iterator*,rtw_8192eu_rx_frame*,uint8_t*)const;
    rtw_8723bu r23Core_{};rtw_8723bu_rates r23Rates_{};bool r23FirmwareReady_=false;
    bool is8723Family()const{return family_==RTW_WIFI_RTL8723BU;}
    bool uses8723FirmwareRate()const{return is8723Family()&&r23FirmwareReady_;}
    rtw_packet_error decode8723USBFrame(rtw_rx_iterator*,rtw_8192eu_rx_frame*,uint8_t*)const;
    rtw_r10bu_rate buRate_{};bool buFirmwareReady_=false;
    bool is8710BUFamily()const{return family_==RTW_WIFI_RTL8710BU;}
    bool uses8710BUFirmwareRate()const{return is8710BUFamily()&&buFirmwareReady_;}
    bool uses8710BUHighThroughput()const{return uses8710BUFirmwareRate()&&buRate_.ht;}
    rtw_packet_error decode8710BUUSBFrame(rtw_rx_iterator*,rtw_8192eu_rx_frame*,uint8_t*)const;
    rtw_r92fu_rate fuRate_{};bool fuFirmwareReady_=false;
    bool is8192FUFamily()const{return family_==RTW_WIFI_RTL8192FU;}
    bool uses8192FUFirmwareRate()const{return is8192FUFamily()&&fuFirmwareReady_;}
    bool uses8192FUHighThroughput()const{return uses8192FUFirmwareRate()&&fuRate_.ht;}
    rtw_packet_error decode8192FUUSBFrame(rtw_rx_iterator*,rtw_8192eu_rx_frame*,uint8_t*)const;
    bool auFirmwareReady_=false,r88FirmwareReady_=false;
    bool is8812Family()const{return family_==RTW_WIFI_RTL8812AU||family_==RTW_WIFI_RTL8821AU;}
    bool is88Family()const{return family_==RTW_WIFI_RTL8821CU||family_==RTW_WIFI_RTL8822BU||family_==RTW_WIFI_RTL8822CU;}
    bool uses8812FirmwareRate()const{return is8812Family()&&auFirmwareReady_;}
    bool uses8812HighThroughput()const{return uses8812FirmwareRate()&&auRateConfig_.ht;}
    bool uses88FirmwareRate()const{return is88Family()&&r88FirmwareReady_;}
    rtw_8814au acCore_{};
    rtw_8814au_rate_config acRateConfig_{};
    bool acRateReady_=false,acMediaConnected_=false;
    uint64_t txHTPackets_=0,txAMPDUPackets_=0,rxBARPackets_=0;
    struct RateControl {
        ieee80211_amrr policy{};
        ieee80211_amrr_node node{};
        uint64_t requests = 0, pendingReports = 0, reports = 0;
        uint64_t failures = 0, retried = 0, invalid = 0, ignored = 0;
        uint64_t repeatedSequences = 0, changes = 0, resets = 0;
        uint64_t htSampleCounter = 0;
        uint8_t bssid[6]{}, lastSequence = 0, finalRate = 0;
        bool active = false, seenSequence = false, requestedSinceReset = false;
    } rateControl_{};
    std::map<std::string,uint64_t> properties;
    unsigned propertyWrites=0;
    bool authorized() const { return core_.sc_ic.ic_bss && core_.sc_ic.ic_bss->ni_port_valid; }
    void setProperty(const char *name,uint64_t value,unsigned) {
        properties[name]=value;
        ++propertyWrites;
    }
    void publishDiagnosticCounter(const char *name, uint64_t value) {
        const auto current = properties.find(name);
        if (current == properties.end() ||
            rtw::diagnosticCounterNeedsWrite(true, 64, current->second, value)) {
            setProperty(name, value, 64);
        }
    }
    void resetRateControl();
    void handleTxReport(const uint8_t *,size_t);
    void chooseRateControl();
    void publishRateControlDiagnostics();
    bool usesFirmwareRate() const;
    bool usesLegacyFirmwareRate() const;
    bool uses8814FirmwareRate() const;
    void handle8814C2H(const uint8_t *,size_t);
    rtw_packet_error decodeUSBFrame(rtw_rx_iterator *,rtw_8192eu_rx_frame *,uint8_t *) const;
    rtw_packet_error decode8812USBFrame(rtw_rx_iterator*,rtw_8192eu_rx_frame*,uint8_t*)const;
    rtw_packet_error decode88USBFrame(rtw_rx_iterator*,rtw_8192eu_rx_frame*,uint8_t*)const;
    void publishWpaDiagnostics();
};
#endif
