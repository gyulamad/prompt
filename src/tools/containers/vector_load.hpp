#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <type_traits>

#include "../utils/ERROR.h"

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
            // LCOV_EXCL_START
            throw ERROR("Error reading data from file: " + filename);
            // LCOV_EXCL_STOP
        }

        in.close();
    }

}

#ifdef TEST

#include "vector_equal.hpp"
#include "../utils/files.hpp"

using namespace tools::containers;

void test_vector_load_success() {
    vector<int> vec;
    vector<int> expected = {1, 2, 3, 4, 5};
    vector_save(expected, "test.bin");
    vector_load(vec, "test.bin");
    
    // cleanup
    remove("test.bin");

    assert(vector_equal(vec, expected) && "Vector load success");
}

void test_vector_load_file_open_error() {
    vector<int> vec;
    bool thrown = false;
    try {
        vector_load(vec, "non_existent_file.bin");
    } catch (exception& e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Cannot open file for reading") && "File open error message");
    }
    assert(thrown && "Test case should throw but it didn't");
}

void test_vector_load_data_read_error() {
    vector<int> vec;
    vector<int> expected = {1, 2, 3, 4, 5};
    vector_save(expected, "test.bin");
    // Corrupt the file
    ofstream corrupt("test.bin", ios::binary);
    corrupt.write("corrupted", 10);
    corrupt.close();
    bool thrown = false;
    try {
        vector_load(vec, "test.bin");
    } catch (exception& e) {
        thrown = true;
        //string what = e.what();
        //assert(str_contains(what, "Error reading data from file") && "Data read error message");
    }
    
    // cleanup
    remove("test.bin");

    assert(thrown && "Test case should throw but it didn't");
}

TEST(test_vector_load_success);
TEST(test_vector_load_file_open_error);
TEST(test_vector_load_data_read_error);

#endif
