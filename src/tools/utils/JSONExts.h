#pragma once

#include "JSON.h"
#include <string>
#include <vector>

using namespace std;

namespace tools::utils {

    class JSONExts {
    public:
        // Add a JSON object to the extensions
        void extends(JSON ext);
    
        template<typename T>
        T get(string jselector) const {
            for (const JSON& ext: exts)
                if (ext.has(jselector))
                    return ext.get<T>(jselector);
            throw ERROR("No value at '" + jselector + "'");
        }

        // Check if a jselector exists
        bool has(string jselector) const;

        template<typename T>
        void set(string jselector, T value) {
            if (exts.empty()) exts.push_back(JSON("{}"));
            exts[0].set(jselector, value);
        }

        // Check if the extensions list is empty
        bool empty() const;

        // Dump the extensions as a JSON array string
        string dump(int indent = -1, char indent_char = ' ') const;

    private:
        vector<JSON> exts;
    };

} // namespace tools::utils

#ifdef RUN_TEST
// Test function declarations (in global namespace)
void test_JSONExts_extends_adds_json();
void test_JSONExts_get_existing_key();
void test_JSONExts_get_missing_key();
void test_JSONExts_has_existing_key();
void test_JSONExts_has_missing_key();
void test_JSONExts_dump_single_json();
void test_JSONExts_dump_multiple_jsons();
#endif