#pragma once

#include <string>
#include <sstream>
#include <iomanip>

#include "../utils/ERROR.hpp"
#include "is_numeric.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::str {

    string set_precision(double number, int precision) {
        if (precision < 0) throw ERROR("Precision can not be negative: " + to_string(precision));
        stringstream ss;
        ss << fixed << setprecision(precision) << number;
        return ss.str();
    }

    string set_precision(const string& numberStr, int precision) {
        if (!is_numeric(numberStr))
            throw ERROR("Input should be numeric" + (numberStr.empty() ? "" : ": " + numberStr));
        double number = stod(numberStr);
        return set_precision(number, precision);
    }
    
}

#ifdef TEST

using namespace tools::str;

void test_set_precision_double_basic() {
    double number = 3.1415926535;
    int precision = 2;
    string actual = set_precision(number, precision);
    string expected = "3.14";
    assert(actual == expected && "Basic double precision");
}

void test_set_precision_double_zero_precision() {
    double number = 3.1415926535;
    int precision = 0;
    string actual = set_precision(number, precision);
    string expected = "3";
    assert(actual == expected && "Zero precision");
}

void test_set_precision_double_negative_precision() {
    double number = 3.1415926535;
    int precision = -1;
    bool thrown = false;
    try {
        set_precision(number, precision);
    } catch (...) {
        thrown = true;
    }
    assert(thrown && "Negative precision should throw");
}

void test_set_precision_double_large_precision() {
    double number = 3.1415926535;
    int precision = 10;
    string actual = set_precision(number, precision);
    string expected = "3.1415926535";
    assert(actual == expected && "Large precision");
}

void test_set_precision_double_negative_number() {
    double number = -3.1415926535;
    int precision = 3;
    string actual = set_precision(number, precision);
    string expected = "-3.142";
    assert(actual == expected && "Negative number");
}

void test_set_precision_double_zero() {
    double number = 0.0;
    int precision = 2;
    string actual = set_precision(number, precision);
    string expected = "0.00";
    assert(actual == expected && "Zero value");
}

void test_set_precision_string_basic() {
    string numberStr = "3.1415926535";
    int precision = 2;
    string actual = set_precision(numberStr, precision);
    string expected = "3.14";
    assert(actual == expected && "Basic string precision");
}

void test_set_precision_string_invalid_input() {
    string numberStr = "not_a_number";
    int precision = 2;
    bool thrown = false;
    try {
        set_precision(numberStr, precision);
    } catch (...) {
        thrown = true;
    }
    assert(thrown && "Invalid input should throw");
}

void test_set_precision_string_empty_input() {
    string numberStr = "";
    int precision = 2;
    bool thrown = false;
    try {
        set_precision(numberStr, precision);
    } catch (...) {
        thrown = true;
    }
    assert(thrown && "Empty input should throw");
}

void test_set_precision_string_negative_number() {
    string numberStr = "-3.1415926535";
    int precision = 3;
    string actual = set_precision(numberStr, precision);
    string expected = "-3.142";
    assert(actual == expected && "Negative number string");
}

void test_set_precision_string_zero() {
    string numberStr = "0.0";
    int precision = 2;
    string actual = set_precision(numberStr, precision);
    string expected = "0.00";
    assert(actual == expected && "Zero value string");
}


TEST(test_set_precision_double_basic);
TEST(test_set_precision_double_zero_precision);
TEST(test_set_precision_double_negative_precision);
TEST(test_set_precision_double_large_precision);
TEST(test_set_precision_double_negative_number);
TEST(test_set_precision_double_zero);
TEST(test_set_precision_string_basic);
TEST(test_set_precision_string_invalid_input);
TEST(test_set_precision_string_empty_input);
TEST(test_set_precision_string_negative_number);
TEST(test_set_precision_string_zero);
#endif
