#pragma once

#include <string>
#include <vector>
#include <map>
#include <stack>
#include <regex>

// Include the nlohmann JSON library: 
// git clone https://github.com/nlohmann/json
#include "../../../libs/nlohmann/json/single_include/nlohmann/json.hpp"  

#include "ERROR.hpp"
#include "strings3.hpp"

using namespace std;
using namespace nlohmann;

namespace tools::utils {

    // Function to remove single-line and multi-line comments
    string json_remove_comments(const string& json) {
        string result;
        bool inString = false; // Track if we're inside a string
        bool escapeNext = false; // Track if the next character is escaped
        bool inSingleLineComment = false; // Track if we're in a single-line comment
        bool inMultiLineComment = false; // Track if we're in a multi-line comment

        for (size_t i = 0; i < json.size(); ++i) {
            char ch = json[i];

            // Handle escape sequences inside strings
            if (inString && ch == '\\' && !escapeNext) {
                escapeNext = true;
                result += ch;
                continue;
            }

            // Handle string literals
            if (ch == '"' && !escapeNext && !inSingleLineComment && !inMultiLineComment) {
                inString = !inString; // Toggle string state
            }

            // Handle single-line comments
            if (!inString && !inMultiLineComment && ch == '/' && i + 1 < json.size() && json[i + 1] == '/') {
                inSingleLineComment = true;
                i++; // Skip the second '/'
                continue;
            }

            // Handle multi-line comments
            if (!inString && !inSingleLineComment && ch == '/' && i + 1 < json.size() && json[i + 1] == '*') {
                inMultiLineComment = true;
                i++; // Skip the '*'
                continue;
            }

            // End of single-line comment
            if (inSingleLineComment && ch == '\n') {
                inSingleLineComment = false;
            }

            // End of multi-line comment
            if (inMultiLineComment && ch == '*' && i + 1 < json.size() && json[i + 1] == '/') {
                inMultiLineComment = false;
                i++; // Skip the '/'
                continue;
            }

            // Append characters that are not part of comments
            if (!inSingleLineComment && !inMultiLineComment) {
                result += ch;
            }

            // Reset escape flag
            escapeNext = false;
        }

        return result;
    }

    // Function to fix JSON by removing trailing commas
    string json_fix(string json) {
        json = json_remove_comments(json);
        string result;
        bool inString = false; // Track if we're inside a string
        bool escapeNext = false; // Track if the next character is escaped
        stack<char> scopeStack; // Track the current scope (object or array)

        for (size_t i = 0; i < json.size(); ++i) {
            char ch = json[i];

            // Handle string literals
            if (ch == '"' && !escapeNext) {
                inString = !inString; // Toggle string state
            }

            // Handle escape sequences
            if (ch == '\\' && !escapeNext) {
                escapeNext = true;
            } else {
                escapeNext = false;
            }

            // Handle scopes (objects and arrays)
            if (!inString) {
                if (ch == '{' || ch == '[') {
                    scopeStack.push(ch);
                } else if (ch == '}' || ch == ']') {
                    if (!scopeStack.empty()) {
                        scopeStack.pop();
                    }
                }
            }

            // Remove trailing commas outside strings
            if (!inString && ch == ',') {
                // Look ahead to see if the next non-whitespace character is a closing brace or bracket
                size_t j = i + 1;
                while (j < json.size() && (json[j] == ' ' || json[j] == '\t' || json[j] == '\n' || json[j] == '\r')) {
                    ++j;
                }
                if (j < json.size() && (json[j] == '}' || json[j] == ']')) {
                    // Skip this comma (it's a trailing comma)
                    continue;
                }
            }

            // Append the character to the result
            result += ch;
        }

        return result;
    }

    enum json_type {
        JSON_TYPE_UNDEFINED,
        JSON_TYPE_NULL,
        JSON_TYPE_STRING,
        JSON_TYPE_INTEGER,
        JSON_TYPE_REAL,
        JSON_TYPE_BOOLEAN,
        JSON_TYPE_ARRAY,
        JSON_TYPE_OBJECT,
    };

    string json_last_error = "";

    // Function to convert jq-style or JavaScript-style selector to json_pointer
    json::json_pointer _json_selector(string jselector) {
        if (jselector.empty()) return json::json_pointer("/");
        if (jselector[0] != '.') jselector = "." + jselector;

        // Split the selector by dots
        vector<string> splits = explode(".", jselector);

        for (size_t i = 1; i < splits.size(); i++) {
            if (splits[i].empty()) 
                throw ERROR("Invalid json selector: " + jselector);

            // Validate array indexing syntax
            regex valid_brackets("\\[\\s*(\\d+)\\s*\\]$"); // Matches [N] for numeric array indexing
            regex invalid_brackets("\\[[^\\]]+\\]$"); // Matches [not-numeric]
            smatch match;

            // Check if the index matches valid_brackets
            if (regex_search(splits[i], match, valid_brackets)) {
                // Replace valid array indexing [N] with /N
                splits[i] = regex_replace(splits[i], valid_brackets, "/$1");
                continue; // Skip further checks for this part
            }

            // Check if the index matches invalid_brackets
            if (regex_search(splits[i], match, invalid_brackets)) {
                throw ERROR("Invalid json selector: " + jselector); // [not-numeric] is invalid
            }
        }

        // Validate mismatched brackets
        int open_brackets = 0, close_brackets = 0;
        for (char ch : jselector) {
            if (ch == '[') open_brackets++;
            if (ch == ']') close_brackets++;
        }
        if (open_brackets != close_brackets) {
            throw ERROR("Invalid json selector: " + jselector); // Mismatched brackets
        }

        return json::json_pointer(implode("/", splits));
    }

