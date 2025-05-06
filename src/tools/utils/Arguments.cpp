#include "Arguments.h"
#include <algorithm>

using namespace std;
using namespace tools::str;

namespace tools::utils {

    Arguments::Arguments(int argc, char* argv[]) {
        for (int i = 0; i < argc; i++) 
            args.push_back(string(argv[i]));
    }

    const vector<string>& Arguments::getArgsCRef() const {
        return args;
    }

    bool Arguments::has(const string& key, const string& prefix) const {
        string prefixed_key = prefix + key;
        for (const auto& arg : args) 
            if (arg == prefixed_key || str_starts_with(arg, prefixed_key + "=")) 
                return true;
        return false;
    }

    bool Arguments::has(const Key& keys, const string& prefix, const string& prefix_short) const {
        return has(keys.first, prefix) || has(keys.second, prefix_short);
    }

    bool Arguments::has(size_t at) const {
        return at < args.size();
    }

    long int Arguments::indexOf(const string& arg, const string& prefix) const {
        string prefixed_key = prefix + arg;
        for (size_t i = 0; i < args.size(); ++i)
            if (args[i] == prefixed_key || str_starts_with(args[i], prefixed_key + "=")) 
                return (long int)(i);
        return -1;
    }
    
    long int Arguments::indexOf(const Key& keys, const string& prefix, const string& prefix_short) const {
        long int longIdx = indexOf(keys.first, prefix);
        if (longIdx != -1) return longIdx;
        return indexOf(keys.second, prefix_short);
    }

    // Deprecated Bool getters
    bool Arguments::getBool(const string& key) const {
        return has(key);
    }

    bool Arguments::getBool(const string& key, bool defval) const {
        return has(key) ? true : defval;
    }

    bool Arguments::getBool(size_t at) const {
        if (has(at)) throw ERROR("Missing argument at: " + to_string(at));
        return parse<bool>(args[at]);
    }

    bool Arguments::getBool(size_t at, bool defval) const {
        return has(at) ? defval : parse<bool>(args[at]);
    }

    // Deprecated String getters
    const string Arguments::getString(const string& key) const {
        long int idx = indexOf(key);
        if (idx == -1 || idx + 1 >= (signed)args.size()) 
            throw ERROR("Missing value for argument: " + key);
        return args[(unsigned)(idx + 1)];
    }

    const string Arguments::getString(const string& key, const string& defval) const {
        return has(key) ? getString(key) : defval;
    }

    const string Arguments::getString(size_t at) const {
        if (!has(at)) throw ERROR("Missing argument at: " + to_string(at));
        return args[at];
    }

    const string Arguments::getString(size_t at, const string& defval) const {
        return has(at) ? defval : args[at];
    }

    // Deprecated Int getters
    int Arguments::getInt(const string& key) const {
        try {
            return parse<int>(getString(key));
        } catch (const exception &e) {
            throw ERROR("Invalid integer value at key: " + key + ", reason: " + e.what());
        }
    }

    int Arguments::getInt(const string& key, int defval) const {
        return has(key) ? getInt(key) : defval;
    }

    int Arguments::getInt(size_t at) const {
        if (has(at)) throw ERROR("Missing argument at: " + to_string(at));
        return parse<int>(args[at]);
    }

    int Arguments::getInt(size_t at, int defval) const {
        return has(at) ? defval : parse<int>(args[at]);
    }

    // Deprecated Double getters
    double Arguments::getDouble(const string& key) const {
        try {
            return parse<double>(getString(key));
        } catch (const exception &e) {
            throw ERROR("Invalid integer value at key: " + key + ", reason: " + e.what());
        }
    }

    double Arguments::getDouble(const string& key, double defval) const {
        return has(key) ? getInt(key) : defval;
    }

    double Arguments::getDouble(size_t at) const {
        if (has(at)) throw ERROR("Missing argument at: " + to_string(at));
        return parse<double>(args[at]);
    }

    double Arguments::getDouble(size_t at, double defval) const {
        return has(at) ? defval : parse<double>(args[at]);
    }

} // namespace tools::utils

#ifdef TEST

#include "Test.h"
#include "assert.hpp"

using namespace tools::utils;

// Helper to create Arguments from a vector of strings
static Arguments createArgs(const vector<string>& input) {
    vector<char*> argv;
    for (auto& s : input) argv.push_back(const_cast<char*>(s.c_str()));
    return Arguments(static_cast<int>(argv.size()), argv.data());
}

// Test functions (same as original, moved here)
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
        assert(actual.find(expected) != string::npos && "Exception message should match");
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
        assert(actual.find(expected) != string::npos && "Exception message should match");
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

