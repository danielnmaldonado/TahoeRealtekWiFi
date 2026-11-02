/* Host boundary for dedicated backend suites. Full orchestration and identity
 * run in family_controller_recovery_tests; these suites retain the real HAL. */
    rtw::StationRecovery familyRecovery_=[] {rtw::StationRecovery r;r.hardwareStopped=false;return r;}();
    bool recoveryDrained_=true;
    int beginFamilyStart(){return familyRecovery_.begin(core_.core_error,ioError(this)==0,recoveryDrained_);}
    void failFamilyStart(int error){if(familyRecovery_.starting||familyRecovery_.diag.failure!=(uint32_t)error||!familyRecovery_.diag.failures)familyRecovery_.failed(error);}
    void finishFamilyStart(){familyRecovery_.succeeded();}
    bool familyStopDrained(){return familyRecovery_.drainedStop(recoveryDrained_);}
    void recordFamilyCleanup(int error){familyRecovery_.cleaned(error);if(error&&!core_.core_error)core_.core_error=error;}
    bool validateFamilyRecoveryBoard()const{return true;}
