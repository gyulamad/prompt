#pragma once

#include <vector>

using namespace std;

namespace tools::containers {

    // Template function to compare two vectors for equality
    template<typename T>
    bool vector_equal(const vector<T>& a, const vector<T>& b) {
        // First, check if the sizes of the vectors are equal
        if (a.size() != b.size()) return false;

        // Then, compare each element of the vectors
        for (size_t i = 0; i < a.size(); ++i)
            if (a[i] != b[i]) return false;

        // If all elements are equal, return true
        return true;
    }
    
}

#ifdef TEST

using namespace tools::containers;

// Test cases for vector_equal
void test_vector_equal_basic_equal() {
    vector<int> vec1 = {1, 2, 3};
    vector<int> vec2 = {1, 2, 3};
    assert(vector_equal(vec1, vec2) && "Basic vectors should be equal");
}

void test_vector_equal_basic_not_equal() {
    vector<int> vec1 = {1, 2, 3};
    vector<int> vec2 = {1, 2, 4};
    assert(!vector_equal(vec1, vec2) && "Basic vectors should not be equal");
}

void test_vector_equal_different_sizes() {
    vector<int> vec1 = {1, 2, 3};
    vector<int> vec2 = {1, 2};
    assert(!vector_equal(vec1, vec2) && "Vectors with different sizes should not be equal");
}

void test_vector_equal_empty_vectors() {
    vector<int> vec1;
    vector<int> vec2;
    assert(vector_equal(vec1, vec2) && "Empty vectors should be equal");
}

void test_vector_equal_one_empty() {
    vector<int> vec1 = {1,2,3};
    vector<int> vec2;
    assert(!vector_equal(vec1, vec2) && "One empty vector and one non empty should not be equal");
}

void test_vector_equal_strings_equal() {
    vector<string> vec1 = {"apple", "banana", "cherry"};
    vector<string> vec2 = {"apple", "banana", "cherry"};
    assert(vector_equal(vec1, vec2) && "String vectors should be equal");
}

void test_vector_equal_strings_not_equal() {
    vector<string> vec1 = {"apple", "banana", "cherry"};
    vector<string> vec2 = {"apple", "grape", "cherry"};
    assert(!vector_equal(vec1, vec2) && "String vectors should not be equal");
}

void test_vector_equal_custom_objects_equal() {
    struct Point {
        int x, y;
        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
    };
    vector<Point> vec1 = {{1, 2}, {3, 4}, {5, 6}};
    vector<Point> vec2 = {{1, 2}, {3, 4}, {5, 6}};
    assert(vector_equal(vec1, vec2) && "Custom object vectors should be equal");
}

void test_vector_equal_custom_objects_not_equal() {
    struct Point {
        int x, y;
        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
    };
    vector<Point> vec1 = {{1, 2}, {3, 4}, {5, 6}};
    vector<Point> vec2 = {{1, 2}, {3, 5}, {5, 6}};
    assert(!vector_equal(vec1, vec2) && "Custom object vectors should not be equal");
}
void test_vector_equal_large_equal() {
    vector<int> vec1;
    vector<int> vec2;
    for (int i=0; i<10000; i++) {
        vec1.push_back(i);
        vec2.push_back(i);
    }
    assert(vector_equal(vec1, vec2) && "Large vectors should be equal");
}
void test_vector_equal_large_not_equal() {
    vector<int> vec1;
    vector<int> vec2;
    for (int i=0; i<10000; i++) {
        vec1.push_back(i);
        vec2.push_back(i+1);
    }
    assert(!vector_equal(vec1, vec2) && "Large vectors should not be equal");
}

// Register tests
TEST(test_vector_equal_basic_equal);
TEST(test_vector_equal_basic_not_equal);
TEST(test_vector_equal_different_sizes);
TEST(test_vector_equal_empty_vectors);
TEST(test_vector_equal_one_empty);
TEST(test_vector_equal_strings_equal);
TEST(test_vector_equal_strings_not_equal);
TEST(test_vector_equal_custom_objects_equal);
TEST(test_vector_equal_custom_objects_not_equal);
TEST(test_vector_equal_large_equal);
TEST(test_vector_equal_large_not_equal);
#endif
