#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <map>
#include <functional>
#include <dirent.h>

#include "../../libs/yhirose/cpp-linenoise/linenoise.hpp"

#include "strings.hpp"
#include "vectors.hpp"

namespace tools {

    class CompletionMatcher {
    private:

        struct PatternPart {
            bool is_parameter;
            string value;
        };

        struct CommandPattern {
            vector<PatternPart> parts;
        };

        vector<PatternPart> parse_pattern(const string& pattern) {
            vector<PatternPart> parts;
            vector<string> raw_parts = split(pattern);
            for (const string& part : raw_parts) {
                if (part.size() >= 2 && part.front() == '{' && part.back() == '}') {
                    string param_type = part.substr(1, part.size() - 2);
                    parts.push_back({true, param_type});
                } else {
                    parts.push_back({false, part});
                }
            }
            return parts;
        }

        bool matches_current_input(const CommandPattern& pattern, const vector<string>& current_parts) {
            if (pattern.parts.size() < current_parts.size()) {
                return false;
            }
            for (size_t i = 0; i < current_parts.size(); ++i) {
                const string& user_part = current_parts[i];
                const PatternPart& pattern_part = pattern.parts[i];
                if (!pattern_part.is_parameter) {
                    if (!str_starts_with(pattern_part.value, user_part)) {
                        return false;
                    }
                }
            }
            return true;
        }

        vector<string> generate_param_completions(const string& param_type, const string& partial) {
            auto it = handlers.find(param_type);
            if (it != handlers.end()) {
                return it->second(partial);
            }
            return {};
        }

        vector<string> get_completions_internal(const string& input, const vector<string>& command_patterns) {
            bool has_trailing_space;
            vector<string> current_parts = parse_input(input, has_trailing_space);

            vector<CommandPattern> patterns;
            for (const auto& pattern_str : command_patterns) {
                CommandPattern pattern;
                pattern.parts = parse_pattern(pattern_str);
                patterns.push_back(pattern);
            }

            vector<string> completions;

            for (const auto& pattern : patterns) {
                if (!matches_current_input(pattern, current_parts)) {
                    continue;
                }

                int next_part_index = current_parts.size() - 1;
                if (next_part_index >= pattern.parts.size()) {
                    continue;
                }

                const PatternPart& next_part = pattern.parts[next_part_index];
                string partial = has_trailing_space ? "" : current_parts.back();

                if (next_part.is_parameter) {
                    vector<string> options = generate_param_completions(next_part.value, partial);
                    completions.insert(completions.end(), options.begin(), options.end());
                } else {
                    if (has_trailing_space) {
                        completions.push_back(next_part.value);
                    } else {
                        if (str_starts_with(next_part.value, partial)) {
                            completions.push_back(next_part.value);
                        }
                    }
                }
            }

            sort(completions.begin(), completions.end());
            auto last = unique(completions.begin(), completions.end());
            completions.erase(last, completions.end());

            // Remove the last word of the input from completions if it exists
            if (!current_parts.empty()) {
                string last_word = current_parts.back();
                auto it = find(completions.begin(), completions.end(), last_word);
                if (it != completions.end()) {
                    completions.erase(it);
                }
            }

            return completions;
        }

    public:
    
        // Add unique handlers if you need to
        const map<string, function<vector<string>(const string&)>> handlers = {
            {"switch", [](const string& partial) {
                vector<string> options = {"on", "off"};
                vector<string> completions;
                for (const auto& opt : options) {
                    if (str_starts_with(opt, partial)) {
                        completions.push_back(opt);
                    }
                }
                return completions;
            }},
            {"filename", [](const string& partial) {
                vector<string> files;
                DIR* dir = opendir(".");
                if (dir == nullptr) return files;
                struct dirent* entry;
                while ((entry = readdir(dir)) != nullptr) {
                    string filename = entry->d_name;
                    if (filename == "." || filename == "..") continue;
                    if (str_starts_with(filename, partial)) {
                        files.push_back(filename);
                    }
                }
                closedir(dir);
                return files;
            }}
        };

        // Define a list of command patterns
        vector<string> command_patterns = {};

