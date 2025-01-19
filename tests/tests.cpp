#include <iostream>

#include "test_tools_str_starts_with.hpp"
#include "test_tools_timef.hpp"
#include "test_tools.hpp"
// #include "test_TerminalEmulator.hpp"
#include "test_JSON.hpp"
#include "test_Proc.hpp"

using namespace std;

int main() {
    test_tools_str_starts_with(); // TODO: to test_tools.hpp
    test_tools_timef(); // TODO: to test_tools.hpp
    test_tools();
    // test_TerminalEmulator(); // TODO: retest
    test_JSON();
    test_Proc();

    cout << "All tests passed successfully!" << endl;
    return 0;
}