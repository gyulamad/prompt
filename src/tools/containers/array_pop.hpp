#pragma once

#include <vector>

using namespace std;

namespace tools::containers {

    template <typename T>
    T array_pop(vector<T>& vec) {
        if (vec.empty()) 
            throw ERROR("Cannot pop from an empty vector");
        
        T last_element = vec.back();  // Get the last element
        vec.pop_back();              // Remove the last element
        return last_element;         // Return the removed element
    }

}
