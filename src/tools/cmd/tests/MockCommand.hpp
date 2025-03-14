#pragma once

#include <string>
#include <vector>

#include "../Command.hpp"
#include "../Commander.hpp"
#include "../CommandLine.hpp"

#include "MockCommandLine.hpp"

using namespace tools::cmd;

// Mock Command
class MockCommand : public Command {
public:
    vector<string> patterns;
    vector<string> last_args;

    vector<string> get_patterns() const override {
        return patterns;
    }

    void run(void*, const vector<string>& args) override {
        last_args = args;
    }
};