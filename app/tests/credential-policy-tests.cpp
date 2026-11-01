// SPDX-License-Identifier: GPL-2.0-only
#include "../CredentialPolicy.hpp"
#include "../LoginPolicy.hpp"
#include "../Model.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>

static unsigned checks=0;
static void check(bool value,const char* message) {
    ++checks;if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
static void testUpsert(int first,int added,int second,int expected,int updates,int adds) {
    int updateCalls=0,addCalls=0;
    const int status=rtwui::upsertCredential<int>(-25300,-25299,
        [&] {return ++updateCalls==1?first:second;},
        [&] {++addCalls;return added;});
    check(status==expected,"upsert status");
    check(updateCalls==updates,"upsert update count");
    check(addCalls==adds,"upsert add count");
}
int main() {
    using namespace rtwui;
    check(credentialAccount(nullptr,3).empty(),"null SSID");
    const uint8_t single[]={0};
    check(credentialAccount(single,0).empty(),"empty SSID");
    check(credentialAccount(single,33).empty(),"oversize SSID");
    check(credentialAccount(single,1)=="wpa2:00","embedded zero");
    const uint8_t network[]={'L','a','b'};
    check(credentialAccount(network,3)=="wpa2:4c6162","fixed expected account");
    check(credentialSSID("wpa2:4c6162")==std::vector<uint8_t>(network,network+3),"account decoding");
    for(unsigned length=1;length<=32;++length) {
        for(unsigned seed=0;seed<256;++seed) {
            std::vector<uint8_t> ssid(length);
            for(unsigned i=0;i<length;++i)ssid[i]=static_cast<uint8_t>(seed+i*17);
            const auto account=credentialAccount(ssid.data(),ssid.size());
            check(account.size()==5+length*2,"account length");
            check(credentialSSID(account)==ssid,"all byte/length round trips");
        }
    }
    const uint8_t nfc[]={0xc3,0xa9},nfd[]={0x65,0xcc,0x81};
    check(credentialAccount(nfc,sizeof(nfc))!=credentialAccount(nfd,sizeof(nfd)),"Unicode byte identity");
    for(const std::string value:{"","wpa2:","wpa2:0","wpa2:gg","WPA2:00","wpa2:FF","wpa2:000","wpa3:00"})
        check(credentialSSID(value).empty(),"reject malformed or foreign account");
    check(credentialSSID("wpa2:"+std::string(66,'0')).empty(),"reject account longer than 32 bytes");
    check(credentialSSID(std::string("wpa2:00\0ff",10)).empty(),"reject embedded NUL in account");
    check(validPassword("12345678",8),"minimum password");
    check(validPassword(std::string(63,'x').data(),63),"maximum password");
    check(!validPassword("1234567",7),"short password");
    check(!validPassword(std::string(64,'a').data(),64),"64 hex unsupported by existing driver");
    check(!validPassword("1234567\n",8),"control byte in password");
    check(!validPassword("1234567\xff",8),"non-ASCII password");
    testUpsert(0,0,0,0,1,0);               // Update existing.
    testUpsert(-25300,0,0,0,1,1);          // Create new.
    testUpsert(-25300,-25299,0,0,2,1);     // Creation race.
    testUpsert(-25293,0,0,-25293,1,0);     // Access denied: no fallback.
    testUpsert(-25308,0,0,-25308,1,0);     // Locked: no fallback.
    testUpsert(-128,0,0,-128,1,0);         // User canceled: no fallback.
    testUpsert(-25300,-25293,0,-25293,1,1);// Add denied.
    testUpsert(-25300,-25299,-25293,-25293,2,1); // Retry denied.
    check(loginToggleAction(false,false)==LoginAction::Enable,"unregistered/not-found stays actionable");
    check(loginToggleAction(true,false)==LoginAction::Disable,"enabled toggles off");
    check(loginToggleAction(false,true)==LoginAction::OpenSettings,"approval opens settings");
    check(loginToggleAction(true,true)==LoginAction::OpenSettings,"approval wins");
    check(loginRegistrationConfirmed(true,false),"enabled confirms registration");
    check(loginRegistrationConfirmed(false,true),"pending is distinguishable registration");
    check(!loginRegistrationConfirmed(false,false),"not-found does not claim success");
    std::printf("credential/login policy: %u checks passed\n",checks);
}
