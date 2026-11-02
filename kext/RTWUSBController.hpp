#pragma once
/* RTL8192EU USB station controller. The imported net80211 stack owns RSN,
 * software CCMP, replay checks and controlled-port authorization. */
#include <IOKit/network/IOEthernetController.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <kern/thread_call.h>
#include <IOKit/usb/IOUSBHostInterface.h>
#include <IOKit/usb/IOUSBHostPipe.h>
#include "RTW8192EUHAL.hpp"
#include "RTWControl.h"
#include "rtw/USBDescriptors.h"
#include "rtw/RTL8192EUDataPath.h"
#include "rtw/USBPackets.h"
#include "rtw/USBRXCapacity.h"
#include "rtw/USBWiFiProfiles.h"
#include "rtw/FamilyBackend.h"
#include "rtw/SignalSample.h"
#include "rtw/DFSStation.h"
#include "rtw/ScanDiagnostics.hpp"
#include "rtw/StationRecovery.hpp"
#include "rtw/USBWiFiFirmware.h"
#include "rtw/RTL8192CDataPath.h"
#include "rtw/RTL8192EU.h"
#include "rtw/RTL8814AU.h"
#include "rtw/RTL8814AUPHY.h"
#include "rtw/RTL8814AUDataPath.h"
#include "rtw/RTL8814AUStation.h"
#include "rtw/RTL8812AURuntime.h"
#include "rtw/RTL8821AUCoex.h"
#include "rtw/RTL8192FU.h"
#include "rtw/RTL8710BU.h"
#include "rtw/RTL8723BUDataPath.h"
#include "rtw/RTL8192DUStation.h"
#include "rtw/RTL8192DUUSB.h"
#include "rtw/RTL8192DUDataPath.h"
#include "rtw/RTW88PHYOperating.h"
#include "rtw/RTW88Coex.h"
#include "rtw/RTW88Station.h"
#include "rtw/RTW88RX.h"
#include "rtw/RTW88USBDataPath.h"
#include <net80211/ieee80211_amrr.h>

