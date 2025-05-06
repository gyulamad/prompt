#pragma once

#include <string>
#include <vector>
#include <queue>

#include "../LineEditor.hpp"
// #include "../CommandLine.hpp"

#include "../../str/str_ends_with.hpp"
#include "../../str/explode.h"

using namespace std;
using namespace tools::cmd;
using namespace tools::str;

// MockLineEditor definition (simplified for testing)
class MockLineEditor : public LineEditor {
public:
    string next_input;
    bool should_exit = false;
    vector<string> history;
    bool multi_line_enabled = false;
    size_t max_history_len = 0;
    string loaded_history_path;
    string saved_history_path;
    CompletionCallback stored_cb;
    vector<string> actual_completions;
    bool wiped = false;

    using LineEditor::LineEditor;
    virtual ~MockLineEditor() {}

    void setCompletionCallback(CompletionCallback cb) override { stored_cb = cb; }
    void setMultiLine(bool enable) override { multi_line_enabled = enable; }
    void setHistoryMaxLen(size_t len) override { max_history_len = len; }
    void loadHistory(const char* path) override { loaded_history_path = path; }
    void saveHistory(const char* path) override { saved_history_path = path; }
    void addHistory(const char* line) override { history.push_back(line); }
    bool readLine(string& line) override {
        string input = line;
        if (str_ends_with(input, "[TAB]")) {
            actual_completions = {}; // ???
            stored_cb(explode("[TAB]", input)[0].c_str(), actual_completions);
            return false;
        }
        line = "";
        // if (useQueue) {
        //     if (inputs.empty()) return true;
        //     line = inputs.front();
        //     inputs.pop();
        //     return line == "ctrl+c";
        // }
        line = next_input;
        return should_exit;
    }
    void wipeLine() override { wiped = true; }
    void refreshLine() override { refreshed = true; }
    
    // void queueInput(const string& input) { useQueue = true; inputs.push(input); }
    bool wasWiped() const { return wiped; }
    bool wasRefreshed() const { return refreshed; }
    void resetFlags() { wiped = false; refreshed = false; }

    // LCOV_EXCL_START
    void setPrompt(const char* /*prompt*/) override  {}
    void setPrompt(string& /*prompt*/) override {}
    // LCOV_EXCL_STOP
    
private:
    // bool useQueue = false;
    string prompt;
    queue<string> inputs;
    bool refreshed = false;
};