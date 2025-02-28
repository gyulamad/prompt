#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <dirent.h>

#include "../strings.hpp"
#include "../vectors.hpp"


using namespace std;
using namespace tools;

namespace tools::cmd {

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
    
}