class RTWUSBController final : public IOEthernetController {
    OSDeclareDefaultStructors(RTWUSBController)
public:
    bool start(IOService *) override;
    void stop(IOService *) override;
    void free() override;
    IOWorkLoop *getWorkLoop() const override;
    IOReturn getHardwareAddress(IOEthernetAddress *) override;
    IOReturn enable(IONetworkInterface *) override;
    IOReturn disable(IONetworkInterface *) override;
    UInt32 outputPacket(mbuf_t, void *) override;
    UInt32 getFeatures() const override;
    IOReturn getPacketFilters(const OSSymbol *, UInt32 *) const override;
    IOReturn setMulticastMode(bool) override;
    IOReturn setMulticastList(IOEthernetAddress *, UInt32) override;
    IOReturn selectMedium(const IONetworkMedium *) override;
    bool setLinkStatus(UInt32, const IONetworkMedium * = nullptr,
                       UInt64 = 0, OSData * = nullptr) override;
    IOReturn message(UInt32, IOService *, void * = nullptr) override;
    IOReturn setPowerState(unsigned long, IOService *) override;
    IOReturn newUserClient(task_t, void *, UInt32, OSDictionary *, IOUserClient **) override;
    IOReturn controlRequest(uint32_t, const void *, size_t, void *, size_t *);
protected:
    IOReturn registerWithPolicyMaker(IOService *) override;
    bool configureInterface(IONetworkInterface *) override;
    IOOutputQueue *createOutputQueue() override;
private:
    enum { RXSlots = 4, TXCapacity = 4352 };
    enum GateOperation { Initialize = 1, Shutdown, Enable, Disable, Output,
                         Control, Terminated, Sleep, Wake, TeardownReady };
    struct Transfer {
        RTWUSBController *owner;
        IOBufferMemoryDescriptor *memory;
        IOUSBHostPipe *pipe;
        unsigned index;
        uint32_t submitted, actual, generation;
        IOReturn result;
        bool prepared;
    };
    struct OutputCall { mbuf_t packet; bool consumed; };
    struct ControlCall { uint32_t selector; const void *input; size_t length;
                         void *output; size_t *outputLength; };
    IOUSBHostInterface *usb_ = nullptr;
    IOUSBHostPipe *rxPipe_ = nullptr;
    IOUSBHostPipe *txPipes_[4]{};
    uint16_t txMPS_[4]{};
    rtw_usb_endpoints endpoints_{};
    IOWorkLoop *loop_ = nullptr;
    IOCommandGate *gate_ = nullptr;
    IOInterruptEventSource *completionEvent_ = nullptr;
    IOTimerEventSource *scanTimer_ = nullptr, *calibTimer_ = nullptr, *watchdogTimer_ = nullptr;
    IOSimpleLock *completionLock_ = nullptr;
    IOLock *lifecycleLock_ = nullptr;
    Transfer transfers_[RTW_TRANSFER_SLOTS]{};
    rtw_transfer_ledger ledger_{};
    /* Only the observed zero-byte RX transaction error is eligible, and only
     * on integrated station backends sharing this IOUSBHost RX transport.
     * All old endpoint reads must drain before a new RX epoch is submitted. */
    struct RXRecovery {
        enum Stage : uint32_t { Idle=0, Clear=1, RetryWait=2, Drain=3, Refill=4, Complete=5, Failed=6, Cancelled=7 };
        uint64_t errors = 0, attempts = 0, completed = 0, failures = 0, exhausted = 0;
        uint64_t clearAttempts = 0, clearFailures = 0, refillFailures = 0, transportRetries = 0, transportExhausted = 0;
        /* Initial station reads roll back synchronously; a later explicit start
         * owns the retry. Stop/ledger resets must not replenish this budget. */
        uint64_t initialErrors = 0, initialClearAttempts = 0, initialFailures = 0, initialExhausted = 0;
        uint32_t initialConsecutive = 0;
        IOReturn initialOriginalError = kIOReturnSuccess, initialClearResult = kIOReturnSuccess;
        /* Old startup completions remain attributable after EBUSY returns.
         * Each recorded status must be validated before ownership is reused. */
        uint32_t initialDrainMask = 0, initialDrainGeneration[RXSlots] = {0};
        IOReturn initialDrainError = kIOReturnSuccess;
        /* Ordinary shutdown must retain callback provenance too. These slots
         * are owned until their recorded metadata is validated and reclaimed. */
        uint32_t stopDrainMask = 0, stopDrainGeneration[RTW_TRANSFER_SLOTS] = {0};
        IOReturn stopDrainError = kIOReturnSuccess;
        uint64_t stopDrainFailures = 0;
        uint64_t startedMS = 0, nextAttemptMS = 0;
        uint32_t consecutive = 0, clearAttempt = 0, refillSlot = RXSlots;
        IOReturn originalError = kIOReturnSuccess, lastError = kIOReturnSuccess;
        IOReturn clearResult = kIOReturnSuccess, refillResult = kIOReturnSuccess;
        Stage stage = Idle, failureStage = Idle;
        uint8_t endpoint = 0;
        bool pending = false, cleared = false, refilling = false;
    } rxRecovery_{};
    rtwn_softc core_{};
    rtw_signal_sample signal_{};
    rtw::ScanDiagnostics scanDiag_{};
    uint64_t diagnosticPublicationGeneration_ = 0, diagnosticPublicationMonotonicMS_ = 0;
    const rtw_usb_wifi_profile *profile_ = nullptr;
    rtw_wifi_family family_ = RTW_WIFI_UNKNOWN;
#include "RTW8812BackendMembers.inc"
#include "RTW88BackendMembers.inc"
#include "RTW8192DUBackendMembers.inc"
#include "RTW8192FUBackendMembers.inc"
#include "RTW8710BUBackendMembers.inc"
#include "RTW8723BUBackendMembers.inc"
    rtw_8814au acCore_{};
    void *acPHYStorage_ = nullptr;
    size_t acPHYSize_ = 0;
    rtw_8814au_rate_config acRateConfig_{};
    bool acMediaConnected_ = false, acRateReady_ = false;
    uint8_t acRSSI_ = 0, acChannel_ = 0;
#include "RTW8814RecoveryMembers.inc"
#include "RTWFamilyRecoveryMembers.inc"
    device device_{};
    IOEthernetInterface *netif_ = nullptr;
    IONetworkStats *stats_ = nullptr;
    IOEthernetAddress mac_{};
    IOReturn usbError_ = kIOReturnSuccess;
    uint64_t txPackets_ = 0, rxPackets_ = 0, txErrors_ = 0, rxErrors_ = 0;
    uint64_t txQueueDrops_ = 0, txUSBTransferErrors_ = 0, txLinkDrops_ = 0;
    struct RateControl {
        ieee80211_amrr policy{};
        ieee80211_amrr_node node{};
        uint64_t requests = 0, pendingReports = 0, reports = 0;
        uint64_t failures = 0, retried = 0, invalid = 0, ignored = 0;
        uint64_t repeatedSequences = 0, changes = 0, resets = 0, htSampleCounter = 0;
        uint8_t bssid[6]{}, lastSequence = 0, finalRate = 0;
        uint8_t hostHTRates[20]{}, hostHTCount = 0, hostHTIndex = 0;
        bool active = false, seenSequence = false, requestedSinceReset = false;
    } rateControl_{};
    bool rateTransportFresh_ = false;
    uint64_t txHTPackets_ = 0, txAMPDUPackets_ = 0, rxBARPackets_ = 0;
    static int ampduRXStart(ieee80211com *, ieee80211_node *, uint8_t);
    static int ampduTXStart(ieee80211com *, ieee80211_node *, uint8_t);
    static void ampduRXStop(ieee80211com *, ieee80211_node *, uint8_t);
    static void ampduTXStop(ieee80211com *, ieee80211_node *, uint8_t);
    bool usesFirmwareRate() const;
    bool usesLegacyFirmwareRate() const;
    bool usesLegacyHighThroughput() const;
    bool acceptsLegacyReceiveRate(const rtw_rx_packet &) const;
    uint8_t legacyHostHTRate() const;
    rtw_packet_error decodeUSBFrame(rtw_rx_iterator *, rtw_8192eu_rx_frame *, uint8_t *) const;
    struct RXDiagnostics {
        uint64_t aggregates = 0, bytes = 0, aggregateErrors = 0, c2h = 0;
        /* Nonempty successful reads; error length refers to structural preflight. */
        uint64_t lastReadBytes = 0, maxReadBytes = 0, lastAggregateErrorReadBytes = 0;
        /* Reason of failed aggregate preflight; no packet data exposed. */
        uint64_t aggregateTruncated = 0, aggregateMalformed = 0;
        uint64_t aggregateTooMany = 0, aggregateBadArgument = 0;
        uint64_t lastAggregateErrorOffset = 0, lastAggregateErrorFrames = 0;
        /* Frames in a structurally valid prefix of a damaged transfer;
         * subject to later CRC, security and net80211 admission checks. */
        uint64_t partialAggregates = 0, partialPrefixFrames = 0;
        uint64_t partialPrefixBytes = 0;
        uint64_t crcDrops = 0, icvDrops = 0, invalidFrameDrops = 0;
        uint64_t hardwareDecryptDrops = 0, allocationDrops = 0;
        uint64_t copyDrops = 0, noNodeDrops = 0, protectedMPDUs = 0;
        uint64_t ethernetQueued = 0, handoffErrors = 0;
    } rxDiag_{};
    /* Observed associated downlink MPDUs before software decrypt/replay checks.
     * Cumulative raw MPDU counts/bytes are not delivered Ethernet throughput. */
    struct PerformanceDiagnostics {
        uint64_t rxRatePackets[12]{}, rxRateBytes[12]{};
        uint64_t rxMCSPackets[16]{}, rxMCSBytes[16]{};
        uint64_t rxVHTPackets[20]{},rxVHTBytes[20]{};
        uint64_t rxVHT20Packets=0,rxVHT40Packets=0,rxVHT80Packets=0,rxVHTUnknownBWPackets=0;
        uint64_t rxUnknownPackets = 0, rxUnknownBytes = 0;
        uint64_t rxHT20Packets = 0, rxHT40Packets = 0, rxHTUnknownBWPackets = 0;
        uint8_t rxLastCode = 255, rxLastRate500Kbps = 0;
    } perfDiag_{};
    uint64_t lastBeaconMs_ = 0;
    rtw_dfs_station dfsStation_{};
    uint8_t dfsTunedChannel_ = 0, dfsBoundChannel_ = 0, dfsBoundBSSID_[6]{};
    bool dfsBound_ = false, dfsTXHold_ = false;
    uint8_t dfsAppliedReduction_ = 0;
    int32_t dfsAppliedLimitMBM_ = INT32_MAX;
    int prepareDFSPower(unsigned);
    int candidateDFSPower(uint8_t,int32_t *,uint8_t *) const;
    bool dfsTXBlocked() const;
    bool dfsStateAllowed(ieee80211_state,unsigned) const;
    void dfsStateChanged(ieee80211_state,unsigned,int);
    bool dfsFrameAllowed(mbuf_t,ieee80211_node *) const;
    void observeDFSFrame(const uint8_t *,size_t,unsigned);
    int pauseDFSStationTX();
    void advanceDFSRecovery();
    void resetDFSStation(bool releaseTX = false);
    void stopDFSStation();
    struct CalibrationDiagnostics {
        uint64_t calls = 0, generation = 0, lastStartMS = 0;
        uint64_t lastMS = 0, maxMS = 0, thermalChanges = 0, thermalIQs = 0;
        uint64_t lastH2CPolls = 0, maxH2CPolls = 0, totalH2CPolls = 0;
        uint64_t lastH2CReadMS = 0, maxH2CReadMS = 0;
        uint32_t thermalBefore = 0, thermalAfter = 0, thermalState = 0;
        bool active = false;
    } calibDiag_{};
    enum WPARecoveryState { WPARecoveryIdle, WPARecoveryHandshake,
                            WPARecoverySearching, WPARecoveryAuthorized,
                            WPARecoveryFailed };
    struct WPARecovery {
        uint64_t lastMS = 0, remainingMS = 0, timeouts = 0;
        uint32_t retries = 0, state = WPARecoveryIdle, error = 0;
        uint32_t lastSupplicantState = 0;
        bool active = false;
    } wpaRecovery_{};
    bool usbOpened_ = false, present_ = false, runtimeLease_ = false;
    bool attached_ = false, running_ = false, enabled_ = false;
    bool radioReady_ = false, joinRequested_ = false, sleeping_ = false;
    bool browseScanComplete_ = false;
    bool stopping_ = false, powerManaged_ = false, retainedForPending_ = false;
    bool powerRegistrationAttempted_ = false;
    IOReturn powerRegistrationResult_ = kIOReturnNotReady;
    void stopPowerManagement();
    bool macReady_ = false, closing_ = false;
    thread_call_t cleanupCall_ = nullptr;
    bool teardownReady_ = false, cleanupQueued_ = false;
    void markTeardownReady();
    void scheduleDeferredCleanup();
    static void deferredCleanup(thread_call_param_t, thread_call_param_t);

