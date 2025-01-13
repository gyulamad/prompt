#pragma once

#include "ERROR.hpp"

using namespace std;

vector<string> explode(const string& str, const string& delimiter) {
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