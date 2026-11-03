/* SPDX-License-Identifier: BSD-3-Clause
 * A manual SCAN/JOIN can retry a recorded AU hardware timeout only after a
 * complete checked power-off, USB drain and unchanged read-only identity. */
static bool same8814Descriptor(const rtw_chip_identity_descriptor &a,
                               const rtw_chip_identity_descriptor &b)
{
    return a.vendor==b.vendor&&a.product==b.product&&a.bcd_device==b.bcd_device&&
        a.configuration==b.configuration&&a.interface_number==b.interface_number&&
        a.interface_class==b.interface_class&&a.interface_subclass==b.interface_subclass&&
        a.interface_protocol==b.interface_protocol&&a.alternate_setting==b.alternate_setting;
}
bool RTWUSBController::capture8814RecoveryIdentity()
{
    if(family_!=RTW_WIFI_RTL8814AU||!profile_||profile_->family!=family_||
       !usb_||!acCore_.board_valid||!read88IdentityDescriptor(&acOriginalDescriptor_))return false;
    acOriginalBoard_=acCore_.board;acOriginalEndpoints_=endpoints_;
    acOriginalProfile_=profile_;acOriginalUSB_=usb_;acIdentitySaved_=true;
    return true;
}
bool RTWUSBController::matches8814RecoveryIdentity() const
{
    rtw_chip_identity_descriptor current{};
    return acIdentitySaved_&&family_==RTW_WIFI_RTL8814AU&&profile_==acOriginalProfile_&&
        profile_&&profile_->family==family_&&usb_==acOriginalUSB_&&
        acCore_.bus.cookie==this&&acCore_.bus.read==acRead&&acCore_.bus.write==acWrite&&
        acCore_.bus.bulk_write==acBulk&&acCore_.bus.delay_us==delayUS&&
        acCore_.board_valid&&acCore_.speed==acSpeed()&&
        !memcmp(&endpoints_,&acOriginalEndpoints_,sizeof(endpoints_))&&
        !memcmp(acCore_.board.logical,acOriginalBoard_.logical,sizeof(acOriginalBoard_.logical))&&
        read88IdentityDescriptor(&current)&&same8814Descriptor(current,acOriginalDescriptor_);
}
void RTWUSBController::record8814Failure(uint32_t stage,int error)
{
    if(!error)return;
    auto&d=acRecoveryDiag_;d.failureStage=stage;d.failureResult=(uint32_t)error;++d.failures;
    if(!d.firstFailureResult){d.firstFailureStage=stage;d.firstFailureResult=(uint32_t)error;}
    /* Initial RX owns its separate bounded halt/drain retry. */
    acFailureEligible_=stage!=AURX&&(error==ETIMEDOUT||error==EIO);
    acRecoveryPrepared_=false;
}
void RTWUSBController::publish8814RecoveryDiagnostics()
{
    const auto&d=acRecoveryDiag_;
    publishDiagnosticCounter("RTWDiagAUStage",d.stage);
    publishDiagnosticCounter("RTWDiagAUResult",d.result);
    publishDiagnosticCounter("RTWDiagAUFailureStage",d.failureStage);
    publishDiagnosticCounter("RTWDiagAUFailureResult",d.failureResult);
    publishDiagnosticCounter("RTWDiagAUFirstFailureStage",d.firstFailureStage);
    publishDiagnosticCounter("RTWDiagAUFirstFailureResult",d.firstFailureResult);
    publishDiagnosticCounter("RTWDiagAUStarts",d.starts);
    publishDiagnosticCounter("RTWDiagAUFailures",d.failures);
    publishDiagnosticCounter("RTWDiagAURecoveryAttempts",d.recoveryAttempts);
    publishDiagnosticCounter("RTWDiagAURecoverySuccesses",d.recoverySuccesses);
    publishDiagnosticCounter("RTWDiagAUCleanupResult",d.cleanupResult);
    publishDiagnosticCounter("RTWDiagAUIdentityResult",d.identityResult);
    publishDiagnosticCounter("RTWDiagAUConsecutiveRecoveries",d.consecutiveRecoveries);
    publishDiagnosticCounter("RTWDiagAUPeriodicCalls",d.periodicCalls);
    publishDiagnosticCounter("RTWDiagAUPeriodicPHYFailures",d.periodicPHYFailures);
    publishDiagnosticCounter("RTWDiagAUPeriodicRSSIAttempts",d.periodicRSSIAttempts);
    publishDiagnosticCounter("RTWDiagAUPeriodicRSSIBusy",d.periodicRSSIBusy);
    publishDiagnosticCounter("RTWDiagAUPeriodicRSSIFailures",d.periodicRSSIFailures);
    publishDiagnosticCounter("RTWDiagAUPeriodicPhase",d.periodicPhase);
    publishDiagnosticCounter("RTWDiagAUPeriodicResult",d.periodicResult);
    publishDiagnosticCounter("RTWDiagAUPeriodicLastFailurePhase",d.periodicLastFailurePhase);
    publishDiagnosticCounter("RTWDiagAUPeriodicLastFailureResult",d.periodicLastFailureResult);
}
int RTWUSBController::recover8814ForControl()
{
    const int original=core_.core_error?core_.core_error:acCore_.io_error;
    if(!original)return 0;
    /* A second, different typed error has no matching retry provenance. */
    if(acCore_.io_error&&acCore_.io_error!=original)return acCore_.io_error;
    if(!acFailureEligible_||acRecoveryBlocked_||
       (original!=ETIMEDOUT&&original!=EIO)||original!=(int)acRecoveryDiag_.failureResult)
        return original;
    if(!attached_||!present_||!usbOpened_||!usb_||sleeping_||closing_||stopping_||
       usbError_!=kIOReturnSuccess||rxRecovery_.initialDrainError!=kIOReturnSuccess||
       !completionLock_||!acPHYStorage_)return ENXIO;
    if(!matches8814RecoveryIdentity())return ENODEV;
    auto&d=acRecoveryDiag_;
    if(d.consecutiveRecoveries>=2)return original;
    if(!acRecoveryPrepared_){
        d.stage=AURecoveryPowerOff;
        quiesce(false);
        if(acRecoveryBlocked_||d.cleanupResult||usbError_!=kIOReturnSuccess)
            return d.cleanupResult?(int)d.cleanupResult:EIO;
        acRecoveryPrepared_=true;
    }
    d.stage=AURecoveryDrain;
    IOInterruptState lock=IOSimpleLockLockDisableInterrupt(completionLock_);
    const bool drained=rtw_transfer_drained(&ledger_)!=0&&!rxRecovery_.initialDrainMask;
    IOSimpleLockUnlockEnableInterrupt(completionLock_,lock);
    if(!drained)return EBUSY;
    freeTransfers();
    if(!present_||sleeping_||closing_||usbError_!=kIOReturnSuccess||
       rxRecovery_.initialDrainError!=kIOReturnSuccess||
       !matches8814RecoveryIdentity())return ENXIO;
    ++d.recoveryAttempts;++d.consecutiveRecoveries;d.stage=AURecoveryIdentity;
    const rtw_8814au_bus_ops ops{this,acRead,acWrite,acBulk,delayUS};
    rtw_8814au_init(&acCore_,&ops);
    int error=rtw_8814au_identify_read_board(&acCore_,acSpeed());
    if(!error&&(memcmp(acCore_.board.logical,acOriginalBoard_.logical,sizeof(acOriginalBoard_.logical))||
       /* SYS_CFG1 also holds mutable power bits; compare the decoded
        * immutable cut/vendor/MP identity, not its complete register word. */
       acCore_.board.sys_cfg2!=acOriginalBoard_.sys_cfg2||
       acCore_.board.cut!=acOriginalBoard_.cut||acCore_.board.vendor!=acOriginalBoard_.vendor||
       acCore_.board.mp_chip!=acOriginalBoard_.mp_chip||acCore_.board.rfe_option!=acOriginalBoard_.rfe_option||
       acCore_.board.physical_rf_paths!=acOriginalBoard_.physical_rf_paths))error=ENODEV;
    d.identityResult=(uint32_t)error;
    if(error||!present_||sleeping_||closing_||usbError_!=kIOReturnSuccess){
        acCore_.board_valid=0;acRecoveryBlocked_=true;d.result=(uint32_t)(error?error:ENXIO);
        return error?error:ENXIO;
    }
    /* Reapply only the existing station stream policy, retaining the new
     * typed context's just-validated ROM and the original physical paths. */
    acCore_.board.tx_streams=acCore_.board.rx_streams=2;
    acCore_.board.antenna_tx=acCore_.board.antenna_rx=3;
    core_.core_error=0;acFailureEligible_=false;acRecoveryPrepared_=false;
    acRecoveryRestartPending_=true;d.stage=AURecoveryReady;d.result=0;
    return 0;
}
void RTWUSBController::finish8814StopAfterDrain()
{
    if(family_!=RTW_WIFI_RTL8814AU||acRecoveryDiag_.cleanupResult!=EBUSY||
       running_||stopping_||!attached_||!present_||!usbOpened_||sleeping_||closing_||
       usbError_!=kIOReturnSuccess||rxRecovery_.initialDrainError!=kIOReturnSuccess||
       !completionLock_||acRecoveryBlocked_)return;
    IOInterruptState lock=IOSimpleLockLockDisableInterrupt(completionLock_);
    const bool drained=rtw_transfer_drained(&ledger_)!=0&&!rxRecovery_.initialDrainMask;
    IOSimpleLockUnlockEnableInterrupt(completionLock_,lock);
    if(!drained)return;
    /* stop8814 clears this marker before its nested abort/drain. This path
     * finalizes a disabled card only; it never starts or resets a context. */
    stop8814();
}