    static IOReturn gated(OSObject *, void *, void *, void *, void *);
    static void usbCompletion(void *, void *, IOReturn, uint32_t);
    static void completionAction(OSObject *, IOInterruptEventSource *, int);
    static void scanAction(OSObject *, IOTimerEventSource *);
    static void calibAction(OSObject *, IOTimerEventSource *);
    static void watchdogAction(OSObject *, IOTimerEventSource *);
    static RTWUSBController *owner(void *);
    static uint8_t read1(void *, uint16_t);
    static uint16_t read2(void *, uint16_t);
    static uint32_t read4(void *, uint16_t);
    static void write1(void *, uint16_t, uint8_t);
    static void write2(void *, uint16_t, uint16_t);
    static void write4(void *, uint16_t, uint32_t);
    static void delayUS(void *, uint32_t);
    static int writeRegion(void *, uint16_t, const uint8_t *, size_t);
    static int ioError(void *);
    static int busPower(void *), busDMA(void *), busFirmware(void *, int, uint8_t *, int);
    static int loadFirmware(void *, u_char **, size_t *);
    static void busMAC(void *), busBB(void *), busAggregation(void *);
    static int busAllocate(void *), busInitialize(void *), busBusy(void *);
    static void busStop(void *), nextScan(void *), cancelScan(void *);
    static void nextCalibration(void *), cancelCalibration(void *), waitAsync(void *);
    static int transmit(void *, mbuf_t, ieee80211_node *);
    static int stateTransition(ieee80211com *, ieee80211_state, int);
    static int scanComplete(ieee80211com *);
    void observeScanTransition(ieee80211_state,ieee80211_state,unsigned,int);
    static int acRead(void *, uint16_t, unsigned, uint32_t *);
    static int acWrite(void *, uint16_t, unsigned, uint32_t);
    static int acBulk(void *, uint8_t, const uint8_t *, size_t);
    rtw_8814au_speed acSpeed() const;
    int attach8814(), start8814(), set8814Channel(uint8_t);
    void stop8814();
    int state8814(ieee80211com *, ieee80211_state, int);
    static void update8814EDCA(ieee80211com *);
    static void watchdog8814(_ifnet *), initTask8814(void *);
    static int ioctl8814(_ifnet *, u_long, caddr_t);
    rtw_8814au_station_params station8814Params() const;
    bool uses8814FirmwareRate() const;
    void handle8814C2H(const uint8_t *, size_t);

