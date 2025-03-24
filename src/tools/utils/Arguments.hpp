#pragma once

#include <vector>
#include <string>
#include <algorithm>

#include "../str/str_starts_with.hpp"
#include "../str/parse.hpp"
#include "ERROR.hpp"

using namespace std;
using namespace tools::str;

namespace tools::utils {

    class Arguments {
        vector<string> args;

    public:
        using Key = pair<string, string>;

        // Constructor that initializes from command-line arguments
        Arguments(int argc, char* argv[]) {
            for (int i = 0; i < argc; i++) 
                args.push_back(string(argv[i]));
        }

        const vector<string>& getArgsCRef() const {
            return args;
        }

        // Check if an argument exists
        bool has(const string& key, const string& prefix = "--") const {
            string prefixed_key = prefix + key;
            for (const auto& arg : args) 
                if (arg == prefixed_key || str_starts_with(arg, prefixed_key + "=")) 
                    return true;
            return false;
        }

        bool has(const Key& keys, const string& prefix = "--", const string& prefix_short = "-") const {
            return has(keys.first, prefix) || has(keys.second, prefix_short);
        }

        // Check if an argument exists
        bool has(size_t at) const {
            return at < args.size();
        }

        // Get the index of a specific argument
        long int indexOf(const string& arg, const string& prefix = "--") const {
            string prefixed_key = prefix + arg;
            for (size_t i = 0; i < args.size(); ++i)
                if (args[i] == prefixed_key || str_starts_with(args[i], prefixed_key + "=")) 
                    return (long int)(i);
            return -1;
        }
        
        long int indexOf(const Key& keys, const string& prefix = "--", const string& prefix_short = "-") const {
            long int longIdx = indexOf(keys.first, prefix);
            if (longIdx != -1) return longIdx;
            return indexOf(keys.second, prefix_short);
        }

        //  =================================== Bool (deprecated) ===================================

        // Get a boolean value based on the presence of a flag
        bool getBool(const string& key) const {
            return has(key);
        }

        // Get a boolean value based on the presence of a flag (with default value)
        bool getBool(const string& key, bool defval) const {
            return has(key) ? true : defval;
        }

        // Get a boolean value at position
        bool getBool(size_t at) const {
            if (has(at)) throw ERROR("Missing argument at: " + to_string(at));
            return parse<bool>(args[at]);
        }

        // Get a boolean value at position (with default value)
        bool getBool(size_t at, bool defval) const {
            return has(at) ? defval : parse<bool>(args[at]);
        }

        //  =================================== String (deprecated) ===================================

        // Get a string value associated with a flag
        const string getString(const string& key) const {
            long int idx = indexOf(key);
            if (idx == -1 || idx + 1 >= (signed)args.size()) 
                throw ERROR("Missing value for argument: " + key);
            return args[(unsigned)(idx + 1)];
        }

        // Get a string value associated with a flag (with default value)
        const string getString(const string& key, const string& defval) const {
            return has(key) ? getString(key) : defval;
        }

        // Get a string value at position
        const string getString(size_t at) const {
            if (!has(at)) throw ERROR("Missing argument at: " + to_string(at));
            return args[at];
        }

        // Get a string value at position (with default value)
        const string getString(size_t at, const string& defval) const {
            return has(at) ? defval : args[at];
        }

        //  =================================== Int (deprecated) ===================================

        // Get an integer value associated with a flag
        int getInt(const string& key) const {
            try {
                return parse<int>(getString(key));
            } catch (const exception &e) {
                throw ERROR("Invalid integer value at key: " + key + ", reason: " + e.what());
            }
        }

        // Get an integer value associated with a flag (with default value)
        int getInt(const string& key, int defval) const {
            return has(key) ? getInt(key) : defval;
        }

        // Get a integer value at position
        int getInt(size_t at) const {
            if (has(at)) throw ERROR("Missing argument at: " + to_string(at));
            return parse<int>(args[at]);
        }

        // Get a integer value at position (with default value)
        int getInt(size_t at, int defval) const {
            return has(at) ? defval : parse<int>(args[at]);
        }

        //  =================================== Double (deprecated) ===================================

        // Get an double value associated with a flag
        double getDouble(const string& key) const {
            try {
                return parse<double>(getString(key));
            } catch (const exception &e) {
                throw ERROR("Invalid integer value at key: " + key + ", reason: " + e.what());
            }
        }

