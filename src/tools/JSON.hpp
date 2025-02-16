#pragma once

#include <string>
#include <vector>
#include <map>
#include <regex>

// Include the nlohmann JSON library: 
// git clone https://github.com/nlohmann/json
#include "../../libs/nlohmann/json/single_include/nlohmann/json.hpp"  

#include "ERROR.hpp"
#include "strings.hpp"

using namespace std;
using namespace nlohmann;

namespace tools {

    // Function to remove single-line and multi-line comments
    string removeComments(const string& json) {
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
    string fixJson(string json) {
        json = removeComments(json);
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

    // Converts jq-style or JavaScript-style selector to json_pointer
    json::json_pointer _json_selector(string jselector) {
        if (jselector.empty()) return json::json_pointer("/");
        if (jselector[0] != '.') jselector = "." + jselector;
        vector<string> splits = explode(".", jselector);
        for (size_t i = 1; i < splits.size(); i++) {
            if (splits[i].empty()) 
                throw ERROR("Invalid json selector: " + jselector);
            regex brackets_to_slash("\\[(\\d+)\\]$"); // Matches [N] for array indexing
            splits[i] = regex_replace(splits[i], brackets_to_slash, "/$1");
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

        // Constructor to initialize the JSON string (can be empty)
        JSON(string jstring = "{}") {
            try {
                j = json::parse(jstring.empty() ? "{}" : fixJson(jstring));
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
            return _error;
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
    struct adl_serializer<tools::JSON> {
        static tools::JSON from_json(const json& j) {
            return tools::JSON(j);
        }
        
        static void to_json(json& j, const tools::JSON& jsonObj) {
            j = jsonObj.get_json();
        }
    };
}