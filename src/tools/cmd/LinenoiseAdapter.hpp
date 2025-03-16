#pragma once

#include "../../../libs/yhirose/cpp-linenoise/linenoise.hpp"

#include "ILineEditor.hpp"

using namespace std;
using namespace linenoise;

namespace tools::cmd {

    class LinenoiseAdapter : public ILineEditor {
    public:
        LinenoiseAdapter(const string& prompt, int stdin_fd = STDIN_FILENO, int stdout_fd = STDOUT_FILENO): 
            l(prompt.empty() ? NULL : prompt.c_str(), stdin_fd, stdout_fd) {}

        void SetCompletionCallback(CompletionCallback callback) override {
            l.SetCompletionCallback(callback);
        }

        void SetMultiLine(bool enable) override {
            if (enable) l.EnableMultiLine();
            else l.DisableMultiLine();
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

        void SetPrompt(const char* prompt) override {
            WipeLine();
            l.SetPrompt(prompt);
            RefreshLine();
        }

        void SetPrompt(string& prompt) override {
            WipeLine();
            l.SetPrompt(prompt);
            RefreshLine();
        }

        // string GetPrompt() override {
        //     return l.prompt;
        // }

        // void SetKeypressCallback(KeypressCallback cb) override {
        //     l.SetKeypressCallback(cb);
        // }

    protected:
        linenoiseState l;
    };

} // namespace tools::cmd