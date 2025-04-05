#pragma once

#include <unordered_map>
#include <vector>
#include <map>
#include <functional>

using namespace std;

namespace tools::utils {
    // TODO: move these under tools::containers namespace

    // ===============================================================
    // =========================== foreach ===========================
    // ===============================================================

    // TODO: foreach needs tests!!

#define FE_BREAK false
#define FE_CONTINUE true

    // Trait to determine key type and iteration behavior
    template<typename T, typename = void>
    struct ContainerTraits {
        using KeyType = size_t;
        using ItemType = typename T::value_type&;

        static void iterate(T& data, function<void(ItemType)> callback) {
            for (auto& item : data) {
                callback(item);
            }
        }

        static void iterate(T& data, function<void(ItemType, KeyType)> callback) {
            size_t index = 0;
            for (auto& item : data) {
                callback(item, index++);
            }
        }

        static void iterate(T& data, function<bool(ItemType)> callback) {
            for (auto& item : data) {
                if (!callback(item)) {
                    break;
                }
            }
        }

        static void iterate(T& data, function<bool(ItemType, KeyType)> callback) {
            size_t index = 0;
            for (auto& item : data) {
                if (!callback(item, index++)) {
                    break;
                }
            }
        }
    };

    // Specialization for unordered_map
    template<typename K, typename V>
    struct ContainerTraits<unordered_map<K, V>> {
        using KeyType = K;
        using ItemType = V&;

        static void iterate(unordered_map<K, V>& data, function<void(ItemType)> callback) {
            for (auto& pair : data) {
                callback(pair.second);
            }
        }

        static void iterate(unordered_map<K, V>& data, function<void(ItemType, KeyType)> callback) {
            for (auto& pair : data) {
                callback(pair.second, pair.first);
            }
        }

        static void iterate(unordered_map<K, V>& data, function<bool(ItemType)> callback) {
            for (auto& pair : data) {
                if (!callback(pair.second)) {
                    break;
                }
            }
        }

        static void iterate(unordered_map<K, V>& data, function<bool(ItemType, KeyType)> callback) {
            for (auto& pair : data) {
                if (!callback(pair.second, pair.first)) {
                    break;
                }
            }
        }
    };

    // map ---


    // Specialization for vector
    template<typename T>
    struct ContainerTraits<vector<T>> {
        using ItemType = T&;
        using KeyType = size_t;  // Add KeyType for consistency
    
        static void iterate(vector<T>& data, function<void(ItemType)> callback) {
            for (auto& item : data) {
                callback(item);
            }
        }
    
        // Add this new overload for iteration with index
        static void iterate(vector<T>& data, function<void(ItemType, KeyType)> callback) {
            size_t index = 0;
            for (auto& item : data) {
                callback(item, index++);
            }
        }
    
        static void iterate(vector<T>& data, function<bool(ItemType)> callback) {
            for (auto& item : data) {
                if (!callback(item)) {
                    break;
                }
            }
        }
    
        // Optionally, add this for completeness (if you want breaking with index)
        static void iterate(vector<T>& data, function<bool(ItemType, KeyType)> callback) {
            size_t index = 0;
            for (auto& item : data) {
                if (!callback(item, index++)) {
                    break;
                }
            }
        }
    };

    // Specialization for const vector
    template<typename T>
    struct ContainerTraits<const vector<T>> {
        using ItemType = const T&;

        static void iterate(const vector<T>& data, function<void(ItemType)> callback) {
            for (const auto& item : data) {
                callback(item);
            }
        }

        static void iterate(const vector<T>& data, function<bool(ItemType)> callback) {
            for (const auto& item : data) {
                if (!callback(item)) {
                    break;
                }
            }
        }
    };

    // Specialization for map
    template<typename K, typename V>
    struct ContainerTraits<map<K, V>> {
        using KeyType = K;
        using ItemType = V&;

        static void iterate(map<K, V>& data, function<void(ItemType)> callback) {
            for (auto& pair : data) {
                callback(pair.second);
            }
        }

