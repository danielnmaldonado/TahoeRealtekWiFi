    enum AUStage : uint32_t {
        AUIdle=0, AUAllocate=1, AUPower=2, AUFirmware=3, AUMAC=4,
        AUPHYContext=5, AUPHYTables=6, AUChannel=7, AURFK=8,
        AUStation=9, AURX=10, AUStartupInfo=11, AUScan=12, AUReady=13,
        AUStop=14, AURecoveryDrain=15, AURecoveryPowerOff=16,
        AURecoveryIdentity=17, AURecoveryReady=18, AUThermal=19
    };
    struct AURecoveryDiagnostics {
        uint64_t starts=0, failures=0, recoveryAttempts=0, recoverySuccesses=0;
        uint32_t stage=AUIdle, result=0, failureStage=AUIdle, failureResult=0;
        uint32_t firstFailureStage=AUIdle, firstFailureResult=0;
        uint32_t cleanupResult=0, identityResult=0, consecutiveRecoveries=0;
        uint64_t periodicCalls=0, periodicPHYFailures=0;
        uint64_t periodicRSSIAttempts=0, periodicRSSIBusy=0, periodicRSSIFailures=0;
        /* Phase: 0=not run, 1=PHY/thermal, 2=advisory firmware RSSI. */
        uint32_t periodicPhase=0, periodicResult=0;
        uint32_t periodicLastFailurePhase=0, periodicLastFailureResult=0;
    } acRecoveryDiag_{};
    rtw_8814au_board acOriginalBoard_{};
    rtw_usb_endpoints acOriginalEndpoints_{};
    rtw_chip_identity_descriptor acOriginalDescriptor_{};
    const rtw_usb_wifi_profile *acOriginalProfile_=nullptr;
    const void *acOriginalUSB_=nullptr;
    bool acIdentitySaved_=false, acFailureEligible_=false, acRecoveryBlocked_=false;
    bool acStarting_=false, acRecoveryPrepared_=false, acRecoveryRestartPending_=false;
    bool capture8814RecoveryIdentity();
    bool matches8814RecoveryIdentity() const;
    void record8814Failure(uint32_t, int);
    void publish8814RecoveryDiagnostics();
    int recover8814ForControl();
    void finish8814StopAfterDrain();
