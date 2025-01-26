#pragma once

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <type_traits>

using namespace std;

namespace tools {

    vector<string> explode(const string& delimiter, const string& str) {
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
            if (i != 0) {
                oss << delimiter;
            }
            oss << elements[i];
        }
        return oss.str();
    }

    string escape(const string& input, const string& chars = "\\\"'`", const string& esc = "\\") {
        string result = input;
        
        // Iterate over each character in the 'chars' string
        for (char ch : chars) {
            // Escape each occurrence of the character in the result string
            size_t pos = 0;
            while ((pos = result.find(ch, pos)) != string::npos) {
                result.insert(pos, esc);
                pos += 2;  // Skip past the newly inserted backslash
            }
        }
        return result;
    }
    

    string str_replace(const map<string, string>& v, const string& s) {
        // Create a modifiable copy of the input string
        string result = s;

        // Iterate through each key-value pair in the map
        for (const auto& pair : v) {
            size_t pos = 0;

            // Search for the key in the string and replace all occurrences
            while ((pos = result.find(pair.first, pos)) != string::npos) {
                result.replace(pos, pair.first.length(), pair.second);
                pos += pair.second.length(); // Move past the replacement
            }
        }

        // Return the modified string
        return result;
    }
    string str_replace(const string& from, const string& to, const string& subject) {
        return str_replace({{from, to}}, subject);
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
        if (s.length() <= maxch) {
            return s;
        }

        // Truncate the string from the beginning and prepend the prefix
        return prepend + s.substr(s.length() - (maxch - prepend.length()));
    }

    string str_cut_end(const string& s, size_t maxch = 300, const string& append = "...") {
        // Check if the string is already shorter than or equal to the limit
        if (s.length() <= maxch) {
            return s;
        }

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
        if (ratio < 0.0 || ratio > 1.0) {
            throw invalid_argument("Ratio must be between 0.0 and 1.0");
        }

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
        if (start == string::npos) {
            return "";
        }

        // Find the first non-whitespace character from the end
        size_t end = str.find_last_not_of(" \t\n\r\f\v");

        // Return the substring from the first non-whitespace to the last non-whitespace character
        return str.substr(start, end - start + 1);
    }

    bool str_starts_with(const string& str, const string& prefix) {
        return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
    }
    
    bool str_contains(const string& str, const string& substring) {
        // Use string::find to check if the substring exists
        return str.find(substring) != string::npos;
    }
    

    template <typename T>
    T parse(const string& str) {
        static_assert(is_arithmetic<T>::value, "T must be an arithmetic type");
        stringstream ss(str);
        T num;
        if (ss >> num) {
            return num;
        } else {
            throw ERROR("Invalid input string (not a number): " + str_cut_end(str));
        }
    }

}