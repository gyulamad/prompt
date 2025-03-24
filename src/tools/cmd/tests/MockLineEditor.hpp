#pragma once

#include <string>
#include <vector>
#include <queue>

#include "../LineEditor.hpp"
#include "../CommandLine.hpp"

using namespace std;
using namespace tools::cmd;

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

    void setCompletionCallback(CompletionCallback) override {}
    void setMultiLine(bool enable) override { multi_line_enabled = enable; }
    void setHistoryMaxLen(size_t len) override { max_history_len = len; }
    void loadHistory(const char* path) override { loaded_history_path = path; }
    void saveHistory(const char* path) override { saved_history_path = path; }
    void addHistory(const char* line) override { history.push_back(line); }
    bool readLine(string& line) override {
        line = "";
        if (useQueue) {
            if (inputs.empty()) return true;
            line = inputs.front();
            inputs.pop();
            return line == "ctrl+c";
        }
        line = next_input;
        return should_exit;
    }
    void wipeLine() override { wiped = true; }
    void refreshLine() override { refreshed = true; }
    
    void queueInput(const string& input) { useQueue = true; inputs.push(input); }
    bool wasWiped() const { return wiped; }
    bool wasRefreshed() const { return refreshed; }
    void resetFlags() { wiped = false; refreshed = false; }
    
private:
    bool useQueue = false;
    string prompt;
    queue<string> inputs;
    bool wiped = false;
    bool refreshed = false;
};