    bool is_valid_json(string jstring) {
        json_last_error = "";
        try {
            auto json = json::parse(jstring);
            (void)json;
            return true;
        } catch (const json::parse_error& e) {
            json_last_error = e.what();
            return false;
        }
    }

    string get_json_error(string jstring) {
        if (is_valid_json(jstring)) return "";
        return json_last_error;
    }

    json_type get_json_value_type(string jstring, string jselector) {
        try {
            auto json = json::parse(jstring);
            auto ptr = _json_selector(jselector);

            if (!json.contains(ptr))
                return JSON_TYPE_UNDEFINED;

            const auto& value = json.at(ptr);
            if (value.is_null()) return JSON_TYPE_NULL;
            if (value.is_string()) return JSON_TYPE_STRING;
            if (value.is_boolean()) return JSON_TYPE_BOOLEAN;
            if (value.is_number_integer()) return JSON_TYPE_INTEGER;
            if (value.is_number_float()) return JSON_TYPE_REAL;
            if (value.is_array()) return JSON_TYPE_ARRAY;
            if (value.is_object()) return JSON_TYPE_OBJECT;

            return JSON_TYPE_UNDEFINED;
        } catch (...) {
            return JSON_TYPE_UNDEFINED;
        }
    }

    string json_type_to_string(json_type type) {
        switch (type) {
            case JSON_TYPE_UNDEFINED: return "undefined";
            case JSON_TYPE_NULL: return "null";
            case JSON_TYPE_STRING: return "string";
            case JSON_TYPE_INTEGER: return "integer";
            case JSON_TYPE_REAL: return "real";
            case JSON_TYPE_BOOLEAN: return "boolean";
            case JSON_TYPE_ARRAY: return "array";
            case JSON_TYPE_OBJECT: return "object";
            default:
                throw ERROR("Invalid JSON type: " + to_string(type));
        }
    }

    string json_get_string(string jstring, string jselector) {
        if (get_json_value_type(jstring, jselector) != JSON_TYPE_STRING)
            throw ERROR("Expected string type at " + jselector);
        try {
            auto json = json::parse(jstring);
            return json.at(_json_selector(jselector)).get<string>();
        } catch (const json::exception& e) {
            throw ERROR("Error retrieving string: " + string(e.what()));
        }
    }

    int json_get_int(string jstring, string jselector) {
        if (get_json_value_type(jstring, jselector) != JSON_TYPE_INTEGER)
            throw ERROR("Expected integer type at " + jselector);
        try {
            auto json = json::parse(jstring);
            return json.at(_json_selector(jselector)).get<int>();
        } catch (const json::exception& e) {
            throw ERROR("Error retrieving integer: " + string(e.what()));
        }
    }

    double json_get_double(string jstring, string jselector) {
        if (get_json_value_type(jstring, jselector) != JSON_TYPE_REAL)
            throw ERROR("Expected real type at " + jselector);
        try {
            auto json = json::parse(jstring);
            return json.at(_json_selector(jselector)).get<double>();
        } catch (const json::exception& e) {
            throw ERROR("Error retrieving double: " + string(e.what()));
        }
    }

    bool json_get_bool(string jstring, string jselector) {
        if (get_json_value_type(jstring, jselector) != JSON_TYPE_BOOLEAN)
            throw ERROR("Expected boolean type at " + jselector);
        try {
            auto json = json::parse(jstring);
            return json.at(_json_selector(jselector)).get<bool>();
        } catch (const json::exception& e) {
            throw ERROR("Error retrieving boolean: " + string(e.what()));
        }
    }

    string json_get_array(string jstring, string jselector) {
        if (get_json_value_type(jstring, jselector) != JSON_TYPE_ARRAY)
            throw ERROR("Expected array type at " + jselector);
        try {
            auto json = json::parse(jstring);
            return json.at(_json_selector(jselector)).dump();
        } catch (const json::exception& e) {
            throw ERROR("Error retrieving array: " + string(e.what()));
        }
    }

    string json_get_object(string jstring, string jselector) {
        if (get_json_value_type(jstring, jselector) != JSON_TYPE_OBJECT)
            throw ERROR("Expected object type at " + jselector);
        try {
            auto json = json::parse(jstring);
            return json.at(_json_selector(jselector)).dump();
        } catch (const json::exception& e) {
            throw ERROR("Error retrieving object: " + string(e.what()));
        }
    }

    class JSON {
    protected:
        string* _error = nullptr;
        // string jstring;
        json j;

    public:
        JSON(const json& j): j(j) {}
        JSON(const char* j): JSON(string(j)) {}

        // Constructor to initialize the JSON string (can be empty)
        JSON(string jstring = "{}") {
            try {
                j = json::parse(jstring.empty() ? "{}" : json_fix(jstring));
            } catch (const exception &e) {
                _error = new string(e.what());
            }
        }

        // Destructor (no special cleanup needed here)
        ~JSON() {
            if (_error) delete _error;
        }

        json get_json() const {
            return j;
        }

        bool isValid(string* error = nullptr) {
            if (error) error = _error;
            return !_error;
        }     

        string dump(const int indent = -1, const char indent_char = ' ') const {
            try {
                // json j;
                // try {
                //     j = json::parse(jstring);  // Parse the JSON string
                // } catch (const exception &e) {
                //     cerr << "JSON parse error: " << e.what() << endl;
                //     DEBUG(jstring);
                //     return "";
                // }
                // string dump = j.dump(indent, indent_char);
                string dump = j.dump(indent, indent_char);
                return dump;
            } catch (const exception& e) {
                throw ERROR("JSON dump error: " + string(e.what()));
            }
        }

