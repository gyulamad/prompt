#pragma once

#include "../Commander.hpp"

using namespace tools::cmd;

class MockCommander : public Commander {
public:
    MockCommander(CommandLine& cl) : Commander(cl) {}
    bool is_exiting() const override { return exiting; }
    void exit() override { exiting = true; }
    bool run_command(void* ctx, const string& cmd) override {
        if (cmd == "/exit") exiting = true;
        return true;
    }
private:
    bool exiting = false;
};