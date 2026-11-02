/* Real controller recovery/binding methods. Native OTP/power and IOKit are
 * boundary doubles; dedicated backend suites execute their portable HALs. */
#include "rtw/StationRecovery.hpp"
#include "rtw/FamilyBackend.h"
#include "rtw/RTL8812AUBootstrap.h"
#include "rtw/RTW88Boot.h"
#include "rtw/RTL8192FU.h"
#include "rtw/RTL8710BU.h"
#include "rtw/RTL8723BU.h"
#include "rtw/RTL8192DUEfuse.h"
#include "rtw/RTL8192EUDataPath.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
using IOReturn=int;using IOInterruptState=int;
enum {kIOReturnSuccess=0};
static int IOSimpleLockLockDisableInterrupt(void*){return 0;}
static void IOSimpleLockUnlockEnableInterrupt(void*,int){}
struct IOUSBHostPipe {};
struct rtwn_softc {int core_error=0;uint32_t chip=1;};
class RTWUSBController {public:
    rtwn_softc core_{};
    rtw_wifi_family family_=RTW_WIFI_RTL8192EU;rtw_usb_wifi_profile catalog_{};
    const rtw_usb_wifi_profile *profile_=&catalog_;
    bool attached_=true,present_=true,usbOpened_=true,sleeping_=false,closing_=false,stopping_=false,running_=false;
    void *usb_=this,*completionLock_=this;IOReturn usbError_=0;
    rtw_usb_endpoints endpoints_{};IOUSBHostPipe pipes_[5];IOUSBHostPipe *rxPipe_=&pipes_[0],*txPipes_[4]{&pipes_[1],nullptr,nullptr,nullptr};uint16_t txMPS_[4]{512,0,0,0};
    struct {uint32_t initialDrainMask=0;IOReturn initialDrainError=0;}rxRecovery_;
    rtw_transfer_ledger ledger_{};
    rtw_chip_identity_descriptor descriptor_{};
    rtw_r12au_boot auCore_{};rtw88_boot_context r88Core_{};
    rtw_r92fu_board fuBoard_{};rtw_r10bu_board buBoard_{};rtw_8723bu r23Core_{};
    rtw_92du_rom duROM_{},duPortROM_[2]{};
    struct DUDataPort {rtw_usb_endpoints endpoints{};IOUSBHostPipe *rx=nullptr,*tx[4]{};uint16_t mps[4]{};}duPorts_[2];
    void *duDevice_=this,*duSibling_=nullptr;bool duWholeDeviceOwned_=true;uint8_t duPortCount_=1,duActivePort_=0;
    bool abortDeferred_=false,boardChanged_=false,descriptorValid_=true,ownerValid_=true;
    int cleanupError_=0,reidentifyError_=0;unsigned writes_=0,starts_=0,reidentifies_=0,frees_=0,stops_=0;
#include "RTWFamilyRecoveryMembers.inc"
    bool is8812Family()const{return family_==RTW_WIFI_RTL8812AU||family_==RTW_WIFI_RTL8821AU;}
    bool is88Family()const{return family_==RTW_WIFI_RTL8821CU||family_==RTW_WIFI_RTL8822BU||family_==RTW_WIFI_RTL8822CU;}
    bool is8192FUFamily()const{return family_==RTW_WIFI_RTL8192FU;}
    bool is8710BUFamily()const{return family_==RTW_WIFI_RTL8710BU;}
    bool is8723Family()const{return family_==RTW_WIFI_RTL8723BU;}
    bool owns8812Authority()const{return ownerValid_;}bool owns88Discovery()const{return ownerValid_;}
    bool read88IdentityDescriptor(rtw_chip_identity_descriptor*out)const{if(!descriptorValid_)return false;*out=descriptor_;return true;}
    void freeTransfers(){assert(familyRecoveryDrained());++frees_;}
    static int ioError(void*p){return static_cast<RTWUSBController*>(p)->usbError_?EIO:0;}
    void publishDiagnosticCounter(const char*,uint64_t){}


