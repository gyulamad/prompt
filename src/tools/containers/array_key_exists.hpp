#pragma once

#include <map>
#include <unordered_map>

using namespace std;

namespace tools::containers {

    // Concept to check if a type is a map-like container
    template<typename T, typename = void>
    struct is_map_like : false_type {};

    template<typename K, typename V>
    struct is_map_like<map<K, V>> : true_type {};

    template<typename K, typename V>
    struct is_map_like<unordered_map<K, V>> : true_type {};

    // Template function with SFINAE constraint
    template<typename Container, typename Key>
    typename enable_if<is_map_like<Container>::value, bool>::type
    array_key_exists(const Key& key, const Container& container) {
        return container.find(key) != container.end();
    }
    
}

#ifdef TEST

using namespace tools::containers;

// Test cases for array_key_exists with std::map
void test_array_key_exists_map_exists() {
    map<string, int> m = {{"one", 1}, {"two", 2}};
    bool actual = array_key_exists("one", m);
    assert(actual == true && "Key 'one' should exist in std::map");
}

void test_array_key_exists_map_not_exists() {
    map<string, int> m = {{"one", 1}, {"two", 2}};
    bool actual = array_key_exists("three", m);
    assert(actual == false && "Key 'three' should not exist in std::map");
}

void test_array_key_exists_map_empty() {
    map<string, int> m;
    bool actual = array_key_exists("one", m);
    assert(actual == false && "Key 'one' should not exist in empty std::map");
}

// Test cases for array_key_exists with std::unordered_map
void test_array_key_exists_unordered_map_exists() {
    unordered_map<string, int> m = {{"one", 1}, {"two", 2}};
    bool actual = array_key_exists("two", m);
    assert(actual == true && "Key 'two' should exist in std::unordered_map");
}

void test_array_key_exists_unordered_map_not_exists() {
    unordered_map<string, int> m = {{"one", 1}, {"two", 2}};
    bool actual = array_key_exists("three", m);
    assert(actual == false && "Key 'three' should not exist in std::unordered_map");
}

void test_array_key_exists_unordered_map_empty() {
    unordered_map<string, int> m;
    bool actual = array_key_exists("one", m);
    assert(actual == false && "Key 'one' should not exist in empty std::unordered_map");
}

// Register tests
TEST(test_array_key_exists_map_exists);
TEST(test_array_key_exists_map_not_exists);
TEST(test_array_key_exists_map_empty);
TEST(test_array_key_exists_unordered_map_exists);
TEST(test_array_key_exists_unordered_map_not_exists);
TEST(test_array_key_exists_unordered_map_empty);

#endif
