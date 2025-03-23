#pragma once

#include <type_traits>

using namespace std;

namespace tools::containers {

    template <typename T, typename = void>
    struct has_key_type : false_type {};

    template <typename T>
    struct has_key_type<T, void_t<typename T::key_type>> : true_type {};

    template <typename Container, typename = void>
    struct KeyTypeTrait {
        using type = size_t;
    };

    template <typename Container>
    struct KeyTypeTrait<Container, void_t<typename Container::key_type>> {
        using type = typename Container::key_type;
    };

    template <typename Container>
    auto array_keys(const Container& container) {
        using KeyType = typename KeyTypeTrait<Container>::type;
        vector<KeyType> keys;

        if constexpr (has_key_type<Container>::value) {
            for (const auto& element : container)
                keys.push_back(element.first);
        } else {
            for (size_t i = 0; i < container.size(); ++i)
                keys.push_back(i);
        }

        return keys;
    }
    
}

#ifdef TEST

using namespace tools::containers;


#endif
