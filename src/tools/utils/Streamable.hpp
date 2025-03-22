#pragma once

#include <iostream>

using namespace std;

namespace tools::utils {

    template<typename T>
    concept Streamable = requires(T t, ostream& os) { os << t; };

    template<typename U, typename = void>
    struct has_ostream : false_type {};
    template<typename U>
    struct has_ostream<U, void_t<decltype(declval<ostream&>() << declval<U>())>> : true_type {};
    
}
