#pragma once

#include <functional>
#include <string>
#include <vector>

#include "../utils/ERROR.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::cmd {

    class ILineEditor {
    public:
        virtual ~ILineEditor() = default;

        using CompletionCallback = function<void(const char*, vector<string>&)>;

        virtual void SetCompletionCallback(CompletionCallback callback) UNIMP
        virtual void SetMultiLine(bool enable) UNIMP
        virtual void SetHistoryMaxLen(size_t max_len) UNIMP
        virtual void LoadHistory(const char* path) UNIMP
        virtual void SaveHistory(const char* path) UNIMP
        virtual void AddHistory(const char* line) UNIMP
        virtual bool Readline(const char* prompt, string& line) UNIMP // Returns true if exited (deprecated)
        virtual bool Readline(string& line) UNIMP // Returns true if exited
        virtual void RefreshLine() UNIMP
        virtual void WipeLine() UNIMP
    };

} // namespace tools::cmd