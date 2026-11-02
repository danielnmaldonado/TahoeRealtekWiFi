/* Real shared recovery policy; only hardware/USB lifecycle callbacks doubled.
 * Removing the checked drain or allowing an unrecorded latch to clear breaks
 * these externally observable retry/write/ownership checks. */
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdint>
#include <initializer_list>

#if __has_include("rtw/StationRecovery.hpp") && !defined(RTW_RECOVERY_BASELINE)
#include "rtw/StationRecovery.hpp"
#else
namespace rtw {struct StationRecovery {
 enum Stage:uint32_t{Idle,Start,RX,Ready,Stop,Drain,Identity};
 struct {uint64_t starts=0,failures=0,attempts=0,successes=0;uint32_t stage=Idle,result=0,failure=0,firstFailure=0,cleanup=0,identity=0,consecutive=0;}diag;
 bool pendingStop=false,inStop=false,blocked=false;
 int begin(int original,bool,bool){return original;}void failed(int error){diag.failure=error;}
 void succeeded(){}bool drainedStop(bool drained){return drained;}void cleaned(int error){diag.cleanup=error;}
 template<class... F>int recover(int original,F...){return original;}
};}
#endif
struct Hardware {
 rtw::StationRecovery recovery;int coreError=0,cleanupError=0,identityError=0;
 bool available=true,same=true,drained=true;
 unsigned powers=0,reidentifies=0,clears=0,stops=0;
 int start(int error=0){int gate=recovery.begin(coreError,available,drained);if(gate)return gate;++powers;if(error){coreError=error;recovery.failed(error);stop();return error;}recovery.succeeded();return 0;}
 void stop(){++stops;recovery.inStop=true;if(!recovery.drainedStop(drained))return;recovery.cleaned(cleanupError);}
 int retry(){return recovery.recover(coreError,[&]{return available;},[&]{return same;},[&]{stop();},[&]{return drained;},[&]{++reidentifies;return identityError;},[&]{++clears;coreError=0;});}
};
int main(){
 {rtw::StationRecovery p;assert(p.begin(0,true,true)==0);assert(!p.transient(EBUSY)&&!p.transient(EAGAIN));p.diag.stage=rtw::StationRecovery::RX;assert(p.transient(EBUSY)&&p.transient(EAGAIN)&&!p.transient(EIO));rtw::StationRecovery pending;assert(pending.begin(0,true,false)==EBUSY&&pending.transient(EBUSY));}

 for(unsigned family=0;family<12;++family){
  Hardware h;assert(h.start(ETIMEDOUT)==ETIMEDOUT);assert(h.coreError==ETIMEDOUT);
  assert(h.start()==ETIMEDOUT&&h.powers==1);assert(h.retry()==0&&h.clears==1&&h.reidentifies==1);assert(h.start()==0&&h.powers==2);
  assert(h.recovery.diag.firstFailure==ETIMEDOUT&&h.recovery.diag.successes==1&&h.recovery.diag.consecutive==0);
 }
 {Hardware h;h.coreError=EIO;assert(h.retry()==EIO&&!h.stops&&!h.clears);}
 for(int error:{EINVAL,ENOTSUP,ENODEV,ENXIO,ENOMEM}){Hardware h;assert(h.start(error)==error);assert(h.retry()==error&&!h.clears&&!h.reidentifies);}
 {Hardware h;assert(h.start(EIO)==EIO);h.drained=false;assert(h.retry()==EBUSY);assert(h.recovery.pendingStop&&!h.clears&&!h.reidentifies);h.drained=true;h.stop();assert(!h.recovery.pendingStop&&h.powers==1);assert(h.retry()==0&&h.start()==0);}
 {Hardware h;h.cleanupError=EIO;assert(h.start(ETIMEDOUT)==ETIMEDOUT);assert(h.recovery.diag.cleanup==EIO&&h.recovery.diag.failure==ETIMEDOUT);assert(h.retry()==EIO&&!h.clears);}
 {Hardware h;assert(h.start(EIO)==EIO);h.identityError=ENODEV;assert(h.retry()==ENODEV&&h.recovery.blocked&&!h.clears);}
 for(unsigned mode=0;mode<2;++mode){Hardware h;assert(h.start(EIO)==EIO);if(mode==0)h.same=false;else h.available=false;assert(h.retry()!=0&&!h.clears&&!h.reidentifies);}
 {Hardware h;assert(h.start(EIO)==EIO);for(unsigned i=0;i<2;++i){assert(h.retry()==0);assert(h.start(EIO)==EIO);}assert(h.retry()==EIO&&h.recovery.diag.attempts==2&&h.powers==3);}
 {Hardware a,b;assert(a.start(ETIMEDOUT)==ETIMEDOUT);assert(b.start()==0);a.drained=false;assert(a.retry()==EBUSY);assert(b.powers==1&&!b.coreError&&!b.recovery.pendingStop);}
 {Hardware h;assert(h.start(ETIMEDOUT)==ETIMEDOUT);h.available=false;assert(h.retry()==ENXIO&&!h.clears);h.available=true;h.same=false;assert(h.retry()==ENODEV&&!h.clears);}
 std::puts("Shared station recovery: explicit typed failure, checked stop/drain/identity, bounded retry, direct-start rejection and adapter isolation passed");
}
