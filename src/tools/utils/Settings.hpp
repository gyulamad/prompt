#pragma once

#include "../str/get_hash.hpp"
#include "JSON.hpp"
#include "Arguments.hpp"

using namespace tools::str;

namespace tools::utils {

    class JSONExts {
    public:
    
        void extends(JSON ext) {
            exts.emplace(exts.begin(), ext);
        }
    
        template<typename T>
        T get(string jselector) const {
            for (const JSON& ext: exts)
                if (ext.has(jselector))
                    return ext.get<T>(jselector);
            throw ERROR("No value at '" + jselector + "'");
        }
    
        bool has(string jselector) const {
            for (const JSON& ext: exts)
                if (ext.has(jselector))
                    return true;
            return false;
        }
    
        template<typename T>
        void set(string jselector, T value) {
            if (exts.empty()) exts.push_back(JSON("{}"));
            exts[0].set(jselector, value);
        }

        bool empty() const {
            return exts.empty();
        }

        string dump(const int indent = -1, const char indent_char = ' ') const {
            vector<string> dumps;
            for (const JSON& ext: exts)
                dumps.push_back(ext.dump(indent, indent_char));
            return "[" + implode(",", dumps) + "]";
        }
    
    private:
        vector<JSON> exts;
    };

    class Settings {
    public:
    
        Settings() {}
        Settings(JSON& conf): conf(&conf) {}
        Settings(Arguments& args): args(&args) {}
        Settings(JSON& conf, Arguments& args): args(&args), conf(&conf) {}
        Settings(Arguments& args, JSON& conf): args(&args), conf(&conf) {}

        void extends(JSON ext) {
            exts.extends(ext);
        }
    
        string hash() const { 
            return get_hash(
                (!exts.empty() ? exts.dump() : "<noexts>") +
                (conf ? conf->dump() : "<noconf>") + 
                (args ? implode(" ", args->getArgsCRef()) : "<noargs>")
            );
        }
    
        bool has(const string& key) {            
            if (args && args->has(key)) return true;
            if (!exts.empty() && exts.has(key)) return true;
            if (conf && conf->has(key)) return true;
            return false;
        }
    
        bool has(const pair<string, string>& keys) {
            if (args && args->has(keys)) return true;
            if (!exts.empty() && exts.has(keys.first)) return true;
            if (conf && conf->has(keys.first)) return true;
            return false;
        }
    
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
        struct is_container: false_type {};
    
        // Specialization for std::vector
        template<typename T>
        struct is_container<vector<T>>: true_type {};
        
        // Specialization for std::map
        template<typename K, typename V>
        struct is_container<map<K, V>>: true_type {};
    };

}

#ifdef TEST

#include "Test.hpp"

using namespace tools::utils;

// Test constructor with no arguments
void test_Settings_constructor_default() {
    Settings settings;
    assert(settings.args == nullptr && "Default constructor should set args to nullptr");
    assert(settings.conf == nullptr && "Default constructor should set conf to nullptr");
}

// Test constructor with JSON only
void test_Settings_constructor_json() {
    JSON json;
    Settings settings(json);
    assert(settings.args == nullptr && "JSON constructor should set args to nullptr");
    assert(settings.conf == &json && "JSON constructor should set conf correctly");
}

// Test constructor with Arguments only
void test_Settings_constructor_args() {
    char* argv[] = {(char*)"program"};
    Arguments args(1, argv);
    Settings settings(args);
    assert(settings.args == &args && "Args constructor should set args correctly");
    assert(settings.conf == nullptr && "Args constructor should set conf to nullptr");
}

// Test hash function with both conf and args
void test_Settings_hash_with_both() {
    JSON json;
    json.set("key", "value");
    char* argv[] = {(char*)"program", (char*)"--test", (char*)"123"};
    Arguments args(3, argv);
    Settings settings(json, args);
    
    // Capture the actual string being hashed
    string conf_str = json.dump();
    string args_str = implode(" ", args.getArgsCRef());
    string actual_input = conf_str + args_str;
    string actual = settings.hash();
    
    // Expected string and hash
    string expected_input = "{\"key\":\"value\"}program --test 123";  // Adjust based on actual json.dump() output
    string expected = get_hash("<noexts>" + expected_input);
    
    // Debug: Check if the input strings match
    assert(actual_input == expected_input && "Input string to hash function does not match expected");
    assert(actual == expected && "Hash should combine conf and args correctly");
}

