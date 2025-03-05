#pragma once

#include "../CommandLine.hpp"

#include "MockLineEditor.hpp"

using namespace tools::cmd;

// Mock CommandLine
class MockCommandLine : public CommandLine {
public:
    MockCommandLine() : CommandLine(make_unique<MockLineEditor>()) {}
};