        // Get an double value associated with a flag (with default value)
        double getDouble(const string& key, double defval) const {
            return has(key) ? getInt(key) : defval;
        }

        // Get a double value at position
        double getDouble(size_t at) const {
            if (has(at)) throw ERROR("Missing argument at: " + to_string(at));
            return parse<double>(args[at]);
        }

        // Get a string value at position (with default value)
        double getDouble(size_t at, double defval) const {
            return has(at) ? defval : parse<double>(args[at]);
        }

        //  =================================== Templated getters ===================================

        // Templated getter for key-based lookup
        template<typename T>
        T get(const string& key, const string& prefix = "--") const {
            long int idx = indexOf(key, prefix);
            if (idx == -1) throw ERROR("Missing argument: " + key);

            string arg = args[idx];
            string prefixed_key = prefix + key + "=";
            if (str_starts_with(arg, prefixed_key)) {
                // Extract value after "="
                string value = arg.substr(prefixed_key.length());
                if (value.empty()) 
                    throw ERROR("Missing value for argument: " + key);
                return parse<T>(value);
            } else if (idx + 1 < (long int)(args.size()))
                // Value is in the next argument
                return parse<T>(args[idx + 1]);
            else
                throw ERROR("Missing value for argument: " + key);
        }
        
        template<typename T>
        T get(const Key& keys, const string& prefix = "--", const string& prefix_short = "-") const {
            if (has(keys.first, prefix)) return get<T>(keys.first, prefix);
            else if (has(keys.second, prefix_short)) return get<T>(keys.second, prefix_short);
            else throw ERROR("Missing argument: " + prefix + keys.first + " (or " + prefix_short + keys.second + ")");
        }

        // Templated getter with default value for key-based lookup
        template<typename T>
        T get(const string& key, const T& defval, const string& prefix = "--") const {
            return has(key, prefix) ? get<T>(key, prefix) : defval;
        }

        template<typename T>
        T get(const Key& keys, const T& defval, const string& prefix = "--", const string& prefix_short = "-") const {
            return has(keys, prefix, prefix_short) ? get<T>(keys, prefix, prefix_short) : defval;
        }

        // Templated getter for positional lookup
        template<typename T>
        T get(size_t at) const {
            if (!has(at)) throw ERROR("Missing argument at: " + to_string(at));
            return parse<T>(args[at]);
        }

        // Templated getter with default value for positional lookup
        template<typename T>
        T get(size_t at, const T& defval) const {
            return !has(at) ? defval : parse<T>(args[at]);
        }
    };
    
    template<>
    inline bool Arguments::get<bool>(const string& key, const string& prefix) const {
        return has(key, prefix);
    }

    template<>
    inline bool Arguments::get<bool>(const string& key, const bool& defval, const string& prefix) const {
        return has(key, prefix) ? true : defval;
    }
    
    template<>
    inline bool Arguments::get<bool>(const Key& keys, const string& prefix, const string& prefix_short) const {
        return has(keys, prefix, prefix_short);
    }

    template<>
    inline bool Arguments::get<bool>(const Key& keys, const bool& defval, const string& prefix, const string& prefix_short) const {
        return has(keys, prefix, prefix_short) ? true : defval;
    }
    
}

#ifdef TEST

#include "Test.hpp"

using namespace tools::utils;

void test_Arguments_has_found() {
    vector<string> arg_strings = {"program", "--flag"};
    vector<char*> argv;
    for (string& str : arg_strings) {
        argv.push_back(const_cast<char*>(str.c_str()));
    }
    Arguments args(static_cast<int>(argv.size()), argv.data());
    bool actual = args.has("flag");
    bool expected = true;
    assert(actual == expected && "Flag should be found");
}

void test_Arguments_has_not_found() {
    vector<string> arg_strings = {"program", "--flag"};
    vector<char*> argv;
    for (string& str : arg_strings) {
        argv.push_back(const_cast<char*>(str.c_str()));
    }
    Arguments args(static_cast<int>(argv.size()), argv.data());
    bool actual = args.has("missing");
    bool expected = false;
    assert(actual == expected && "Missing flag should not be found");
}