// Test hash function with no conf
void test_Settings_hash_no_conf() {
    char* argv[] = {(char*)"program", (char*)"--test", (char*)"456"};
    Arguments args(3, argv);
    Settings settings(args);
    string actual = settings.hash();
    string expected = get_hash("<noexts><noconf>program --test 456");  // No space between "<noconf>" and "program"
    assert(actual == expected && "Hash should handle missing conf correctly");
}

// Test has() with single key in args
void test_Settings_has_single_key_args() {
    char* argv[] = {(char*)"program", (char*)"--flag"};
    Arguments args(2, argv);
    Settings settings(args);
    bool actual = settings.has("flag");
    assert(actual == true && "Should find key in args");
}

// Test has() with single key in conf
void test_Settings_has_single_key_conf() {
    JSON json;
    json.set("key", "value");
    Settings settings(json);
    bool actual = settings.has("key");
    assert(actual == true && "Should find key in conf");
}

// Test get() with missing key throwing exception
void test_Settings_get_missing_key() {
    Settings settings;
    bool thrown = false;
    string what;
    
    try {
        settings.get<int>("missing");
    } catch (exception& e) {
        thrown = true;
        what = e.what();
        assert(str_contains(what, "Settings is missing for 'missing'") && "Exception message should match");
    }
    
    assert(thrown && "Get should throw when key is missing");
}

// Test get() with default value
void test_Settings_get_with_default() {
    Settings settings;
    int actual = settings.get<int>("missing", 42);
    assert(actual == 42 && "Should return default value when key is missing");
}

// Test get() priority args over conf
void test_Settings_get_priority_args_over_conf() {
    JSON json;
    json.set("key", 1);
    char* argv[] = {(char*)"program", (char*)"--key", (char*)"2"};
    Arguments args(3, argv);
    Settings settings(json, args);
    int actual = settings.get<int>("key");
    assert(actual == 2 && "Args should take priority over conf");
}

// Test get() with pair keys from args
void test_Settings_get_pair_from_args() {
    char* argv[] = {(char*)"program", (char*)"--flag", (char*)"true"};
    Arguments args(3, argv);
    Settings settings(args);
    bool actual = settings.get<bool>(pair("flag", "other"));
    assert(actual == true && "Should get value from args using pair key");
}

// Test get() with pair keys missing
void test_Settings_get_pair_missing() {
    Settings settings;
    bool thrown = false;
    string what;
    
    try {
        settings.get<int>(pair("key1", "key2"));
    } catch (exception& e) {
        thrown = true;
        what = e.what();
        assert(str_contains(what, "Settings is missing for 'key1' (or 'key2')") && "Exception message should match");
    }
    
    assert(thrown && "Get with pair should throw when both keys are missing");
}

// Test get() with pair and default
void test_Settings_get_pair_with_default() {
    Settings settings;
    string actual = settings.get<string>(pair("key1", "key2"), "default");
    assert(actual == "default" && "Should return default value when pair keys are missing");
}

// Register tests
TEST(test_Settings_constructor_default);
TEST(test_Settings_constructor_json);
TEST(test_Settings_constructor_args);
TEST(test_Settings_hash_with_both);
TEST(test_Settings_hash_no_conf);
TEST(test_Settings_has_single_key_args);
TEST(test_Settings_has_single_key_conf);
TEST(test_Settings_get_missing_key);
TEST(test_Settings_get_with_default);
TEST(test_Settings_get_priority_args_over_conf);
TEST(test_Settings_get_pair_from_args);
TEST(test_Settings_get_pair_missing);
TEST(test_Settings_get_pair_with_default);

#endif