#pragma once

#include "../CommandLine.hpp"
#include "../ILineEditor.hpp"

using namespace tools::cmd;

// Mock CommandLine
class MockCommandLine : public CommandLine {
public:
    MockCommandLine(ILineEditor& editor) : CommandLine(editor) {}
};