#pragma once

#include <regex>

#include "ERROR.hpp"

using namespace std;
using namespace regex_constants;

namespace tools::utils {

    inline int regx_match(
        const string& pattern, 
        const string& str, 
        vector<string>* matches = nullptr,
        syntax_option_type flags = ECMAScript  // Add flags parameter
    ) {
        regex r(pattern, flags);
        smatch m;
        if (regex_search(str, m, r)) {
            if (matches != nullptr) {
                // Clear the vector before adding more matches
                matches->clear();
                for (unsigned int i = 0; i < m.size(); i++)
                    matches->push_back(m[i].str());
            }
            return 1;
        }
        return 0;
    }

    inline int regx_match_all(
        const string& pattern,
        const string& str,
        vector<string>* matches = nullptr,
        syntax_option_type flags = ECMAScript
    ) {
        // Check for empty pattern
        if (pattern.empty()) {
            if (matches != nullptr) {
                matches->clear();
                for (size_t i = 0; i <= str.size(); ++i)
                    matches->push_back(""); // Empty pattern matches every position
            }
            return static_cast<int>(str.size() + 1); // Return number of matches (positions)
        }
    
        // Regular regex matching
        try {
            regex r(pattern, flags);
            sregex_iterator begin(str.begin(), str.end(), r), end;
            int match_count = 0;
    
            if (matches != nullptr) {
                matches->clear();
                for (sregex_iterator it = begin; it != end; ++it) {
                    smatch m = *it;
                    matches->push_back(m[0].str()); // Add the full match
                    for (size_t i = 1; i < m.size(); ++i)
                        matches->push_back(m[i].str()); // Add each capture group
                    ++match_count;
                }
            }
    
            return match_count;
        } catch (const regex_error&) {
            // Handle invalid regex patterns
            if (matches != nullptr) matches->clear();
            return 0; // No matches for invalid patterns
        }
    }

    /// @brief Replace first regex match in string.
    /// @param pattern regex pattern to match 
    /// @param str input string
    /// @param replace string to replace matches with
    /// @return string with first match replaced
    inline string regx_replace(const string& pattern, const string& str, const string& replace) {
        if (pattern.empty()) throw ERROR("Regex pattern can not be empty"); // Explicitly throw for empty patterns
        regex r(pattern);
        return regex_replace(str, r, replace, regex_constants::format_first_only);
    }

    /// @brief Replace all regex matches in string.
    /// @param pattern regex pattern to match
    /// @param str input string 
    /// @param replace string to replace matches with 
    /// @return string with all matches replaced
    inline string regx_replace_all(const string& pattern, const string& str, const string& replace) {
        if (pattern.empty()) throw ERROR("Regex pattern can not be empty"); // Explicitly throw for empty patterns
        regex r(pattern);
        return regex_replace(str, r, replace);
    }
    
}


#ifdef TEST

using namespace tools::utils;

void test_regx_match_basic() {
    string pattern = "hello";
    string str = "hello world";
    int result = regx_match(pattern, str);
    assert(result == 1 && "Basic match");
}

void test_regx_match_no_match() {
    string pattern = "goodbye";
    string str = "hello world";
    int result = regx_match(pattern, str);
    assert(result == 0 && "No match");
}

void test_regx_match_capture_groups() {
    string pattern = "(hello) (world)";
    string str = "hello world";
    vector<string> matches;
    int result = regx_match(pattern, str, &matches);
    assert(result == 1 && "Match with capture groups");
    assert(matches.size() == 3 && "Capture group count");
    assert(matches[0] == "hello world" && "Full match");
    assert(matches[1] == "hello" && "First capture group");
    assert(matches[2] == "world" && "Second capture group");
}

void test_regx_match_empty_pattern() {
    string pattern = "";
    string str = "hello world";
    int result = regx_match(pattern, str);
    assert(result == 1 && "Empty pattern (matches any string)");
}

void test_regx_match_empty_string() {
    string pattern = "hello";
    string str = "";
    int result = regx_match(pattern, str);
    assert(result == 0 && "Empty string");
}

void test_regx_match_special_characters() {
    string pattern = "\\d+"; // Matches one or more digits
    string str = "123abc";
    int result = regx_match(pattern, str);
    assert(result == 1 && "Special characters (digits)");
}

// Test case-insensitive match with multiple matches
void test_regx_match_case_insensitive_multiple_matches() {
    string pattern = "hello";
    string str = "HELLO hello HeLlO";
    vector<string> matches;
    int result = regx_match(pattern, str, &matches, regex_constants::icase);
    assert(result == 1 && "test_regx_match_case_insensitive_multiple_matches failed");
    assert(matches.size() == 1 && matches[0] == "HELLO" && "First match incorrect");
}

