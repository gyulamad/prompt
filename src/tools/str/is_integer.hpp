#pragma once

#include <string>
#include <algorithm>

using namespace std;

namespace tools::str {

    bool is_integer(const string& s) {
        if (s.empty()) return false;
        size_t start = (s[0] == '+' || s[0] == '-') ? 1 : 0;
        return s.length() > start && all_of(s.begin() + start, s.end(), ::isdigit);
    }
    
}

#ifdef TEST

using namespace tools::str;

void test_is_integer_valid_positive_integer() {
    assert(is_integer("123") && "test_is_integer_valid_positive_integer failed");
}

void test_is_integer_valid_negative_integer() {
    assert(is_integer("-123") && "test_is_integer_valid_negative_integer failed");
}

void test_is_integer_valid_positive_with_plus() {
    assert(is_integer("+123") && "test_is_integer_valid_positive_with_plus failed");
}

void test_is_integer_empty_string() {
    assert(!is_integer("") && "test_is_integer_empty_string failed");
}

void test_is_integer_only_sign() {
    assert(!is_integer("+") && "test_is_integer_only_sign failed");
    assert(!is_integer("-") && "test_is_integer_only_sign failed");
}

void test_is_integer_invalid_characters() {
    assert(!is_integer("123abc") && "test_is_integer_invalid_characters failed");
    assert(!is_integer("12.34") && "test_is_integer_invalid_characters failed");
    assert(!is_integer("123!") && "test_is_integer_invalid_characters failed");
}

void test_is_integer_whitespace() {
    assert(!is_integer(" 123 ") && "test_is_integer_whitespace failed");
    assert(!is_integer("\t123\n") && "test_is_integer_whitespace failed");
}

void test_is_integer_leading_zeros() {
    assert(is_integer("00123") && "test_is_integer_leading_zeros failed");
}

void test_is_integer_zero() {
    assert(is_integer("0") && "test_is_integer_zero failed");
}


TEST(test_is_integer_valid_positive_integer);
TEST(test_is_integer_valid_negative_integer);
TEST(test_is_integer_valid_positive_with_plus);
TEST(test_is_integer_empty_string);
TEST(test_is_integer_only_sign);
TEST(test_is_integer_invalid_characters);
TEST(test_is_integer_whitespace);
TEST(test_is_integer_leading_zeros);
TEST(test_is_integer_zero);
#endif
