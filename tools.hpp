#pragma once

#include <fstream>
#include <regex>
#include <iomanip>

#include "ERROR.hpp"

using namespace std;

// TODO: write tests to tools

vector<string> explode(const string& delimiter, const string& str) {
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

std::string implode(const std::string& delimiter, const std::vector<std::string>& elements) {
    std::ostringstream oss;
    for (size_t i = 0; i < elements.size(); ++i) {
        if (i != 0) {
            oss << delimiter;
        }
        oss << elements[i];
    }
    return oss.str();
}


template <typename T>
T array_shift(vector<T>& vec) {
    if (vec.empty()) {
        throw ERROR("Cannot shift from an empty vector");
    }

    // Save the first element (to return it)
    T firstElement = move(vec.front());

    // Remove the first element
    vec.erase(vec.begin());

    // Return the shifted element
    return firstElement;
}



string file_get_contents(const string& filename) {
    // Open the file in binary mode and position the cursor at the end
    ifstream file(filename, ios::in | ios::binary);
    if (!file.is_open()) {
        throw ios_base::failure("Failed to open file: " + filename);
    }

    // Seek to the end to determine file size
    file.seekg(0, ios::end);
    streamsize size = file.tellg();
    file.seekg(0, ios::beg);

    // Read file content into a string
    string content(size, '\0');
    if (!file.read(&content[0], size)) {
        throw ios_base::failure("Failed to read file: " + filename);
    }

    return content;
}

void file_put_contents(const string& filename, const string& content) {
    ofstream file(filename, ios::out | ios::binary);
    if (!file.is_open()) {
        throw ios_base::failure("Failed to open file: " + filename);
    }

    if (!file.write(content.data(), content.size())) {
        throw ios_base::failure("Failed to write to file: " + filename);
    }
}


string trim(const string& str) {
    // Find the first non-whitespace character from the beginning
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    
    // If there is no non-whitespace character, return an empty string
    if (start == string::npos) {
        return "";
    }

    // Find the first non-whitespace character from the end
    size_t end = str.find_last_not_of(" \t\n\r\f\v");

    // Return the substring from the first non-whitespace to the last non-whitespace character
    return str.substr(start, end - start + 1);
}

string str_cut_begin(const string& s, int maxch, const string& prepend = "...") {
    // Check if the string is already shorter than or equal to the limit
    if (s.length() <= maxch) {
        return s;
    }

    // Truncate the string from the beginning and prepend the prefix
    return prepend + s.substr(s.length() - (maxch - prepend.length()));
}

string str_cut_end(const string& s, int maxch = 300, const string& append = "...") {
    // Check if the string is already shorter than or equal to the limit
    if (s.length() <= maxch) {
        return s;
    }

    // Truncate the string and append the suffix
    return s.substr(0, maxch - append.length()) + append;
}

bool str_contains(const string& str, const string& substring) {
    // Use string::find to check if the substring exists
    return str.find(substring) != string::npos;
}


string esc(const string& input, const string& chars = "\\\"'`") {
    string result = input;
    
    // Iterate over each character in the 'chars' string
    for (char ch : chars) {
        // Escape each occurrence of the character in the result string
        size_t pos = 0;
        while ((pos = result.find(ch, pos)) != string::npos) {
            result.insert(pos, "\\");
            pos += 2;  // Skip past the newly inserted backslash
        }
    }
    return result;
}


string str_replace(const map<string, string>& v, const string& s) {
    // Create a modifiable copy of the input string
    string result = s;

    // Iterate through each key-value pair in the map
    for (const auto& pair : v) {
        size_t pos = 0;

        // Search for the key in the string and replace all occurrences
        while ((pos = result.find(pair.first, pos)) != string::npos) {
            result.replace(pos, pair.first.length(), pair.second);
            pos += pair.second.length(); // Move past the replacement
        }
    }

    // Return the modified string
    return result;
}
string str_replace(const string& from, const string& to, const string& subject) {
    return str_replace({{from, to}}, subject);
}


// Function to find placeholders with custom markers
std::vector<std::string> find_placeholders(const std::string& template_str, 
                                           const std::string& open_marker, 
                                           const std::string& close_marker) {
    std::vector<std::string> placeholders;

    // Escape the markers for use in regex
    auto escape_regex = [](const std::string& str) -> std::string {
        std::string escaped;
        for (char c : str) {
            if (std::string("^$.*+?()[]{}|\\").find(c) != std::string::npos) {
                escaped += '\\'; // Add escape for regex special characters
            }
            escaped += c;
        }
        return escaped;
    };

    std::string escaped_open = escape_regex(open_marker);
    std::string escaped_close = escape_regex(close_marker);

    // Build the regex pattern that allows matching across multiple lines
    std::string pattern = escaped_open + "([\\s\\S]*?)" + escaped_close;

    std::regex placeholder_regex(pattern); // Match the pattern
    std::smatch match;

    std::string::const_iterator search_start(template_str.cbegin());
    while (std::regex_search(search_start, template_str.cend(), match, placeholder_regex)) {
        // match[1] contains the content inside the markers
        placeholders.push_back(match[1].str());
        search_start = match.suffix().first; // Move past the current match
    }

    return placeholders;
}


std::string bash(const std::string& script, int timeout = -1) {
    // Create temporary files for the script and output
    const char* tempScriptFile = "/tmp/temp_script.sh";    // Temporary bash script file
    const char* tempOutputFile = "/tmp/temp_output.txt";    // Temporary output file

    remove(tempScriptFile);
    remove(tempOutputFile);

    // Create and write the script to the temp script file
    std::ofstream scriptFile(tempScriptFile);
    if (!scriptFile) {
        return "Error creating the temporary script file.";
    }
    scriptFile << script;
    scriptFile.close();

    // Set permissions to make the script executable
    std::string chmodCommand = "chmod +x " + std::string(tempScriptFile);
    if (system(chmodCommand.c_str()) != 0) {
        return "Error setting script permissions.";
    }

    // Build the command to execute the script with timeout if specified
    std::string fullCommand;
    if (timeout > 0) {
        // Prepend the timeout command with the specified timeout duration
        fullCommand = "timeout " + std::to_string(timeout) + "s " + std::string(tempScriptFile) + " >> " + std::string(tempOutputFile) + " 2>&1";
    } else {
        // No timeout, execute script normally
        fullCommand = std::string(tempScriptFile) + " >> " + std::string(tempOutputFile) + " 2>&1";
    }

    // Execute the bash script
    int result = system(fullCommand.c_str());
    // TODO: measure the execution time and if it's greater than timeout we can append this information to the return value later to let the caller know what may just happened: output += "\nPossible timeout happened.";

    // Read the content of the temporary output file
    std::ifstream outputFile(tempOutputFile);
    if (!outputFile.is_open()) {
        return "Error reading the temporary output file.";
    }

    // Read the content into a string
    std::stringstream buffer;
    buffer << outputFile.rdbuf();
    outputFile.close();

    string output = buffer.str();
    if (result != 0) {
        return "Error executing the script (" + to_string(result) + "), output: " + (output.empty() ? "<empty>" : "\n" + output);
    }

    return output;  // Return the content of the output file
}

std::string timef(const std::string& format = "%Y-%m-%d %H:%M:%S", const std::time_t* timestamp = nullptr) {
    std::time_t currentTime = (timestamp == nullptr) ? std::time(nullptr) : *timestamp;
    std::tm* localTime = std::localtime(&currentTime);
    std::stringstream ss;
    ss << std::put_time(localTime, format.c_str());
    return ss.str();
}