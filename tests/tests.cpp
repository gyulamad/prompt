#include <iostream>

#include "test_tools_timef.hpp"
#include "test_TerminalEmulator.hpp"
#include "test_JSON.hpp"

using namespace std;

int main() {
    test_tools_timef();
    test_TerminalEmulator();
    test_JSON();

    cout << "All tests passed successfully!" << endl;
    return 0;
}