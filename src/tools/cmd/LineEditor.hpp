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
        using KeypressCallback = function<void()>;

        virtual void setCompletionCallback(CompletionCallback /*callback*/) UNIMP_THROWS
        virtual void setKeypressCallback(KeypressCallback /*callback*/) UNIMP_THROWS
        virtual void setMultiLine(bool /*enable*/) UNIMP_THROWS
        virtual void setHistoryMaxLen(size_t /*max_len*/) UNIMP_THROWS
        virtual void loadHistory(const char* /*path*/) UNIMP_THROWS
        virtual void saveHistory(const char* /*path*/) UNIMP_THROWS
        virtual void addHistory(const char* /*line*/) UNIMP_THROWS
        virtual bool readLine(const char* /*prompt*/, string& /*line*/) UNIMP_THROWS // Returns true if exited (deprecated)
        virtual bool readLine(string& /*line*/) UNIMP_THROWS // Returns true if exited
        virtual void refreshLine() UNIMP_THROWS
        virtual void wipeLine() UNIMP_THROWS
        virtual void setPrompt(const char* /*prompt*/) UNIMP_THROWS
        virtual void setPrompt(string& /*prompt*/) UNIMP_THROWS
        virtual string getPrompt() UNIMP_THROWS
    };

} // namespace tools::cmd