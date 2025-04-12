#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <type_traits>

#include "../utils/ERROR.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::containers {

    // Load a vector from a binary file
    template<typename T>
    void vector_load(vector<T>& vec, const string& filename) {
        // Ensure T is POD
        static_assert(is_trivially_copyable<T>::value, "Type T must be trivially copyable (POD)");

        ifstream in(filename, ios::binary);
        if (!in) {
            throw runtime_error("Cannot open file for reading: " + filename);
        }

        // Read the size
        size_t size;
        in.read(reinterpret_cast<char*>(&size), sizeof(size));

        // Resize the vector
        vec.resize(size);

        // Read the data
        if (size > 0) {
            in.read(reinterpret_cast<char*>(vec.data()), size * sizeof(T));
        }

        if (!in) {
            throw ERROR("Error reading data from file: " + filename);
        }

        in.close();
    }

}