// Test case-insensitive match with regex_constants::icase
void test_regx_match_case_insensitive_with_flag() {
    string pattern = "WORLD";
    string str = "Hello World";
    int result = regx_match(pattern, str, nullptr, regex_constants::icase);
    assert(result == 1 && "test_regx_match_case_insensitive_with_flag failed");
}

// Test case-sensitive match (default behavior)
void test_regx_match_case_sensitive() {
    string pattern = "HELLO";
    string str = "hello world";
    int result = regx_match(pattern, str);  // No flag (case-sensitive)
    assert(result == 0 && "test_regx_match_case_sensitive failed");
}

void test_regx_match_case_insensitive() {
    string pattern = "HELLO";
    string str = "hello world";
    int result = regx_match(pattern, str, nullptr, regex_constants::icase);  // Pass icase flag
    assert(result == 1 && "test_regx_match_case_insensitive failed");
}

void test_regx_match_multiple_matches() {
    string pattern = "\\b\\w+\\b"; // Matches whole words
    string str = "hello world";
    vector<string> matches;
    int result = regx_match(pattern, str, &matches);
    assert(result == 1 && "Multiple matches");
    assert(matches.size() == 1 && "First match only (regex_search behavior)");
    assert(matches[0] == "hello" && "First match");
}

// Test invalid regex pattern
void test_regx_match_invalid_pattern() {
    bool thrown = false;
    try {
        string pattern = "(?i)HELLO";  // Invalid syntax for ECMAScript
        regx_match(pattern, "hello world");
    } catch (const regex_error&) {
        thrown = true;
    }
    assert(thrown && "test_regx_match_invalid_pattern failed");
}

void test_regx_match_all_basic() {
    string pattern = "\\d+"; // Matches one or more digits
    string str = "123 abc 456 def";
    vector<string> matches;
    int result = regx_match_all(pattern, str, &matches);

    // Expect 2 matches: "123" and "456"
    assert(result == 2 && "Basic match all");
    assert(matches.size() == 2 && "Match count");
    assert(matches[0] == "123" && "First match");
    assert(matches[1] == "456" && "Second match");
}

void test_regx_match_all_single_match() {
    string pattern = "\\d+"; // Matches one or more digits
    string str = "123";
    vector<string> matches;
    int result = regx_match_all(pattern, str, &matches);

    assert(result == 1 && "Single match");
    assert(matches.size() == 1 && "Match count");
    assert(matches[0] == "123" && "First match");
}

void test_regx_match_all_no_matches() {
    string pattern = "\\d+"; // Matches one or more digits
    string str = "abc def";
    vector<string> matches;
    int result = regx_match_all(pattern, str, &matches);

    assert(result == 0 && "No matches");
    assert(matches.empty() && "Matches vector should be empty");
}

void test_regx_match_all_no_match() {
    string pattern = "\\d+"; // Matches one or more digits
    string str = "abc def";
    vector<string> matches;
    int result = regx_match_all(pattern, str, &matches);
    assert(result == 0 && "No match");
    assert(matches.empty() && "Empty matches");
}

void test_regx_match_all_capture_groups() {
    string pattern = "(\\d+)(\\w+)"; // Matches digits followed by letters
    string str = "123abc 456def";
    vector<string> matches;
    int result = regx_match_all(pattern, str, &matches);

    // Expected matches: ["123abc", "123", "abc", "456def", "456", "def"]
    assert(result == 2 && "Match count incorrect");
    assert(matches.size() == 6 && "Match count (2 matches, 2 capture groups each)");
    assert(matches[0] == "123abc" && "First full match");
    assert(matches[1] == "123" && "First capture group (digits)");
    assert(matches[2] == "abc" && "Second capture group (letters)");
    assert(matches[3] == "456def" && "Second full match");
    assert(matches[4] == "456" && "Third capture group (digits)");
    assert(matches[5] == "def" && "Fourth capture group (letters)");
}

void test_regx_match_all_single_match_multiple_capture_groups() {
    string pattern = "(\\d+)-(\\w+)";
    string str = "123-abc";
    vector<string> matches;
    int result = regx_match_all(pattern, str, &matches);

    assert(result == 1 && "Match count incorrect");
    assert(matches.size() == 3 && "Match count (1 match, 2 capture groups)");
    assert(matches[0] == "123-abc" && "Full match");
    assert(matches[1] == "123" && "Capture group 1 (digits)");
    assert(matches[2] == "abc" && "Capture group 2 (letters)");
}

void test_regx_match_all_no_matches2() {
    string pattern = "(\\d+)-(\\w+)";
    string str = "no-match-here";
    vector<string> matches;
    int result = regx_match_all(pattern, str, &matches);

    assert(result == 0 && "Match count incorrect");
    assert(matches.empty() && "Matches vector should be empty");
}

