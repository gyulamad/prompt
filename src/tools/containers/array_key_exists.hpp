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


#endif
