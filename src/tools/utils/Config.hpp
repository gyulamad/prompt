#pragma once

#include "Arguments.hpp"
#include "JSON.hpp"
#include "files.hpp"
#include "str/get_path.hpp"

using namespace std;
using namespace tools::str;

namespace tools::utils {

    // TODO: !!! DEPRECATED !!! - use Settings instead!
    class Config {
    public:

        Config(int argc, char* argv[], const string& cfile = ""): 
            args(argc, argv), 
            json(cfile.empty() ? "{}" : (file_exists(cfile) ? file_get_contents(cfile) : cfile))
        {}

        virtual ~Config() {}

        void load(const string& cfile) {
            json = JSON(file_exists(cfile) ? file_get_contents(cfile) : cfile);
        }
    
        string hash() const { return get_hash(json.dump() + implode(" ", args.getArgsCRef())); }
    
        bool has(const string& key) {
            return (args.has(key) || json.has(key));
        }

        // Default case: assume types are not containers
        template<typename T>
        struct is_container : std::false_type {};

        // Specialization for std::vector
        template<typename T>
        struct is_container<std::vector<T>> : std::true_type {};
        
        // Specialization for std::map
        template<typename K, typename V>
        struct is_container<std::map<K, V>> : std::true_type {};

        template<typename T>
        T get(const string& key) {
            // Only check args for non-container types
            if constexpr (!is_container<T>::value)
                if (args.has(key)) return args.get<T>(key);
            // For containers or if args doesn't have it, use JSON
            if (json.has(key)) return json.get<T>(key);
            throw ERROR("Missing config argument: " + key);
        }
    
        template<typename T>
        T get(const string& key, T defval) {
            return has(key) ? get<T>(key) : defval;
        }
    
        template<typename T>
        T get(int index) {
            return args.get<T>(index);
        }
    
        template<typename T>
        T get(int index, T defval) {
            return args.has(index) ? args.get<T>(index) : defval;
        }
    
    private:
        Arguments args;
        JSON json;
    };

}

#ifdef TEST

#include "Test.hpp"

using namespace tools::utils;

// Existing tests (unchanged for brevity, add them back as needed)
void test_Config_constructor_default() {
    const char* argv[] = {"program", "--flag"};
    int argc = 2;
    Config config(argc, (char**)argv);
    string actual = config.get<string>(0);
    assert(actual == "program" && "Constructor should initialize args with argv[0]");
}

void test_Config_constructor_with_config_file() {
    string cfile = "test.config.json"; // Assume this file exists or is mocked externally
    assert(file_put_contents(cfile, "{\"key\": \"value\"}"));

    const char* argv[] = {"program"};
    int argc = 1;
    Config config(argc, (char**)argv, cfile);
    string actual = config.get<string>("key");

    unlink(cfile);

    assert(actual == "value" && "Constructor should load JSON from provided config file");
}

void test_Config_hash() {
    const char* argv1[] = {"program", "--flag1"};
    int argc1 = 2;
    Config config1(argc1, (char**)argv1);
    string actual1 = config1.hash();

    const char* argv2[] = {"program", "--flag2"};
    int argc2 = 2;
    Config config2(argc2, (char**)argv2);
    string actual2 = config2.hash();

    assert(actual1 != actual2 && "Hash should combine JSON dump and args");
}

void test_Config_has_key_in_args() {
    const char* argv[] = {"program", "--key=value"};
    int argc = 2;
    Config config(argc, (char**)argv);
    bool actual = config.has("key");
    assert(actual == true && "has() should return true if key exists in args");
}

void test_Config_has_key_in_json() {
    string cfile = "test.config.json"; // Assume this file exists or is mocked externally
    assert(file_put_contents(cfile, "{\"key\": \"value\"}"));

    const char* argv[] = {"program"};
    int argc = 1;
    Config config(argc, (char**)argv, cfile);
    bool actual = config.has("key");

    unlink(cfile);

    assert(actual == true && "has() should return true if key exists in JSON");
}

void test_Config_has_key_missing() {
    const char* argv[] = {"program"};
    int argc = 1;
    Config config(argc, (char**)argv);
    bool actual = config.has("nonexistent");
    assert(actual == false && "has() should return false if key is missing");
}

void test_Config_get_string_from_args() {
    const char* argv[] = {"program", "--key=value"};
    int argc = 2;
    Config config(argc, (char**)argv);
    string actual = config.get<string>("key");
    assert(actual == "value" && "get<string> should retrieve value from args");
}

void test_Config_get_string_from_json() {
    const char* argv[] = {"program"};
    int argc = 1;
    string cfile = "test.config.json"; // Assume this file exists or is mocked externally
    file_put_contents(cfile, "{\"key\": \"value\"}");
    Config config(argc, (char**)argv, cfile);
    string actual = config.get<string>("key");
    unlink(cfile);
    assert(actual == "value" && "get<string> should retrieve value from JSON");
}

void test_Config_get_string_missing_throws() {
    const char* argv[] = {"program"};
    int argc = 1;
    Config config(argc, (char**)argv);
    bool thrown = false;
    string what;
    try {
        config.get<string>("missing");
    } catch (exception& e) {
        thrown = true;
        what = e.what();
        assert(str_contains(what, "Missing config argument: missing") && "Exception message should contain key name");
    }
    assert(thrown && "get<string> should throw when key is missing");
}

void test_Config_get_string_with_default() {
    const char* argv[] = {"program"};
    int argc = 1;
    Config config(argc, (char**)argv);
    string actual = config.get<string>("missing", "default");
    assert(actual == "default" && "get<string> with default should return default value when key is missing");
}

void test_Config_get_int_from_index() {
    const char* argv[] = {"program", "42"};
    int argc = 2;
    Config config(argc, (char**)argv);
    int actual = config.get<int>(1);
    assert(actual == 42 && "get<int> should retrieve integer from args by index");
}

void test_Config_get_int_with_default_index() {
    const char* argv[] = {"program"};
    int argc = 1;
    Config config(argc, (char**)argv);
    int actual = config.get<int>(1, 99);
    assert(actual == 99 && "get<int> with default should return default value when index is out of range");
}

// Register tests
TEST(test_Config_constructor_default);
TEST(test_Config_constructor_with_config_file);
TEST(test_Config_hash);
TEST(test_Config_has_key_in_args);
TEST(test_Config_has_key_in_json);
TEST(test_Config_has_key_missing);
TEST(test_Config_get_string_from_args);
TEST(test_Config_get_string_from_json);
TEST(test_Config_get_string_missing_throws);
TEST(test_Config_get_string_with_default);
TEST(test_Config_get_int_from_index);
TEST(test_Config_get_int_with_default_index);

#endif