        static void iterate(map<K, V>& data, function<void(ItemType, KeyType)> callback) {
            for (auto& pair : data) {
                callback(pair.second, pair.first);
            }
        }

        static void iterate(map<K, V>& data, function<bool(ItemType)> callback) {
            for (auto& pair : data) {
                if (!callback(pair.second)) {
                    break;
                }
            }
        }

        static void iterate(map<K, V>& data, function<bool(ItemType, KeyType)> callback) {
            for (auto& pair : data) {
                if (!callback(pair.second, pair.first)) {
                    break;
                }
            }
        }
    };// Specialization for const map
    template<typename K, typename V>
    struct ContainerTraits<const std::map<K, V>> {
        using KeyType = const K;
        using ItemType = const V&;

        static void iterate(const std::map<K, V>& data, std::function<void(ItemType)> callback) {
            for (const auto& pair : data) {
                callback(pair.second);
            }
        }

        static void iterate(const std::map<K, V>& data, std::function<void(ItemType, KeyType)> callback) {
            for (const auto& pair : data) {
                callback(pair.second, pair.first);
            }
        }

        static void iterate(const std::map<K, V>& data, std::function<bool(ItemType)> callback) {
            for (const auto& pair : data) {
                if (!callback(pair.second)) {
                    break;
                }
            }
        }

        static void iterate(const std::map<K, V>& data, std::function<bool(ItemType, KeyType)> callback) {
            for (const auto& pair : data) {
                if (!callback(pair.second, pair.first)) {
                    break;
                }
            }
        }
    };


    // --------

    // Helper to detect if a functor returns bool
    template<typename F, typename... Args>
    using returns_bool = is_same<invoke_result_t<F, Args...>, bool>;

    // Foreach overloads with SFINAE
    template<typename T, typename F, enable_if_t<!returns_bool<F, typename ContainerTraits<T>::ItemType>::value, int> = 0>
    void foreach(T& data, F callback) {
        ContainerTraits<T>::iterate(data, function<void(typename ContainerTraits<T>::ItemType)>(callback));
    }

    template<typename T, typename F, enable_if_t<!returns_bool<F, typename ContainerTraits<T>::ItemType, typename ContainerTraits<T>::KeyType>::value, int> = 0>
    void foreach(T& data, F callback) {
        ContainerTraits<T>::iterate(data, function<void(typename ContainerTraits<T>::ItemType, typename ContainerTraits<T>::KeyType)>(callback));
    }

    template<typename T, typename F, enable_if_t<returns_bool<F, typename ContainerTraits<T>::ItemType>::value, int> = 0>
    void foreach(T& data, F callback) {
        ContainerTraits<T>::iterate(data, function<bool(typename ContainerTraits<T>::ItemType)>(callback));
    }

    template<typename T, typename F, enable_if_t<returns_bool<F, typename ContainerTraits<T>::ItemType, typename ContainerTraits<T>::KeyType>::value, int> = 0>
    void foreach(T& data, F callback) {
        ContainerTraits<T>::iterate(data, function<bool(typename ContainerTraits<T>::ItemType, typename ContainerTraits<T>::KeyType)>(callback));
    }


    // --- map

    // Helper to detect if a functor returns bool
    template<typename F, typename... Args>
    using returns_bool = is_same<invoke_result_t<F, Args...>, bool>;

    // ===============================================================
    // ===============================================================
    // ===============================================================

}

#ifdef TEST

#include "Test.hpp"

using namespace tools::utils;

void test_ContainerTraits_vector_iterate_no_index_no_break() {
    vector<int> vec = {1, 2, 3};
    vector<int> result;
    foreach(vec, [&result](int& item) {
        result.push_back(item);
    });
    assert(result == vec && "Should iterate all items without index or break");
}

