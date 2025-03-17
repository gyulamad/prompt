#pragma once

#include <map>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <type_traits>

#include "ERROR.hpp"
#include "regx.hpp" // not compiles with -Ofast + -fsanitize=address
#include "vectors.hpp"

using namespace std;

namespace tools::utils {

    vector<string> explode(const string& delimiter, const string& str) {
        if (delimiter.empty()) throw ERROR("Delimeter can not be empty.");
        vector<string> result;
        size_t start = 0;
        size_t end = str.find(delimiter);

        // Split the string by the delimiter
        while (end != string::npos) {
            result.push_back(str.substr(start, end - start));
            start = end + delimiter.length();
            end = str.find(delimiter, start);
        }

        // Add the last part of the string
        result.push_back(str.substr(start));

        return result;
    }

    string implode(const string& delimiter, const vector<string>& elements) {
        ostringstream oss;
        for (size_t i = 0; i < elements.size(); ++i) {
            if (i != 0) oss << delimiter;
            oss << elements[i];
        }
        return oss.str();
    }
    
    string escape(const string& input, const string& chars = "\\$\"'`", const string& esc = "\\") {
        string result;
        for (size_t i = 0; i < input.size(); ++i) {
            char c = input[i];
            bool needs_escape = (chars.find(c) != string::npos);
            bool is_escaped = false;

            // Check if the previous character in the result is an escape
            if (!result.empty() && result.back() == esc[0]) {
                // Count consecutive escape characters from the end of the result
                size_t escape_count = 0;
                for (int j = result.size() - 1; j >= 0 && result[j] == esc[0]; --j) escape_count++;

                // Odd number of escapes means this character is already escaped
                is_escaped = (escape_count % 2 == 1);
            }

            if (needs_escape && !is_escaped) result += esc;
            result += c;
        }
        return result;
    }

    string quote_cmd(const string& input) {
        return "\"" + escape(input, "$\\\"") + "\"";
    }
    
    inline bool str_contains(const string& str, const string& substring) {
        // Use string::find to check if the substring exists
        return str.find(substring) != string::npos;
    }

    int levenshtein(const string &s1, const string &s2) {
        int m = s1.length();
        int n = s2.length();

        // Create a matrix to store distances between prefixes of s1 and s2
        vector<vector<int>> dp(m + 1, vector<int>(n + 1));

        // Initialize the first row and column of the matrix
        for (int i = 0; i <= m; ++i) dp[i][0] = i;
        for (int j = 0; j <= n; ++j) dp[0][j] = j;

        // Fill in the rest of the matrix using dynamic programming
        for (int i = 1; i <= m; ++i)
            for (int j = 1; j <= n; ++j) {
                int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1; // Cost of substitution
                dp[i][j] = min({
                    dp[i - 1][j] + 1,       // Deletion
                    dp[i][j - 1] + 1,       // Insertion
                    dp[i - 1][j - 1] + cost // Substitution
                });
            }

        return dp[m][n]; // The Levenshtein distance is in the bottom-right cell
    }

    string str_cut_begin(const string& s, size_t maxch = 300, const string& prepend = "...") {
        // Check if the string is already shorter than or equal to the limit
        if (s.length() <= maxch) return s;

        // Handle the case where maxch is smaller than the length of prepend
        if (maxch <= prepend.length()) return prepend;
        
        // Truncate the string from the beginning and prepend the prefix
        return prepend + s.substr(s.length() - (maxch - prepend.length()));
    }

    string str_cut_end(const string& s, size_t maxch = 300, const string& append = "...") {
        // Check if the string is already shorter than or equal to the limit
        if (s.length() <= maxch) return s;

        // Handle the case where maxch is smaller than or equal to the length of append
        if (maxch <= append.length()) return append;

        // Truncate the string and append the suffix
        return s.substr(0, maxch - append.length()) + append;
    }


    /**
     * Splits a string into two parts based on a given ratio.
     * @param str The input string.
     * @param ratio The ratio at which to split the string (default is 0.5).
     * @return A pair of strings representing the two parts.
     */
    pair<string, string> str_cut_ratio(const string& str, double ratio = 0.5) {
        // Ensure the ratio is between 0 and 1
        if (ratio < 0.0 || ratio > 1.0)
            throw invalid_argument("Ratio must be between 0.0 and 1.0");

        size_t splitPoint = static_cast<size_t>(str.length() * ratio);

        // Split the string into two parts
        string firstPart = str.substr(0, splitPoint);
        string secondPart = str.substr(splitPoint);

        return {firstPart, secondPart};
    }

    string trim(const string& str) {
        // Find the first non-whitespace character from the beginning
        size_t start = str.find_first_not_of(" \t\n\r\f\v");
        
        // If there is no non-whitespace character, return an empty string
        if (start == string::npos) return "";

        // Find the first non-whitespace character from the end
        size_t end = str.find_last_not_of(" \t\n\r\f\v");

        // Return the substring from the first non-whitespace to the last non-whitespace character
        return str.substr(start, end - start + 1);
    }

    bool str_starts_with(const string& str, const string& prefix) {
        return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
    }