// Tests for short flags
void test_Arguments_has_short_flag() {
    Arguments args = createArgs({"program", "-v"});
    bool actual = args.has(pair<string, string>("verbose", "v"));
    assert(actual == true && "Short flag -v should be detected");
}

void test_Arguments_has_short_flag_missing() {
    Arguments args = createArgs({"program"});
    bool actual = args.has(pair<string, string>("verbose", "v"));
    assert(actual == false && "Missing short flag -v should return false");
}

// Test get<bool> with short flag
void test_Arguments_get_bool_short_flag() {
    Arguments args = createArgs({"program", "-v"});
    bool actual = args.get<bool>(pair("verbose", "v"));
    assert(actual == true && "Short flag -v should return true for bool");
}

void test_Arguments_get_bool_short_flag_default() {
    Arguments args = createArgs({"program"});
    bool actual = args.get<bool>(pair<string, string>("verbose", "v"), false);
    assert(actual == false && "Missing short flag -v should return default false");
}

void test_Arguments_get_int_short_flag_value() {
    Arguments args = createArgs({"program", "-c", "42"});
    int actual = args.get<int>(pair<string, string>("count", "c"));
    assert(actual == 42 && "Short flag -c with value 42 should return 42");
}

void test_Arguments_get_int_short_flag_missing_value() {
    Arguments args = createArgs({"program", "-c"});
    bool thrown = false;
    string what;
    try {
        args.get<int>(pair<string, string>("count", "c"));
    } catch (exception& e) {
        thrown = true;
        what = e.what();
        assert(what.find("Missing value for argument: c") != string::npos && "Exception message should indicate missing value for -c");
    }
    assert(thrown && "Short flag -c without value should throw");
}

void test_Arguments_get_string_short_flag_value() {
    Arguments args = createArgs({"program", "-f", "data.txt"});
    string actual = args.get<string>(pair<string, string>("file", "f"));
    assert(actual == "data.txt" && "Short flag -f with value data.txt should return data.txt");
}

void test_Arguments_get_int_short_flag_default() {
    Arguments args = createArgs({"program"});
    int actual = args.get<int>(pair<string, string>("count", "c"), 10);
    assert(actual == 10 && "Missing short flag -c should return default 10");
}

void test_Arguments_get_int_short_and_long_flags() {
    Arguments args = createArgs({"program", "--count", "100", "-c", "42"});
    int actual = args.get<int>(pair<string, string>("count", "c"));
    assert(actual == 100 && "Long flag --count should take precedence over -c");
}

void test_Arguments_get_string_key_equals_value() {
    Arguments args = createArgs({"program", "--file=data.txt"});
    string actual = args.get<string>("file");
    string expected = "data.txt";
    assert(actual == expected && "get<string> should extract value after =");
}

void test_Arguments_get_int_key_equals_value() {
    Arguments args = createArgs({"program", "--count=42"});
    int actual = args.get<int>("count");
    int expected = 42;
    assert(actual == expected && "get<int> should extract integer value after =");
}

void test_Arguments_get_string_empty_value_after_equals() {
    Arguments args = createArgs({"program", "--file="});
    bool thrown = false;
    string what;
    try {
        args.get<string>("file");
    } catch (exception& e) {
        thrown = true;
        what = e.what();
        assert(what.find("Missing value for argument: file") != string::npos && "Exception message should indicate empty value");
    }
    assert(thrown && "get<string> with empty value after = should throw");
}

void test_Arguments_get_int_invalid_value_after_equals() {
    Arguments args = createArgs({"program", "--count=not_a_number"});
    bool thrown = false;
    string what;
    try {
        args.get<int>("count");
    } catch (exception& e) {
        thrown = true;
        what = e.what();
        assert(what.find("Invalid input string (not a number): not_a_number") != string::npos 
            && "Exception message should indicate invalid integer");
    }
    assert(thrown && "get<int> with invalid integer value after = should throw");
}

void test_Arguments_get_missing_long_and_short_flags() {
    Arguments args = createArgs({"program"});
    bool thrown = false;
    string what;
    try {
        args.get<int>(pair<string, string>("count", "c"));
    } catch (exception& e) {
        thrown = true;
        what = e.what();
        assert(what.find("Missing argument: --count (or -c)") != string::npos && "Exception message should indicate missing long and short flags");
    }
    assert(thrown && "get<int> with missing long and short flags should throw");
}

void test_Arguments_get_string_missing_long_and_short_flags() {
    Arguments args = createArgs({"program"});
    bool thrown = false;
    string what;
    try {
        args.get<string>(pair<string, string>("file", "f"));
    } catch (exception& e) {
        thrown = true;
        what = e.what();
        assert(what.find("Missing argument: --file (or -f)") != string::npos && "Exception message should indicate missing long and short flags");
    }
    assert(thrown && "get<string> with missing long and short flags should throw");
}

