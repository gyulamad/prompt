#pragma once

#include "JSON.h"

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
}


#ifdef TEST

// #include "Test.hpp"

using namespace tools::utils;


// Test JSONExts::extends
void test_JSONExts_extends_adds_json() {
    JSONExts exts;
    JSON json;
    json.set("key", "value");
    exts.extends(json);
    bool actual = exts.has("key");
    assert(actual == true && "Extends should add JSON with key");
}

// Test JSONExts::get with existing key
void test_JSONExts_get_existing_key() {
    JSONExts exts;
    JSON json;
    json.set("key", 42);
    exts.extends(json);
    int actual = exts.get<int>("key");
    assert(actual == 42 && "Get should return value for existing key");
}

// Test JSONExts::get with missing key throwing exception
void test_JSONExts_get_missing_key() {
    JSONExts exts;
    bool thrown = false;
    string what;
    try {
        exts.get<int>("missing");
    } catch (exception& e) {
        thrown = true;
        what = e.what();
        assert(str_contains(what, "No value at 'missing'") && "Exception message should match");
    }
    assert(thrown && "Get should throw when key is missing");
}

// Test JSONExts::has with existing key
void test_JSONExts_has_existing_key() {
    JSONExts exts;
    JSON json;
    json.set("key", "value");
    exts.extends(json);
    bool actual = exts.has("key");
    assert(actual == true && "Has should return true for existing key");
}

// Test JSONExts::has with missing key
void test_JSONExts_has_missing_key() {
    JSONExts exts;
    bool actual = exts.has("missing");
    assert(actual == false && "Has should return false for missing key");
}

// Test JSONExts::dump with single JSON
void test_JSONExts_dump_single_json() {
    JSONExts exts;
    JSON json;
    json.set("key", "value");
    exts.extends(json);
    string actual = exts.dump();
    string expected = "[{\"key\":\"value\"}]";
    assert(actual == expected && "Dump should return correct JSON array string");
}

// Test JSONExts::dump with multiple JSONs
void test_JSONExts_dump_multiple_jsons() {
    JSONExts exts;
    JSON json1, json2;
    json1.set("key1", 1);
    json2.set("key2", 2);
    exts.extends(json1);
    exts.extends(json2);
    string actual = exts.dump();
    string expected = "[{\"key2\":2},{\"key1\":1}]";
    assert(actual == expected && "Dump should return correct JSON array string for multiple JSONs");
}

TEST(test_JSONExts_extends_adds_json);
TEST(test_JSONExts_get_existing_key);
TEST(test_JSONExts_get_missing_key);
TEST(test_JSONExts_has_existing_key);
TEST(test_JSONExts_has_missing_key);
TEST(test_JSONExts_dump_single_json);
TEST(test_JSONExts_dump_multiple_jsons);

#endif