        vector<string> parse_input(const string& input, bool& has_trailing_space, bool quote_strings = true) {
            vector<string> parts;
            string current_part;
            bool in_quotes = false;
            bool escape_next = false;
            bool is_string = false;
            has_trailing_space = false;

            for (size_t i = 0; i < input.size(); ++i) {
                char c = input[i];
                if (escape_next) {
                    current_part += c;
                    escape_next = false;
                    continue;
                }
                if (c == '\\') {
                    escape_next = true;
                    continue;
                }
                if (c == '"') {
                    in_quotes = !in_quotes;
                    is_string = true;
                    continue;
                }
                if (c == ' ' && !in_quotes) {
                    if (!current_part.empty()) {
                        if (is_string && quote_strings) current_part = '"' + escape(current_part) + '"';
                        parts.push_back(current_part);
                        current_part.clear();
                        is_string = false;
                    }
                    continue;
                }
                current_part += c;
            }

            if (escape_next) {
                current_part += '\\';
            }

            if (!current_part.empty()) {
                if (is_string && quote_strings) current_part = '"' + escape(current_part) + '"';
                parts.push_back(current_part);
            }

            has_trailing_space = !input.empty() && input.back() == ' ' && !in_quotes;
            if (has_trailing_space) {
                parts.push_back("");
            }

            return parts;
        }

        vector<string> get_completions(const string& input) {
            if (input.empty() || input.back() == ' ') {
                return get_completions_internal(input, command_patterns);
            }

            vector<string> completions = get_completions_internal(input, command_patterns);
            vector<string> completions_with_space;
            if (completions.empty()) completions_with_space = get_completions_internal(input + " ", command_patterns);

            vector<string> completion_all = array_merge(completions, completions_with_space);
            sort(completion_all);
            return completion_all;
        }

    };

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


    class Command {
    public:
        virtual ~Command() {}

        virtual vector<string> get_patterns() const UNIMP
        
        /**
         * @brief 
         * 
         * @param user 
         * @param args 
         * @return string
         */
        virtual string run(void*, const vector<string>&) UNIMP
    };

    class Commander {
    private:
        CommandLine command_line;
        CompletionMatcher cmatcher;
        bool exiting = false;
        vector<void*> commands;
    public:
        Commander(const CommandLine& command_line): command_line(command_line) {}

        virtual ~Commander() {}

        CommandLine& get_command_line_ref() {
            return command_line;
        }

        CompletionMatcher& get_cmatcher_ref() {
            return cmatcher;
        }

        bool is_exiting() const {
            return exiting || command_line.is_exited();
        }
        
        void set_commands(const vector<void*>& commands) {
            this->commands = commands;
            
            // also update command patterns:

            cmatcher.command_patterns = {};
            for (const void* command: commands)
                cmatcher.command_patterns = array_merge(
                    cmatcher.command_patterns, 
                    ((Command*)command)->get_patterns()
                );
            command_line.set_completion_matcher(cmatcher);
        }

        vector<void*> get_commands_ref() {
            return commands;
        }

        const CompletionMatcher& get_cmatcher_ref() const {
            return cmatcher;
        }

        void exit() {            
            exiting = true;
        }


        bool run_command(void* user_context, const string& input) {
            if (input.empty()) return false;
            
            bool trlspc;
            vector<string> input_parts = array_filter(cmatcher.parse_input(input, trlspc, false));

            bool command_found = false;
            bool command_arguments_matches = false;
            for (void* command_void: commands) {
                Command* command = (Command*)command_void;
                for (const string& command_pattern: command->get_patterns()) {
                    vector<string> command_pattern_parts = array_filter(explode(" ", command_pattern));
                    if (input_parts[0] == command_pattern_parts[0]) {
                        command_found = true;
                        if (input_parts.size() == command_pattern_parts.size()) {
                            command_arguments_matches = true;
                            cout << command->run(user_context, input_parts) << endl;
                            break;
                        }
                    }
                }
                if (command_found) break;
            }
            if (!command_found) cout << "Command not found: " << input_parts[0] << endl;
            else if (!command_arguments_matches) cout << "Invalid argument(s)." << endl;
            return command_arguments_matches;
        }
    };

}
