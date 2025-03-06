#pragma once

#include "ILineEditor.hpp"
#include "../../../libs/yhirose/cpp-linenoise/linenoise.hpp"

using namespace std;

namespace tools::cmd {

    class LinenoiseAdapter : public ILineEditor {
    public:
        void SetCompletionCallback(CompletionCallback callback) override {
            linenoise::SetCompletionCallback(callback);
        }

        void SetMultiLine(bool enable) override {
            linenoise::SetMultiLine(enable);
        }

        void SetHistoryMaxLen(size_t max_len) override {
            linenoise::SetHistoryMaxLen(max_len);
        }

        void LoadHistory(const char* path) override {
            linenoise::LoadHistory(path);
        }

        void SaveHistory(const char* path) override {
            linenoise::SaveHistory(path);
        }

        void AddHistory(const char* line) override {
            linenoise::AddHistory(line);
        }

        bool Readline(const char* prompt, string& line) override {
            return linenoise::Readline(prompt, line);
        }
    };

} // namespace tools::cmd