#pragma once

#include <string>

#include "../../../libs/yhirose/cpp-linenoise/linenoise.hpp"

#include "CompletionMatcher.hpp"

using namespace std;
using namespace tools;

namespace tools::cmd {

    class CommandLine {
    public:
        // CompletionMatcher* completion_matcher = nullptr;
        // typedef function<void(const char*, vector<string>&)> completion_callback_t;
        // completion_callback_t completion_callback = [&](const char* user_input, vector<string>& completions) {
        //     // Get completions based on the current input
        //     completions = completion_matcher.get_completions(user_input);
        // };
    
        bool exited = false;
        string prompt;
        string history_path;
        bool multi_line;
        size_t history_max_length;
    public:

        CommandLine(
            const string& prompt = "> ",
            const string& history_path = "",
            bool multi_line = true,
            size_t history_max_length = 0
        ):
            prompt(prompt),
            history_path(history_path),
            multi_line(multi_line),
            history_max_length(history_max_length)
        {}

        ~CommandLine() {}

        bool is_exited() const {
            return exited;
        }

        void set_prompt(const string& prompt) {
            this->prompt = prompt;
        }

        string get_prompt() const {
            return this->prompt;
        }

        void set_completion_matcher(CompletionMatcher& completion_matcher) {
            // Setup completion words every time when a user types
            linenoise::SetCompletionCallback([&](const char* input, vector<string>& completions) {
                // Get completions based on the current input
                    
                    vector<string> all_completions = completion_matcher.get_completions(input);
                    if (all_completions.size() <= 1) {
                        string input_s(input ? input : "");
                        bool has_trailing_space;
                        vector<string> parts = completion_matcher.parse_input(input, has_trailing_space);
                        if (!has_trailing_space && !parts.empty()) {
                            parts.pop_back();
                            input_s = implode(" ", parts);
                        }

                        if (!input_s.empty()) input_s += " ";
                        for (const string& elem: all_completions) 
                            completions.push_back(input_s + elem);

                        // completions = all_completions;
                        return;                        
                    }
                    
                    // cout << "\r";
                    // for (size_t i = 0; i < prompt.size(); i++) cout << " ";
                    // for (int i = 0; input[i++] != '\0';) cout << " ";
                    cout << endl;
                    
                    for (const string& elem: all_completions) 
                        cout << elem << endl;

                    show(input);
            });
        }

        void show(const string& input = "") {
            cout << prompt << input << flush;
        }

        string readln() {
            

            // linenoise::SetCompletionCallback(completion_callback);

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