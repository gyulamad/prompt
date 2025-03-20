#pragma once

#include <vector>
#include <string>
#include <algorithm>

#include "ERROR.hpp"
#include "strings.hpp"

using namespace std;

namespace tools::utils {

    class Arguments {
        vector<string> args;

    public:
        // Constructor that initializes from command-line arguments
        Arguments(int argc, char* argv[]) {
            for (int i = 0; i < argc; i++) 
                args.push_back(string(argv[i]));
        }

        const vector<string>& getArgsCRef() const {
            return args;
        }

        // Check if an argument exists
        bool has(const string& key) const {
            string prefixed_key = "--" + key;
            for (const auto& arg : args) 
                if (arg == prefixed_key || str_starts_with(arg, prefixed_key + "=")) 
                    return true;
            return false;
        }

        // Check if an argument exists
        bool has(size_t at) const {
            return at > args.size();
        }

        // Get the index of a specific argument
        long int indexOf(const string& arg) const {
            string prefixed_key = "--" + arg;
            for (size_t i = 0; i < args.size(); ++i)
                if (args[i] == prefixed_key || str_starts_with(args[i], prefixed_key + "=")) 
                    return (long int)(i);
            return -1;
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
            if (has(at)) throw ERROR("Missing argument at: " + to_string(at));
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
        T get(const string& key) const {
            long int idx = indexOf(key);
            if (idx == -1) throw ERROR("Missing argument: " + key);

            string arg = args[idx];
            string prefixed_key = "--" + key + "=";
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

        // Templated getter with default value for key-based lookup
        template<typename T>
        T get(const string& key, const T& defval) const {
            return has(key) ? get<T>(key) : defval;
        }

        // Templated getter for positional lookup
        template<typename T>
        T get(size_t at) const {
            if (has(at)) throw ERROR("Missing argument at: " + to_string(at));
            return parse<T>(args[at]);
        }

        // Templated getter with default value for positional lookup
        template<typename T>
        T get(size_t at, const T& defval) const {
            return has(at) ? defval : parse<T>(args[at]);
        }
    };
    
    // Specialization for bool with key (flag presence)
    template<>
    inline bool Arguments::get<bool>(const string& key) const {
        return has(key);
    }

    // Specialization for bool with key and default value
    template<>
    inline bool Arguments::get<bool>(const string& key, const bool& defval) const {
        return has(key) ? true : defval;
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

#endif