        // Method to check if a selector is defined in the JSON (exists)
        bool isDefined(string jselector) const {
            try {
                // json j = json::parse(jstring);  // Parse the JSON string
                json::json_pointer ptr = _json_selector(jselector);  // Convert selector to pointer
                // return j.contains(ptr);  // Check if the pointer exists in the JSON
                return j.contains(ptr);
            } catch (...) {
                return false;  // If parsing fails or any error occurs, consider undefined
            }
        }
        bool has(string jselector) const { return isDefined(jselector); }

        // Method to check if a selector is null in the JSON
        bool isNull(string jselector) {
            try {
                // json j = json::parse(jstring);  // Parse the JSON string
                json::json_pointer ptr = _json_selector(jselector);  // Convert selector to pointer
                // return j.at(ptr).is_null();  // Check if the value at the pointer is null
                return j.at(ptr).is_null(); 
            } catch (...) {
                return false;  // If an error occurs, assume null
            }
        }
        
        bool isArray(string jselector) {
            try {
                json::json_pointer ptr = _json_selector(jselector);
                return j.at(ptr).is_array();
            } catch (...) {
                return false;
            }
        }
        
        bool isObject(string jselector) {
            try {
                json::json_pointer ptr = _json_selector(jselector);
                return j.at(ptr).is_object();
            } catch (...) {
                return false;
            }
        }

        template<typename T>
        T get(string jselector) const {
            try {
                json::json_pointer ptr = _json_selector(jselector);  // Convert selector to pointer
                if constexpr (is_same_v<T, JSON>) return JSON(j.at(ptr));
                return j.at(ptr).get<T>();
            } catch (const exception& e) {
                //DEBUG(j.dump());
                throw ERROR("JSON Error at: " + jselector + ", reason: " + string(e.what()));
            }
        }

        // void set(string value) {
        //     jstring = value;
        // }

        template<typename T>
        void set(string jselector, T value) {
            try {
                // json j = json::parse(jstring);
                json::json_pointer ptr = _json_selector(jselector);
                // j[ptr] = value;
                // jstring = j.dump();
                j[ptr] = value;
            } catch (const json::exception& e) {
                //DEBUG(j.dump());
                throw ERROR("JSON Error at: " + jselector + ", reason: " + string(e.what()));
            }
        }

    };

}

namespace nlohmann {
    template<>
    struct adl_serializer<tools::utils::JSON> {
        static tools::utils::JSON from_json(const json& j) {
            return tools::utils::JSON(j);
        }
        
        static void to_json(json& j, const tools::utils::JSON& jsonObj) {
            j = jsonObj.get_json();
        }
    };
}

#ifdef TEST

#include "Test.hpp"

using namespace tools::utils;


void test_json_remove_comments_no_comments() {
    string input = R"({"key": "value"})";
    string expected = R"({"key": "value"})";
    string actual = json_remove_comments(input);
    assert(str_diffs_show(actual, expected).empty() && "No comments should remain unchanged");
}

void test_json_remove_comments_single_line_comment() {
    string input = R"({"key": "value"} // This is a comment)";
    string expected = R"({"key": "value"} )";
    string actual = json_remove_comments(input);
    assert(str_diffs_show(actual, expected).empty() && "Single-line comment should be removed");
}

void test_json_remove_comments_multi_line_comment() {
    string input = R"({"key": "value"} /* This is a multi-line comment */)";
    string expected = R"({"key": "value"} )";
    string actual = json_remove_comments(input);
    assert(str_diffs_show(actual, expected).empty() && "Multi-line comment should be removed");
}

void test_json_remove_comments_mixed_comments() {
    string input = R"({
        "key": "value", // Single-line comment
        /* Multi-line comment */
        "anotherKey": "anotherValue"
    })";
    string expected = R"({
        "key": "value", 
        
        "anotherKey": "anotherValue"
    })";
    string actual = json_remove_comments(input);
    assert(str_diffs_show(actual, expected).empty() && "Mixed comments should be removed");
}

void test_json_remove_comments_escaped_characters() {
    string input = R"({"key": "value with escaped \/ and \\"})";
    string expected = R"({"key": "value with escaped \/ and \\"})";
    string actual = json_remove_comments(input);
    assert(str_diffs_show(actual, expected).empty() && "Escaped characters should remain intact");
}

void test_json_remove_comments_strings_with_slashes() {
    string input = R"({"key": "http://example.com", "comment": "/* not a comment */"})";
    string expected = R"({"key": "http://example.com", "comment": "/* not a comment */"})";
    string actual = json_remove_comments(input);
    assert(str_diffs_show(actual, expected).empty() && "Slashes inside strings should not trigger comment removal");
}

void test_json_remove_comments_nested_comments() {
    string input = R"({
        /* Outer comment /* Nested comment */ */
        "key": "value"
    })";
    string expected = R"({
         */
        "key": "value"
    })";
    string actual = json_remove_comments(input);
    assert(str_diffs_show(actual, expected).empty() && "Nested comments are not allowed");
}

void test_json_remove_comments_empty_input() {
    string input = "";
    string expected = "";
    string actual = json_remove_comments(input);
    assert(str_diffs_show(actual, expected).empty() && "Empty input should remain empty");
}