void test_regx_match_all_empty_pattern() {
    string pattern = "";
    string str = "hello world";
    vector<string> matches;
    int result = regx_match_all(pattern, str, &matches);

    // Expect 12 matches (empty pattern matches every position)
    assert(result == 12 && "Empty pattern (matches any string)");
    assert(matches.size() == 12 && "Match count (empty pattern matches every position)");

    // Verify that all matches are empty strings
    for (const string& match : matches) {
        assert(match.empty() && "Each match should be an empty string");
    }
}

void test_regx_match_all_empty_string() {
    string pattern = "\\d+";
    string str = "";
    vector<string> matches;
    int result = regx_match_all(pattern, str, &matches);
    assert(result == 0 && "Empty string");
    assert(matches.empty() && "Empty matches");
}

void test_regx_match_all_special_characters() {
    string pattern = "\\W+"; // Matches one or more non-word characters
    string str = "hello, world!";
    vector<string> matches;
    int result = regx_match_all(pattern, str, &matches);

    // Expect 2 matches: ", " and "!"
    assert(result == 2 && "Special characters (non-word)");
    assert(matches.size() == 2 && "Match count");
    assert(matches[0] == ", " && "First match");
    assert(matches[1] == "!" && "Second match");
}

void test_regx_match_all_special_characters_no_matches() {
    string pattern = "\\W+"; // Matches one or more non-word characters
    string str = "helloworld";
    vector<string> matches;
    int result = regx_match_all(pattern, str, &matches);

    assert(result == 0 && "No special characters");
    assert(matches.empty() && "Matches vector should be empty");
}

void test_regx_match_all_special_characters_with_capture_groups() {
    string pattern = "(\\W+)"; // Matches one or more non-word characters with a capture group
    string str = "hello, world!";
    vector<string> matches;
    int result = regx_match_all(pattern, str, &matches);

    // Expect 2 matches: ", " and "!", each with a capture group
    assert(result == 2 && "Special characters with capture groups");
    assert(matches.size() == 4 && "Match count (2 full matches + 2 capture groups)");
    assert(matches[0] == ", " && "First full match");
    assert(matches[1] == ", " && "First capture group");
    assert(matches[2] == "!" && "Second full match");
    assert(matches[3] == "!" && "Second capture group");
}

void test_regx_match_all_case_sensitive() {
    string pattern = "HELLO";
    string str = "hello world HELLO";
    vector<string> matches;
    int result = regx_match_all(pattern, str, &matches);
    assert(result == 1 && "Case sensitive");
    assert(matches.size() == 1 && "Match count");
    assert(matches[0] == "HELLO" && "Case-sensitive match");
}

void test_regx_match_all_case_insensitive() {
    string pattern = "HELLO"; // Case-insensitive match
    string str = "hello world HELLO";
    vector<string> matches;
    int result = regx_match_all(pattern, str, &matches, regex_constants::icase);

    // Expect 2 matches: "hello" and "HELLO"
    assert(result == 2 && "Case insensitive");
    assert(matches.size() == 2 && "Match count");
    assert(matches[0] == "hello" && "First case-insensitive match");
    assert(matches[1] == "HELLO" && "Second case-insensitive match");
}

void test_regx_match_all_invalid_pattern() {
    string pattern = "[invalid"; // Invalid regex pattern
    string str = "hello world";
    vector<string> matches;
    int result = regx_match_all(pattern, str, &matches);

    // Expect no matches for invalid patterns
    assert(result == 0 && "Invalid pattern should return 0 matches");
    assert(matches.empty() && "Matches vector should be empty");
}

void test_regx_match_all_non_empty_pattern() {
    string pattern = "\\w+"; // Matches words
    string str = "hello world";
    vector<string> matches;
    int result = regx_match_all(pattern, str, &matches);

    assert(result == 2 && "Non-empty pattern match count");
    assert(matches.size() == 2 && "Match count");
    assert(matches[0] == "hello" && "First match");
    assert(matches[1] == "world" && "Second match");
}

// Test cases for regx_replace
void test_regx_replace_single_match() {
    string input = "hello world";
    string pattern = "world";
    string replace = "universe";
    string expected = "hello universe";
    string actual = regx_replace(pattern, input, replace);
    assert(actual == expected && "test_regx_replace_single_match failed");
}

void test_regx_replace_no_match() {
    string input = "hello world";
    string pattern = "foo";
    string replace = "bar";
    string expected = "hello world";
    string actual = regx_replace(pattern, input, replace);
    assert(actual == expected && "test_regx_replace_no_match failed");
}

