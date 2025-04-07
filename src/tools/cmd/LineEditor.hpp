#pragma once

#include <functional>
#include <string>
#include <vector>

#include "../utils/ERROR.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::cmd {

    class LineEditor {
    public:
        virtual ~LineEditor() = default;

        using CompletionCallback = function<void(const char*, vector<string>&)>;
        // using KeypressCallback = function<void()>;

        virtual void setCompletionCallback(CompletionCallback /*callback*/) = 0;
        // virtual void setKeypressCallback(KeypressCallback /*callback*/) = 0;
        virtual void setMultiLine(bool /*enable*/) = 0;
        virtual void setHistoryMaxLen(size_t /*max_len*/) = 0;
        virtual void loadHistory(const char* /*path*/) = 0;
        virtual void saveHistory(const char* /*path*/) = 0;
        virtual void addHistory(const char* /*line*/) = 0; // TODO: sometimes when I /exit: Worker 'user' error: Unimplemented function: addHistory
        // virtual bool readLine(const char* /*prompt*/, string& /*line*/) = 0; // Returns true if exited (deprecated)
        virtual bool readLine(string& /*line*/) = 0; // Returns true if exited
        virtual void refreshLine() = 0;
        virtual void wipeLine() = 0;
        virtual void setPrompt(const char* /*prompt*/) = 0;
        virtual void setPrompt(string& /*prompt*/) = 0;
        // virtual string getPrompt() = 0;
    };

} // namespace tools::cmd