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
        using KeypressCallback = function<void()>;

        virtual void SetCompletionCallback(CompletionCallback /*callback*/) UNIMP_THROWS
        virtual void SetKeypressCallback(KeypressCallback /*callback*/) UNIMP_THROWS
        virtual void SetMultiLine(bool /*enable*/) UNIMP_THROWS
        virtual void SetHistoryMaxLen(size_t /*max_len*/) UNIMP_THROWS
        virtual void LoadHistory(const char* /*path*/) UNIMP_THROWS
        virtual void SaveHistory(const char* /*path*/) UNIMP_THROWS
        virtual void AddHistory(const char* /*line*/) UNIMP_THROWS
        virtual bool Readline(const char* /*prompt*/, string& /*line*/) UNIMP_THROWS // Returns true if exited (deprecated)
        virtual bool Readline(string& /*line*/) UNIMP_THROWS // Returns true if exited
        virtual void RefreshLine() UNIMP_THROWS
        virtual void WipeLine() UNIMP_THROWS
        virtual void SetPrompt(const char* /*prompt*/) UNIMP_THROWS
        virtual void SetPrompt(string& /*prompt*/) UNIMP_THROWS
        virtual string GetPrompt() UNIMP_THROWS
    };

} // namespace tools::cmd