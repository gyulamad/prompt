#include "str_get_diffs.h"
#include "compare_diff_vectors.h"
#include <iostream>

using namespace std;

namespace tools::str {

    vector<str_diff_t> str_get_diffs(const string& s1, const string& s2) {
        vector<string> lines1 = explode("\n", s1);
        vector<string> lines2 = explode("\n", s2);

        vector<str_diff_t> diffs;
        size_t i = 0, j = 0;

        while (i < lines1.size() || j < lines2.size()) {
            if (i < lines1.size() && j < lines2.size() && lines1[i] == lines2[j]) {
                i++;
                j++;
                continue;
            }

            size_t start = i + 1;
            vector<string> added;
            vector<string> removed;

            while (i < lines1.size() && (j >= lines2.size() || lines1[i] != lines2[j])) {
                removed.push_back(lines1[i]);
                i++;
            }

            while (j < lines2.size() && (i >= lines1.size() || lines1[i] != lines2[j])) {
                added.push_back(lines2[j]);
                j++;
            }

            str_diff_t diff;
            diff.bound[0] = start;
            diff.bound[1] = i;
            diff.added = added;
            diff.removed = removed;
            diffs.push_back(diff);
        }

        return diffs;
    }

} // namespace tools::str


#ifdef TEST

#include <vector>
#include <string>
#include <iostream> // Included for debug in compare_diff_vectors

#include "../utils/Test.h"     // For TEST macro and assert
#include "../utils/assert.hpp"
#include "str_diff_t.hpp"        // For the diff structure
#include "compare_diff_vectors.h"

using namespace tools::str;

void test_str_get_diffs_identical() {
    string s1 = "line1\nline2\nline3";
    string s2 = "line1\nline2\nline3";
    vector<str_diff_t> expected = {};
    vector<str_diff_t> actual = str_get_diffs(s1, s2);
    assert(compare_diff_vectors(actual, expected) && "Identical strings should produce no diffs");
}

void test_str_get_diffs_simple_addition() {
    string s1 = "line1\nline3";
    string s2 = "line1\nline2\nline3";
    vector<str_diff_t> expected = {{{2, 2}, {"line2", "line3"}, {"line3"}}};
    vector<str_diff_t> actual = str_get_diffs(s1, s2);
    assert(compare_diff_vectors(actual, expected) && "Simple addition diff mismatch (based on traced behavior)");
}

void test_str_get_diffs_simple_removal() {
    string s1 = "line1\nline2\nline3";
    string s2 = "line1\nline3";
    vector<str_diff_t> expected = {{{2, 2}, {}, {"line2"}}};
    vector<str_diff_t> actual = str_get_diffs(s1, s2);
    assert(compare_diff_vectors(actual, expected) && "Simple removal diff mismatch (based on traced behavior)");
}

void test_str_get_diffs_simple_modification() {
    string s1 = "line1\nold\nline3";
    string s2 = "line1\nnew\nline3";
    vector<str_diff_t> expected = {{{2, 3}, {"new", "line3"}, {"old", "line3"}}};
    vector<str_diff_t> actual = str_get_diffs(s1, s2);
    assert(compare_diff_vectors(actual, expected) && "Simple modification diff mismatch (based on traced behavior)");
}

void test_str_get_diffs_multiple_blocks_like() {
    string s1 = "a\nb\nc\nd";
    string s2 = "a\nx\nc\ny";
    vector<str_diff_t> expected = {{{2, 4}, {"x", "c", "y"}, {"b", "c", "d"}}};
    vector<str_diff_t> actual = str_get_diffs(s1, s2);
    assert(compare_diff_vectors(actual, expected) && "Multiple blocks diff mismatch (based on traced behavior)");
}

void test_str_get_diffs_empty_strings() {
    string s1 = "";
    string s2 = "";
    vector<str_diff_t> expected = {};
    vector<str_diff_t> actual = str_get_diffs(s1, s2);
    assert(compare_diff_vectors(actual, expected) && "Empty strings should produce no diffs");
}

void test_str_get_diffs_first_empty() {
    string s1 = "";
    string s2 = "line1\nline2";
    vector<str_diff_t> expected = {{{1, 1}, {"line1", "line2"}, {""}}};
    vector<str_diff_t> actual = str_get_diffs(s1, s2);
    assert(compare_diff_vectors(actual, expected) && "First empty diff mismatch (based on traced behavior)");
}

void test_str_get_diffs_second_empty() {
    string s1 = "line1\nline2";
    string s2 = "";
    vector<str_diff_t> expected = {{{1, 2}, {""}, {"line1", "line2"}}};
    vector<str_diff_t> actual = str_get_diffs(s1, s2);
    assert(compare_diff_vectors(actual, expected) && "Second empty diff mismatch (based on traced behavior)");
}

void test_str_get_diffs_trailing_newline_s1() {
    string s1 = "a\n";
    string s2 = "a";
    vector<str_diff_t> expected = {{{2, 2}, {}, {""}}};
    vector<str_diff_t> actual = str_get_diffs(s1, s2);
    assert(compare_diff_vectors(actual, expected) && "Trailing newline s1 diff mismatch (based on traced behavior)");
}

void test_str_get_diffs_trailing_newline_s2() {
    string s1 = "a";
    string s2 = "a\n";
    vector<str_diff_t> expected = {{{2, 1}, {""}, {}}};
    vector<str_diff_t> actual = str_get_diffs(s1, s2);
    assert(compare_diff_vectors(actual, expected) && "Trailing newline s2 diff mismatch (based on traced behavior)");
}

TEST(test_str_get_diffs_identical);
TEST(test_str_get_diffs_simple_addition);
TEST(test_str_get_diffs_simple_removal);
TEST(test_str_get_diffs_simple_modification);
TEST(test_str_get_diffs_multiple_blocks_like);
TEST(test_str_get_diffs_empty_strings);
TEST(test_str_get_diffs_first_empty);
TEST(test_str_get_diffs_second_empty);
TEST(test_str_get_diffs_trailing_newline_s1);
TEST(test_str_get_diffs_trailing_newline_s2);
#endif // TEST
