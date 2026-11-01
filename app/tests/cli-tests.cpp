// SPDX-License-Identifier: GPL-2.0-only
#define main rtw_control_application_main
#include "../../tools/rtw-control.cpp"
#undef main
#include <cassert>
#include <initializer_list>
#include <vector>
static Arguments arguments(std::initializer_list<const char*> words) {
    std::vector<char*> values;for(auto word:words)values.push_back(const_cast<char*>(word));
    return parse(static_cast<int>(values.size()),values.data());
}
int main() {
    assert(arguments({"rtw-control","list"}).command==Command::List);
    assert(arguments({"rtw-control","status"}).command==Command::Status);
    auto explicitID=arguments({"rtw-control","--device","18446744073709551615","status"});
    assert(explicitID.command==Command::Status&&explicitID.registryID==UINT64_MAX);
    auto join=arguments({"rtw-control","--device","1234","join","Lab"});
    assert(join.command==Command::Join&&join.registryID==1234&&!strcmp(join.ssid,"Lab"));
    for(const char* bad:{"0","-1","+1","0x123","1x","18446744073709551616",""})
        assert(arguments({"rtw-control","--device",bad,"status"}).command==Command::Invalid);
    assert(arguments({"rtw-control","--device","1234"}).command==Command::Invalid);
    assert(arguments({"rtw-control","--device","1234","list"}).command==Command::Invalid);
    assert(arguments({"rtw-control","join",""}).command==Command::Invalid);
    assert(arguments({"rtw-control","join","12345678901234567890123456789012345"}).command==Command::Invalid);
    assert(arguments({"rtw-control","--help"}).command==Command::Help);
    puts("CLI exact adapter argument parsing: passed");
}
