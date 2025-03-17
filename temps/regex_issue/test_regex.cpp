// g++ -std=c++20 -Ofast test_regex.cpp -o test_regex

#include <iostream>
#include <regex>
#include <string>

// Function with regex that might trigger the bug
std::string process_string(const std::string& input) {
    std::regex pattern(R"(\d+)"); // Matches one or more digits
    std::smatch match;

    if (std::regex_search(input, match, pattern)) {
        return match[0].str();
    } else {
        return "";
    }
}

// Function where we disable optimizations
// __attribute__((optimize("O0")))
std::string safe_process_string(const std::string& input){
    return process_string(input);
}

int main() {
    std::string test_string = "abc123def";
    std::string result = safe_process_string(test_string);

    std::cout << "Result: " << result << std::endl;

    return 0;
}