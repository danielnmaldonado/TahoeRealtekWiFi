#pragma once
/* SPDX-License-Identifier: BSD-3-Clause
 * Lifecycle policy only. Every register/ROM/transport action remains in the
 * owning controller's dedicated silicon backend. Serialized by its workloop. */
#include <stdint.h>
#ifdef KERNEL
#include <sys/errno.h>
#else
#include <errno.h>
#endif
namespace rtw {
struct StationRecovery {
    enum Stage : uint32_t {Idle,Start,RX,Ready,Stop,Drain,Identity};
    struct Diagnostics {
        uint64_t starts=0,failures=0,attempts=0,successes=0;
        uint32_t stage=Idle,result=0,failure=0,firstFailure=0,cleanup=0,identity=0,consecutive=0;
    } diag;
    bool pendingStop=false,inStop=false,blocked=false;
    bool eligible=false,starting=false,restarting=false,prepared=false,hardwareStopped=true;
    int begin(int original,bool available,bool drained) {
        if(original)return original;
        if(blocked)return diag.cleanup?(int)diag.cleanup:ENXIO;
        if(!available)return ENXIO;
        if(!drained||pendingStop){diag.stage=Drain;starting=false;return EBUSY;}
        ++diag.starts;diag.stage=Start;diag.result=0;starting=true;hardwareStopped=false;return 0;
    }
    bool transient(int error) const {
        return (error==EAGAIN||error==EBUSY)&&
            (diag.stage==RX||(diag.stage==Drain&&!starting&&!blocked));
    }
    void failed(int error) {
        starting=false;if(!error)return;
        ++diag.failures;diag.result=diag.failure=(uint32_t)error;
        if(!diag.firstFailure)diag.firstFailure=(uint32_t)error;
        eligible=diag.stage!=RX&&(error==EIO||error==ETIMEDOUT);prepared=false;
    }
    void succeeded() {
        starting=false;eligible=prepared=false;diag.stage=Ready;diag.result=0;diag.consecutive=0;
        if(restarting){++diag.successes;restarting=false;}
    }
    bool drainedStop(bool drained) {
        inStop=false;
        if(!drained){pendingStop=true;if(!blocked)diag.cleanup=EBUSY;return false;}
        pendingStop=false;if(diag.cleanup==EBUSY)diag.cleanup=0;
        return !blocked&&!hardwareStopped;
    }
    void cleaned(int error) {
        if(error){if(!blocked)diag.cleanup=(uint32_t)error;blocked=true;}
        else if(!blocked){diag.cleanup=0;hardwareStopped=true;}
    }
    template<class Available,class SameIdentity,class StopHardware,class Drained,class Reidentify,class Clear>
    int recover(int original,Available available,SameIdentity sameIdentity,StopHardware stop,
                Drained drained,Reidentify reidentify,Clear clear) {
        if(!original)return 0;
        if(!eligible||original!=(int)diag.failure||(original!=EIO&&original!=ETIMEDOUT))return original;
        if(blocked)return diag.cleanup?(int)diag.cleanup:ENXIO;
        if(!available())return ENXIO;
        if(!sameIdentity())return ENODEV;
        if(diag.consecutive>=2)return original;
        if(!prepared){diag.stage=Stop;stop();
            if(blocked)return diag.cleanup?(int)diag.cleanup:EIO;
            if(diag.cleanup&&diag.cleanup!=EBUSY)return (int)diag.cleanup;
        }
        diag.stage=Drain;if(!drained()||pendingStop)return EBUSY;
        prepared=true;
        if(!available()||!sameIdentity())return ENXIO;
        ++diag.attempts;++diag.consecutive;diag.stage=Identity;
        const int error=reidentify();diag.identity=(uint32_t)error;
        if(error||!available()||!sameIdentity()){
            blocked=true;diag.result=(uint32_t)(error?error:ENXIO);return (int)diag.result;
        }
        clear();eligible=prepared=false;restarting=true;diag.result=0;return 0;
    }
};
}
