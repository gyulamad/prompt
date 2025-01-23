#pragma once

#include "../../libs/yhirose/cpp-linenoise/linenoise.hpp"

namespace tools {

    class CommandLine {
    public:
        typedef function<void(const char*, vector<string>&)> completion_callback_t;
    private:
        bool exited = false;
        string prompt;
        completion_callback_t completion_callback;
        string history_path;
        bool multi_line;
        size_t history_max_length;
    public:
        CommandLine(
            const string& prompt = "> ",
            completion_callback_t completion_callback = [](const char*, vector<string>&) {},
            const string& history_path = "",
            bool multi_line = true,
            size_t history_max_length = 0
        ):
            prompt(prompt),
            completion_callback(completion_callback),
            history_path(history_path),
            multi_line(multi_line),
            history_max_length(history_max_length)
        {}

        ~CommandLine() {}

        bool is_exited() {
            return exited;
        }

        void set_prompt(const string& prompt) {
            this->prompt = prompt;
        }

        string readln() {
            // Setup completion words every time when a user types
            // linenoise::SetCompletionCallback([](const char* editBuffer, std::vector<std::string>& completions) {
            //     if (editBuffer[0] == 'h') {
            //         completions.push_back("hi");
            //         completions.push_back("hello");
            //         completions.push_back("hello there");
            //     }
            // });
            linenoise::SetCompletionCallback(completion_callback);

            // Enable the multi-line mode
            linenoise::SetMultiLine(multi_line);

            // Set max length of the history
            if (history_max_length) linenoise::SetHistoryMaxLen(history_max_length);

            // Load history
            if (!history_path.empty()) linenoise::LoadHistory(history_path.c_str());

            // Read line
            string line;
            exited = linenoise::Readline(prompt.c_str(), line);

            // Add text to history
            linenoise::AddHistory(line.c_str());

            // Save history
            if (!history_path.empty()) linenoise::SaveHistory(history_path.c_str());

            return line;
        }
    };

}
