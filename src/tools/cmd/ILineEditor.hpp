#pragma once

#include <functional>
#include <string>
#include <vector>

using namespace std;

namespace tools::cmd {

    class ILineEditor {
    public:
        virtual ~ILineEditor() = default;

        using CompletionCallback = function<void(const char*, vector<string>&)>;

        virtual void SetCompletionCallback(CompletionCallback callback) = 0;
        virtual void SetMultiLine(bool enable) = 0;
        virtual void SetHistoryMaxLen(size_t max_len) = 0;
        virtual void LoadHistory(const char* path) = 0;
        virtual void SaveHistory(const char* path) = 0;
        virtual void AddHistory(const char* line) = 0;
        virtual bool Readline(const char* prompt, string& line) = 0; // Returns true if exited
    };

} // namespace tools::cmd