void test_json_remove_comments_only_comments() {
    string input = R"(// Only a comment)";
    string expected = "";
    string actual = json_remove_comments(input);
    assert(str_diffs_show(actual, expected).empty() && "Input with only comments should be empty");
}


void test_json_fix_no_trailing_commas() {
    string input = R"({"key": "value", "array": [1, 2, 3]})";
    string expected = R"({"key": "value", "array": [1, 2, 3]})";
    string actual = json_fix(input);
    assert(str_diffs_show(actual, expected).empty() && "JSON without trailing commas should remain unchanged");
}

void test_json_fix_with_trailing_commas() {
    string input = R"({"key": "value", "array": [1, 2, 3,],})";
    string expected = R"({"key": "value", "array": [1, 2, 3]})";
    string actual = json_fix(input);
    assert(str_diffs_show(actual, expected).empty() && "Trailing commas should be removed");
}

void test_json_fix_with_comments() {
    string input = R"({
        "key": "value", // This is a comment
        "array": [1, 2, 3,], /* Another comment */
    })";
    string expected = R"({
        "key": "value", 
        "array": [1, 2, 3] 
    })";
    string actual = json_fix(input);
    assert(str_diffs_show(actual, expected).empty() && "Comments and trailing commas should be removed");
}

void test_json_fix_empty_input() {
    string input = "";
    string expected = "";
    string actual = json_fix(input);
    assert(str_diffs_show(actual, expected).empty() && "Empty input should remain empty");
}

void test_json_fix_nested_objects_and_arrays() {
    string input = R"({
        "object": {
            "key": "value",
            "array": [1, 2, 3,],
        },
        "array": [4, 5, 6,],
    })";
    string expected = R"({
        "object": {
            "key": "value",
            "array": [1, 2, 3]
        },
        "array": [4, 5, 6]
    })";
    string actual = json_fix(input);
    assert(str_diffs_show(actual, expected).empty() && "Nested objects and arrays should handle trailing commas");
}

void test_json_fix_malformed_json() {
    string input = R"({"key": "value", "array": [1, 2, 3,])";
    string expected = R"({"key": "value", "array": [1, 2, 3])";
    string actual = json_fix(input);
    assert(str_diffs_show(actual, expected).empty() && "Malformed JSON should still remove trailing commas");
}


void test_json_selector_empty() {
    json::json_pointer actual = _json_selector("");
    json::json_pointer expected("/");
    assert(actual == expected && "Empty selector should return '/'");
}

void test_json_selector_simple_key() {
    json::json_pointer actual = _json_selector(".key");
    json::json_pointer expected("/key");
    assert(actual == expected && "Simple key selector should work");
}

void test_json_selector_nested_keys() {
    json::json_pointer actual = _json_selector(".key1.key2.key3");
    json::json_pointer expected("/key1/key2/key3");
    assert(actual == expected && "Nested keys selector should work");
}

void test_json_selector_array_indexing() {
    json::json_pointer actual = _json_selector(".array[0]");
    json::json_pointer expected("/array/0");
    assert(actual == expected && "Array indexing should be converted to '/N'");
}

void test_json_selector_mixed_keys_and_indices() {
    json::json_pointer actual = _json_selector(".key1.key2[3].key3[5]");
    json::json_pointer expected("/key1/key2/3/key3/5");
    assert(actual == expected && "Mixed keys and indices should work");
}

void test_json_selector_invalid_empty_part() {
    bool thrown = false;
    try {
        _json_selector(".key..key2");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Invalid json selector: .key..key2";
        assert(str_contains(actual, expected) && "Exception message should match");
    }
    assert(thrown && "Empty part in selector should throw an exception");
}

void test_json_selector_missing_dot_prefix() {
    json::json_pointer actual = _json_selector("key");
    json::json_pointer expected("/key");
    assert(actual == expected && "Missing dot prefix should be handled");
}

void test_json_selector_invalid_bracket_syntax() {
    bool thrown = false;
    try {
        _json_selector(".array[invalid]");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Invalid json selector: .array[invalid]"; // [not-numeric] is invalid
        assert(str_contains(actual, expected) && "Exception message should match");
    }
    assert(thrown && "Invalid bracket syntax should throw an exception");
}

void test_json_selector_invalid_bracket_syntax2() {
    bool thrown = false;
    try {
        _json_selector(".array]invalid]");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Invalid json selector: .array]invalid]"; // number of closing and opening braces mismacth
        assert(str_contains(actual, expected) && "Exception message should match");
    }
    assert(thrown && "Invalid bracket syntax should throw an exception");
}

void test_is_valid_json_empty_string() {
    bool actual = is_valid_json("");
    assert(!actual && "Empty string should be invalid JSON");
    assert(str_contains(json_last_error, "parse error") && "Error message should match");
}

void test_is_valid_json_valid_object() {
    bool actual = is_valid_json("{\"key\": \"value\"}");
    assert(actual && "Valid JSON object should pass");
    assert(json_last_error.empty() && "No error message should be set");
}

void test_is_valid_json_valid_array() {
    bool actual = is_valid_json("[1, 2, 3]");
    assert(actual && "Valid JSON array should pass");
    assert(json_last_error.empty() && "No error message should be set");
}

void test_is_valid_json_invalid_syntax() {
    bool actual = is_valid_json("{");
    assert(!actual && "Incomplete JSON should fail");
    assert(str_contains(json_last_error, "parse error") && "Error message should match");
}

void test_is_valid_json_mismatched_brackets() {
    bool actual = is_valid_json("[}");
    assert(!actual && "Mismatched brackets should fail");
    assert(str_contains(json_last_error, "parse error") && "Error message should match");
}

