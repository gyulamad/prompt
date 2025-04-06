#pragma once

#include <vector>

using namespace std;

namespace tools::containers {

    template <typename T, typename... Args>
    int array_push(vector<T>& vec, Args&&... values) {
        // Store initial size
        size_t initial_size = vec.size();
        
        // Use parameter pack expansion to push all values
        (vec.push_back(forward<Args>(values)), ...);
        
        // Return new size
        return static_cast<int>(vec.size());
    }

}
