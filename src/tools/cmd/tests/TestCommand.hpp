#pragma once

#include "../Command.hpp"

using namespace tools::cmd;

class TestCommand : public Command {
public:
    TestCommand() : Command("test") {}

    // LCOV_EXCL_START
    vector<string> getPatterns() const override { return {}; }
    string getName() const override { return ""; }
    string getDescription() const override { return ""; }
    string getUsage() const override { return ""; }
    void run(void*, const vector<string>&) override {}
    // LCOV_EXCL_STOP
};