#pragma once

#include "../str/get_hash.h"
#include "JSON.h"
#include "JSONExts.h"
#include "Arguments.h"
#include <string>
#include <vector>
#include <map>
#include <type_traits>

using namespace std;

namespace tools::utils {

    class Settings {
    public:
        Settings();
        Settings(JSON& conf);
        Settings(Arguments& args);
        Settings(JSON& conf, Arguments& args);
        Settings(Arguments& args, JSON& conf);

        // Add a JSON extension
        void extends(JSON ext);

        // Compute a hash of the settings
        string hash() const;

        // Check if a key exists
        bool has(const string& key);
        bool has(const pair<string, string>& keys);

        template<typename T>
        T get(const string& key) {
            if constexpr (!is_container<T>::value) if (args && args->has(key)) return args->get<T>(key);
            if (!exts.empty() && exts.has(key)) return exts.get<T>(key);
            if (conf && conf->has(key)) return conf->get<T>(key);
            throw ERROR("Settings is missing for '" + key + "'");
        }
    
        template<typename T>
        T get(const string& key, const T defval) {
            if constexpr (!is_container<T>::value) if (args && args->has(key)) return args->get<T>(key);
            if (!exts.empty() && exts.has(key)) return exts.get<T>(key);
            if (conf && conf->has(key)) return conf->get<T>(key);
            return defval;
        }
    
        template<typename T>
        T get(const pair<string, string>& keys) {
            if constexpr (!is_container<T>::value) if (args && args->has(keys)) return args->get<T>(keys);
            if (!exts.empty() && exts.has(keys.first)) return exts.get<T>(keys.first);
            if (conf && conf->has(keys.first)) return conf->get<T>(keys.first);
            throw ERROR("Settings is missing for '" + keys.first + "' (or '" + keys.second + "')");
        }
    
        template<typename T>
        T get(const pair<string, string>& keys, const T defval) {
            if constexpr (!is_container<T>::value) if (args && args->has(keys)) return args->get<T>(keys);
            if (!exts.empty() && exts.has(keys.first)) return exts.get<T>(keys.first);
            if (conf && conf->has(keys.first)) return conf->get<T>(keys.first);
            return defval;
        }

        Arguments* args = nullptr;
        JSON* conf = nullptr;
        JSONExts exts;

    private:
        // Default case: assume types are not containers
        template<typename T>
        struct is_container : false_type {};

        // Specialization for vector
        template<typename T>
        struct is_container<vector<T>> : true_type {};

        // Specialization for map
        template<typename K, typename V>
        struct is_container<map<K, V>> : true_type {};
    };

} // namespace tools::utils

#ifdef TEST
// Test function declarations (in global namespace)
void test_Settings_constructor_default();
void test_Settings_constructor_json();
void test_Settings_constructor_args();
void test_Settings_hash_with_both();
void test_Settings_hash_no_conf();
void test_Settings_has_single_key_args();
void test_Settings_has_single_key_conf();
void test_Settings_get_missing_key();
void test_Settings_get_with_default();
void test_Settings_get_priority_args_over_conf();
void test_Settings_get_pair_from_args();
void test_Settings_get_pair_missing();
void test_Settings_get_pair_with_default();
void test_Settings_constructor_args_conf();
void test_Settings_extends_adds_json();
void test_Settings_has_returns_false();
#endif