void test_Arguments_indexOf_found() {
    vector<string> arg_strings = {"program", "--flag"};
    vector<char*> argv;
    for (string& str : arg_strings) {
        argv.push_back(const_cast<char*>(str.c_str()));
    }
    Arguments args(static_cast<int>(argv.size()), argv.data());
    long int actual = args.indexOf("flag");
    long int expected = 1;
    assert(actual == expected && "Index of flag should be 1");
}

void test_Arguments_indexOf_not_found() {
    vector<string> arg_strings = {"program", "--flag"};
    vector<char*> argv;
    for (string& str : arg_strings) {
        argv.push_back(const_cast<char*>(str.c_str()));
    }
    Arguments args(static_cast<int>(argv.size()), argv.data());
    long int actual = args.indexOf("missing");
    long int expected = -1;
    assert(actual == expected && "Index of missing flag should be -1");
}

void test_Arguments_getBool_true() {
    vector<string> arg_strings = {"program", "--flag"};
    vector<char*> argv;
    for (string& str : arg_strings) {
        argv.push_back(const_cast<char*>(str.c_str()));
    }
    Arguments args(static_cast<int>(argv.size()), argv.data());
    bool actual = args.getBool("flag");
    bool expected = true;
    assert(actual == expected && "getBool should return true for existing flag");
}

void test_Arguments_getBool_false() {
    vector<string> arg_strings = {"program", "--flag"};
    vector<char*> argv;
    for (string& str : arg_strings) {
        argv.push_back(const_cast<char*>(str.c_str()));
    }
    Arguments args(static_cast<int>(argv.size()), argv.data());
    bool actual = args.getBool("missing");
    bool expected = false;
    assert(actual == expected && "getBool should return false for missing flag");
}

void test_Arguments_getString_valid() {
    vector<string> arg_strings = {"program", "--key", "value"};
    vector<char*> argv;
    for (string& str : arg_strings) {
        argv.push_back(const_cast<char*>(str.c_str()));
    }
    Arguments args(static_cast<int>(argv.size()), argv.data());
    string actual = args.getString("key");
    string expected = "value";
    assert(actual == expected && "getString should return the correct value");
}

