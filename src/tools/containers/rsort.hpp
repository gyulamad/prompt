#pragma once

#include <vector>

using namespace std;

namespace tools::containers {

    template <typename T>
    void rsort(vector<T>& vec) {
        sort(vec.begin(), vec.end(), greater<T>()); // Sorts in descending order
    }
    
}

#ifdef TEST

using namespace tools::containers;

void test_rsort_basic() {
    vector<int> vec = {3, 1, 4, 1, 5, 9, 2, 6};
    rsort(vec);
    vector<int> expected = {9, 6, 5, 4, 3, 2, 1, 1};
    assert(vec == expected && "Basic reverse sort (descending)");
}

void test_rsort_empty_vector() {
    vector<int> vec;
    rsort(vec);
    assert(vec.empty() && "Empty vector reverse sort");
}

void test_rsort_single_element() {
    vector<int> vec = {42};
    rsort(vec);
    vector<int> expected = {42};
    assert(vec == expected && "Single element reverse sort");
}

void test_rsort_strings() {
    vector<string> vec = {"banana", "apple", "cherry"};
    rsort(vec);
    vector<string> expected = {"cherry", "banana", "apple"};
    assert(vec == expected && "Reverse sort strings (descending)");
}

TEST(test_rsort_basic);
TEST(test_rsort_empty_vector);
TEST(test_rsort_single_element);
TEST(test_rsort_strings);
#endif
