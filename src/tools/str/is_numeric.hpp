#pragma once

#include <string>

using namespace std;

namespace tools::str {

    bool is_numeric(const string& s) {
        if (s.empty()) return false;
        
        size_t start = (s[0] == '+' || s[0] == '-') ? 1 : 0;
        bool hasDecimal = false;
        
        for (size_t i = start; i < s.length(); i++) {
            if (s[i] == '.') {
                if (hasDecimal) return false;  // Multiple decimals
                hasDecimal = true;
                continue;
            }
            if (!isdigit(s[i])) return false;
        }
        
        return s.length() > start && !s.ends_with(".");
    }
    
}

#ifdef TEST

using namespace tools::str;

void test_is_numeric_valid_integer() {
    assert(is_numeric("123") && "test_is_numeric_valid_integer failed");
}

void test_is_numeric_valid_decimal() {
    assert(is_numeric("123.456") && "test_is_numeric_valid_decimal failed");
}

void test_is_numeric_negative_integer() {
    assert(is_numeric("-123") && "test_is_numeric_negative_integer failed");
}

void test_is_numeric_positive_integer() {
    assert(is_numeric("+123") && "test_is_numeric_positive_integer failed");
}

void test_is_numeric_negative_decimal() {
    assert(is_numeric("-123.456") && "test_is_numeric_negative_decimal failed");
}

void test_is_numeric_positive_decimal() {
    assert(is_numeric("+123.456") && "test_is_numeric_positive_decimal failed");
}

void test_is_numeric_empty_string() {
    assert(!is_numeric("") && "test_is_numeric_empty_string failed");
}

void test_is_numeric_only_sign() {
    assert(!is_numeric("+") && "test_is_numeric_only_sign failed");
    assert(!is_numeric("-") && "test_is_numeric_only_sign failed");
}

void test_is_numeric_multiple_decimals() {
    assert(!is_numeric("123.45.67") && "test_is_numeric_multiple_decimals failed");
}

void test_is_numeric_trailing_decimal() {
    assert(!is_numeric("123.") && "test_is_numeric_trailing_decimal failed");
}

void test_is_numeric_leading_decimal() {
    assert(is_numeric(".123") && "test_is_numeric_leading_decimal failed");
}

void test_is_numeric_invalid_characters() {
    assert(!is_numeric("123abc") && "test_is_numeric_invalid_characters failed");
    assert(!is_numeric("12.34.56") && "test_is_numeric_invalid_characters failed");
    assert(!is_numeric("123!") && "test_is_numeric_invalid_characters failed");
}

void test_is_numeric_whitespace() {
    assert(!is_numeric(" 123 ") && "test_is_numeric_whitespace failed");
    assert(!is_numeric("\t123\n") && "test_is_numeric_whitespace failed");
}

TEST(test_is_numeric_valid_integer);
TEST(test_is_numeric_valid_decimal);
TEST(test_is_numeric_negative_integer);
TEST(test_is_numeric_positive_integer);
TEST(test_is_numeric_negative_decimal);
TEST(test_is_numeric_positive_decimal);
TEST(test_is_numeric_empty_string);
TEST(test_is_numeric_only_sign);
TEST(test_is_numeric_multiple_decimals);
TEST(test_is_numeric_trailing_decimal);
TEST(test_is_numeric_leading_decimal);
TEST(test_is_numeric_invalid_characters);
TEST(test_is_numeric_whitespace);
#endif
