#pragma once

#include <string>
#include <vector>

#include "../ILineEditor.hpp"
#include "../CommandLine.hpp"

using namespace std;
using namespace tools::cmd;

// MockLineEditor definition (simplified for testing)
class MockLineEditor : public ILineEditor {
public:
    string next_input;
    bool should_exit = false;
    vector<string> history;
    bool multi_line_enabled = false;
    size_t max_history_len = 0;
    string loaded_history_path;
    string saved_history_path;

    void SetCompletionCallback(CompletionCallback) override {}
    void SetMultiLine(bool enable) override { multi_line_enabled = enable; }
    void SetHistoryMaxLen(size_t len) override { max_history_len = len; }
    void LoadHistory(const char* path) override { loaded_history_path = path; }
    void SaveHistory(const char* path) override { saved_history_path = path; }
    void AddHistory(const char* line) override { history.push_back(line); }
    bool Readline(const char* prompt, string& line) override {
        line = next_input;
        return should_exit;
    }
};

// Helper to create a CommandLine with a mock editor
unique_ptr<CommandLine> create_command_line(unique_ptr<MockLineEditor> editor = make_unique<MockLineEditor>()) {
    return make_unique<CommandLine>(move(editor), "> ", "test_history.txt", true, 10);
}