#include "str_contains.h"

using namespace std;

namespace tools::str {

    bool str_contains(const string& str, const string& substring) {
        // Use string::find to check if the substring exists
        return str.find(substring) != string::npos;
    }

} // namespace tools::str