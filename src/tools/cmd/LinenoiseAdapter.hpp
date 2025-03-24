#pragma once

#include "../../../libs/yhirose/cpp-linenoise/linenoise.hpp"

#include "LineEditor.hpp"

using namespace std;
using namespace linenoise;

namespace tools::cmd {

    class LinenoiseAdapter : public LineEditor {
    public:
        LinenoiseAdapter(const string& prompt, int stdin_fd = STDIN_FILENO, int stdout_fd = STDOUT_FILENO): 
            l(prompt.empty() ? NULL : prompt.c_str(), stdin_fd, stdout_fd) {}

        void setCompletionCallback(CompletionCallback callback) override {
            l.SetCompletionCallback(callback);
        }

        void setMultiLine(bool enable) override {
            if (enable) l.EnableMultiLine();
            else l.DisableMultiLine();
        }

        void setHistoryMaxLen(size_t max_len) override {
            l.SetHistoryMaxLen(max_len);
        }

        void loadHistory(const char* path) override {
            l.LoadHistory(path);
        }

        void saveHistory(const char* path) override {
            l.SaveHistory(path);
        }

        void addHistory(const char* line) override {
            l.AddHistory(line);
        }

        bool readLine(string& line) override {
            return l.Readline(line);
        }

        void refreshLine() override {
            l.RefreshLine();
        }

        void wipeLine() override {
            l.WipeLine();
        }

        void setPrompt(const char* prompt) override {
            wipeLine();
            l.SetPrompt(prompt);
            refreshLine();
        }

        void setPrompt(string& prompt) override {
            wipeLine();
            l.SetPrompt(prompt);
            refreshLine();
        }

    protected:
        linenoiseState l;
    };

} // namespace tools::cmd