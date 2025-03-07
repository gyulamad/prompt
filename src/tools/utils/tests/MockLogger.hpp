#pragma once

#include "../Logger.hpp"

using namespace tools::utils;

class MockLogger : public Logger {
public:
    MockLogger() : Logger("MockLogger") {}
    vector<string> loggedMessages;

    void log(Level level, const string& message) override {
        loggedMessages.push_back(formatter(level, name, message));
    }

    bool hasMessageContaining(const string& substring) const {
        return any_of(loggedMessages.begin(), loggedMessages.end(),
                        [&substring](const string& msg) { return msg.find(substring) != string::npos; });
    }
};