void test_is_valid_json_empty_object() {
    bool actual = is_valid_json("{}");
    assert(actual && "Empty JSON object should pass");
    assert(json_last_error.empty() && "No error message should be set");
}

void test_is_valid_json_empty_array() {
    bool actual = is_valid_json("[]");
    assert(actual && "Empty JSON array should pass");
    assert(json_last_error.empty() && "No error message should be set");
}

void test_is_valid_json_malformed_json() {
    bool actual = is_valid_json("not a json");
    assert(!actual && "Malformed JSON should fail");
    assert(str_contains(json_last_error, "parse error") && "Error message should match");
}


void test_get_json_error_valid_object() {
    string actual = get_json_error("{\"key\": \"value\"}");
    assert(actual.empty() && "Valid JSON object should return no error");
}

void test_get_json_error_valid_array() {
    string actual = get_json_error("[1, 2, 3]");
    assert(actual.empty() && "Valid JSON array should return no error");
}

void test_get_json_error_empty_string() {
    string actual = get_json_error("");
    assert(str_contains(actual, "parse error") && "Empty string should return an error message");
}

void test_get_json_error_invalid_syntax() {
    string actual = get_json_error("{");
    assert(str_contains(actual, "parse error") && "Incomplete JSON should return an error message");
}

void test_get_json_error_mismatched_brackets() {
    string actual = get_json_error("[}");
    assert(str_contains(actual, "parse error") && "Mismatched brackets should return an error message");
}

void test_get_json_error_empty_object() {
    string actual = get_json_error("{}");
    assert(actual.empty() && "Empty JSON object should return no error");
}

void test_get_json_error_empty_array() {
    string actual = get_json_error("[]");
    assert(actual.empty() && "Empty JSON array should return no error");
}

void test_get_json_error_malformed_json() {
    string actual = get_json_error("not a json");
    assert(str_contains(actual, "parse error") && "Malformed JSON should return an error message");
}

void test_get_json_value_type_null() {
    json_type actual = get_json_value_type("{\"key\": null}", ".key");
    assert(actual == JSON_TYPE_NULL && "Null value should return JSON_TYPE_NULL");
}

void test_get_json_value_type_string() {
    json_type actual = get_json_value_type("{\"key\": \"value\"}", ".key");
    assert(actual == JSON_TYPE_STRING && "String value should return JSON_TYPE_STRING");
}

void test_get_json_value_type_boolean() {
    json_type actual = get_json_value_type("{\"key\": true}", ".key");
    assert(actual == JSON_TYPE_BOOLEAN && "Boolean value should return JSON_TYPE_BOOLEAN");
}

void test_get_json_value_type_integer() {
    json_type actual = get_json_value_type("{\"key\": 42}", ".key");
    assert(actual == JSON_TYPE_INTEGER && "Integer value should return JSON_TYPE_INTEGER");
}

void test_get_json_value_type_real() {
    json_type actual = get_json_value_type("{\"key\": 3.14}", ".key");
    assert(actual == JSON_TYPE_REAL && "Real value should return JSON_TYPE_REAL");
}

void test_get_json_value_type_array() {
    json_type actual = get_json_value_type("{\"key\": []}", ".key");
    assert(actual == JSON_TYPE_ARRAY && "Array value should return JSON_TYPE_ARRAY");
}

void test_get_json_value_type_object() {
    json_type actual = get_json_value_type("{\"key\": {}}", ".key");
    assert(actual == JSON_TYPE_OBJECT && "Object value should return JSON_TYPE_OBJECT");
}

void test_get_json_value_type_undefined_key() {
    json_type actual = get_json_value_type("{\"key\": \"value\"}", ".missing");
    assert(actual == JSON_TYPE_UNDEFINED && "Undefined key should return JSON_TYPE_UNDEFINED");
}

void test_get_json_value_type_invalid_json() {
    json_type actual = get_json_value_type("invalid json", ".key");
    assert(actual == JSON_TYPE_UNDEFINED && "Invalid JSON should return JSON_TYPE_UNDEFINED");
}

void test_get_json_value_type_empty_json() {
    json_type actual = get_json_value_type("", ".key");
    assert(actual == JSON_TYPE_UNDEFINED && "Empty JSON should return JSON_TYPE_UNDEFINED");
}

void test_json_type_to_string_undefined() {
    string actual = json_type_to_string(JSON_TYPE_UNDEFINED);
    assert(actual == "undefined" && "JSON_TYPE_UNDEFINED should return 'undefined'");
}

void test_json_type_to_string_null() {
    string actual = json_type_to_string(JSON_TYPE_NULL);
    assert(actual == "null" && "JSON_TYPE_NULL should return 'null'");
}

void test_json_type_to_string_string() {
    string actual = json_type_to_string(JSON_TYPE_STRING);
    assert(actual == "string" && "JSON_TYPE_STRING should return 'string'");
}

void test_json_type_to_string_integer() {
    string actual = json_type_to_string(JSON_TYPE_INTEGER);
    assert(actual == "integer" && "JSON_TYPE_INTEGER should return 'integer'");
}

void test_json_type_to_string_real() {
    string actual = json_type_to_string(JSON_TYPE_REAL);
    assert(actual == "real" && "JSON_TYPE_REAL should return 'real'");
}

void test_json_type_to_string_boolean() {
    string actual = json_type_to_string(JSON_TYPE_BOOLEAN);
    assert(actual == "boolean" && "JSON_TYPE_BOOLEAN should return 'boolean'");
}

