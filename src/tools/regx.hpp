#pragma once

#include <regex>

using namespace std;

namespace tools {

    inline int regx_match(
        const string& pattern, 
        const string& str, 
        vector<string>* matches = nullptr
    ) {
        regex r(pattern);
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
        vector<string>* matches = nullptr
    ) {
        regex r(pattern);
        smatch m;

        if (matches != nullptr) {
            // Clear the vector before adding more matches
            matches->clear();
        }

        // Use regex_iterator to find all matches
        for (sregex_iterator it(str.begin(), str.end(), r), end; it != end; ++it) {
            if (matches != nullptr) {
                for (const auto& match : *it) {
                    matches->push_back(match.str());
                }
            }
        }

        return (int)(matches != nullptr && !matches->empty());
    }

    /// @brief Replace first regex match in string.
    /// @param pattern regex pattern to match 
    /// @param str input string
    /// @param replace string to replace matches with
    /// @return string with first match replaced
    inline string regx_replace(const string& pattern, const string& str, const string& replace) { // TODO tests
        regex r(pattern);
        return regex_replace(str, r, replace);
    }

    /// @brief Replace all regex matches in string.
    /// @param pattern regex pattern to match
    /// @param str input string 
    /// @param replace string to replace matches with 
    /// @return string with all matches replaced
    inline string regx_replace_all(const string& pattern, const string& str, const string& replace) { // TODO tests
        regex r(pattern);
        return regex_replace(str, r, replace); 
    }

}