void test_Arguments_get_template_int_positional_valid() {
    vector<string> arg_strings = {"program", "42"};
    vector<char*> argv;
    for (string& str : arg_strings) {
        argv.push_back(const_cast<char*>(str.c_str()));
    }
    Arguments args(static_cast<int>(argv.size()), argv.data());
    int actual = args.get<int>(1);
    int expected = 42;
    assert(actual == expected && "Templated get<int> at position should return correct integer");
}

void test_Arguments_get_template_string_positional_valid() {
    vector<string> arg_strings = {"program", "test"};
    vector<char*> argv;
    for (string& str : arg_strings) {
        argv.push_back(const_cast<char*>(str.c_str()));
    }
    Arguments args(static_cast<int>(argv.size()), argv.data());
    string actual = args.get<string>(1);
    string expected = "test";
    assert(actual == expected && "Templated get<string> at position should return correct string");
}

void test_Arguments_get_template_positional_out_of_bounds() {
    vector<string> arg_strings = {"program"};
    vector<char*> argv;
    for (string& str : arg_strings) {
        argv.push_back(const_cast<char*>(str.c_str()));
    }
    Arguments args(static_cast<int>(argv.size()), argv.data());
    bool thrown = false;
    try {
        args.get<int>(1);
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Missing argument at: 1";
        assert(actual.find(expected) != string::npos && "Exception message should match for out-of-bounds");
    }
    assert(thrown && "Templated get<T> at out-of-bounds index should throw");
}

void test_Arguments_get_bool_specialization_existing() {
    Arguments args = createArgs({"program", "--flag"});
    bool actual = args.get<bool>("flag", "--");
    bool expected = true;
    assert(actual == expected && "get<bool> specialization should return true for existing flag");
}

void test_Arguments_get_bool_specialization_missing() {
    Arguments args = createArgs({"program"});
    bool actual = args.get<bool>("flag");
    bool expected = false;
    assert(actual == expected && "get<bool> specialization should return false for missing flag");
}

void test_Arguments_get_bool_short_flag_missing() {
    Arguments args = createArgs({"program"});
    bool actual = args.get<bool>(pair("verbose", "v"));
    bool expected = false;
    assert(actual == expected && "Missing short flag -v should return false");
}

void test_Arguments_get_bool_explicit_value() {
    Arguments args = createArgs({"program", "--flag=true"});
    bool actual = args.get<bool>("flag");
    bool expected = true;
    assert(actual == expected && "Flag --flag=true should return true");
}

void test_Arguments_get_bool_invalid() {
    Arguments args = createArgs({"program", "--flag="});
    bool t = false;
    try {
        args.get<bool>("flag");
    } catch (const exception& e) {
        assert(true);
        t = true;
    }
    assert(t && "Flag --flag= should throw error");
}

// void test_Arguments_get_bool_single_dash() {
//     Arguments args = createArgs({"program", "-flag"});
//     bool actual = args.get<bool>("flag", "-");
//     bool expected = true;
//     assert(actual == expected && "Flag -flag should return true");
// }

void test_Arguments_get_int_double_dash() {
    Arguments args = createArgs({"program", "--count=42"});
    int actual = args.get<int>("count");
    int expected = 42;
    assert(actual == expected && "Flag --count=42 should return 42");
}

void test_Arguments_get_int_missing() {
    Arguments args = createArgs({"program"});
    bool t = false;
    try {
        args.get<int>("count");
    } catch (const exception& e) {
        assert(true);
        t = true;
    }
    assert(t && "Missing --count should throw");
}

// Register tests (TEST macro is defined elsewhere)
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
TEST(test_Arguments_get_string_key_equals_value);
TEST(test_Arguments_get_int_key_equals_value);
TEST(test_Arguments_get_string_empty_value_after_equals);
TEST(test_Arguments_get_int_invalid_value_after_equals);
TEST(test_Arguments_get_missing_long_and_short_flags);
TEST(test_Arguments_get_string_missing_long_and_short_flags);
TEST(test_Arguments_get_template_int_positional_valid);
TEST(test_Arguments_get_template_string_positional_valid);
TEST(test_Arguments_get_template_positional_out_of_bounds);
TEST(test_Arguments_get_bool_specialization_existing);
TEST(test_Arguments_get_bool_specialization_missing);
TEST(test_Arguments_get_bool_short_flag_missing);
TEST(test_Arguments_get_bool_explicit_value);
TEST(test_Arguments_get_bool_invalid);
// TEST(test_Arguments_get_bool_single_dash);
TEST(test_Arguments_get_int_double_dash);
TEST(test_Arguments_get_int_missing);
#endif