void test_ContainerTraits_vector_iterate_with_index_no_break() {
    vector<int> vec = {1, 2, 3};
    vector<pair<int, size_t>> result;
    foreach(vec, [&result](int& item, size_t index) {
        result.emplace_back(item, index);
    });
    vector<pair<int, size_t>> expected = {{1, 0}, {2, 1}, {3, 2}};
    assert(result == expected && "Should iterate with correct indices, no break");
}

void test_ContainerTraits_vector_iterate_no_index_with_break() {
    vector<int> vec = {1, 2, 3, 4};
    vector<int> result;
    foreach(vec, [&result](int& item) {
        result.push_back(item);
        return item != 2;  // Break after 2
    });
    vector<int> expected = {1, 2};  // Fixed: Correct initializer list
    assert(result == expected && "Should break after item 2 without index");
}

void test_ContainerTraits_vector_iterate_with_index_with_break() {
    vector<int> vec = {1, 2, 3, 4};
    vector<pair<int, size_t>> result;
    foreach(vec, [&result](int& item, size_t index) {
        result.emplace_back(item, index);
        return item != 3;  // Break after 3
    });
    vector<pair<int, size_t>> expected = {{1, 0}, {2, 1}, {3, 2}};
    assert(result == expected && "Should break after item 3 with index");
}

void test_ContainerTraits_unordered_map_iterate_no_key_no_break() {
    unordered_map<string, int> umap = {{"a", 1}, {"b", 2}, {"c", 3}};
    vector<int> result;
    foreach(umap, [&result](int& value) {
        result.push_back(value);
    });
    vector<int> expected = {1, 2, 3};
    sort(result.begin(), result.end());
    sort(expected.begin(), expected.end());
    assert(result == expected && "Should iterate all values without key or break");
}

void test_ContainerTraits_unordered_map_iterate_with_key_no_break() {
    unordered_map<string, int> umap = {{"a", 1}, {"b", 2}, {"c", 3}};
    vector<pair<string, int>> result;
    foreach(umap, [&result](int& value, const string& key) {
        result.emplace_back(key, value);
    });
    vector<pair<string, int>> expected = {{"a", 1}, {"b", 2}, {"c", 3}};
    sort(result.begin(), result.end());
    sort(expected.begin(), expected.end());
    assert(result == expected && "Should iterate with correct keys, no break");
}

void test_ContainerTraits_unordered_map_iterate_no_key_with_break() {
    unordered_map<string, int> umap = {{"a", 1}, {"b", 2}, {"c", 3}};
    vector<int> result;
    int count = 0;
    foreach(umap, [&result, &count](int& value) {
        result.push_back(value);
        count++;
        return count < 2;  // Break after 2 iterations
    });
    auto actual_size = result.size();
    assert(actual_size == 2 && "Should break after 2 items without key");
}

void test_ContainerTraits_unordered_map_iterate_with_key_with_break() {
    unordered_map<string, int> umap = {{"a", 1}, {"b", 2}, {"c", 3}};
    vector<pair<string, int>> result;
    foreach(umap, [&result](int& value, const string& key) {
        result.emplace_back(key, value);
        return value != 2;  // Break when value is 2
    });
    auto actual_size = result.size();
    bool contains_break_value = false;
    for (const auto& p : result) {
        if (p.second == 2) {
            contains_break_value = true;
            break;
        }
    }
    assert(actual_size <= 3 && contains_break_value && "Should break at value 2 with key");
}


TEST(test_ContainerTraits_vector_iterate_no_index_no_break);
TEST(test_ContainerTraits_vector_iterate_with_index_no_break);
TEST(test_ContainerTraits_vector_iterate_no_index_with_break);
TEST(test_ContainerTraits_vector_iterate_with_index_with_break);
TEST(test_ContainerTraits_unordered_map_iterate_no_key_no_break);
TEST(test_ContainerTraits_unordered_map_iterate_with_key_no_break);
TEST(test_ContainerTraits_unordered_map_iterate_no_key_with_break);
TEST(test_ContainerTraits_unordered_map_iterate_with_key_with_break);
#endif