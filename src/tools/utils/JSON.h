#pragma once

#include <string>
#include <vector>
#include <map>
#include <stack>
#include <regex>
#include "../../../libs/nlohmann/json/single_include/nlohmann/json.hpp"
#include "../str/explode.h"
#include "../str/implode.h"
#include "../str/str_diffs_show.h"
#include "../str/str_contains.h"
#include "ERROR.h"

using namespace std;
using namespace nlohmann;
using namespace tools::str;

namespace tools::utils {

    // Function to remove single-line and multi-line comments
    string json_remove_comments(const string& json);

    // Function to fix JSON by removing trailing commas
    string json_fix(string json);

    // Enum to represent JSON value types
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

    // Global variable to store the last JSON parsing error
    extern string json_last_error;

    // Function to convert jq-style or JavaScript-style selector to json_pointer
    nlohmann::json::json_pointer _json_selector(string jselector);

    // Check if a JSON string is valid
    bool is_valid_json(string jstring);

    // Get the last JSON parsing error
    string get_json_error(string jstring);

    // Get the type of a JSON value at a given selector
    json_type get_json_value_type(string jstring, string jselector);

    // Convert a json_type enum to its string representation
    string json_type_to_string(json_type type);

    // Retrieve specific JSON value types
    string json_get_string(string jstring, string jselector);
    int json_get_int(string jstring, string jselector);
    double json_get_double(string jstring, string jselector);
    bool json_get_bool(string jstring, string jselector);
    string json_get_array(string jstring, string jselector);
    string json_get_object(string jstring, string jselector);

    // JSON class to manage JSON data
    class JSON {
    protected:
        string* _error = nullptr;
        nlohmann::json j;

    public:
        JSON(const nlohmann::json& j);
        JSON(const char* j);
        JSON(string jstring = "{}");
        ~JSON();

        nlohmann::json get_json() const;
        bool isValid(string* error = nullptr);
        string dump(int indent = -1, char indent_char = ' ') const;
        bool isDefined(string jselector) const;
        bool has(string jselector) const;
        bool isNull(string jselector);
        bool isArray(string jselector);
        bool isObject(string jselector);

        

        template<typename T>
        T get(string jselector) const {
            try {
                json::json_pointer ptr = _json_selector(jselector);  // Convert selector to pointer
                if constexpr (is_same_v<T, JSON>) return JSON(j.at(ptr));
                return j.at(ptr).get<T>();
            } catch (const exception& e) {
                DEBUG(j.dump());
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

        // Commented-out validation helpers (retained for future use)
        // void need(const string& field) const;
        // void need(const vector<string>& fields) const;
    };

} // namespace tools::utils

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

#ifdef TEST// Test function declarations
void test_json_remove_comments_no_comments();
void test_json_remove_comments_single_line_comment();
void test_json_remove_comments_multi_line_comment();
void test_json_remove_comments_mixed_comments();
void test_json_remove_comments_escaped_characters();
void test_json_remove_comments_strings_with_slashes();
void test_json_remove_comments_nested_comments();
void test_json_remove_comments_empty_input();
void test_json_remove_comments_only_comments();
void test_json_fix_no_trailing_commas();
void test_json_fix_with_trailing_commas();
void test_json_fix_with_comments();
void test_json_fix_empty_input();
void test_json_fix_nested_objects_and_arrays();
void test_json_fix_malformed_json();
void test_json_fix_escaped_backslash();
void test_json_selector_empty();
void test_json_selector_simple_key();
void test_json_selector_nested_keys();
void test_json_selector_array_indexing();
void test_json_selector_mixed_keys_and_indices();
void test_json_selector_invalid_empty_part();
void test_json_selector_missing_dot_prefix();
void test_json_selector_invalid_bracket_syntax();
void test_json_selector_invalid_bracket_syntax2();
void test_is_valid_json_empty_string();
void test_is_valid_json_valid_object();
void test_is_valid_json_valid_array();
void test_is_valid_json_invalid_syntax();
void test_is_valid_json_mismatched_brackets();
void test_is_valid_json_empty_object();
void test_is_valid_json_empty_array();
void test_is_valid_json_malformed_json();
void test_get_json_error_valid_object();
void test_get_json_error_valid_array();
void test_get_json_error_empty_string();
void test_get_json_error_invalid_syntax();
void test_get_json_error_mismatched_brackets();
void test_get_json_error_empty_object();
void test_get_json_error_empty_array();
void test_get_json_error_malformed_json();
void test_get_json_value_type_null();
void test_get_json_value_type_string();
void test_get_json_value_type_boolean();
void test_get_json_value_type_integer();
void test_get_json_value_type_real();
void test_get_json_value_type_array();
void test_get_json_value_type_object();
void test_get_json_value_type_undefined_key();
void test_get_json_value_type_invalid_json();
void test_get_json_value_type_empty_json();
void test_json_type_to_string_undefined();
void test_json_type_to_string_null();
void test_json_type_to_string_string();
void test_json_type_to_string_integer();
void test_json_type_to_string_real();
void test_json_type_to_string_boolean();
void test_json_type_to_string_array();
void test_json_type_to_string_object();
// void test_json_type_to_string_invalid_type();
void test_json_get_string_valid();
void test_json_get_string_invalid_type();
void test_json_get_string_undefined_key();
void test_json_get_string_invalid_json();
void test_json_get_string_empty_json();
void test_json_get_int_valid();
void test_json_get_int_invalid_type();
void test_json_get_int_undefined_key();
void test_json_get_int_invalid_json();
void test_json_get_int_empty_json();
void test_json_get_double_valid();
void test_json_get_double_invalid_type();
void test_json_get_double_undefined_key();
void test_json_get_double_invalid_json();
void test_json_get_double_empty_json();
void test_json_get_bool_true();
void test_json_get_bool_false();
void test_json_get_bool_invalid_type();
void test_json_get_bool_undefined_key();
void test_json_get_bool_invalid_json();
void test_json_get_bool_empty_json();
void test_json_get_array_valid();
void test_json_get_array_invalid_type();
void test_json_get_array_undefined_key();
void test_json_get_array_invalid_json();
void test_json_get_array_empty_json();
void test_json_get_object_valid();
void test_json_get_object_invalid_type();
void test_json_get_object_undefined_key();
void test_json_get_object_invalid_json();
void test_json_get_object_empty_json();
void test_JSON_constructor_valid();
void test_JSON_constructor_invalid();
void test_JSON_dump();
void test_JSON_isDefined_true();
void test_JSON_isDefined_false();
void test_JSON_isNull_true();
void test_JSON_isNull_false();
void test_JSON_isArray_true();
void test_JSON_isArray_false();
void test_JSON_isObject_true();
void test_JSON_isObject_false();
void test_JSON_get_int();
void test_JSON_get_string();
void test_JSON_set();
#endif