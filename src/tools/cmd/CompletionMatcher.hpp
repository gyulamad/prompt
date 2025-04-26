#pragma once

#include <map>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <dirent.h>

#include "../str/str_starts_with.hpp"
#include "../str/split.hpp"
#include "../str/escape.hpp"
#include "../containers/array_merge.hpp"
#include "../containers/sort.hpp"

using namespace std;
using namespace tools::str;
using namespace tools::containers;

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

        // Helper function (example implementation)
        vector<string> generate_param_completions(const string& param_type, const string& partial) {
            if (param_type == "switch") {
                vector<string> options = {"on", "off"};
                vector<string> matches;
                for (const string& opt: options) {
                    if (str_starts_with(opt, partial)) {
                        matches.push_back(opt);
                    }
                }
                return matches; // For "of", returns {"off"}
            }
            return {};
        }

        vector<string> get_completions_internal(const string& input, const vector<string>& commandPatterns) {
            // Parse input and detect trailing space
            bool has_trailing_space;
            vector<string> current_parts = parse_input(input, has_trailing_space);
        
            // Parse command patterns into structured form
            vector<CommandPattern> patterns;
            for (const string& pattern_str: commandPatterns) {
                CommandPattern pattern;
                pattern.parts = parse_pattern(pattern_str);
                patterns.push_back(pattern);
            }
        
            vector<string> completions;
        
            // Case 1: Empty input - suggest first word of each pattern
            if (current_parts.empty() || (current_parts.size() == 1 && current_parts[0].empty())) {
                for (const CommandPattern& pattern: patterns) {
                    if (!pattern.parts.empty()) {
                        completions.push_back(pattern.parts[0].value);
                    }
                }
            }
            // Case 2: Non-empty input
            else {
                for (const CommandPattern& pattern: patterns) {
                    // Check if the input matches the pattern up to the current point
                    if (!matches_current_input(pattern, current_parts)) {
                        continue;
                    }
        
                    // Number of meaningful input parts (exclude trailing empty part if present)
                    int input_parts_count = has_trailing_space ? current_parts.size() - 1 : current_parts.size();
                    int parts_size = (int)pattern.parts.size();
        
                    // If input exceeds pattern length, skip
                    if (input_parts_count > parts_size) {
                        continue;
                    }
        
                    // Determine the next part to suggest
                    int next_index = input_parts_count - (has_trailing_space ? 0 : 1);
                    string partial = has_trailing_space ? "" : current_parts.back();
        
                    // If there's a next part to complete
                    if (next_index >= 0 && next_index < parts_size) {
                        const PatternPart& next_part = pattern.parts[next_index];
                        if (next_part.is_parameter) {
                            // Handle parameter (e.g., switch) completion
                            vector<string> options = generate_param_completions(next_part.value, partial);
                            completions.insert(completions.end(), options.begin(), options.end());
                        } else if (str_starts_with(next_part.value, partial)) {
                            // Handle regular word completion
                            completions.push_back(next_part.value);
                        }
                    }
                }
            }
        
            // Sort and deduplicate completions
            sort(completions.begin(), completions.end());
            auto last = unique(completions.begin(), completions.end());
            completions.erase(last, completions.end());
            return completions;
        }

    public:

        CompletionMatcher() {}
        virtual ~CompletionMatcher() {}
    
        // Add unique handlers if you need to
        const map<string, function<vector<string>(const string&)>> handlers = {
            {"switch", [](const string& partial) {
                vector<string> options = {"on", "off"};
                vector<string> completions;
                for (const string& opt: options) {
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
        vector<string> commandPatterns = {};

        virtual vector<string> parse_input(const string& input, bool& has_trailing_space, bool quote_strings = true) {
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

        virtual vector<string> get_completions(const string& input) {
            if (input.empty() || input.back() == ' ') {
                return get_completions_internal(input, commandPatterns);
            }

            vector<string> completions = get_completions_internal(input, commandPatterns);
            vector<string> completions_with_space;
            if (completions.empty()) completions_with_space = get_completions_internal(input + " ", commandPatterns);

            vector<string> completion_all = array_merge(completions, completions_with_space);
            sort(completion_all);
            return completion_all;
        }

    };
    
}

#ifdef TEST

#include "../utils/Test.hpp"

using namespace tools::cmd;

void test_CompletionMatcher_parse_input_basic() {
    CompletionMatcher cm;
    bool has_trailing_space;
    vector<string> actual = cm.parse_input("test cmd", has_trailing_space);
    assert(actual.size() == 2 && "parse_input should split basic input into two parts");
    assert(actual[0] == "test" && "parse_input should correctly parse first word");
    assert(actual[1] == "cmd" && "parse_input should correctly parse second word");
    assert(has_trailing_space == false && "parse_input should detect no trailing space");
}

void test_CompletionMatcher_parse_input_trailing_space() {
    CompletionMatcher cm;
    bool has_trailing_space;
    vector<string> actual = cm.parse_input("test ", has_trailing_space);
    assert(actual.size() == 2 && "parse_input should include empty part for trailing space");
    assert(actual[0] == "test" && "parse_input should correctly parse word");
    assert(actual[1] == "" && "parse_input should add empty string for trailing space");
    assert(has_trailing_space == true && "parse_input should detect trailing space");
}

void test_CompletionMatcher_parse_input_quoted_string() {
    CompletionMatcher cm;
    bool has_trailing_space;
    vector<string> actual = cm.parse_input("test \"quoted arg\"", has_trailing_space);
    assert(actual.size() == 2 && "parse_input should handle quoted string as one part");
    assert(actual[0] == "test" && "parse_input should parse first word");
    assert(actual[1] == "\"quoted arg\"" && "parse_input should keep quoted string intact");
    assert(has_trailing_space == false && "parse_input should detect no trailing space");
}

void test_CompletionMatcher_get_completions_empty_input() {
    CompletionMatcher cm;
    cm.commandPatterns = {"test cmd", "other"};
    vector<string> actual = cm.get_completions("");
    assert(actual.size() == 2 && "get_completions should return all first words for empty input");
    assert(actual[0] == "other" && "get_completions should include first pattern's first word (sorted)");
    assert(actual[1] == "test" && "get_completions should include second pattern's first word (sorted)");
}

void test_CompletionMatcher_get_completions_partial_match() {
    CompletionMatcher cm;
    cm.commandPatterns = {"test cmd", "test param", "other"};
    vector<string> actual = cm.get_completions("test ");
    assert(actual.size() == 2 && "get_completions should return next words for matching patterns");
    assert(find(actual.begin(), actual.end(), "cmd") != actual.end() && "get_completions should include 'cmd'");
    assert(find(actual.begin(), actual.end(), "param") != actual.end() && "get_completions should include 'param'");
}

void test_CompletionMatcher_get_completions_trailing_space() {
    CompletionMatcher cm;
    cm.commandPatterns = {"test cmd arg"};
    vector<string> actual = cm.get_completions("test ");
    assert(actual.size() == 1 && "get_completions should suggest next part with trailing space");
    assert(actual[0] == "cmd" && "get_completions should suggest next word after space");
}

void test_CompletionMatcher_get_completions_switch_handler() {
    CompletionMatcher cm;
    cm.commandPatterns = {"set {switch}"};
    vector<string> actual = cm.get_completions("set ");
    assert(actual.size() == 2 && "get_completions should return switch options");
    assert(actual[0] == "off" && "get_completions should include 'off' from switch handler");
    assert(actual[1] == "on" && "get_completions should include 'on' from switch handler");
}

void test_CompletionMatcher_get_completions_partial_switch() {
    CompletionMatcher cm;
    cm.commandPatterns = {"set {switch}"};
    vector<string> actual = cm.get_completions("set of");
    assert(actual.size() == 1 && "get_completions should filter switch options");
    assert(actual[0] == "off" && "get_completions should match partial 'of' to 'off'");
}

void test_CompletionMatcher_get_completions_no_matches() {
    CompletionMatcher cm;
    cm.commandPatterns = {"test cmd"};
    vector<string> actual = cm.get_completions("xyz");
    assert(actual.empty() && "get_completions should return empty vector for no matches");
}


TEST(test_CompletionMatcher_parse_input_basic);
TEST(test_CompletionMatcher_parse_input_trailing_space);
TEST(test_CompletionMatcher_parse_input_quoted_string);
TEST(test_CompletionMatcher_get_completions_empty_input);
TEST(test_CompletionMatcher_get_completions_partial_match);
TEST(test_CompletionMatcher_get_completions_trailing_space);
TEST(test_CompletionMatcher_get_completions_switch_handler);
TEST(test_CompletionMatcher_get_completions_partial_switch);
TEST(test_CompletionMatcher_get_completions_no_matches);

#endif