#pragma once

#include <vector>
#include <string>
#include <algorithm>

#include "ERROR.hpp"
#include "strings.hpp"

using namespace std;

namespace tools {

    class Arguments {
        vector<string> args;

    public:
        // Constructor that initializes from command-line arguments
        Arguments(int argc, char* argv[]) {
            for (int i = 0; i < argc; i++) 
                args.push_back(string(argv[i]));
        }

        // Check if an argument exists
        bool has(const string& arg) const {
            return find(args.begin(), args.end(), "--" + arg) != args.end();
        }

        // Get the index of a specific argument
        long int indexOf(const string& arg) const {
            auto it = find(args.begin(), args.end(), "--" + arg);
            return it != args.end() ? distance(args.begin(), it) : -1;
        }

        // Get a boolean value based on the presence of a flag
        bool getBool(const string& key) const {
            return has(key);
        }

        // Get a string value associated with a flag
        const string getString(const string& key) const {
            long int idx = indexOf(key);
            if (idx == -1 || idx + 1 >= (signed)args.size()) 
                throw ERROR("Missing value for argument: " + key);
            return args[(unsigned)(idx + 1)];
        }

        // Get a string value at position
        const string getString(size_t at) const {
            if (at >= args.size()) 
                throw ERROR("Missing argument at: " + to_string(at));
            return args[at];
        }

        // Get an integer value associated with a flag
        int getInt(const string& key) const {
            try {
                return stoi(getString(key));
            } catch (const exception &e) {
                throw ERROR("Invalid integer value at key: " + key + ", reason: " + e.what());
            }
        }
    };
    
}

#ifdef TEST

using namespace tools;

void test_Arguments_has_found() {
    vector<string> arg_strings = {"program", "--flag"};
    vector<char*> argv;
    for (auto& str : arg_strings) {
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
    for (auto& str : arg_strings) {
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
    for (auto& str : arg_strings) {
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
    for (auto& str : arg_strings) {
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
    for (auto& str : arg_strings) {
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
    for (auto& str : arg_strings) {
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
    for (auto& str : arg_strings) {
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
    for (auto& str : arg_strings) {
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
    for (auto& str : arg_strings) {
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
    for (auto& str : arg_strings) {
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
    for (auto& str : arg_strings) {
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
    for (auto& str : arg_strings) {
        argv.push_back(const_cast<char*>(str.c_str()));
    }
    Arguments args(static_cast<int>(argv.size()), argv.data());
    bool thrown = false;
    try {
        args.getInt("key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Invalid integer value at key: key, reason: stoi";
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