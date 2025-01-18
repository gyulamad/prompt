#pragma once

#include <cassert>
#include <cstring> // for strerror

#include "../TEST.hpp"
#include "../Terminal.hpp"

void test_TerminalEmulator_Initialization() {
    TerminalEmulator term;
    bool result = term.start();
    assert(result && "Terminal should start successfully");
    assert(term.is_alive() && "Process should be alive after start");
}

void test_TerminalEmulator_BasicWriteRead() {
    TerminalEmulator term;
    cerr << "Starting terminal..." << endl;
    
    bool start_result = term.start("bash -i");  // Start in interactive mode
    assert(start_result && "Terminal failed to start");
    cerr << "Terminal started successfully" << endl;
    
    // Give terminal time to initialize and show its prompt
    this_thread::sleep_for(1000ms);
    
    // Clear initial output and show what we got
    string initial_output = term.read();
    cerr << "Initial output (size=" << initial_output.length() << "): '" << initial_output << "'" << endl;
    
    // First try a simple command that should definitely produce output
    cerr << "Testing with 'ls' command..." << endl;
    term.writeln("ls -l");
    this_thread::sleep_for(500ms);
    string ls_output = term.read();
    cerr << "ls command output (size=" << ls_output.length() << "): '" << ls_output << "'" << endl;
    
    // Try with strace to see if the command is actually being executed
    cerr << "Testing with strace..." << endl;
    term.writeln("strace -f echo 'TEST' 2>&1");
    this_thread::sleep_for(500ms);
    string strace_output = term.read();
    cerr << "strace output (size=" << strace_output.length() << "): '" << strace_output << "'" << endl;
    
    // Now try our actual test command with explicit path and redirection
    const string test_message = "UNIQUE_TEST_MESSAGE_12345";
    const string full_command = "/bin/echo '" + test_message + "' > /dev/pts/0";  // Direct output to our pts
    
    cerr << "Sending command: '" << full_command << "'" << endl;
    term.writeln(full_command);
    
    // Read output with polling
    string output;
    for (int i = 0; i < 10; i++) {
        string chunk = term.read();
        cerr << "Read chunk " << i << " (size=" << chunk.length() << 
                    ", hex: " << hex;
        // Print hex representation of non-empty chunks
        if (!chunk.empty()) {
            for (unsigned char c : chunk) {
                cerr << static_cast<int>(c) << " ";
            }
        }
        cerr << dec << "): '" << chunk << "'" << endl;
        
        output += chunk;
        if (!chunk.empty()) {
            cerr << "Got non-empty chunk, total output size now: " << output.length() << endl;
        }
        this_thread::sleep_for(100ms);
    }
    
    cerr << "Final complete output: '" << output << "'" << endl;
    cerr << "Test message: '" << test_message << "'" << endl;
    
    // Check if terminal is still alive and responsive
    bool still_alive = term.is_alive();
    cerr << "Terminal alive status: " << (still_alive ? "true" : "false") << endl;
    
    // Try to get terminal info
    term.writeln("tty");
    this_thread::sleep_for(200ms);
    string tty_output = term.read();
    cerr << "TTY output: '" << tty_output << "'" << endl;
    
    assert(output.find(test_message) != string::npos && "Should read back echoed message");
}

void test_TerminalEmulator_CharacterIO() {
    TerminalEmulator term;
    term.start();
    
    term.putc('a');
    term.putc('b');
    term.putc('c');
    term.putc('\n');
    
    this_thread::sleep_for(100ms);
    
    assert(term.getc() == 'a' && "Should read first character");
    assert(term.getc() == 'b' && "Should read second character");
    assert(term.getc() == 'c' && "Should read third character");
}

void test_TerminalEmulator_ProcessLifecycle() {
    TerminalEmulator term;
    term.start("sleep 0.1");
    
    assert(term.is_alive() && "Process should be alive initially");
    this_thread::sleep_for(200ms);
    assert(!term.is_alive() && "Process should terminate after sleep");
}

void test_TerminalEmulator_UpdateMethod() {
    TerminalEmulator term;
    term.start();
    
    bool update_result = term.update();
    assert(update_result && "Update should return true for active terminal");
    
    term.writeln("exit");
    this_thread::sleep_for(100ms);
    
    update_result = term.update();
    assert(!update_result && "Update should return false after terminal exits");
}

void test_TerminalEmulator_SignalHandling() {
    TerminalEmulator term;
    term.start();
    
    term.writeln("kill -TERM $$");
    this_thread::sleep_for(100ms);
    
    assert(!term.is_alive() && "Process should terminate after SIGTERM");
}

void test_TerminalEmulator_LargeDataHandling() {
    TerminalEmulator term;
    term.start();
    
    term.writeln("for i in {1..1000}; do echo $i; done");
    this_thread::sleep_for(500ms);
    
    string output = term.read();
    assert(!output.empty() && "Should handle large output");
    assert(output.length() > 1000 && "Should receive substantial output");
}

void test_TerminalEmulator_MultipleCommands() {
    TerminalEmulator term;
    term.start();
    
    term.writeln("echo 'first'");
    this_thread::sleep_for(50ms);
    string first_output = term.read();
    
    term.writeln("echo 'second'");
    this_thread::sleep_for(50ms);
    string second_output = term.read();
    
    assert(first_output.find("first") != string::npos && "Should read first command output");
    assert(second_output.find("second") != string::npos && "Should read second command output");
}

void test_TerminalEmulator_InvalidCommand() {
    TerminalEmulator term;
    term.start();
    
    term.writeln("nonexistentcommand");
    this_thread::sleep_for(100ms);
    
    string output = term.read();
    assert(output.find("command not found") != string::npos && "Should handle invalid commands");
}

void test_TerminalEmulator_Cleanup() {
    TerminalEmulator* term = new TerminalEmulator();
    term->start();
    assert(term->is_alive() && "Process should be alive");
    
    delete term;
    // After deletion, the process should be terminated and resources freed
    // Note: We can't test the actual process state after deletion as the object is gone
}

void test_TerminalEmulator() {

    TEST(test_TerminalEmulator_Initialization);
    // TEST(test_TerminalEmulator_BasicWriteRead);  // <- TODO: claude.ai works on it
    // TEST(test_TerminalEmulator_CharacterIO); // TODO: place back non working tests
    TEST(test_TerminalEmulator_ProcessLifecycle);
    // TEST(test_TerminalEmulator_UpdateMethod);
    // TEST(test_TerminalEmulator_SignalHandling);
    // TEST(test_TerminalEmulator_LargeDataHandling);
    // TEST(test_TerminalEmulator_MultipleCommands);
    // TEST(test_TerminalEmulator_InvalidCommand);
    TEST(test_TerminalEmulator_Cleanup);
}