void test_json_type_to_string_array() {
    string actual = json_type_to_string(JSON_TYPE_ARRAY);
    assert(actual == "array" && "JSON_TYPE_ARRAY should return 'array'");
}

void test_json_type_to_string_object() {
    string actual = json_type_to_string(JSON_TYPE_OBJECT);
    assert(actual == "object" && "JSON_TYPE_OBJECT should return 'object'");
}

void test_json_type_to_string_invalid_type() {
    bool thrown = false;
    try {
        json_type_to_string(static_cast<json_type>(99)); // Invalid type
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Invalid JSON type: 99";
        assert(str_contains(actual, expected) && "Exception message should match");
    }
    assert(thrown && "Invalid JSON type should throw an exception");
}

void test_json_get_string_valid() {
    string actual = json_get_string("{\"key\": \"value\"}", ".key");
    assert(actual == "value" && "Valid string should be retrieved");
}

void test_json_get_string_invalid_type() {
    bool thrown = false;
    try {
        json_get_string("{\"key\": 42}", ".key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected string type at .key";
        assert(str_contains(actual, expected) && "Exception message should match");
    }
    assert(thrown && "Non-string type should throw an exception");
}

void test_json_get_string_undefined_key() {
    bool thrown = false;
    try {
        json_get_string("{\"key\": \"value\"}", ".missing");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected string type at .missing";
        assert(str_contains(actual, expected) && "Exception message should contain expected substring");
    }
    assert(thrown && "Undefined key should throw an exception");
}

void test_json_get_string_invalid_json() {
    bool thrown = false;
    try {
        json_get_string("invalid json", ".key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected string type at .key";
        assert(str_contains(actual, expected) && "Exception message should contain expected substring");
    }
    assert(thrown && "Invalid JSON should throw an exception");
}

void test_json_get_string_empty_json() {
    bool thrown = false;
    try {
        json_get_string("", ".key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected string type at .key";
        assert(str_contains(actual, expected) && "Exception message should contain expected substring");
    }
    assert(thrown && "Empty JSON should throw an exception");
}

void test_json_get_int_valid() {
    int actual = json_get_int("{\"key\": 42}", ".key");
    assert(actual == 42 && "Valid integer should be retrieved");
}

void test_json_get_int_invalid_type() {
    bool thrown = false;
    try {
        json_get_int("{\"key\": \"value\"}", ".key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected integer type at .key";
        assert(str_contains(actual, expected) && "Exception message should match");
    }
    assert(thrown && "Non-integer type should throw an exception");
}

void test_json_get_int_undefined_key() {
    bool thrown = false;
    try {
        json_get_int("{\"key\": 42}", ".missing");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected integer type at .missing";
        assert(str_contains(actual, expected) && "Exception message should contain expected substring");
    }
    assert(thrown && "Undefined key should throw an exception");
}

void test_json_get_int_invalid_json() {
    bool thrown = false;
    try {
        json_get_int("invalid json", ".key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected integer type at .key";
        assert(str_contains(actual, expected) && "Exception message should contain expected substring");
    }
    assert(thrown && "Invalid JSON should throw an exception");
}

void test_json_get_int_empty_json() {
    bool thrown = false;
    try {
        json_get_int("", ".key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected integer type at .key";
        assert(str_contains(actual, expected) && "Exception message should contain expected substring");
    }
    assert(thrown && "Empty JSON should throw an exception");
}

void test_json_get_double_valid() {
    double actual = json_get_double("{\"key\": 3.14}", ".key");
    assert(actual == 3.14 && "Valid double should be retrieved");
}

void test_json_get_double_invalid_type() {
    bool thrown = false;
    try {
        json_get_double("{\"key\": \"value\"}", ".key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected real type at .key";
        assert(str_contains(actual, expected) && "Exception message should match");
    }
    assert(thrown && "Non-double type should throw an exception");
}

void test_json_get_double_undefined_key() {
    bool thrown = false;
    try {
        json_get_double("{\"key\": 3.14}", ".missing");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected real type at .missing";
        assert(str_contains(actual, expected) && "Exception message should contain expected substring");
    }
    assert(thrown && "Undefined key should throw an exception");
}

void test_json_get_double_invalid_json() {
    bool thrown = false;
    try {
        json_get_double("invalid json", ".key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected real type at .key";
        assert(str_contains(actual, expected) && "Exception message should contain expected substring");
    }
    assert(thrown && "Invalid JSON should throw an exception");
}

void test_json_get_double_empty_json() {
    bool thrown = false;
    try {
        json_get_double("", ".key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected real type at .key";
        assert(str_contains(actual, expected) && "Exception message should contain expected substring");
    }
    assert(thrown && "Empty JSON should throw an exception");
}

void test_json_get_bool_true() {
    bool actual = json_get_bool("{\"key\": true}", ".key");
    assert(actual && "Valid boolean 'true' should be retrieved");
}

void test_json_get_bool_false() {
    bool actual = json_get_bool("{\"key\": false}", ".key");
    assert(!actual && "Valid boolean 'false' should be retrieved");
}

void test_json_get_bool_invalid_type() {
    bool thrown = false;
    try {
        json_get_bool("{\"key\": \"value\"}", ".key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected boolean type at .key";
        assert(str_contains(actual, expected) && "Exception message should match");
    }
    assert(thrown && "Non-boolean type should throw an exception");
}

void test_json_get_bool_undefined_key() {
    bool thrown = false;
    try {
        json_get_bool("{\"key\": true}", ".missing");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected boolean type at .missing";
        assert(str_contains(actual, expected) && "Exception message should contain expected substring");
    }
    assert(thrown && "Undefined key should throw an exception");
}

void test_json_get_bool_invalid_json() {
    bool thrown = false;
    try {
        json_get_bool("invalid json", ".key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected boolean type at .key";
        assert(str_contains(actual, expected) && "Exception message should contain expected substring");
    }
    assert(thrown && "Invalid JSON should throw an exception");
}

void test_json_get_bool_empty_json() {
    bool thrown = false;
    try {
        json_get_bool("", ".key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected boolean type at .key";
        assert(str_contains(actual, expected) && "Exception message should contain expected substring");
    }
    assert(thrown && "Empty JSON should throw an exception");
}

void test_json_get_array_valid() {
    string actual = json_get_array("{\"key\": [1, 2, 3]}", ".key");
    assert(actual == "[1,2,3]" && "Valid array should be retrieved");
}

void test_json_get_array_invalid_type() {
    bool thrown = false;
    try {
        json_get_array("{\"key\": \"value\"}", ".key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected array type at .key";
        assert(str_contains(actual, expected) && "Exception message should match");
    }
    assert(thrown && "Non-array type should throw an exception");
}

void test_json_get_array_undefined_key() {
    bool thrown = false;
    try {
        json_get_array("{\"key\": [1, 2, 3]}", ".missing");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected array type at .missing";
        assert(str_contains(actual, expected) && "Exception message should contain expected substring");
    }
    assert(thrown && "Undefined key should throw an exception");
}

void test_json_get_array_invalid_json() {
    bool thrown = false;
    try {
        json_get_array("invalid json", ".key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected array type at .key";
        assert(str_contains(actual, expected) && "Exception message should contain expected substring");
    }
    assert(thrown && "Invalid JSON should throw an exception");
}

void test_json_get_array_empty_json() {
    bool thrown = false;
    try {
        json_get_array("", ".key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected array type at .key";
        assert(str_contains(actual, expected) && "Exception message should contain expected substring");
    }
    assert(thrown && "Empty JSON should throw an exception");
}

void test_json_get_object_valid() {
    string actual = json_get_object("{\"key\": {\"nested\": \"value\"}}", ".key");
    assert(actual == "{\"nested\":\"value\"}" && "Valid object should be retrieved");
}

void test_json_get_object_invalid_type() {
    bool thrown = false;
    try {
        json_get_object("{\"key\": \"value\"}", ".key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected object type at .key";
        assert(str_contains(actual, expected) && "Exception message should match");
    }
    assert(thrown && "Non-object type should throw an exception");
}

void test_json_get_object_undefined_key() {
    bool thrown = false;
    try {
        json_get_object("{\"key\": {\"nested\": \"value\"}}", ".missing");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected object type at .missing";
        assert(str_contains(actual, expected) && "Exception message should contain expected substring");
    }
    assert(thrown && "Undefined key should throw an exception");
}

void test_json_get_object_invalid_json() {
    bool thrown = false;
    try {
        json_get_object("invalid json", ".key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected object type at .key";
        assert(str_contains(actual, expected) && "Exception message should contain expected substring");
    }
    assert(thrown && "Invalid JSON should throw an exception");
}

void test_json_get_object_empty_json() {
    bool thrown = false;
    try {
        json_get_object("", ".key");
    } catch (const exception& e) {
        thrown = true;
        string actual = e.what();
        string expected = "Expected object type at .key";
        assert(str_contains(actual, expected) && "Exception message should contain expected substring");
    }
    assert(thrown && "Empty JSON should throw an exception");
}

void test_JSON_constructor_valid() {
    JSON json("{\"key\": \"value\"}");
    assert(json.isValid() && "Valid JSON should be parsed without errors");
}

void test_JSON_constructor_invalid() {
    JSON json("{invalid json}");
    assert(!json.isValid() && "Invalid JSON should set an error");
}

void test_JSON_dump() {
    JSON json("{\"key\": \"value\"}");
    string actual = json.dump();
    assert(actual == "{\"key\":\"value\"}" && "Dump should return simulated JSON string");
}

void test_JSON_isDefined_true() {
    JSON json("{\"key\": \"value\"}");
    bool actual = json.isDefined(".key");
    assert(actual && "Defined key should return true");
}

void test_JSON_isDefined_false() {
    JSON json("{\"key\": \"value\"}");
    bool actual = json.isDefined(".missing");
    assert(!actual && "Undefined key should return false");
}

void test_JSON_isNull_true() {
    JSON json("{\"key\": null}");
    bool actual = json.isNull(".key");
    assert(actual && "Null value should return true");
}

void test_JSON_isNull_false() {
    JSON json("{\"key\": \"value\"}");
    bool actual = json.isNull(".key");
    assert(!actual && "Non-null value should return false");
}

void test_JSON_isArray_true() {
    JSON json("{\"key\": []}");
    bool actual = json.isArray(".key");
    assert(actual && "Array value should return true");
}

void test_JSON_isArray_false() {
    JSON json("{\"key\": \"value\"}");
    bool actual = json.isArray(".key");
    assert(!actual && "Non-array value should return false");
}

void test_JSON_isObject_true() {
    JSON json("{\"key\": {}}");
    bool actual = json.isObject(".key");
    assert(actual && "Object value should return true");
}

void test_JSON_isObject_false() {
    JSON json("{\"key\": \"value\"}");
    bool actual = json.isObject(".key");
    assert(!actual && "Non-object value should return false");
}

void test_JSON_get_int() {
    JSON json("{\"key\": 42}");
    int actual = json.get<int>(".key");
    assert(actual == 42 && "Integer value should be retrieved correctly");
}

void test_JSON_get_string() {
    JSON json("{\"key\": \"value\"}");
    string actual = json.get<string>(".key");
    assert(actual == "value" && "String value should be retrieved correctly");
}

void test_JSON_set() {
    JSON json("{\"key\": \"value\"}");
    json.set(".key", 42);
    int actual = json.get<int>(".key");
    assert(actual == 42 && "Set value should update the JSON object");
}


TEST(test_json_remove_comments_no_comments);
TEST(test_json_remove_comments_single_line_comment);
TEST(test_json_remove_comments_multi_line_comment);
TEST(test_json_remove_comments_mixed_comments);
TEST(test_json_remove_comments_escaped_characters);
TEST(test_json_remove_comments_strings_with_slashes);
TEST(test_json_remove_comments_nested_comments);
TEST(test_json_remove_comments_empty_input);
TEST(test_json_remove_comments_only_comments);
TEST(test_json_fix_no_trailing_commas);
TEST(test_json_fix_with_trailing_commas);
TEST(test_json_fix_with_comments);
TEST(test_json_fix_empty_input);
TEST(test_json_fix_nested_objects_and_arrays);
TEST(test_json_fix_malformed_json);
TEST(test_json_selector_empty);
TEST(test_json_selector_simple_key);
TEST(test_json_selector_nested_keys);
TEST(test_json_selector_array_indexing);
TEST(test_json_selector_mixed_keys_and_indices);
TEST(test_json_selector_invalid_empty_part);
TEST(test_json_selector_missing_dot_prefix);
TEST(test_json_selector_invalid_bracket_syntax);
TEST(test_json_selector_invalid_bracket_syntax2);
TEST(test_is_valid_json_empty_string);
TEST(test_is_valid_json_valid_object);
TEST(test_is_valid_json_valid_array);
TEST(test_is_valid_json_invalid_syntax);
TEST(test_is_valid_json_mismatched_brackets);
TEST(test_is_valid_json_empty_object);
TEST(test_is_valid_json_empty_array);
TEST(test_is_valid_json_malformed_json);
TEST(test_get_json_error_valid_object);
TEST(test_get_json_error_valid_array);
TEST(test_get_json_error_empty_string);
TEST(test_get_json_error_invalid_syntax);
TEST(test_get_json_error_mismatched_brackets);
TEST(test_get_json_error_empty_object);
TEST(test_get_json_error_empty_array);
TEST(test_get_json_error_malformed_json);
TEST(test_get_json_value_type_null);
TEST(test_get_json_value_type_string);
TEST(test_get_json_value_type_boolean);
TEST(test_get_json_value_type_integer);
TEST(test_get_json_value_type_real);
TEST(test_get_json_value_type_array);
TEST(test_get_json_value_type_object);
TEST(test_get_json_value_type_undefined_key);
TEST(test_get_json_value_type_invalid_json);
TEST(test_get_json_value_type_empty_json);
TEST(test_json_type_to_string_undefined);
TEST(test_json_type_to_string_null);
TEST(test_json_type_to_string_string);
TEST(test_json_type_to_string_integer);
TEST(test_json_type_to_string_real);
TEST(test_json_type_to_string_boolean);
TEST(test_json_type_to_string_array);
TEST(test_json_type_to_string_object);
TEST(test_json_type_to_string_invalid_type);
TEST(test_json_get_string_valid);
TEST(test_json_get_string_invalid_type);
TEST(test_json_get_string_undefined_key);
TEST(test_json_get_string_invalid_json);
TEST(test_json_get_string_empty_json);
TEST(test_json_get_int_valid);
TEST(test_json_get_int_invalid_type);
TEST(test_json_get_int_undefined_key);
TEST(test_json_get_int_invalid_json);
TEST(test_json_get_int_empty_json);
TEST(test_json_get_double_valid);
TEST(test_json_get_double_invalid_type);
TEST(test_json_get_double_undefined_key);
TEST(test_json_get_double_invalid_json);
TEST(test_json_get_double_empty_json);
TEST(test_json_get_bool_true);
TEST(test_json_get_bool_false);
TEST(test_json_get_bool_invalid_type);
TEST(test_json_get_bool_undefined_key);
TEST(test_json_get_bool_invalid_json);
TEST(test_json_get_bool_empty_json);
TEST(test_json_get_array_valid);
TEST(test_json_get_array_invalid_type);
TEST(test_json_get_array_undefined_key);
TEST(test_json_get_array_invalid_json);
TEST(test_json_get_array_empty_json);
TEST(test_json_get_object_valid);
TEST(test_json_get_object_invalid_type);
TEST(test_json_get_object_undefined_key);
TEST(test_json_get_object_invalid_json);
TEST(test_json_get_object_empty_json);
TEST(test_JSON_constructor_valid);
TEST(test_JSON_constructor_invalid);
TEST(test_JSON_dump);
TEST(test_JSON_isDefined_true);
TEST(test_JSON_isDefined_false);
TEST(test_JSON_isNull_true);
TEST(test_JSON_isNull_false);
TEST(test_JSON_isArray_true);
TEST(test_JSON_isArray_false);
TEST(test_JSON_isObject_true);
TEST(test_JSON_isObject_false);
TEST(test_JSON_get_int);
TEST(test_JSON_get_string);
TEST(test_JSON_set);
#endif