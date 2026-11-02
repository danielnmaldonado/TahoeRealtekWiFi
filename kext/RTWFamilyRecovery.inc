/* SPDX-License-Identifier: BSD-3-Clause
 * Common lifecycle, dedicated silicon operations. AU keeps its independently
 * qualified implementation. No timer/status path can retire a failure latch. */
extern void rtwn_efuse_read(struct rtwn_softc *,uint8_t *,size_t);
bool RTWUSBController::isLegacyRecoveryFamily() const {
    return family_==RTW_WIFI_RTL8192CU||family_==RTW_WIFI_RTL8188EU||family_==RTW_WIFI_RTL8192EU;
}
bool RTWUSBController::familyRecoveryAvailable() const {
    return family_!=RTW_WIFI_RTL8814AU&&attached_&&present_&&usbOpened_&&usb_&&
        !sleeping_&&!closing_&&!stopping_&&usbError_==kIOReturnSuccess&&completionLock_&&
        rxRecovery_.initialDrainError==kIOReturnSuccess;
}
bool RTWUSBController::familyRecoveryDrained() const {
    if(!completionLock_)return false;
    const IOInterruptState lock=IOSimpleLockLockDisableInterrupt(completionLock_);
    const bool drained=rtw_transfer_drained(&ledger_)!=0&&!rxRecovery_.initialDrainMask;
    IOSimpleLockUnlockEnableInterrupt(completionLock_,lock);return drained;
}
int RTWUSBController::readLegacyRecoveryIdentity(bool compare) {
    if(!isLegacyRecoveryFamily()||ioError(this))return ENXIO;
    const int original=core_.core_error;core_.core_error=0;
    const uint32_t cfg=readRegister(R92C_SYS_CFG,4),bond=readRegister(R92C_HPON_FSM,4);
    const size_t bytes=family_==RTW_WIFI_RTL8192CU?sizeof(struct r92c_rom):512U;
    rtwn_efuse_read(&core_,recoveryLegacyRead_,bytes);
    int error=core_.core_error?core_.core_error:ioError(this);core_.core_error=original;
    if(error)return error;
    /* SYS_CFG contains live clocks/power bits. Compare only silicon cut,
     * foundry, bus/package, test/MP and chain bonding identity. */
    const uint32_t mask=R92C_SYS_CFG_CHIP_VER_RTL_M|R92C_SYS_CFG_VENDOR_UMC|
        R92C_SYS_CFG_TRP_VAUX_EN|R92C_SYS_CFG_TYPE_92C|R92C_SYS_CFG_BD_PKG_SEL|R92C_SYS_CFG_BD_HCI_SEL;
    if(compare){
        if(core_.chip!=recoveryLegacyChip_||(cfg&mask)!=recoveryLegacySysCfg_||
           (bond&0x00c00000U)!=recoveryLegacyBonding_||
           memcmp(recoveryLegacyRead_,recoveryLegacyROM_,bytes))return ENODEV;
    }else {
        recoveryLegacyChip_=core_.chip;recoveryLegacySysCfg_=cfg&mask;
        recoveryLegacyBonding_=bond&0x00c00000U;memcpy(recoveryLegacyROM_,recoveryLegacyRead_,bytes);
    }
    return 0;
}
int RTWUSBController::captureFamilyRecoveryIdentity() {
    if(family_==RTW_WIFI_RTL8814AU)return 0;
    const auto*b=rtw_backend_for_family(family_);
    if(!b||!(b->capabilities&RTW_BACKEND_CAP_KEXT_STATION)||!profile_||
       profile_->family!=family_||!read88IdentityDescriptor(&recoveryDescriptor_))return ENODEV;
    recoveryFamily_=family_;recoveryProfile_=profile_;recoveryUSB_=usb_;recoveryRX_=rxPipe_;
    recoveryEndpoints_=endpoints_;
    for(unsigned i=0;i<4;++i){recoveryTX_[i]=txPipes_[i];recoveryMPS_[i]=txMPS_[i];}
    if(is8812Family()){recoveryAUBoard_=auCore_.board;memcpy(recoveryAURom_,auCore_.logical_rom,sizeof(recoveryAURom_));}
    else if(is88Family())recovery88Board_=r88Core_.board;
    else if(is8192FUFamily())recoveryFUBoard_=fuBoard_;
    else if(is8710BUFamily())recoveryBUBoard_=buBoard_;
    else if(is8723Family())recovery23Board_=r23Core_.board;
    else if(family_==RTW_WIFI_RTL8192DU){
        recoveryDUPortCount_=duPortCount_;recoveryDUDevice_=duDevice_;recoveryDUSibling_=duSibling_;
        for(unsigned i=0;i<2;++i){recoveryDURom_[i]=duPortROM_[i];recoveryDUPorts_[i]=duPorts_[i];}
    }else if(isLegacyRecoveryFamily()){int error=readLegacyRecoveryIdentity(false);if(error)return error;}
    else return ENOTSUP;
    familyIdentitySaved_=true;return 0;
}
bool RTWUSBController::matchesFamilyRecoveryIdentity() const {
    rtw_chip_identity_descriptor descriptor{};
    if(!familyIdentitySaved_||family_!=recoveryFamily_||profile_!=recoveryProfile_||
       !profile_||profile_->family!=family_||usb_!=recoveryUSB_||!usbOpened_||
       !read88IdentityDescriptor(&descriptor)||!same8814Descriptor(descriptor,recoveryDescriptor_))return false;
    if(family_==RTW_WIFI_RTL8192DU){
        if(!duWholeDeviceOwned_||duDevice_!=recoveryDUDevice_||duSibling_!=recoveryDUSibling_||
           duPortCount_!=recoveryDUPortCount_||duActivePort_>=duPortCount_)return false;
        for(unsigned n=0;n<duPortCount_;++n){const auto&a=duPorts_[n];const auto&b=recoveryDUPorts_[n];
            if(a.rx!=b.rx||memcmp(&a.endpoints,&b.endpoints,sizeof(a.endpoints)))return false;
            for(unsigned i=0;i<4;++i)if(a.tx[i]!=b.tx[i]||a.mps[i]!=b.mps[i])return false;
        }
        const auto&p=recoveryDUPorts_[duActivePort_];
        if(rxPipe_!=p.rx||memcmp(&endpoints_,&p.endpoints,sizeof(endpoints_)))return false;
        for(unsigned i=0;i<4;++i)if(txPipes_[i]!=p.tx[i]||txMPS_[i]!=p.mps[i])return false;
    }else {
        if(rxPipe_!=recoveryRX_||memcmp(&endpoints_,&recoveryEndpoints_,sizeof(endpoints_)))return false;
        for(unsigned i=0;i<4;++i)if(txPipes_[i]!=recoveryTX_[i]||txMPS_[i]!=recoveryMPS_[i])return false;
    }
    if(is8812Family()&&!owns8812Authority())return false;
    if(is88Family()&&!owns88Discovery())return false;
    return true;
}
bool RTWUSBController::validateFamilyRecoveryBoard() const {
    if(!familyIdentitySaved_)return true; /* attach captures the first completed read */
    if(family_!=recoveryFamily_)return false;
    if(is8812Family())return rtw8812SamePHYIdentity(auCore_.board.phy,recoveryAUBoard_.phy)&&
        !memcmp(auCore_.logical_rom,recoveryAURom_,sizeof(recoveryAURom_))&&
        !memcmp(auCore_.board.mac,recoveryAUBoard_.mac,6)&&
        auCore_.board.eeprom_vendor==recoveryAUBoard_.eeprom_vendor&&auCore_.board.eeprom_product==recoveryAUBoard_.eeprom_product;
    if(is88Family()){
        rtw88_board board=r88Core_.board;
        /* Decoded immutable fields below retain cut/vendor/path identity. */
        board.sys_cfg=recovery88Board_.sys_cfg;board.power_control=recovery88Board_.power_control;
        return !memcmp(&board,&recovery88Board_,sizeof(board));
    }
    if(is8192FUFamily())return !memcmp(&fuBoard_,&recoveryFUBoard_,sizeof(fuBoard_));
    if(is8710BUFamily())return !memcmp(&buBoard_,&recoveryBUBoard_,sizeof(buBoard_));
    if(is8723Family())return !memcmp(&r23Core_.board,&recovery23Board_,sizeof(r23Core_.board));
    if(family_==RTW_WIFI_RTL8192DU)return duActivePort_<recoveryDUPortCount_&&
        !memcmp(duROM_.logical,recoveryDURom_[duActivePort_].logical,sizeof(duROM_.logical))&&
        duROM_.cut==recoveryDURom_[duActivePort_].cut&&duROM_.board.rf_paths==recoveryDURom_[duActivePort_].board.rf_paths;
    return isLegacyRecoveryFamily()&&core_.chip==recoveryLegacyChip_;
}
int RTWUSBController::reidentifyFamilyRecovery() {
    if(!familyRecoveryDrained()||!matchesFamilyRecoveryIdentity())return ENODEV;
    freeTransfers();int error=0;
    if(is8812Family())error=initialize8812Core();
    else if(is8192FUFamily())error=initialize8192FUCore();
    else if(is8710BUFamily())error=initialize8710BUCore();
    else if(is8723Family())error=bootstrap8723(true);
    else if(is88Family()){
        /* OTP requires running firmware on B/C. Reconstruct only the typed
         * context here; bootstrap compares its complete saved board before
         * MAC/PHY/station initialization can consume the new ROM. */
        const rtw88_fw_bus bus{this,r88Read,r88Write,r88Bulk,delayUS,nullptr,nullptr};
        error=prepare88Identity(bus);if(!error)error=rtw88_boot_identify(&r88Core_);
    }else if(family_==RTW_WIFI_RTL8192DU){
        const rtw_usb_rw_transport raw{this,duControlIn,duControlOut};const rtw_waiter waiter{this,duWait};
        const rtw_92du_boot_options options{1,1,10};
        error=rtw8192DUError(rtw_92du_read_rom(&duPlan_,&raw,&waiter,&options,duActivePort_,&recoveryDURead_));
        if(!error){const auto&saved=recoveryDURom_[duActivePort_];
            if(memcmp(recoveryDURead_.physical,saved.physical,sizeof(saved.physical))||
               memcmp(recoveryDURead_.logical,saved.logical,sizeof(saved.logical))||
               recoveryDURead_.cut!=saved.cut||recoveryDURead_.board.rf_paths!=saved.board.rf_paths)error=ENODEV;
        }
        if(!error){duROM_=recoveryDURead_;duPortROM_[duActivePort_]=duROM_;error=initialize8192DUCore();}
    }else if(isLegacyRecoveryFamily())error=readLegacyRecoveryIdentity(true);
    else error=ENOTSUP;
    if(!error&&!is88Family()&&!validateFamilyRecoveryBoard())error=ENODEV;
    return error;
}
int RTWUSBController::beginFamilyStart() {
    if(family_==RTW_WIFI_RTL8814AU)return 0;
    if(familyIdentitySaved_&&(!matchesFamilyRecoveryIdentity()||!validateFamilyRecoveryBoard()))return ENODEV;
    const int error=familyRecovery_.begin(core_.core_error,familyRecoveryAvailable(),familyRecoveryDrained());
    return error;
}
void RTWUSBController::failFamilyStart(int error) {
    if(familyRecovery_.starting||familyRecovery_.diag.failure!=(uint32_t)error||!familyRecovery_.diag.failures)
        familyRecovery_.failed(error);
}
void RTWUSBController::finishFamilyStart() {
    familyRecovery_.succeeded();
}
bool RTWUSBController::familyStopDrained() {
    if(!familyRecovery_.drainedStop(familyRecoveryDrained()))return false;
    if(familyIdentitySaved_&&!matchesFamilyRecoveryIdentity()){
        recordFamilyCleanup(ENODEV);return false;
    }
    return true;
}
void RTWUSBController::recordFamilyCleanup(int error) {
    familyRecovery_.cleaned(error);
    if(error&&!core_.core_error)core_.core_error=error;
}
int RTWUSBController::recoverFamilyForControl() {
    if(family_==RTW_WIFI_RTL8814AU)return 0;
    return familyRecovery_.recover(core_.core_error,[&]{return familyRecoveryAvailable();},
        [&]{return matchesFamilyRecoveryIdentity();},[&]{quiesce(false);},
        [&]{return familyRecoveryDrained();},[&]{return reidentifyFamilyRecovery();},[&]{core_.core_error=0;});
}
void RTWUSBController::finishFamilyStopAfterDrain() {
    if(!familyRecovery_.pendingStop||familyRecovery_.inStop||familyRecovery_.blocked||
       running_||!familyRecoveryAvailable()||!familyRecoveryDrained()||!matchesFamilyRecoveryIdentity())return;
    /* Completing a stopped epoch never performs recovery or starts hardware. */
    if(is8723Family())stop8723();else if(is8710BUFamily())stop8710BU();
    else if(is8192FUFamily())stop8192FU();else if(family_==RTW_WIFI_RTL8192DU)stop8192DU();
    else if(is88Family())stop88();else if(is8812Family())stop8812();
    else if(isLegacyRecoveryFamily())busStop(this);
}
void RTWUSBController::publishFamilyRecoveryDiagnostics() {
    if(family_==RTW_WIFI_RTL8814AU)return;
    const auto&d=familyRecovery_.diag;
    publishDiagnosticCounter("RTWDiagFamilyRecoveryStage",d.stage);publishDiagnosticCounter("RTWDiagFamilyRecoveryResult",d.result);
    publishDiagnosticCounter("RTWDiagFamilyRecoveryFailure",d.failure);publishDiagnosticCounter("RTWDiagFamilyRecoveryFirstFailure",d.firstFailure);
    publishDiagnosticCounter("RTWDiagFamilyRecoveryCleanup",d.cleanup);publishDiagnosticCounter("RTWDiagFamilyRecoveryIdentity",d.identity);
    publishDiagnosticCounter("RTWDiagFamilyRecoveryStarts",d.starts);publishDiagnosticCounter("RTWDiagFamilyRecoveryFailures",d.failures);
    publishDiagnosticCounter("RTWDiagFamilyRecoveryAttempts",d.attempts);publishDiagnosticCounter("RTWDiagFamilyRecoverySuccesses",d.successes);
    publishDiagnosticCounter("RTWDiagFamilyRecoveryConsecutive",d.consecutive);publishDiagnosticCounter("RTWDiagFamilyRecoveryDeferredStop",familyRecovery_.pendingStop?1:0);
}
