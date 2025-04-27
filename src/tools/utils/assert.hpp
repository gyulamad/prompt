#pragma once

#include "ERROR.hpp"

namespace tools::utils {

#ifdef TEST_CASSERT
#include <cassert>
#else
#ifdef assert
#undef assert
#endif
// void assert(bool expr) { if (!(expr)) throw ERROR("Assert failed"; }
#define assert(expr) if (!(expr)) throw ERROR("Assert failed: "#expr);
#endif

}