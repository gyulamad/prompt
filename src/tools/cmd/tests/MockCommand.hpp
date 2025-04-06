#pragma once

#include <string>
#include <vector>

#include "../Command.hpp"
// #include "../Commander.hpp"
#include "../CommandLine.hpp"

#include "MockCommandLine.hpp"

using namespace tools::cmd;

// Mock Command
class MockCommand : public Command {
public:

    using Command::Command;
    virtual ~MockCommand() {}

    vector<string> patterns;
    vector<string> last_args;

    vector<string> getPatterns() const override {
        return patterns;
    }

    string getName() const override { return ""; }
    string getDescription() const override { return ""; }
    string getUsage() const override { return ""; }

    void run(void*, const vector<string>& args) override {
        last_args = args;
    }
};