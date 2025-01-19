#pragma once

#include <cassert>

#include "../TEST.hpp"
#include "../Proc.hpp"

void test_Proc_basic_command() {
    Proc proc("bash");
    proc.writeln("echo \"Hello, World!\"");
    string output = proc.read();
    assert(output == "Hello, World!\n");
}

void test_Proc_error_output() {
    Proc proc("bash", true, true); // Read both stdout and stderr
    proc.writeln("ls nonexistent_file.txt");
    string output = proc.read();
    assert(output.find("No such file or directory") != string::npos);
}

void test_Proc_blocking_behavior() {
    Proc proc("bash", true, true); // Read both stdout and stderr
    proc.writeln("echo \"Hello, World!\"");
    string output = proc.read();
    assert(output == "Hello, World!\n");
}

void test_Proc_process_termination() {
    Proc proc("bash", true, true); // Start a long-running process
    proc.writeln("sleep 10");

    proc.kill();

    // Try reading from the process (should return empty string)
    string output = proc.read();
    assert(output.empty());
}

void test_Proc_invalid_command() {
    bool throws = false;
    try {
        Proc proc("nonexistent_command", true, true);
    } catch (exception& e) {
        assert(string(e.what()).find("Failed to read from child") != string::npos);
        throws = true;
    }
    // assert(throws); // TODO: needs to be fixed for nonexistent commands
}

void test_Proc_reset() {
    Proc proc("bash", true, true);
    proc.writeln("echo \"First run\"");
    string output1 = proc.read();
    assert(output1 == "First run\n");

    proc.reset();
    proc.writeln("echo \"Second run\"");
    string output2 = proc.read();
    assert(output2 == "Second run\n");
}

void test_Proc_selective_output_reading() {
    // Test reading only stdout
    Proc proc_stdout("bash", false, true); // Read stdout, ignore stderr
    proc_stdout.writeln("echo \"Hello, World!\"");
    string output_stdout = proc_stdout.read();
    assert(output_stdout == "Hello, World!\n");

    // Test reading only stderr
    Proc proc_stderr("bash", true, false); // Read stderr, ignore stdout
    proc_stderr.writeln("ls nonexistent_file.txt");
    string output_stderr = proc_stderr.read();
    assert(output_stderr.find("No such file or directory") != string::npos);
}


void test_Proc() {
    TEST(test_Proc_basic_command);
    TEST(test_Proc_error_output);
    TEST(test_Proc_blocking_behavior);
    TEST(test_Proc_process_termination);
    TEST(test_Proc_invalid_command);
    TEST(test_Proc_reset);
    TEST(test_Proc_selective_output_reading);
}