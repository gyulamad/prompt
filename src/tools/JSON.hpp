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
        string jstring;
        json _json;

    public:
        // Constructor to initialize the JSON string (can be empty)
        JSON(string jstring = "{}") : jstring(jstring.empty() ? "{}" : jstring) {
            try {
                _json = json::parse(jstring);
            } catch (const exception &e) {
                _error = new string(e.what());
            }
        }

        // Destructor (no special cleanup needed here)
        ~JSON() {
            if (_error) delete _error;
        }

        bool isValid(string* error = nullptr) {
            if (error) error = _error;
            return _error;
        }     

        string dump(const int indent = -1, const char indent_char = ' ') {
            try {
                json j;
                try {
                    j = json::parse(jstring);  // Parse the JSON string
                } catch (const exception &e) {
                    cerr << "JSON parse error: " << e.what() << endl;
                    DEBUG(jstring);
                    return "";
                }
                string dump = j.dump(indent, indent_char);
                return dump;
            } catch (const exception& e) {
                throw ERROR("JSON dump error: " + string(e.what()) + "\nJSON was:\n" + jstring);
            }
        }

        // Method to check if a selector is defined in the JSON (exists)
        bool isDefined(string jselector) {
            try {
                json j = json::parse(jstring);  // Parse the JSON string
                json::json_pointer ptr = _json_selector(jselector);  // Convert selector to pointer
                return j.contains(ptr);  // Check if the pointer exists in the JSON
            } catch (...) {
                return false;  // If parsing fails or any error occurs, consider undefined
            }
        }

        // Method to check if a selector is null in the JSON
        bool isNull(string jselector) {
            try {
                json j = json::parse(jstring);  // Parse the JSON string
                json::json_pointer ptr = _json_selector(jselector);  // Convert selector to pointer
                return j.at(ptr).is_null();  // Check if the value at the pointer is null
            } catch (...) {
                return false;  // If an error occurs, assume null
            }
        }

        template<typename T>
        T get(string jselector) {
            try {
                json j = json::parse(jstring);  // Parse the JSON string
                json::json_pointer ptr = _json_selector(jselector);  // Convert selector to pointer
                return j.at(ptr).get<T>();  // Return the value
            } catch (const exception& e) {
                throw ERROR("JSON Error at: " + jselector + ", reason: " + string(e.what()) + "\njson-string was: " + jstring);
            }
        }

        void set(string value) {
            jstring = value;
        }

        template<typename T>
        void set(string jselector, T value) {
            try {
                json j = json::parse(jstring);
                json::json_pointer ptr = _json_selector(jselector);
                j[ptr] = value;
                jstring = j.dump();
            } catch (const json::exception& e) {
                throw ERROR("JSON Error at: " + jselector + ", reason: " + string(e.what()) + "\njson was: " + jstring);
            }
        }

    };

}