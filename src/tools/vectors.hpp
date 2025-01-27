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
    
    // Function to merge two vectors
    template <typename T>
    vector<T> array_merge(const vector<T>& vec1, const vector<T>& vec2) {
        vector<T> result;

        // Reserve space to avoid repeated reallocations
        result.reserve(vec1.size() + vec2.size());

        // Add elements from the first vector
        result.insert(result.end(), vec1.begin(), vec1.end());

        // Add elements from the second vector
        result.insert(result.end(), vec2.begin(), vec2.end());

        return result;
    }

    template<typename T>
    void array_dump(vector<T> vec, bool dbg = true) {
        if (dbg) {
            DEBUG("dump vector(" + to_string(vec.size()) + "):");
        }
        size_t nth = 0;
        for (const T& elem: vec) cout << nth++ << ": " << elem << endl;
    }
    

    template <typename T>
    void sort(vector<T>& vec) {
        sort(vec.begin(), vec.end()); // Sorts in ascending order
    }

    template <typename T>
    void rsort(vector<T>& vec) {
        sort(vec.begin(), vec.end(), greater<T>()); // Sorts in descending order
    }

    template<typename T>
    vector<T> array_filter(const vector<T>& input, function<bool(const T&)> predicate = [](const T& val) { 
        return !val.empty(); 
    }) {
        vector<T> result;
        copy_if(input.begin(), input.end(), back_inserter(result), predicate);
        return result;
    }
    
};
