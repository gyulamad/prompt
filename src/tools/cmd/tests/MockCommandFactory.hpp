#pragma once

// #include "../CommandFactory.hpp"

using namespace tools::cmd;

class MockCommandFactory: public CommandFactory {
public:
    using CommandFactory::CommandFactory;
    virtual ~MockCommandFactory() {}
    void public_reset() { reset(); }
};