void test_regx_replace_first_occurrence_only() {
    string input = "apple apple apple";
    string pattern = "apple";
    string replace = "fruit";
    string expected = "fruit apple apple";
    string actual = regx_replace(pattern, input, replace);
    assert(actual == expected && "test_regx_replace_first_occurrence_only failed");
}

void test_regx_replace_all() {
    string input = "apple apple apple";
    string pattern = "apple";
    string replace = "fruit";
    string expected = "fruit fruit fruit";
    string actual = regx_replace_all(pattern, input, replace);
    assert(actual == expected && "test_regx_replace_all failed");
}

void test_regx_replace_empty_input() {
    string input = "";
    string pattern = "foo";
    string replace = "bar";
    string expected = "";
    string actual = regx_replace(pattern, input, replace);
    assert(actual == expected && "test_regx_replace_empty_input failed");
}

void test_regx_replace_empty_pattern() {
    bool thrown = false;
    try {
        string input = "hello world";
        string pattern = "";
        string replace = "bar";
        regx_replace(pattern, input, replace);
    } catch (...) {
        thrown = true;
    }
    assert(thrown && "test_regx_replace_empty_pattern failed");
}

// Test cases for regx_replace_all
void test_regx_replace_all_multiple_matches() {
    string input = "apple banana apple";
    string pattern = "apple";
    string replace = "fruit";
    string expected = "fruit banana fruit";
    string actual = regx_replace_all(pattern, input, replace);
    assert(actual == expected && "test_regx_replace_all_multiple_matches failed");
}

void test_regx_replace_all_no_match() {
    string input = "hello world";
    string pattern = "foo";
    string replace = "bar";
    string expected = "hello world";
    string actual = regx_replace_all(pattern, input, replace);
    assert(actual == expected && "test_regx_replace_all_no_match failed");
}

void test_regx_replace_all_empty_input() {
    string input = "";
    string pattern = "foo";
    string replace = "bar";
    string expected = "";
    string actual = regx_replace_all(pattern, input, replace);
    assert(actual == expected && "test_regx_replace_all_empty_input failed");
}

void test_regx_replace_all_empty_pattern() {
    bool thrown = false;
    try {
        string input = "hello world";
        string pattern = "";
        string replace = "bar";
        regx_replace_all(pattern, input, replace);
    } catch (...) {
        thrown = true;
    }
    assert(thrown && "test_regx_replace_all_empty_pattern failed");
}

void test_regx_replace_all_regex_special_characters() {
    string input = "123-456-7890";
    string pattern = "\\d{3}-\\d{3}-\\d{4}";
    string replace = "[phone]";
    string expected = "[phone]";
    string actual = regx_replace_all(pattern, input, replace);
    assert(actual == expected && "test_regx_replace_all_regex_special_characters failed");
}

// Register tests
TEST(test_regx_match_basic);
TEST(test_regx_match_no_match);
TEST(test_regx_match_capture_groups);
TEST(test_regx_match_empty_pattern);
TEST(test_regx_match_empty_string);
TEST(test_regx_match_special_characters);
TEST(test_regx_match_case_sensitive);
TEST(test_regx_match_case_insensitive_multiple_matches);
TEST(test_regx_match_case_insensitive_with_flag);
TEST(test_regx_match_case_insensitive);
TEST(test_regx_match_multiple_matches);
TEST(test_regx_match_invalid_pattern);
TEST(test_regx_match_all_basic);
TEST(test_regx_match_all_single_match);
TEST(test_regx_match_all_no_matches);
TEST(test_regx_match_all_no_match);
TEST(test_regx_match_all_capture_groups);
TEST(test_regx_match_all_single_match_multiple_capture_groups);
TEST(test_regx_match_all_no_matches2);
TEST(test_regx_match_all_empty_pattern);
TEST(test_regx_match_all_empty_string);
TEST(test_regx_match_all_special_characters);
TEST(test_regx_match_all_special_characters_no_matches);
TEST(test_regx_match_all_special_characters_with_capture_groups);
TEST(test_regx_match_all_case_sensitive);
TEST(test_regx_match_all_case_insensitive);
TEST(test_regx_match_all_invalid_pattern);
TEST(test_regx_match_all_non_empty_pattern);
TEST(test_regx_replace_single_match);
TEST(test_regx_replace_no_match);
TEST(test_regx_replace_first_occurrence_only);
TEST(test_regx_replace_all);
TEST(test_regx_replace_empty_input);
TEST(test_regx_replace_empty_pattern);
TEST(test_regx_replace_all_multiple_matches);
TEST(test_regx_replace_all_no_match);
TEST(test_regx_replace_all_empty_input);
TEST(test_regx_replace_all_empty_pattern);
TEST(test_regx_replace_all_regex_special_characters);
#endif