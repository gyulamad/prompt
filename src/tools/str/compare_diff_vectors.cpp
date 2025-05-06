#include "compare_diff_vectors.h"

using namespace std;

namespace tools::str {

    bool compare_diff_vectors(const vector<str_diff_t>& actual, const vector<str_diff_t>& expected) {
        if (actual.size() != expected.size()) return false;
        for (size_t i = 0; i < actual.size(); ++i) {
            if (actual[i].bound[0] != expected[i].bound[0] ||
                actual[i].bound[1] != expected[i].bound[1] ||
                actual[i].added != expected[i].added ||       // Assumes vector<string> comparison works
                actual[i].removed != expected[i].removed) {   // Assumes vector<string> comparison works
                return false;
            }
        }
        return true;
    }
}


#ifdef TEST

#include "../utils/Test.h"
#include "../utils/assert.hpp"

using namespace tools::str;

void test_compare_diff_vectors_identical() {
    vector<str_diff_t> actual = {
        {{1, 2}, {"added_line"}, {"removed_line"}},
        {{3, 4}, {"another_added"}, {"another_removed"}}
    };
    vector<str_diff_t> expected = {
        {{1, 2}, {"added_line"}, {"removed_line"}},
        {{3, 4}, {"another_added"}, {"another_removed"}}
    };
    bool result = compare_diff_vectors(actual, expected);
    assert(result == true && "Identical diff vectors should return true");
}

void test_compare_diff_vectors_different_sizes() {
    vector<str_diff_t> actual = {
        {{1, 2}, {"added_line"}, {"removed_line"}}
    };
    vector<str_diff_t> expected = {
        {{1, 2}, {"added_line"}, {"removed_line"}},
        {{3, 4}, {"another_added"}, {"another_removed"}}
    };
    bool result = compare_diff_vectors(actual, expected);
    assert(result == false && "Vectors with different sizes should return false");
}

void test_compare_diff_vectors_different_bounds() {
    vector<str_diff_t> actual = {
        {{1, 2}, {"added_line"}, {"removed_line"}}
    };
    vector<str_diff_t> expected = {
        {{2, 3}, {"added_line"}, {"removed_line"}}
    };
    bool result = compare_diff_vectors(actual, expected);
    assert(result == false && "Vectors with different bounds should return false");
}

void test_compare_diff_vectors_different_added() {
    vector<str_diff_t> actual = {
        {{1, 2}, {"added_line"}, {"removed_line"}}
    };
    vector<str_diff_t> expected = {
        {{1, 2}, {"different_added"}, {"removed_line"}}
    };
    bool result = compare_diff_vectors(actual, expected);
    assert(result == false && "Vectors with different added lines should return false");
}

void test_compare_diff_vectors_different_removed() {
    vector<str_diff_t> actual = {
        {{1, 2}, {"added_line"}, {"removed_line"}}
    };
    vector<str_diff_t> expected = {
        {{1, 2}, {"added_line"}, {"different_removed"}}
    };
    bool result = compare_diff_vectors(actual, expected);
    assert(result == false && "Vectors with different removed lines should return false");
}

void test_compare_diff_vectors_empty() {
    vector<str_diff_t> actual = {};
    vector<str_diff_t> expected = {};
    bool result = compare_diff_vectors(actual, expected);
    assert(result == true && "Empty diff vectors should return true");
}

// Register tests
TEST(test_compare_diff_vectors_identical);
TEST(test_compare_diff_vectors_different_sizes);
TEST(test_compare_diff_vectors_different_bounds);
TEST(test_compare_diff_vectors_different_added);
TEST(test_compare_diff_vectors_different_removed);
TEST(test_compare_diff_vectors_empty);

#endif