    bool str_ends_with(const string& str, const string& suffix) {
        // Check if the suffix is longer than the string
        if (str.size() < suffix.size()) return false;

        // Compare the end of the string with the suffix
        return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
    
    string str_replace(const map<string, string>& v, const string& s) {
        // Create a modifiable copy of the input string
        string result = s;
        // Iterate through each key-value pair in the map
        for (const auto& pair : v) {
            // Check if the key is empty
            if (pair.first.empty()) throw ERROR("Cannot replace from an empty string");
    
            size_t pos = 0;
            // Search for the key in the string and replace all occurrences
            while ((pos = result.find(pair.first, pos)) != string::npos) {
                result.replace(pos, pair.first.length(), pair.second);
                // Move past the replacement
                if (!pair.second.empty()) pos += pair.second.length(); // If the replacement is empty, do not increment pos
            }
        }
        // Return the modified string
        return result;
    }
    
    string str_replace(const string& from, const string& to, const string& subject) {
        if (from.empty()) throw ERROR("Cannot replace from an empty string");
        return str_replace({{from, to}}, subject);
    }
    
    string tpl_replace(const map<string, string>& replacements, const string& template_str, const string& placeholder_ptrn = "\\{\\{[^}]+\\}\\}") {
        
        // Check if all provided replacements exist in template        
        for (const auto& pair : replacements) {
            if (!regx_match("^" + placeholder_ptrn + "$", pair.first))
                throw ERROR(
                    "Replacement variable provided that does not match to the placeholder regex: " + pair.first + 
                    ", pattern: ^" + placeholder_ptrn + "$" + 
                    "\nTemplate: " + str_cut_end(template_str));
            if (!str_contains(template_str, pair.first))
                throw ERROR(
                    "Replacement variable provided for a template that is not having this placeholder: " + pair.first + 
                    "\nTemplate: " + str_cut_end(template_str));
        }
        
        // Check if all template expected variable are provided
        vector<string> matches;
        regx_match_all(placeholder_ptrn, template_str, &matches);
        vector<string> variables = array_keys(replacements);
        for (const string& match: matches)
            if (!in_array(match, variables))
                throw ERROR(
                    "Replacement value is not provided for the following placeholder(s): " + 
                    implode(", ", matches) + 
                    "\nTemplate: " + str_cut_end(template_str));

        // If validation passes, use the existing str_replace function
        return str_replace(replacements, template_str);
    }
    // Overload for single replacement
    string tpl_replace(const string& from, const string& to, const string& subject, const string& placeholder_ptrn = "\\{\\{[^}]+\\}\\}") {
        return tpl_replace({{from, to}}, subject, placeholder_ptrn);
    }


    template <typename T>
    T parse(const string& str) {
        static_assert(is_arithmetic<T>::value, "T must be an arithmetic type");
        stringstream ss(str);
        T num;
        if (ss >> num) return num;
        throw ERROR("Invalid input string (not a number): " + (str.empty() ? "<empty>" : str_cut_end(str)));
    }
    // Specialization for bool
    template <>
    bool parse<bool>(const string& str) {
        string lower = str;
        transform(lower.begin(), lower.end(), lower.begin(), ::tolower);        
        if (in_array(lower, vector<string>({ "true", "on", "1", "yes"}))) return true;
        if (in_array(lower, vector<string>({ "false", "off", "0", "no"}))) return false;
        throw ERROR("Invalid input string (not a boolean): " + (str.empty() ? "<empty>" : str_cut_end(str)));
    }

    vector<string> split(const string& s) {
        vector<string> parts;
        stringstream ss(s);
        string part;
        while (ss >> part) parts.push_back(part);
        return parts;
    }

    bool is_numeric(const string& s) {
        if (s.empty()) return false;
        
        size_t start = (s[0] == '+' || s[0] == '-') ? 1 : 0;
        bool hasDecimal = false;
        
        for (size_t i = start; i < s.length(); i++) {
            if (s[i] == '.') {
                if (hasDecimal) return false;  // Multiple decimals
                hasDecimal = true;
                continue;
            }
            if (!isdigit(s[i])) return false;
        }
        
        return s.length() > start && !s.ends_with(".");
    }

    bool is_integer(const string& s) {
        if (s.empty()) return false;
        size_t start = (s[0] == '+' || s[0] == '-') ? 1 : 0;
        return s.length() > start && all_of(s.begin() + start, s.end(), ::isdigit);
    }
    bool is_int(const string& s) {
        return is_integer(s);
    }
    
    inline string json_escape(const string& s) {
        return str_replace("\n", "\\n", escape(s, "\\\""));
    }

    string set_precision(double number, int precision) {
        if (precision < 0) throw ERROR("Precision can not be negative: " + to_string(precision));
        stringstream ss;
        ss << fixed << setprecision(precision) << number;
        return ss.str();
    }

    string set_precision(const string& numberStr, int precision) {
        if (!is_numeric(numberStr))
            throw ERROR("Input should be numeric" + (numberStr.empty() ? "" : ": " + numberStr));
        double number = stod(numberStr);
        return set_precision(number, precision);
    }

    // PHP like function to convert string to lower case
    string strtolower(const string& s) {
        string result = s; // Create a copy of the input string
        for (char& c : result) // Iterate over each character
            c = tolower(static_cast<unsigned char>(c)); // Convert to lower case
        return result;
    }

    // PHP like function to convert string to upper case
    string strtoupper(const string& s) {
        string result = s; // Create a copy of the input string
        for (char& c : result) // Iterate over each character
            c = toupper(static_cast<unsigned char>(c)); // Convert to upper case
        return result;
    }


    string to_string(bool b, const string& t = "true", const string& f = "false") { // TODO to common libs
        return b ? t : f;
    }

    template<class T> string to_string(const T& t) {
        stringstream sstr;    
        sstr << t;    
        return sstr.str();
    }

    bool is_valid_filepath(const string& filename) {
        // List of invalid characters (you can customize this)
        string invalidChars = "\\:*?\"<>|";
      
        // Check for empty filename
        if (filename.empty()) return false;
      
        // Check if filename contains any invalid characters
        for (char c : invalidChars)
          if (filename.find(c) != string::npos) return false;
        
      
        // Check for reserved names (CON, PRN, AUX, NUL, COM1, COM2, etc.) - Optional
        // This is a more advanced check and might not be necessary for all systems
      
        // If all checks pass, the filename is considered valid
        return true;
    }

    bool is_valid_filename(const string& filename) {
        // List of invalid characters (you can customize this)
        string invalidChars = "\\/:*?\"<>|";
      
        // Check for empty filename
        if (filename.empty()) return false;
      
        // Check if filename contains any invalid characters
        for (char c : invalidChars)
          if (filename.find(c) != string::npos) return false;
      
        // Check for reserved names (CON, PRN, AUX, NUL, COM1, COM2, etc.) - Optional
        // This is a more advanced check and might not be necessary for all systems
      
        // If all checks pass, the filename is considered valid
        return true;
    }

    // --------------- DIFFER -----------------

    // Struct to represent a single diff block
    typedef struct {
        int bound[2];          // Line range where the difference occurs [start, end]
        vector<string> added;  // Lines added in s2
        vector<string> removed; // Lines removed from s1
    } str_diff_t;

    // Function to compute differences between two strings
    vector<str_diff_t> str_get_diffs(const string& s1, const string& s2) {
        vector<string> lines1 = explode("\n", s1);
        vector<string> lines2 = explode("\n", s2);

        vector<str_diff_t> diffs;
        size_t i = 0, j = 0;

        while (i < lines1.size() || j < lines2.size()) {
            // Find the start of a difference
            if (i < lines1.size() && j < lines2.size() && lines1[i] == lines2[j]) {
                i++;
                j++;
                continue;
            }

            // Start of a diff block
            int start = i + 1; // 1-based line numbering
            vector<string> added;
            vector<string> removed;

            // Collect removed lines
            while (i < lines1.size() && (j >= lines2.size() || lines1[i] != lines2[j])) {
                removed.push_back(lines1[i]);
                i++;
            }

            // Collect added lines
            while (j < lines2.size() && (i >= lines1.size() || lines1[i] != lines2[j])) {
                added.push_back(lines2[j]);
                j++;
            }

            // Record the diff block
            str_diff_t diff;
            diff.bound[0] = start;
            diff.bound[1] = i; // End of the removed block (1-based)
            diff.added = added;
            diff.removed = removed;
            diffs.push_back(diff);
        }

        return diffs;
    }

    // Function to display a single diff block
    void str_show_diff(const str_diff_t& diff) {
        cout << "changed line(s) " << diff.bound[0] << ".." << diff.bound[1] << ":\n";
        for (const string& line : diff.added) {
            cout << ANSI_FMT(ANSI_FMT_C_GREEN, "+ " + line) << "\n";
        }
        for (const string& line : diff.removed) {
            cout << ANSI_FMT(ANSI_FMT_C_RED, "- " + line) << "\n";
        }
    }

    // Function to compute and display all differences between two strings
    vector<str_diff_t> str_diffs_show(const string& s1, const string& s2) {
        vector<str_diff_t> diffs = str_get_diffs(s1, s2);
        for (const str_diff_t& diff : diffs) {
            str_show_diff(diff);
        }
        return diffs;
    }

}

#ifdef TEST

#include "Test.hpp"

using namespace tools::utils;

void test_explode_basic() {
    auto parts = explode(",", "a,b,c");
    assert(parts.size() == 3 && "Basic split count");
    assert(parts[0] == "a" && parts[1] == "b" && parts[2] == "c");
}

void test_explode_no_delimiter() {
    auto parts = explode("-", "test");
    assert(parts.size() == 1 && "No delimiter found");
    assert(parts[0] == "test");
}

void test_explode_start_with_delimiter() {
    auto parts = explode("-", "-apples");
    assert(parts.size() == 2 && "Start with delimiter");
    assert(parts[0].empty() && parts[1] == "apples");
}

void test_explode_end_with_delimiter() {
    auto parts = explode("-", "apples-");
    assert(parts.size() == 2 && "End with delimiter");
    assert(parts[0] == "apples" && parts[1].empty());
}

void test_explode_consecutive_delimiters() {
    auto parts = explode(",", "a,,b");
    assert(parts.size() == 3 && "Consecutive delimiters");
    assert(parts[0] == "a" && parts[1].empty() && parts[2] == "b");
}

void test_explode_only_delimiters() {
    auto parts = explode("--", "----");
    assert(parts.size() == 3 && "Only delimiters");
    assert(parts[0].empty() && parts[1].empty() && parts[2].empty());
}

void test_explode_empty_input() {
    auto parts = explode(",", "");
    assert(parts.size() == 1 && "Empty input");
    assert(parts[0].empty());
}

void test_explode_invalid_delimiter() {
    bool exception_thrown = false;
    try { explode("", "test"); }
    catch (...) { exception_thrown = true; }
    assert(exception_thrown && "Empty delimiter exception");
}

void test_implode_basic() {
    vector<string> parts = {"a", "b", "c"};
    string result = implode(",", parts);
    assert(result == "a,b,c" && "Basic implode");
}

void test_implode_single_element() {
    vector<string> parts = {"hello"};
    string result = implode("-", parts);
    assert(result == "hello" && "Single element");
}

void test_implode_empty_elements() {
    vector<string> parts = {"", "", ""};
    string result = implode(":", parts);
    assert(result == "::" && "Empty elements");
}

void test_implode_empty_delimiter() {
    vector<string> parts = {"a", "b", "c"};
    string result = implode("", parts);
    assert(result == "abc" && "Empty delimiter");
}

void test_implode_empty_vector() {
    vector<string> parts = {};
    string result = implode(",", parts);
    assert(result.empty() && "Empty vector");
}

void test_implode_mixed_elements() {
    vector<string> parts = {"a", "", "c"};
    string result = implode("-", parts);
    assert(result == "a--c" && "Mixed elements");
}

void test_escape_empty_input() {
    string input = "";
    string expected = "";
    string actual = escape(input);
    assert(actual == expected && "Empty input");
}

void test_escape_no_special_chars() {
    string input = "hello world";
    string expected = "hello world";
    string actual = escape(input);
    assert(actual == expected && "No special chars");
}

void test_escape_single_char() {
    string input = "$";
    string expected = "\\$";
    string actual = escape(input);
    assert(actual == expected && "Single char");
}

void test_escape_already_escaped() {
    string input = "\\\\$";
    string expected = "\\\\\\\\\\$";
    string actual = escape(input);
    assert(actual == expected && "Already escaped characters");
}

void test_escape_no_chars() {
    string input = "hello";
    string expected = "hello";
    string actual = escape(input);
    assert(actual == expected && "No characters to escape");
}

void test_escape_custom_chars() {
    string input = "a%b&c";
    string expected = "a\\%b\\&c";
    string actual = escape(input, "%&");
    assert(actual == expected && "Custom characters to escape");
}

void test_escape_mixed_content() {
    string input = "hello $world\\ \" \' `";
    string expected = "hello \\$world\\\\ \\\" \\' \\`";
    string actual = escape(input);
    assert(actual == expected && "Mixed content");
}

void test_escape_custom_chars2() {
    string input = "abc123";
    string chars = "123";
    string expected = "abc\\1\\2\\3";
    string actual = escape(input, chars);
    assert(actual == expected && "Custom chars 2");
}

void test_escape_custom_escape_sequence() {
    string input = "abc$";
    string chars = "$";
    string esc = "/";
    string expected = "abc/$";
    string actual = escape(input, chars, esc);
    assert(actual == expected && "Custom escape sequence");
}

void test_escape_already_escaped_in_input() {
    string input = "\\\\$"; // Escaped backslash and $
    string expected = "\\\\\\\\\\$"; // Expect: \\\$
    string actual = escape(input);
    assert(actual == expected && "Already escaped in input");
}

void test_escape_multiple_chars() {
    string input = "$\\\"'`";
    string expected = "\\$\\\\\\\"\\'\\`";
    string actual = escape(input);
    assert(actual == expected && "Multiple chars");
}

void test_quote_cmd_basic() {
    string input = "hello";
    string expected = "\"hello\"";
    string actual = quote_cmd(input);
    assert(actual == expected && "Basic quoting");
}

void test_quote_cmd_with_special_chars() {
    string input = "hello$world\\\"";
    string expected = "\"hello\\$world\\\\\\\"\"";
    string actual = quote_cmd(input);
    assert(actual == expected && "Special characters");
}

void test_quote_cmd_empty_input() {
    string input = "";
    string expected = "\"\"";
    string actual = quote_cmd(input);
    assert(actual == expected && "Empty input");
}

void test_quote_cmd_already_escaped() {
    string input = "hello\\$world";
    string expected = "\"hello\\\\\\$world\"";
    string actual = quote_cmd(input);
    assert(actual == expected && "Already escaped characters");
}

void test_quote_cmd_mixed_content() {
    string input = "hello\"world$";
    string expected = "\"hello\\\"world\\$\"";
    string actual = quote_cmd(input);
    assert(actual == expected && "Mixed content");
}

void test_quote_cmd_only_special_chars() {
    string input = "$\\\"";
    string expected = "\"\\$\\\\\\\"\"";
    string actual = quote_cmd(input);
    assert(actual == expected && "Only special characters");
}

void test_levenshtein_same_string() {
    string s1 = "hello";
    string s2 = "hello";
    int actual = levenshtein(s1, s2);
    int expected = 0;
    assert(actual == expected && "Same string should have distance 0");
}

void test_levenshtein_empty_string() {
    string s1 = "";
    string s2 = "hello";
    int actual = levenshtein(s1, s2);
    int expected = 5;
    assert(actual == expected && "Distance between empty and non-empty string");
}

void test_levenshtein_one_empty_string() {
    string s1 = "hello";
    string s2 = "";
    int actual = levenshtein(s1, s2);
    int expected = 5;
    assert(actual == expected && "Distance between non-empty and empty string");
}

void test_levenshtein_single_insertion() {
    string s1 = "kitten";
    string s2 = "kittens";
    int actual = levenshtein(s1, s2);
    int expected = 1;
    assert(actual == expected && "Single insertion");
}

void test_levenshtein_single_deletion() {
    string s1 = "kittens";
    string s2 = "kitten";
    int actual = levenshtein(s1, s2);
    int expected = 1;
    assert(actual == expected && "Single deletion");
}

void test_levenshtein_single_substitution() {
    string s1 = "kitten";
    string s2 = "sitten";
    int actual = levenshtein(s1, s2);
    int expected = 1;
    assert(actual == expected && "Single substitution");
}

void test_levenshtein_multiple_operations() {
    string s1 = "sitting";
    string s2 = "kitten";
    int actual = levenshtein(s1, s2);
    int expected = 3;
    assert(actual == expected && "Multiple operations (substitution, insertion, deletion)");
}

void test_levenshtein_case_sensitivity() {
    string s1 = "Hello";
    string s2 = "hello";
    int actual = levenshtein(s1, s2);
    int expected = 1;
    assert(actual == expected && "Case sensitivity");
}

void test_levenshtein_unicode() {
    string s1 = "café";
    string s2 = "coffee";
    int actual = levenshtein(s1, s2);
    int expected = 4;
    assert(actual == expected && "Unicode characters");
}

void test_str_cut_begin_when_string_is_short() {
    string input = "hello";
    string expected = "hello";
    string actual = str_cut_begin(input, 10);
    assert(actual == expected && "test_str_cut_begin_when_string_is_short failed");
}

void test_str_cut_begin_when_string_is_exact_length() {
    string input = "abcdefghij";
    string expected = "abcdefghij";
    string actual = str_cut_begin(input, 10);
    assert(actual == expected && "test_str_cut_begin_when_string_is_exact_length failed");
}

void test_str_cut_begin_when_string_needs_truncation() {
    string input = "abcdefghijklmnopqrstuvwxyz";
    string expected = "...opqrstuvwxyz";
    string actual = str_cut_begin(input, 15);
    assert(actual == expected && "test_str_cut_begin_when_string_needs_truncation failed");
}

void test_str_cut_begin_when_custom_prepend_is_used() {
    string input = "abcdefghijklmnopqrstuvwxyz";
    string expected = "!!nopqrstuvwxyz";
    string actual = str_cut_begin(input, 15, "!!");
    assert(actual == expected && "test_str_cut_begin_when_custom_prepend_is_used failed");
}

void test_str_cut_begin_when_string_is_empty() {
    string input = "";
    string expected = "";
    string actual = str_cut_begin(input, 10);
    assert(actual == expected && "test_str_cut_begin_when_string_is_empty failed");
}

void test_str_cut_begin_when_maxch_is_less_than_prepend_length() {
    string input = "abcdefghijklmnopqrstuvwxyz";
    string expected = "...";
    string actual = str_cut_begin(input, 2);
    assert(actual == expected && "test_str_cut_begin_when_maxch_is_less_than_prepend_length failed");
}

// Test cases for str_cut_end
void test_str_cut_end_when_string_is_short() {
    string input = "hello";
    string expected = "hello";
    string actual = str_cut_end(input, 10);
    assert(actual == expected && "test_str_cut_end_when_string_is_short failed");
}

void test_str_cut_end_when_string_is_exact_length() {
    string input = "abcdefghij";
    string expected = "abcdefghij";
    string actual = str_cut_end(input, 10);
    assert(actual == expected && "test_str_cut_end_when_string_is_exact_length failed");
}

void test_str_cut_end_when_string_needs_truncation() {
    string input = "abcdefghijklmnopqrstuvwxyz";
    string expected = "abcdefghijkl...";
    string actual = str_cut_end(input, 15);
    assert(actual == expected && "test_str_cut_end_when_string_needs_truncation failed");
}

void test_str_cut_end_when_custom_append_is_used() {
    string input = "abcdefghijklmnopqrstuvwxyz";
    string expected = "abcdefghijklm!!";
    string actual = str_cut_end(input, 15, "!!");
    assert(actual == expected && "test_str_cut_end_when_custom_append_is_used failed");
}

void test_str_cut_end_when_string_is_empty() {
    string input = "";
    string expected = "";
    string actual = str_cut_end(input, 10);
    assert(actual == expected && "test_str_cut_end_when_string_is_empty failed");
}

void test_str_cut_end_when_maxch_is_less_than_append_length() {
    string input = "abcdefghijklmnopqrstuvwxyz";
    string expected = "...";
    string actual = str_cut_end(input, 2);
    assert(actual == expected && "test_str_cut_end_when_maxch_is_less_than_append_length failed");
}

// Test cases for str_cut_ratio
void test_str_cut_ratio_when_ratio_is_default() {
    string input = "abcdefgh";
    pair<string, string> expected = {"abcd", "efgh"};
    pair<string, string> actual = str_cut_ratio(input);
    assert(actual.first == expected.first && actual.second == expected.second &&
           "test_str_cut_ratio_when_ratio_is_default failed");
}

void test_str_cut_ratio_when_ratio_is_half() {
    string input = "abcdefgh";
    pair<string, string> expected = {"abcd", "efgh"};
    pair<string, string> actual = str_cut_ratio(input, 0.5);
    assert(actual.first == expected.first && actual.second == expected.second &&
           "test_str_cut_ratio_when_ratio_is_half failed");
}

void test_str_cut_ratio_when_ratio_is_zero() {
    string input = "abcdefgh";
    pair<string, string> expected = {"", "abcdefgh"};
    pair<string, string> actual = str_cut_ratio(input, 0.0);
    assert(actual.first == expected.first && actual.second == expected.second &&
           "test_str_cut_ratio_when_ratio_is_zero failed");
}

void test_str_cut_ratio_when_ratio_is_one() {
    string input = "abcdefgh";
    pair<string, string> expected = {"abcdefgh", ""};
    pair<string, string> actual = str_cut_ratio(input, 1.0);
    assert(actual.first == expected.first && actual.second == expected.second &&
           "test_str_cut_ratio_when_ratio_is_one failed");
}

void test_str_cut_ratio_when_ratio_is_small() {
    string input = "abcdefgh";
    pair<string, string> expected = {"a", "bcdefgh"};
    pair<string, string> actual = str_cut_ratio(input, 0.125);
    assert(actual.first == expected.first && actual.second == expected.second &&
           "test_str_cut_ratio_when_ratio_is_small failed");
}

void test_str_cut_ratio_when_ratio_is_large() {
    string input = "abcdefgh";
    pair<string, string> expected = {"abcdefg", "h"};
    pair<string, string> actual = str_cut_ratio(input, 0.875);
    assert(actual.first == expected.first && actual.second == expected.second &&
           "test_str_cut_ratio_when_ratio_is_large failed");
}

void test_str_cut_ratio_when_input_is_empty() {
    string input = "";
    pair<string, string> expected = {"", ""};
    pair<string, string> actual = str_cut_ratio(input, 0.5);
    assert(actual.first == expected.first && actual.second == expected.second &&
           "test_str_cut_ratio_when_input_is_empty failed");
}

void test_str_cut_ratio_when_ratio_is_invalid() {
    bool thrown = false;
    try {
        str_cut_ratio("abcdefgh", 1.5);
    } catch (const invalid_argument& e) {
        thrown = true;
    }
    assert(thrown && "test_str_cut_ratio_when_ratio_is_invalid failed");
}

void test_trim_no_whitespace() {
    string input = "hello";
    string expected = "hello";
    string actual = trim(input);
    assert(actual == expected && "No whitespace to trim");
}

void test_trim_leading_whitespace() {
    string input = "   hello";
    string expected = "hello";
    string actual = trim(input);
    assert(actual == expected && "Leading whitespace");
}

void test_trim_trailing_whitespace() {
    string input = "hello   ";
    string expected = "hello";
    string actual = trim(input);
    assert(actual == expected && "Trailing whitespace");
}

void test_trim_both_leading_and_trailing_whitespace() {
    string input = "   hello   ";
    string expected = "hello";
    string actual = trim(input);
    assert(actual == expected && "Leading and trailing whitespace");
}

void test_trim_only_whitespace() {
    string input = "   \t\n\r\f\v   ";
    string expected = "";
    string actual = trim(input);
    assert(actual == expected && "Only whitespace");
}

void test_trim_empty_string() {
    string input = "";
    string expected = "";
    string actual = trim(input);
    assert(actual == expected && "Empty string");
}

void test_trim_mixed_whitespace() {
    string input = " \t\nhello\r\f\v ";
    string expected = "hello";
    string actual = trim(input);
    assert(actual == expected && "Mixed whitespace");
}

void test_trim_whitespace_in_middle() {
    string input = "hello world";
    string expected = "hello world";
    string actual = trim(input);
    assert(actual == expected && "Whitespace in middle (should not be trimmed)");
}

void test_str_starts_with_basic() {
    string str = "hello world";
    string prefix = "hello";
    bool actual = str_starts_with(str, prefix);
    bool expected = true;
    assert(actual == expected && "Basic starts_with");
}

void test_str_starts_with_empty_prefix() {
    string str = "hello world";
    string prefix = "";
    bool actual = str_starts_with(str, prefix);
    bool expected = true;
    assert(actual == expected && "Empty prefix");
}

void test_str_starts_with_empty_string() {
    string str = "";
    string prefix = "hello";
    bool actual = str_starts_with(str, prefix);
    bool expected = false;
    assert(actual == expected && "Empty string");
}

void test_str_starts_with_prefix_longer_than_string() {
    string str = "hello";
    string prefix = "hello world";
    bool actual = str_starts_with(str, prefix);
    bool expected = false;
    assert(actual == expected && "Prefix longer than string");
}

void test_str_starts_with_no_match() {
    string str = "hello world";
    string prefix = "world";
    bool actual = str_starts_with(str, prefix);
    bool expected = false;
    assert(actual == expected && "No match");
}

void test_str_starts_with_case_sensitive() {
    string str = "Hello World";
    string prefix = "hello";
    bool actual = str_starts_with(str, prefix);
    bool expected = false;
    assert(actual == expected && "Case sensitive");
}

void test_str_ends_with_basic() {
    string str = "hello world";
    string suffix = "world";
    bool actual = str_ends_with(str, suffix);
    bool expected = true;
    assert(actual == expected && "Basic ends_with");
}

void test_str_ends_with_empty_suffix() {
    string str = "hello world";
    string suffix = "";
    bool actual = str_ends_with(str, suffix);
    bool expected = true;
    assert(actual == expected && "Empty suffix");
}

void test_str_ends_with_empty_string() {
    string str = "";
    string suffix = "world";
    bool actual = str_ends_with(str, suffix);
    bool expected = false;
    assert(actual == expected && "Empty string");
}

void test_str_ends_with_suffix_longer_than_string() {
    string str = "world";
    string suffix = "hello world";
    bool actual = str_ends_with(str, suffix);
    bool expected = false;
    assert(actual == expected && "Suffix longer than string");
}

void test_str_ends_with_no_match() {
    string str = "hello world";
    string suffix = "hello";
    bool actual = str_ends_with(str, suffix);
    bool expected = false;
    assert(actual == expected && "No match");
}

void test_str_ends_with_case_sensitive() {
    string str = "Hello World";
    string suffix = "world";
    bool actual = str_ends_with(str, suffix);
    bool expected = false;
    assert(actual == expected && "Case sensitive");
}

// Test cases for str_replace
void test_str_replace_single_replacement() {
    string input = "hello world";
    string expected = "hi world";
    string actual = str_replace("hello", "hi", input);
    assert(actual == expected && "test_str_replace_single_replacement failed");
}

void test_str_replace_multiple_occurrences() {
    string input = "apple banana apple";
    string expected = "fruit banana fruit";
    string actual = str_replace("apple", "fruit", input);
    assert(actual == expected && "test_str_replace_multiple_occurrences failed");
}

void test_str_replace_no_match() {
    string input = "hello world";
    string expected = "hello world";
    string actual = str_replace("foo", "bar", input);
    assert(actual == expected && "test_str_replace_no_match failed");
}

void test_str_replace_empty_to_multiple_occurrences() {
    string input = "apple banana apple";
    string expected = " banana ";
    string actual = str_replace("apple", "", input);
    assert(actual == expected && "test_str_replace_empty_to_multiple_occurrences failed");
}

void test_str_replace_empty_to_no_match() {
    string input = "hello world";
    string expected = "hello world";
    string actual = str_replace("foo", "", input);
    assert(actual == expected && "test_str_replace_empty_to_no_match failed");
}

void test_str_replace_non_empty_to() {
    string input = "hello world";
    string expected = "hello universe";
    string actual = str_replace("world", "universe", input);
    assert(actual == expected && "test_str_replace_non_empty_to failed");
}

void test_str_replace_empty_input() {
    string input = "";
    string expected = "";
    string actual = str_replace("foo", "bar", input);
    assert(actual == expected && "test_str_replace_empty_input failed");
}

void test_str_replace_empty_to() {
    string input = "hello world";
    string expected = "hello ";
    string actual = str_replace("world", "", input);
    assert(actual == expected && "test_str_replace_empty_to failed");
}

void test_str_replace_empty_from() {
    bool thrown = false;
    try {
        string input = "hello world";
        string actual = str_replace("", "bar", input);
    } catch (const exception&) {
        thrown = true;
    }
    assert(thrown && "test_str_replace_empty_from failed");
}

void test_str_replace_map_multiple_replacements() {
    string input = "hello world, hello universe";
    map<string, string> replacements = {{"hello", "hi"}, {"world", "earth"}};
    string expected = "hi earth, hi universe";
    string actual = str_replace(replacements, input);
    assert(actual == expected && "test_str_replace_map_multiple_replacements failed");
}

void test_str_replace_map_overlapping_keys() {
    string input = "abcde";
    map<string, string> replacements = {{"abc", "123"}, {"cde", "456"}};
    string expected = "123de";
    string actual = str_replace(replacements, input);
    assert(actual == expected && "test_str_replace_map_overlapping_keys failed");
}

void test_str_replace_map_empty_input() {
    string input = "";
    map<string, string> replacements = {{"hello", "hi"}, {"world", "earth"}};
    string expected = "";
    string actual = str_replace(replacements, input);
    assert(actual == expected && "test_str_replace_map_empty_input failed");
}

void test_tpl_replace_single_replacement() {
    string input = "Hello {{name}}!";
    string expected = "Hello John!";
    string actual = tpl_replace("{{name}}", "John", input);
    assert(actual == expected && "test_tpl_replace_single_replacement failed");
}

void test_tpl_replace_multiple_replacements() {
    string input = "{{greeting}}, {{name}}! Welcome to {{place}}.";
    map<string, string> replacements = {
        {"{{greeting}}", "Hi"},
        {"{{name}}", "Alice"},
        {"{{place}}", "Wonderland"}
    };
    string expected = "Hi, Alice! Welcome to Wonderland.";
    string actual = tpl_replace(replacements, input);
    assert(actual == expected && "test_tpl_replace_multiple_replacements failed");
}

void test_tpl_replace_no_placeholder_in_template() {
    bool thrown = false;
    try {
        string input = "Hello World!";
        tpl_replace("{{name}}", "John", input);
    } catch (const exception&) {
        thrown = true;
    }
    assert(thrown && "test_tpl_replace_no_placeholder_in_template failed");
}

void test_tpl_replace_missing_replacement_value() {
    bool thrown = false;
    try {
        string input = "Hello {{name}} and {{friend}}!";
        map<string, string> replacements = {{"{{name}}", "John"}};
        tpl_replace(replacements, input);
    } catch (const exception&) {
        thrown = true;
    }
    assert(thrown && "test_tpl_replace_missing_replacement_value failed");
}

void test_tpl_replace_invalid_placeholder_regex() {
    bool thrown = false;
    try {
        string input = "Hello [name]!";
        tpl_replace("[name]", "John", input, "\\{\\{[^}]+\\}\\}");
    } catch (const exception&) {
        thrown = true;
    }
    assert(thrown && "test_tpl_replace_invalid_placeholder_regex failed");
}

void test_tpl_replace_empty_template() {
    bool thrown = false;
    try {
        string input = "";
        string expected = "";
        string actual = tpl_replace("{{name}}", "John", input);
    } catch (const exception&) {
        thrown = true;
    }
    assert(thrown && "test_tpl_replace_empty_template failed");
}

void test_tpl_replace_empty_replacement_value() {
    string input = "Hello {{name}}!";
    string expected = "Hello !";
    string actual = tpl_replace("{{name}}", "", input);
    assert(actual == expected && "test_tpl_replace_empty_replacement_value failed");
}

void test_parse_valid_integer() {
    int expected = 42;
    int actual = parse<int>("42");
    assert(actual == expected && "test_parse_valid_integer failed");
}

void test_parse_valid_double() {
    double expected = 3.14;
    double actual = parse<double>("3.14");
    assert(actual == expected && "test_parse_valid_double failed");
}

void test_parse_negative_number() {
    int expected = -10;
    int actual = parse<int>("-10");
    assert(actual == expected && "test_parse_negative_number failed");
}

void test_parse_invalid_input() {
    bool thrown = false;
    try {
        parse<int>("abc");
    } catch (const exception&) {
        thrown = true;
    }
    assert(thrown && "test_parse_invalid_input failed");
}

void test_parse_empty_string() {
    bool thrown = false;
    try {
        parse<double>("");
    } catch (const exception&) {
        thrown = true;
    }
    assert(thrown && "test_parse_empty_string failed");
}

void test_parse_whitespace_only() {
    bool thrown = false;
    try {
        parse<int>("   ");
    } catch (const exception&) {
        thrown = true;
    }
    assert(thrown && "test_parse_whitespace_only failed");
}

void test_parse_trailing_characters() {    
    int actual = parse<int>("42abc");
    assert(actual == 42 && "test_parse_trailing_characters failed");
}

void test_parse_floating_point_with_exponent() {
    double expected = 1.23e3;
    double actual = parse<double>("1.23e3");
    assert(actual == expected && "test_parse_floating_point_with_exponent failed");
}

void test_parse_zero() {
    int expected = 0;
    int actual = parse<int>("0");
    assert(actual == expected && "test_parse_zero failed");
}

void test_parse_large_number() {
    long long expected = 9223372036854775807LL;
    long long actual = parse<long long>("9223372036854775807");
    assert(actual == expected && "test_parse_large_number failed");
}

void test_split_single_word() {
    vector<string> expected = {"hello"};
    vector<string> actual = split("hello");
    assert(vector_equal(actual, expected) && "test_split_single_word failed");
}

void test_split_multiple_words() {
    vector<string> expected = {"hello", "world"};
    vector<string> actual = split("hello world");
    assert(vector_equal(actual, expected) && "test_split_multiple_words failed");
}

void test_split_with_extra_whitespace() {
    vector<string> expected = {"hello", "world"};
    vector<string> actual = split("   hello   world   ");
    assert(vector_equal(actual, expected) && "test_split_with_extra_whitespace failed");
}

void test_split_empty_string() {
    vector<string> expected = {};
    vector<string> actual = split("");
    assert(vector_equal(actual, expected) && "test_split_empty_string failed");
}

void test_split_only_whitespace() {
    vector<string> expected = {};
    vector<string> actual = split("     ");
    assert(vector_equal(actual, expected) && "test_split_only_whitespace failed");
}

void test_split_with_punctuation() {
    vector<string> expected = {"hello,", "world!"};
    vector<string> actual = split("hello, world!");
    assert(vector_equal(actual, expected) && "test_split_with_punctuation failed");
}

void test_split_mixed_whitespace() {
    vector<string> expected = {"hello", "world"};
    vector<string> actual = split("hello\nworld\t");
    assert(vector_equal(actual, expected) && "test_split_mixed_whitespace failed");
}

void test_split_numbers() {
    vector<string> expected = {"123", "456", "789"};
    vector<string> actual = split("123 456 789");
    assert(vector_equal(actual, expected) && "test_split_numbers failed");
}

void test_split_with_tabs_and_newlines() {
    vector<string> expected = {"hello", "world"};
    vector<string> actual = split("hello\n\tworld");
    assert(vector_equal(actual, expected) && "test_split_with_tabs_and_newlines failed");
}

void test_is_numeric_valid_integer() {
    assert(is_numeric("123") && "test_is_numeric_valid_integer failed");
}

void test_is_numeric_valid_decimal() {
    assert(is_numeric("123.456") && "test_is_numeric_valid_decimal failed");
}

void test_is_numeric_negative_integer() {
    assert(is_numeric("-123") && "test_is_numeric_negative_integer failed");
}

void test_is_numeric_positive_integer() {
    assert(is_numeric("+123") && "test_is_numeric_positive_integer failed");
}

void test_is_numeric_negative_decimal() {
    assert(is_numeric("-123.456") && "test_is_numeric_negative_decimal failed");
}

void test_is_numeric_positive_decimal() {
    assert(is_numeric("+123.456") && "test_is_numeric_positive_decimal failed");
}

void test_is_numeric_empty_string() {
    assert(!is_numeric("") && "test_is_numeric_empty_string failed");
}

void test_is_numeric_only_sign() {
    assert(!is_numeric("+") && "test_is_numeric_only_sign failed");
    assert(!is_numeric("-") && "test_is_numeric_only_sign failed");
}

void test_is_numeric_multiple_decimals() {
    assert(!is_numeric("123.45.67") && "test_is_numeric_multiple_decimals failed");
}

void test_is_numeric_trailing_decimal() {
    assert(!is_numeric("123.") && "test_is_numeric_trailing_decimal failed");
}

void test_is_numeric_leading_decimal() {
    assert(is_numeric(".123") && "test_is_numeric_leading_decimal failed");
}

void test_is_numeric_invalid_characters() {
    assert(!is_numeric("123abc") && "test_is_numeric_invalid_characters failed");
    assert(!is_numeric("12.34.56") && "test_is_numeric_invalid_characters failed");
    assert(!is_numeric("123!") && "test_is_numeric_invalid_characters failed");
}

void test_is_numeric_whitespace() {
    assert(!is_numeric(" 123 ") && "test_is_numeric_whitespace failed");
    assert(!is_numeric("\t123\n") && "test_is_numeric_whitespace failed");
}

void test_is_integer_valid_positive_integer() {
    assert(is_integer("123") && "test_is_integer_valid_positive_integer failed");
}

void test_is_integer_valid_negative_integer() {
    assert(is_integer("-123") && "test_is_integer_valid_negative_integer failed");
}

void test_is_integer_valid_positive_with_plus() {
    assert(is_integer("+123") && "test_is_integer_valid_positive_with_plus failed");
}

void test_is_integer_empty_string() {
    assert(!is_integer("") && "test_is_integer_empty_string failed");
}

void test_is_integer_only_sign() {
    assert(!is_integer("+") && "test_is_integer_only_sign failed");
    assert(!is_integer("-") && "test_is_integer_only_sign failed");
}

void test_is_integer_invalid_characters() {
    assert(!is_integer("123abc") && "test_is_integer_invalid_characters failed");
    assert(!is_integer("12.34") && "test_is_integer_invalid_characters failed");
    assert(!is_integer("123!") && "test_is_integer_invalid_characters failed");
}

void test_is_integer_whitespace() {
    assert(!is_integer(" 123 ") && "test_is_integer_whitespace failed");
    assert(!is_integer("\t123\n") && "test_is_integer_whitespace failed");
}

void test_is_integer_leading_zeros() {
    assert(is_integer("00123") && "test_is_integer_leading_zeros failed");
}

void test_is_integer_zero() {
    assert(is_integer("0") && "test_is_integer_zero failed");
}

void test_is_int_alias() {
    // Ensure is_int behaves identically to is_integer
    assert(is_int("123") && "test_is_int_alias failed");
    assert(!is_int("12.34") && "test_is_int_alias failed");
    assert(!is_int("") && "test_is_int_alias failed");
}

void test_set_precision_double_basic() {
    double number = 3.1415926535;
    int precision = 2;
    string actual = set_precision(number, precision);
    string expected = "3.14";
    assert(actual == expected && "Basic double precision");
}

void test_set_precision_double_zero_precision() {
    double number = 3.1415926535;
    int precision = 0;
    string actual = set_precision(number, precision);
    string expected = "3";
    assert(actual == expected && "Zero precision");
}

void test_set_precision_double_negative_precision() {
    double number = 3.1415926535;
    int precision = -1;
    bool thrown = false;
    try {
        set_precision(number, precision);
    } catch (...) {
        thrown = true;
    }
    assert(thrown && "Negative precision should throw");
}

void test_set_precision_double_large_precision() {
    double number = 3.1415926535;
    int precision = 10;
    string actual = set_precision(number, precision);
    string expected = "3.1415926535";
    assert(actual == expected && "Large precision");
}

void test_set_precision_double_negative_number() {
    double number = -3.1415926535;
    int precision = 3;
    string actual = set_precision(number, precision);
    string expected = "-3.142";
    assert(actual == expected && "Negative number");
}

void test_set_precision_double_zero() {
    double number = 0.0;
    int precision = 2;
    string actual = set_precision(number, precision);
    string expected = "0.00";
    assert(actual == expected && "Zero value");
}

void test_set_precision_string_basic() {
    string numberStr = "3.1415926535";
    int precision = 2;
    string actual = set_precision(numberStr, precision);
    string expected = "3.14";
    assert(actual == expected && "Basic string precision");
}

void test_set_precision_string_invalid_input() {
    string numberStr = "not_a_number";
    int precision = 2;
    bool thrown = false;
    try {
        set_precision(numberStr, precision);
    } catch (...) {
        thrown = true;
    }
    assert(thrown && "Invalid input should throw");
}

void test_set_precision_string_empty_input() {
    string numberStr = "";
    int precision = 2;
    bool thrown = false;
    try {
        set_precision(numberStr, precision);
    } catch (...) {
        thrown = true;
    }
    assert(thrown && "Empty input should throw");
}

void test_set_precision_string_negative_number() {
    string numberStr = "-3.1415926535";
    int precision = 3;
    string actual = set_precision(numberStr, precision);
    string expected = "-3.142";
    assert(actual == expected && "Negative number string");
}

void test_set_precision_string_zero() {
    string numberStr = "0.0";
    int precision = 2;
    string actual = set_precision(numberStr, precision);
    string expected = "0.00";
    assert(actual == expected && "Zero value string");
}

void test_strtolower_basic_conversion() {
    string input = "HELLO WORLD";
    string expected = "hello world";
    string actual = strtolower(input);
    assert(actual == expected && "test_strtolower_basic_conversion failed");
}

void test_strtolower_mixed_case() {
    string input = "HeLLo WoRLd";
    string expected = "hello world";
    string actual = strtolower(input);
    assert(actual == expected && "test_strtolower_mixed_case failed");
}

void test_strtolower_already_lowercase() {
    string input = "hello world";
    string expected = "hello world";
    string actual = strtolower(input);
    assert(actual == expected && "test_strtolower_already_lowercase failed");
}

void test_strtolower_empty_string() {
    string input = "";
    string expected = "";
    string actual = strtolower(input);
    assert(actual == expected && "test_strtolower_empty_string failed");
}

void test_strtolower_with_numbers_and_symbols() {
    string input = "123 ABC^!%&#$@";
    string expected = "123 abc^!%&#$@";
    string actual = strtolower(input);
    assert(actual == expected && "test_strtolower_with_numbers_and_symbols failed");
}

void test_strtoupper_basic_conversion() {
    string input = "hello world";
    string expected = "HELLO WORLD";
    string actual = strtoupper(input);
    assert(actual == expected && "test_strtoupper_basic_conversion failed");
}

void test_strtoupper_mixed_case() {
    string input = "HeLLo WoRLd";
    string expected = "HELLO WORLD";
    string actual = strtoupper(input);
    assert(actual == expected && "test_strtoupper_mixed_case failed");
}

void test_strtoupper_already_uppercase() {
    string input = "HELLO WORLD";
    string expected = "HELLO WORLD";
    string actual = strtoupper(input);
    assert(actual == expected && "test_strtoupper_already_uppercase failed");
}

void test_strtoupper_empty_string() {
    string input = "";
    string expected = "";
    string actual = strtoupper(input);
    assert(actual == expected && "test_strtoupper_empty_string failed");
}

void test_strtoupper_with_numbers_and_symbols() {
    string input = "123 abc^!%&#$@";
    string expected = "123 ABC^!%&#$@";
    string actual = strtoupper(input);
    assert(actual == expected && "test_strtoupper_with_numbers_and_symbols failed");
}

// Test cases for is_valid_filepath
void test_is_valid_filepath_valid() {
    assert(is_valid_filepath("file.txt") && "test_is_valid_filepath_valid failed");
}

void test_is_valid_filepath_invalid_characters() {
    assert(!is_valid_filepath("file<name>.txt") && "test_is_valid_filepath_invalid_characters failed");
    assert(!is_valid_filepath("file:name.txt") && "test_is_valid_filepath_invalid_characters failed");
    assert(!is_valid_filepath("file?name.txt") && "test_is_valid_filepath_invalid_characters failed");
    assert(!is_valid_filepath("file*name.txt") && "test_is_valid_filepath_invalid_characters failed");
    assert(!is_valid_filepath("file\"name.txt") && "test_is_valid_filepath_invalid_characters failed");
    assert(!is_valid_filepath("file|name.txt") && "test_is_valid_filepath_invalid_characters failed");
}

void test_is_valid_filepath_empty_string() {
    assert(!is_valid_filepath("") && "test_is_valid_filepath_empty_string failed");
}

void test_is_valid_filepath_with_path_separator() {
    assert(is_valid_filepath("folder/file.txt") && "test_is_valid_filepath_with_path_separator failed");
}

// Test cases for is_valid_filename
void test_is_valid_filename_valid() {
    assert(is_valid_filename("file.txt") && "test_is_valid_filename_valid failed");
}

void test_is_valid_filename_invalid_characters() {
    assert(!is_valid_filename("file/name.txt") && "test_is_valid_filename_invalid_characters failed");
    assert(!is_valid_filename("file<name>.txt") && "test_is_valid_filename_invalid_characters failed");
    assert(!is_valid_filename("file:name.txt") && "test_is_valid_filename_invalid_characters failed");
    assert(!is_valid_filename("file?name.txt") && "test_is_valid_filename_invalid_characters failed");
    assert(!is_valid_filename("file*name.txt") && "test_is_valid_filename_invalid_characters failed");
    assert(!is_valid_filename("file\"name.txt") && "test_is_valid_filename_invalid_characters failed");
    assert(!is_valid_filename("file|name.txt") && "test_is_valid_filename_invalid_characters failed");
}

void test_is_valid_filename_empty_string() {
    assert(!is_valid_filename("") && "test_is_valid_filename_empty_string failed");
}

void test_is_valid_filename_with_path_separator() {
    assert(!is_valid_filename("folder/file.txt") && "test_is_valid_filename_with_path_separator failed");
}

// Register tests
TEST(test_explode_basic);
TEST(test_explode_no_delimiter);
TEST(test_explode_start_with_delimiter);
TEST(test_explode_end_with_delimiter);
TEST(test_explode_consecutive_delimiters);
TEST(test_explode_only_delimiters);
TEST(test_explode_empty_input);
TEST(test_explode_invalid_delimiter);
TEST(test_implode_basic);
TEST(test_implode_single_element);
TEST(test_implode_empty_elements);
TEST(test_implode_empty_delimiter);
TEST(test_implode_empty_vector);
TEST(test_implode_mixed_elements);
TEST(test_escape_empty_input);
TEST(test_escape_no_special_chars);
TEST(test_escape_single_char);
TEST(test_escape_already_escaped);
TEST(test_escape_no_chars);
TEST(test_escape_custom_chars);
TEST(test_escape_mixed_content);
TEST(test_escape_custom_chars2);
TEST(test_escape_custom_escape_sequence);
TEST(test_escape_already_escaped_in_input);
TEST(test_escape_multiple_chars);
TEST(test_quote_cmd_basic);
TEST(test_quote_cmd_with_special_chars);
TEST(test_quote_cmd_empty_input);
TEST(test_quote_cmd_already_escaped);
TEST(test_quote_cmd_mixed_content);
TEST(test_quote_cmd_only_special_chars);
TEST(test_levenshtein_same_string);
TEST(test_levenshtein_empty_string);
TEST(test_levenshtein_one_empty_string);
TEST(test_levenshtein_single_insertion);
TEST(test_levenshtein_single_deletion);
TEST(test_levenshtein_single_substitution);
TEST(test_levenshtein_multiple_operations);
TEST(test_levenshtein_case_sensitivity);
TEST(test_levenshtein_unicode);
TEST(test_str_cut_begin_when_string_is_short);
TEST(test_str_cut_begin_when_string_is_exact_length);
TEST(test_str_cut_begin_when_string_needs_truncation);
TEST(test_str_cut_begin_when_custom_prepend_is_used);
TEST(test_str_cut_begin_when_string_is_empty);
TEST(test_str_cut_begin_when_maxch_is_less_than_prepend_length);
TEST(test_str_cut_end_when_string_is_short);
TEST(test_str_cut_end_when_string_is_exact_length);
TEST(test_str_cut_end_when_string_needs_truncation);
TEST(test_str_cut_end_when_custom_append_is_used);
TEST(test_str_cut_end_when_string_is_empty);
TEST(test_str_cut_end_when_maxch_is_less_than_append_length);
TEST(test_str_cut_ratio_when_ratio_is_default);
TEST(test_str_cut_ratio_when_ratio_is_half);
TEST(test_str_cut_ratio_when_ratio_is_zero);
TEST(test_str_cut_ratio_when_ratio_is_one);
TEST(test_str_cut_ratio_when_ratio_is_small);
TEST(test_str_cut_ratio_when_ratio_is_large);
TEST(test_str_cut_ratio_when_input_is_empty);
TEST(test_str_cut_ratio_when_ratio_is_invalid);
TEST(test_trim_no_whitespace);
TEST(test_trim_leading_whitespace);
TEST(test_trim_trailing_whitespace);
TEST(test_trim_both_leading_and_trailing_whitespace);
TEST(test_trim_only_whitespace);
TEST(test_trim_empty_string);
TEST(test_trim_mixed_whitespace);
TEST(test_trim_whitespace_in_middle);
TEST(test_str_starts_with_basic);
TEST(test_str_starts_with_empty_prefix);
TEST(test_str_starts_with_empty_string);
TEST(test_str_starts_with_prefix_longer_than_string);
TEST(test_str_starts_with_no_match);
TEST(test_str_starts_with_case_sensitive);
TEST(test_str_ends_with_basic);
TEST(test_str_ends_with_empty_suffix);
TEST(test_str_ends_with_empty_string);
TEST(test_str_ends_with_suffix_longer_than_string);
TEST(test_str_ends_with_no_match);
TEST(test_str_ends_with_case_sensitive);
TEST(test_str_replace_single_replacement);
TEST(test_str_replace_multiple_occurrences);
TEST(test_str_replace_no_match);
TEST(test_str_replace_empty_to_multiple_occurrences);
TEST(test_str_replace_empty_to_no_match);
TEST(test_str_replace_non_empty_to);
TEST(test_str_replace_empty_input);
TEST(test_str_replace_empty_to);
TEST(test_str_replace_empty_from);
TEST(test_str_replace_map_multiple_replacements);
TEST(test_str_replace_map_overlapping_keys);
TEST(test_str_replace_map_empty_input);
TEST(test_tpl_replace_single_replacement);
TEST(test_tpl_replace_multiple_replacements);
TEST(test_tpl_replace_no_placeholder_in_template);
TEST(test_tpl_replace_missing_replacement_value);
TEST(test_tpl_replace_invalid_placeholder_regex);
TEST(test_tpl_replace_empty_template);
TEST(test_tpl_replace_empty_replacement_value);
TEST(test_parse_valid_integer);
TEST(test_parse_valid_double);
TEST(test_parse_negative_number);
TEST(test_parse_invalid_input);
TEST(test_parse_empty_string);
TEST(test_parse_whitespace_only);
TEST(test_parse_trailing_characters);
TEST(test_parse_floating_point_with_exponent);
TEST(test_parse_zero);
TEST(test_parse_large_number);
TEST(test_split_single_word);
TEST(test_split_multiple_words);
TEST(test_split_with_extra_whitespace);
TEST(test_split_empty_string);
TEST(test_split_only_whitespace);
TEST(test_split_with_punctuation);
TEST(test_split_mixed_whitespace);
TEST(test_split_numbers);
TEST(test_split_with_tabs_and_newlines);
TEST(test_is_numeric_valid_integer);
TEST(test_is_numeric_valid_decimal);
TEST(test_is_numeric_negative_integer);
TEST(test_is_numeric_positive_integer);
TEST(test_is_numeric_negative_decimal);
TEST(test_is_numeric_positive_decimal);
TEST(test_is_numeric_empty_string);
TEST(test_is_numeric_only_sign);
TEST(test_is_numeric_multiple_decimals);
TEST(test_is_numeric_trailing_decimal);
TEST(test_is_numeric_leading_decimal);
TEST(test_is_numeric_invalid_characters);
TEST(test_is_numeric_whitespace);
TEST(test_is_integer_valid_positive_integer);
TEST(test_is_integer_valid_negative_integer);
TEST(test_is_integer_valid_positive_with_plus);
TEST(test_is_integer_empty_string);
TEST(test_is_integer_only_sign);
TEST(test_is_integer_invalid_characters);
TEST(test_is_integer_whitespace);
TEST(test_is_integer_leading_zeros);
TEST(test_is_integer_zero);
TEST(test_is_int_alias);
TEST(test_set_precision_double_basic);
TEST(test_set_precision_double_zero_precision);
TEST(test_set_precision_double_negative_precision);
TEST(test_set_precision_double_large_precision);
TEST(test_set_precision_double_negative_number);
TEST(test_set_precision_double_zero);
TEST(test_set_precision_string_basic);
TEST(test_set_precision_string_invalid_input);
TEST(test_set_precision_string_empty_input);
TEST(test_set_precision_string_negative_number);
TEST(test_set_precision_string_zero);
TEST(test_strtolower_basic_conversion);
TEST(test_strtolower_mixed_case);
TEST(test_strtolower_already_lowercase);
TEST(test_strtolower_empty_string);
TEST(test_strtolower_with_numbers_and_symbols);
TEST(test_strtoupper_basic_conversion);
TEST(test_strtoupper_mixed_case);
TEST(test_strtoupper_already_uppercase);
TEST(test_strtoupper_empty_string);
TEST(test_strtoupper_with_numbers_and_symbols);
TEST(test_is_valid_filepath_valid);
TEST(test_is_valid_filepath_invalid_characters);
TEST(test_is_valid_filepath_empty_string);
TEST(test_is_valid_filepath_with_path_separator);
TEST(test_is_valid_filename_valid);
TEST(test_is_valid_filename_invalid_characters);
TEST(test_is_valid_filename_empty_string);
TEST(test_is_valid_filename_with_path_separator);
#endif