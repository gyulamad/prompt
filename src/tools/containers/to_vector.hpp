#pragma once

#include <string>
#include <vector>

using namespace std;

namespace tools::containers {

    // Trait to determine how to convert TData* to T
    template<typename T, typename TData>
    struct ToVectorTraits {
        static T convert(TData* ptr) {
            return static_cast<T>(*ptr); // Dereference for value types (e.g., int*, double*)
        }
    };

    // Specialization for converting const char* to string
    template<>
    struct ToVectorTraits<string, const char> {
        static string convert(const char* ptr) {
            return ptr ? string(ptr) : string(); // Construct string from pointer
        }
    };

    // Convert an array of pointers to a vector of values with explicit size
    template<typename T, typename TData>
    vector<T> to_vector(TData* data[], size_t size) {
        vector<T> vec;
        vec.reserve(size); // Pre-allocate to avoid reallocations
        for (size_t i = 0; i < size; ++i) {
            if (data[i]) {
                vec.push_back(ToVectorTraits<T, TData>::convert(data[i]));
            } else {
                vec.push_back(T{}); // Default value for nullptr
            }
        }
        return vec;
    }

    // Overload for null-terminated arrays (like char* argv[])
    template<typename T, typename TData>
    vector<T> to_vector(TData* data[]) {
        vector<T> vec;
        size_t i = 0;
        while (data[i] != nullptr) {
            vec.push_back(ToVectorTraits<T, TData>::convert(data[i]));
            ++i;
        }
        return vec;
    }
    
}

#ifdef TEST

using namespace tools::containers;

// Test converting a null-terminated char* array (like argv) to vector<string>
void test_to_vector_null_terminated_char_to_string() {
    const char* input[] = {"program", "--flag", "value", nullptr};
    auto result = to_vector<string>(input);
    vector<string> expected = {"program", "--flag", "value"};
    assert(vector_equal(result, expected) && "Null-terminated char* array to vector<string> failed");
}

// Test converting an empty null-terminated char* array
void test_to_vector_empty_null_terminated() {
    const char* input[] = {nullptr};
    auto result = to_vector<string>(input);
    vector<string> expected = {};
    assert(vector_equal(result, expected) && "Empty null-terminated array should yield empty vector");
}

// Test converting a sized char* array to vector<string>
void test_to_vector_sized_char_to_string() {
    const char* input[] = {"one", "two", "three"};
    auto result = to_vector<string>(input, 3);
    vector<string> expected = {"one", "two", "three"};
    assert(vector_equal(result, expected) && "Sized char* array to vector<string> failed");
}

// Test handling nullptr in a sized array
void test_to_vector_sized_with_nullptr() {
    const char* input[] = {"first", nullptr, "third"};
    auto result = to_vector<string>(input, 3);
    vector<string> expected = {"first", "", "third"}; // nullptr becomes empty string
    assert(vector_equal(result, expected) && "Sized array with nullptr to vector<string> failed");
}

// Test converting a sized int* array to vector<int>
void test_to_vector_sized_int_to_int() {
    int values[] = {1, 2, 3};
    int* input[] = {&values[0], &values[1], &values[2]};
    auto result = to_vector<int>(input, 3);
    vector<int> expected = {1, 2, 3};
    assert(vector_equal(result, expected) && "Sized int* array to vector<int> failed");
}

TEST(test_to_vector_null_terminated_char_to_string);
TEST(test_to_vector_empty_null_terminated);
TEST(test_to_vector_sized_char_to_string);
TEST(test_to_vector_sized_with_nullptr);
TEST(test_to_vector_sized_int_to_int);
#endif
