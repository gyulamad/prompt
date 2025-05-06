#include "get_hash.h"
#include <functional>

using namespace std;

namespace tools::str {

    string get_hash(const string& str) {
        return to_string(hash<string>{}(str));
    }

} // namespace tools::str