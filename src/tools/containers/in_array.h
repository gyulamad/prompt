#pragma once

#include <tuple>

using namespace std;

namespace tools::containers {

    template <typename T>
    struct is_pair : false_type {};

    template <typename T1, typename T2>
    struct is_pair<pair<T1, T2>> : true_type {};

    template <typename Needle, typename Container>
    bool in_array(const Needle& needle, const Container& container) {
        using Element = typename Container::value_type;

        if constexpr (is_pair<Element>::value) {
            // Check values in associative containers (maps, unordered_maps)
            for (const auto& element : container)
                if (element.second == needle) return true;
        } else {
            // Check elements in sequence containers (vectors, lists, etc.)
            for (const auto& element : container)
                if (element == needle) return true;
        }

        return false;
    }
    
} // namespace tools::containers

#ifdef TEST
// Test function declarations (in global namespace)
void test_in_array_integer_vector_exists();
void test_in_array_integer_vector_not_exists();
void test_in_array_integer_vector_empty();
void test_in_array_string_vector_exists();
void test_in_array_string_vector_not_exists();
void test_in_array_string_vector_empty();
void test_in_array_map_exists();
void test_in_array_map_not_exists();
void test_in_array_map_empty();
void test_in_array_unordered_map_exists();
void test_in_array_unordered_map_not_exists();
void test_in_array_unordered_map_empty();
void test_in_array_custom_object_exists();
void test_in_array_custom_object_not_exists();
void test_in_array_custom_object_vector_empty();
#endif