#pragma once

#include <vector>

using namespace std;

namespace tools::containers {

    template <typename T, typename... Args>
    int array_unshift(vector<T>& vec, Args&&... values) {
        // Insert all values at the beginning in reverse order
        // We need to reverse the order because insert adds elements before the position
        size_t initial_size = vec.size();
        vector<T> temp{forward<Args>(values)...};
        
        // Insert at the beginning (position 0)
        vec.insert(vec.begin(), temp.rbegin(), temp.rend());
        
        // Return new size
        return static_cast<int>(vec.size());
    }

}
