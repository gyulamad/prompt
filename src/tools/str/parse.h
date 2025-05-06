#pragma once

#include "../utils/ERROR.h"
#include "../str/str_cut_end.h"
#include "../str/str_contains.h"
#include "../containers/in_array.h"
#include <string>
#include <type_traits>
#include <sstream>
#include <algorithm>
#include <vector>

using namespace std;
using namespace tools::utils;
using namespace tools::str;
using namespace tools::containers;

namespace tools::str {

    template <typename T>
    T parse(const string& str) {
        if constexpr (is_same_v<T, string>) {
            return str; // Return the string directly
        } else {
            static_assert(is_arithmetic<T>::value, "T must be an arithmetic type");
            stringstream ss(str);
            T num;
            if (ss >> num) return num;
            throw ERROR("Invalid input string (not a number): " + (
                str.empty() ? "<empty>" : str_cut_end(str))
            );
        }
    }

    // Specialization for bool
    template<>
    bool parse<bool>(const string& str);
    
} // namespace tools::str

#ifdef TEST
// Test function declarations (in global namespace)
void test_parse_valid_integer();
void test_parse_valid_double();
void test_parse_negative_number();
void test_parse_invalid_input();
void test_parse_empty_string();
void test_parse_whitespace_only();
void test_parse_trailing_characters();
void test_parse_trailing_characters_negative();
void test_parse_floating_point_with_exponent();
void test_parse_zero();
void test_parse_large_number();
void test_parse_invalid_boolean();
#endif