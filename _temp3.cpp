#include "Terminal.hpp"

int main() {
    TerminalIO term;
    TerminalEmulator temu;
    while(temu.update()) {
        term.write(temu.read());
        temu.write(term.read());
    }

    return 0;
}