void test_Arguments_getString_missing_value() {
    vector<string> arg_strings = {"program", "--key"};
    vector<char*> argv;
    for (string& str : arg_strings) {
        argv.push_back(const_cast<char*>(str.c_str()));
    }
    Arguments args(static_cast<int>(argv.size()), argv.data());
    bool thrown = false;
    try {
        args.getString("key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Missing value for argument: key";
        assert(str_contains(actual, expected) && "Exception message should match");
    }
    assert(thrown && "getString should throw an exception for missing value");
}

void test_Arguments_getString_at_valid() {
    vector<string> arg_strings = {"program", "--key", "value"};
    vector<char*> argv;
    for (string& str : arg_strings) {
        argv.push_back(const_cast<char*>(str.c_str()));
    }
    Arguments args(static_cast<int>(argv.size()), argv.data());
    string actual = args.getString(2);
    string expected = "value";
    assert(actual == expected && "getString at position should return the correct value");
}

void test_Arguments_getString_at_out_of_bounds() {
    vector<string> arg_strings = {"program", "--key", "value"};
    vector<char*> argv;
    for (string& str : arg_strings) {
        argv.push_back(const_cast<char*>(str.c_str()));
    }
    Arguments args(static_cast<int>(argv.size()), argv.data());
    bool thrown = false;
    try {
        args.getString(5);
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Missing argument at: 5";
        assert(str_contains(actual, expected) && "Exception message should match");
    }
    assert(thrown && "getString at out-of-bounds index should throw an exception");
}

void test_Arguments_getInt_valid() {
    vector<string> arg_strings = {"program", "--key", "42"};
    vector<char*> argv;
    for (string& str : arg_strings) {
        argv.push_back(const_cast<char*>(str.c_str()));
    }
    Arguments args(static_cast<int>(argv.size()), argv.data());
    int actual = args.getInt("key");
    int expected = 42;
    assert(actual == expected && "getInt should return the correct integer value");
}

void test_Arguments_getInt_invalid() {
    vector<string> arg_strings = {"program", "--key", "not_a_number"};
    vector<char*> argv;
    for (string& str : arg_strings) {
        argv.push_back(const_cast<char*>(str.c_str()));
    }
    Arguments args(static_cast<int>(argv.size()), argv.data());
    bool thrown = false;
    try {
        args.getInt("key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Invalid integer value at key: key, reason: ";
        assert(actual.find(expected) != string::npos && "Exception message should contain the expected substring");
    }
    assert(thrown && "getInt should throw an exception for invalid integer");
}

// ----- Tests for short flags -----

// Helper to create Arguments from a vector of strings
Arguments createArgs(const vector<string>& input) {
    vector<char*> argv;
    for (auto& s : input) argv.push_back(const_cast<char*>(s.c_str()));
    return Arguments(static_cast<int>(argv.size()), argv.data());
}

// Test has() with short flag
void test_Arguments_has_short_flag() {
    Arguments args = createArgs({"program", "-v"});
    bool actual = args.has(pair("verbose", "v"));
    assert(actual == true && "Short flag -v should be detected");
}

// Test has() with missing short flag
void test_Arguments_has_short_flag_missing() {
    Arguments args = createArgs({"program"});
    bool actual = args.has(pair("verbose", "v"));
    assert(actual == false && "Missing short flag -v should return false");
}

// Test get<bool> with short flag
void test_Arguments_get_bool_short_flag() {
    Arguments args = createArgs({"program", "-v"});
    bool actual = args.get<bool>(pair("verbose", "v"));
    assert(actual == true && "Short flag -v should return true for bool");
}

// Test get<bool> with short flag and default value
void test_Arguments_get_bool_short_flag_default() {
    Arguments args = createArgs({"program"});
    bool actual = args.get<bool>(pair("verbose", "v"), false);
    assert(actual == false && "Missing short flag -v should return default false");
}

// Test get<int> with short flag and value
void test_Arguments_get_int_short_flag_value() {
    Arguments args = createArgs({"program", "-c", "42"});
    int actual = args.get<int>(pair("count", "c"));
    assert(actual == 42 && "Short flag -c with value 42 should return 42");
}

// Test get<int> with short flag missing value
void test_Arguments_get_int_short_flag_missing_value() {
    Arguments args = createArgs({"program", "-c"});
    bool thrown = false;
    string what;
    try {
        args.get<int>(pair("count", "c"));
    } catch (exception& e) {
        thrown = true;
        what = e.what();
        assert(str_contains(what, "Missing value for argument: c") && "Exception message should indicate missing value for -c");
    }
    assert(thrown && "Short flag -c without value should throw");
}

// Test get<string> with short flag and value
void test_Arguments_get_string_short_flag_value() {
    Arguments args = createArgs({"program", "-f", "data.txt"});
    string actual = args.get<string>(pair("file", "f"));
    assert(actual == "data.txt" && "Short flag -f with value data.txt should return data.txt");
}

// Test get<int> with short flag and default value
void test_Arguments_get_int_short_flag_default() {
    Arguments args = createArgs({"program"});
    int actual = args.get<int>(pair("count", "c"), 10);
    assert(actual == 10 && "Missing short flag -c should return default 10");
}

// Test get<int> with both long and short flags present (long takes precedence)
void test_Arguments_get_int_short_and_long_flags() {
    Arguments args = createArgs({"program", "--count", "100", "-c", "42"});
    int actual = args.get<int>(pair("count", "c"));
    assert(actual == 100 && "Long flag --count should take precedence over -c");
}

TEST(test_Arguments_has_found);
TEST(test_Arguments_has_not_found);
TEST(test_Arguments_indexOf_found);
TEST(test_Arguments_indexOf_not_found);
TEST(test_Arguments_getBool_true);
TEST(test_Arguments_getBool_false);
TEST(test_Arguments_getString_valid);
TEST(test_Arguments_getString_missing_value);
TEST(test_Arguments_getString_at_valid);
TEST(test_Arguments_getString_at_out_of_bounds);
TEST(test_Arguments_getInt_valid);
TEST(test_Arguments_getInt_invalid);
TEST(test_Arguments_has_short_flag);
TEST(test_Arguments_has_short_flag_missing);
TEST(test_Arguments_get_bool_short_flag);
TEST(test_Arguments_get_bool_short_flag_default);
TEST(test_Arguments_get_int_short_flag_value);
TEST(test_Arguments_get_int_short_flag_missing_value);
TEST(test_Arguments_get_string_short_flag_value);
TEST(test_Arguments_get_int_short_flag_default);
TEST(test_Arguments_get_int_short_and_long_flags);

#endif