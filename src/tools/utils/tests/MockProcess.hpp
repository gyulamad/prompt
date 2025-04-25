#pragma once

#include <string>

#include "../Process.hpp"

using namespace std;
using namespace tools::utils;

// Mock Process class for testing TTS
class MockProcess: public Process {
public:
    string last_command = "";
    string mock_output = "[SPEAK-DONE]";
    bool ready_flag = true;

    MockProcess() : Process("bash") {} // Default constructor matching Process

    void write(const string& input) override {
        last_command = input; // Simplified for testing
    }

    void writeln(const string& input) override {
        last_command = input + "\n"; // Match TTS usage
    }

    string read(int /*timeout_ms*/ = 0) override { // Match Process signature
        return mock_output;
    }

    int ready() override {
        return ready_flag ? (PIPE_STDOUT | PIPE_STDERR) : 0; // Match Process’s PipeStatus
    }

    static string execute(const string& cmd) {
        return "Executed: " + cmd;
    }
};