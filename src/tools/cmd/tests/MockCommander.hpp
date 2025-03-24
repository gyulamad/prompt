#pragma once

#include "../Commander.hpp"

using namespace tools::cmd;

class MockCommander : public Commander {
public:
    MockCommander(CommandLine& cl) : Commander(cl) {}
    bool isExiting() const override { return exiting; }
    void exit() override { exiting = true; }
    bool runCommand(void* ctx, const string& cmd) override {
        if (cmd == "/exit") exiting = true;
        return true;
    }
private:
    bool exiting = false;
};