    bool discoverUSB();
    void reportStartStage(const char *, IOReturn = kIOReturnSuccess);
    IOReturn initializePlatform();
    IOReturn enableStation();
    void quiesce(bool eraseProfile);
    void shutdownPlatform();
    void closeUSBProvider();
    void releasePlatform();
    void eraseProfile();
    void fault(IOReturn);
    uint32_t readRegister(uint16_t, uint8_t);
    int writeRegister(uint16_t, uint8_t, uint32_t);
    int vendorRequest(bool, uint16_t, uint8_t *, size_t);
    int allocateTransfers();
    void freeTransfers();
    int submit(unsigned, IOUSBHostPipe *, uint32_t);
    bool beginRXRecovery(IOReturn);
    void advanceRXRecovery();
    void failRXRecovery(bool exhausted = false);
    void publishRXRecoveryDiagnostics();
    void recordCompletion(Transfer *, IOReturn, uint32_t, uint32_t);
    IOCommandGate *copyGate(IOWorkLoop **);
    static uint64_t nowMS();
    void publishCalibrationDiagnostics();
    bool associatedBeacon(const uint8_t *, size_t) const;
    void drainCompletions(bool);
    void serviceTXOutputQueue();
    void abortTransfers();
    void receive(const uint8_t *, size_t);
    void publishDiagnosticCounter(const char *, uint64_t);
    void publishDiagnostics();
    void publishRateControlDiagnostics();
    void publishScanDiagnostics();
    void publishWpaDiagnostics();
    void observeReceivedRate(const rtw_rx_packet &);
    void publishPerformanceDiagnostics();
    void resetWpaRecovery();
    void cancelWpaRecovery();
    void wpaStateChanged(ieee80211_state);
    void advanceWpaRecovery();
    void publishWpaRecoveryDiagnostics();
    int sendFrame(mbuf_t, ieee80211_node *);
    void resetRateControl();
    void handleTxReport(const uint8_t *, size_t);
    void chooseRateControl();
    bool hasFreeTX() const;
    bool authorized() const;
    uint32_t supported5GHzFlags() const;
    bool permitsDFSChannelByCountry(uint8_t) const;
    int16_t factoryDFSChannelLimitMBM(uint8_t) const;
    IOReturn handleControl(ControlCall *);
    void status(RTWControlStatus *);
    void scanResults(RTWControlScanResults *);
    void channelMap(RTWControlChannelMap *) const;
};
