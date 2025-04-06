#pragma once

#include "../CommandLine.hpp"
#include "../LineEditor.hpp"

using namespace tools::cmd;

// Mock CommandLine
class MockCommandLine : public CommandLine {
public:
    MockCommandLine(LineEditor& editor) : CommandLine(editor) {}
    virtual ~MockCommandLine() {}
};