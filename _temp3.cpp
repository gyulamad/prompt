
#include "Terminal.hpp"

int main() {
    try {
        // Create objects in reverse order of destruction
        TerminalIO terminalIO;
        TerminalEmulator terminalEmu;
        
        if (!terminalEmu.start()) {
            terminalIO.writeln("Failed to start terminal");
            return 1;
        }

        // Main loop is now much cleaner
        while (terminalEmu.update()) {
            string output = terminalEmu.read();
            if (!output.empty()) {
                terminalIO.write(output);
            }

            string input = terminalIO.read();
            if (!input.empty()) {
                terminalEmu.write(input);
            }
        }

        return 0;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    // TerminalIO destructor will be called here, restoring terminal settings
    // Then TerminalEmulator destructor will be called
}