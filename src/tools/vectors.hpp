#pragma once

#include <unordered_set>

#include "ERROR.hpp"

namespace tools {

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

    template <typename T>
    vector<T> array_unique(const vector<T>& vec) {
        // Use an unordered_set to filter out duplicates
        unordered_set<T> seen;
        vector<T> result;

        // Reserve space for the result vector to avoid repeated reallocations
        result.reserve(vec.size());

        // Iterate through the input vector
        for (const auto& element : vec) {
            // Insert the element into the set (if it's not already present)
            if (seen.insert(element).second) {
                // If the element was inserted (i.e., it's unique), add it to the result
                result.push_back(element);
            }
        }

        return result;
    }

    template<typename T>
    void array_dump(vector<T> vec) {
        DEBUG("dump vector(" + to_string(vec.size()) + "):");
        size_t nth = 0;
        for (const T& elem: vec) cout << nth++ << ": " << elem << endl;
    }

};