    void quiesce(bool){++stops_;familyRecovery_.inStop=true;running_=false;rtw_transfer_stop(&ledger_);if(!abortDeferred_)for(unsigned i=0;i<RTW_TRANSFER_SLOTS;++i)if(ledger_.pending&(1U<<i)){if(!(ledger_.completed&(1U<<i)))assert(rtw_transfer_complete(&ledger_,i));assert(rtw_transfer_take(&ledger_,i));}if(familyStopDrained()){++writes_;recordFamilyCleanup(cleanupError_);}}
    void stop8812(){quiesce(false);}void stop88(){quiesce(false);}void stop8192DU(){quiesce(false);}void stop8192FU(){quiesce(false);}void stop8710BU(){quiesce(false);}void stop8723(){quiesce(false);}static void busStop(void*p){static_cast<RTWUSBController*>(p)->quiesce(false);}
    explicit RTWUSBController(rtw_wifi_family family):family_(family){catalog_.family=family;catalog_.primary_verified=1;descriptor_.vendor=0xbda;descriptor_.product=0x1234;descriptor_.configuration=1;endpoints_.bulk_in[0]=0x81;endpoints_.num_bulk_in=endpoints_.num_bulk_out=1;endpoints_.bulk_out[0]=2;duPorts_[0].endpoints=endpoints_;duPorts_[0].rx=rxPipe_;duPorts_[0].tx[0]=txPipes_[0];duPorts_[0].mps[0]=512;rtw_transfer_reset(&ledger_);assert(!captureFamilyRecoveryIdentity());}
    int start(int error=0){const int gate=beginFamilyStart();if(gate)return gate;++starts_;if(error){failFamilyStart(error);core_.core_error=error;quiesce(false);return error;}running_=true;finishFamilyStart();return 0;}
};
int RTWUSBController::readLegacyRecoveryIdentity(bool compare){if(compare&&boardChanged_)return ENODEV;recoveryLegacyChip_=core_.chip;return 0;}
int RTWUSBController::reidentifyFamilyRecovery(){assert(familyRecoveryDrained()&&familyRecovery_.hardwareStopped);++reidentifies_;if(reidentifyError_)return reidentifyError_;if(boardChanged_){if(is8812Family())auCore_.logical_rom[77]^=1;else if(is88Family())r88Core_.board.logical[77]^=1;else if(is8192FUFamily())fuBoard_.logical[0xf8]^=1;else if(is8710BUFamily())buBoard_.logical[0xf8]^=1;else if(is8723Family())r23Core_.board.logical[77]^=1;else if(family_==RTW_WIFI_RTL8192DU)duROM_.logical[77]^=1;else return ENODEV;}return validateFamilyRecoveryBoard()?0:ENODEV;}
/* ACTUAL_METHODS */
int main(){
 unsigned families=0,cases=0;
 for(unsigned value=RTW_WIFI_RTL8192CU;value<=RTW_WIFI_RTL8822CU;++value){auto family=(rtw_wifi_family)value;if(family==RTW_WIFI_RTL8814AU||family==RTW_WIFI_RTL8814BU)continue;++families;
  {auto c=std::make_unique<RTWUSBController>(family);assert(c->start(ETIMEDOUT)==ETIMEDOUT);c->failFamilyStart(ETIMEDOUT);assert(c->familyRecovery_.diag.failures==1);assert(c->start()==ETIMEDOUT&&c->starts_==1);assert(!c->recoverFamilyForControl());assert(!c->core_.core_error&&c->reidentifies_==1);assert(!c->start()&&c->starts_==2&&c->running_);assert(c->familyRecovery_.diag.successes==1&&c->familyRecovery_.diag.firstFailure==ETIMEDOUT);++cases;}
  for(unsigned mutation=0;mutation<10;++mutation){auto c=std::make_unique<RTWUSBController>(family);assert(c->start(EIO)==EIO);if(mutation==0)c->usb_=&c->catalog_;if(mutation==1)c->profile_=nullptr;if(mutation==2)c->descriptor_.product^=1;if(mutation==3)c->endpoints_.bulk_out[0]^=1;if(mutation==4)c->txMPS_[0]=64;if(mutation==5)c->rxPipe_=&c->pipes_[4];if(mutation==6)c->family_=RTW_WIFI_UNKNOWN;if(mutation==7)c->sleeping_=true;if(mutation==8)c->closing_=true;if(mutation==9)c->usbError_=EIO;const auto writes=c->writes_;assert(c->recoverFamilyForControl()!=0&&c->core_.core_error==EIO&&c->writes_==writes&&!c->reidentifies_&&!c->running_);++cases;}
  {auto c=std::make_unique<RTWUSBController>(family);assert(c->start(EIO)==EIO);c->boardChanged_=true;assert(c->recoverFamilyForControl()==ENODEV&&c->familyRecovery_.blocked&&c->core_.core_error==EIO);++cases;}
  {auto c=std::make_unique<RTWUSBController>(family);c->cleanupError_=ENXIO;assert(c->start(ETIMEDOUT)==ETIMEDOUT);assert(c->familyRecovery_.diag.failure==ETIMEDOUT&&c->familyRecovery_.diag.cleanup==ENXIO);assert(c->recoverFamilyForControl()==ENXIO&&!c->reidentifies_);++cases;}
  {auto c=std::make_unique<RTWUSBController>(family);assert(!c->start());assert(rtw_transfer_restart(&c->ledger_));assert(rtw_transfer_begin(&c->ledger_,0));c->abortDeferred_=true;auto writes=c->writes_;c->quiesce(false);assert(c->familyRecovery_.pendingStop&&c->writes_==writes);assert(c->start()==EBUSY);assert(rtw_transfer_complete(&c->ledger_,0));assert(rtw_transfer_take(&c->ledger_,0));c->abortDeferred_=false;c->finishFamilyStopAfterDrain();if(c->familyRecovery_.pendingStop||c->writes_!=writes+1||c->running_||c->starts_!=1)std::fprintf(stderr,"deferred family=%u pending=%d writes=%u expected=%u stopped=%d blocked=%d inStop=%d avail=%d matches=%d\n",value,c->familyRecovery_.pendingStop,c->writes_,writes+1,c->familyRecovery_.hardwareStopped,c->familyRecovery_.blocked,c->familyRecovery_.inStop,c->familyRecoveryAvailable(),c->matchesFamilyRecoveryIdentity());assert(!c->familyRecovery_.pendingStop&&c->writes_==writes+1&&!c->running_&&c->starts_==1);assert(!c->start()&&c->starts_==2);++cases;}
  {auto a=std::make_unique<RTWUSBController>(family),b=std::make_unique<RTWUSBController>(family);assert(a->start(EIO)==EIO&&b->start()==0);a->boardChanged_=true;assert(a->recoverFamilyForControl()==ENODEV);assert(b->running_&&b->starts_==1&&b->familyRecovery_.diag.attempts==0);++cases;}
 }
 assert(families==12);std::printf("Actual controller family recovery/binding: %u families, %u cases passed\n",families,cases);
}
