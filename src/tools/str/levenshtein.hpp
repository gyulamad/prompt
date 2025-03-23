#pragma once

#include <string>
#include <vector>
#include <algorithm>

using namespace std;

namespace tools::str {

    int levenshtein(const string &s1, const string &s2) {
        int m = s1.length();
        int n = s2.length();

        // Create a matrix to store distances between prefixes of s1 and s2
        vector<vector<int>> dp(m + 1, vector<int>(n + 1));

        // Initialize the first row and column of the matrix
        for (int i = 0; i <= m; ++i) dp[i][0] = i;
        for (int j = 0; j <= n; ++j) dp[0][j] = j;

        // Fill in the rest of the matrix using dynamic programming
        for (int i = 1; i <= m; ++i)
            for (int j = 1; j <= n; ++j) {
                int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1; // Cost of substitution
                dp[i][j] = min({
                    dp[i - 1][j] + 1,       // Deletion
                    dp[i][j - 1] + 1,       // Insertion
                    dp[i - 1][j - 1] + cost // Substitution
                });
            }

        return dp[m][n]; // The Levenshtein distance is in the bottom-right cell
    }

}

#ifdef TEST

using namespace tools::str;

void test_levenshtein_same_string() {
    string s1 = "hello";
    string s2 = "hello";
    int actual = levenshtein(s1, s2);
    int expected = 0;
    assert(actual == expected && "Same string should have distance 0");
}

void test_levenshtein_empty_string() {
    string s1 = "";
    string s2 = "hello";
    int actual = levenshtein(s1, s2);
    int expected = 5;
    assert(actual == expected && "Distance between empty and non-empty string");
}

void test_levenshtein_one_empty_string() {
    string s1 = "hello";
    string s2 = "";
    int actual = levenshtein(s1, s2);
    int expected = 5;
    assert(actual == expected && "Distance between non-empty and empty string");
}

void test_levenshtein_single_insertion() {
    string s1 = "kitten";
    string s2 = "kittens";
    int actual = levenshtein(s1, s2);
    int expected = 1;
    assert(actual == expected && "Single insertion");
}

void test_levenshtein_single_deletion() {
    string s1 = "kittens";
    string s2 = "kitten";
    int actual = levenshtein(s1, s2);
    int expected = 1;
    assert(actual == expected && "Single deletion");
}

void test_levenshtein_single_substitution() {
    string s1 = "kitten";
    string s2 = "sitten";
    int actual = levenshtein(s1, s2);
    int expected = 1;
    assert(actual == expected && "Single substitution");
}

void test_levenshtein_multiple_operations() {
    string s1 = "sitting";
    string s2 = "kitten";
    int actual = levenshtein(s1, s2);
    int expected = 3;
    assert(actual == expected && "Multiple operations (substitution, insertion, deletion)");
}

void test_levenshtein_case_sensitivity() {
    string s1 = "Hello";
    string s2 = "hello";
    int actual = levenshtein(s1, s2);
    int expected = 1;
    assert(actual == expected && "Case sensitivity");
}

void test_levenshtein_unicode() {
    string s1 = "café";
    string s2 = "coffee";
    int actual = levenshtein(s1, s2);
    int expected = 4;
    assert(actual == expected && "Unicode characters");
}


TEST(test_levenshtein_same_string);
TEST(test_levenshtein_empty_string);
TEST(test_levenshtein_one_empty_string);
TEST(test_levenshtein_single_insertion);
TEST(test_levenshtein_single_deletion);
TEST(test_levenshtein_single_substitution);
TEST(test_levenshtein_multiple_operations);
TEST(test_levenshtein_case_sensitivity);
TEST(test_levenshtein_unicode);
#endif
