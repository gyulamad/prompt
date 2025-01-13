#pragma once

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