#pragma once

#include <string>
#include <vector>
#include <queue>

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
    bool Readline(string& line) override {
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
    void WipeLine() override { wiped = true; }
    void RefreshLine() override { refreshed = true; }
    
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