#include <regex>
#include "../libs/nlohmann/json/single_include/nlohmann/json.hpp"  
using namespace std;
using namespace nlohmann;

vector<string> explode(const string& delimiter, const string& str) {
    if (delimiter.empty()) throw runtime_error("Delimeter can not be empty.");
    vector<string> result;
    size_t start = 0;
    size_t end = str.find(delimiter);

    // Split the string by the delimiter
    while (end != string::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }

    // Add the last part of the string
    result.push_back(str.substr(start));

    return result;
}

string implode(const string& delimiter, const vector<string>& elements) {
    ostringstream oss;
    for (size_t i = 0; i < elements.size(); ++i) {
        if (i != 0) oss << delimiter;
        oss << elements[i];
    }
    return oss.str();
}

    // Function to convert jq-style or JavaScript-style selector to json_pointer
    json::json_pointer _json_selector(string jselector) {
        if (jselector.empty()) return json::json_pointer("/");
        if (jselector[0] != '.') jselector = "." + jselector;

        // Split the selector by dots
        vector<string> splits = explode(".", jselector);

        for (size_t i = 1; i < splits.size(); i++) {
            if (splits[i].empty()) 
                throw runtime_error("Invalid json selector: " + jselector);

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
                throw runtime_error("Invalid json selector: " + jselector); // [not-numeric] is invalid
            }
        }

        // Validate mismatched brackets
        int open_brackets = 0, close_brackets = 0;
        for (char ch : jselector) {
            if (ch == '[') open_brackets++;
            if (ch == ']') close_brackets++;
        }
        if (open_brackets != close_brackets) {
            throw runtime_error("Invalid json selector: " + jselector); // Mismatched brackets
        }

        return json::json_pointer(implode("/", splits));
    }

int main() {
    // run_tests({
    //     "test_json_selector_empty"
    // }); 
    
    json::json_pointer actual = _json_selector("");
    json::json_pointer expected("/");
    assert(actual == expected && "Empty selector should return '/'");   
}