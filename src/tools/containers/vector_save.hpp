#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <type_traits>

#include "../utils/ERROR.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::containers {

    // Save a vector to a binary file
    template<typename T>
    void vector_save(const vector<T>& vec, const string& filename) {
        // Ensure T is POD (Plain Old Data)
        static_assert(is_trivially_copyable<T>::value, "Type T must be trivially copyable (POD)");

        ofstream out(filename, ios::binary);
        if (!out) {
            throw ERROR("Cannot open file for writing: " + filename);
        }

        // Write the size of the vector
        size_t size = vec.size();
        out.write(reinterpret_cast<const char*>(&size), sizeof(size));

        // Write the vector data
        if (size > 0) {
            out.write(reinterpret_cast<const char*>(vec.data()), size * sizeof(T));
        }

        out.close();
    }

}
