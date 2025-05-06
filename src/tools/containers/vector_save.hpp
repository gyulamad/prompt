#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <type_traits>

#include "../utils/ERROR.h"

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

#ifdef TEST

using namespace tools::containers;

void test_vector_save_empty() {
    vector<int> vec;
    vector_save(vec, "test_empty.bin");
    ifstream file("test_empty.bin", ios::binary);
    assert(file.is_open());
    size_t size;
    file.read(reinterpret_cast<char*>(&size), sizeof(size));
    assert(size == 0);
    file.close();
    
    // cleanup
    remove("test_empty.bin");

}

void test_vector_save_with_elements() {
    vector<int> vec = {1, 2, 3, 4, 5};
    vector_save(vec, "test_with_elements.bin");
    ifstream file("test_with_elements.bin", ios::binary);
    assert(file.is_open());
    size_t size;
    file.read(reinterpret_cast<char*>(&size), sizeof(size));
    assert(size == 5);
    int data;
    for (int i = 0; i < 5; ++i) {
        file.read(reinterpret_cast<char*>(&data), sizeof(data));
        assert(data == i + 1);
    }
    file.close();
    
    // cleanup
    remove("test_with_elements.bin");

}

void test_vector_save_error() {
    vector<int> vec = {1, 2, 3};
    bool t = false;
    try {
        vector_save(vec, "/invalid/path/test_error.bin");
    } catch (const exception& e) {
        t = true;
    }
    assert(t && "Expected an exception");
}

TEST(test_vector_save_empty);
TEST(test_vector_save_with_elements);
TEST(test_vector_save_error);

#endif
