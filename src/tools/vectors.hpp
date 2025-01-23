#pragma once

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

    template<typename T>
    void array_dump(vector<T> vec) {
        DEBUG("dump vector(" + to_string(vec.size()) + "):");
        size_t nth = 0;
        for (const T& elem: vec) cout << nth++ << ": " << elem << endl;
    }

};
