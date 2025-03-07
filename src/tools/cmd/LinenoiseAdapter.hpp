#pragma once

#include "ILineEditor.hpp"
#include "../../../libs/gyulamad/cpp-linenoise/linenoise.hpp"

using namespace std;

namespace tools::cmd {

    class LinenoiseAdapter : public ILineEditor {
    protected:
        linenoise::linenoiseState l;
    public:
        LinenoiseAdapter(const string& prompt): l(prompt.c_str()) {}

        void SetCompletionCallback(CompletionCallback callback) override {
            l.SetCompletionCallback(callback);
        }

        void SetMultiLine(bool enable) override {
            l.EnableMultiLine(enable);
        }

        void SetHistoryMaxLen(size_t max_len) override {
            l.SetHistoryMaxLen(max_len);
        }

        void LoadHistory(const char* path) override {
            l.LoadHistory(path);
        }

        void SaveHistory(const char* path) override {
            l.SaveHistory(path);
        }

        void AddHistory(const char* line) override {
            l.AddHistory(line);
        }

        bool Readline(string& line) override {
            return l.Readline(line);
        }

        void RefreshLine() override {
            l.RefreshLine();
        }

        void WipeLine() override {
            l.WipeLine();
        }
    };

} // namespace tools::cmd