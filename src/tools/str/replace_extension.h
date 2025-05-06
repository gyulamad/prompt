#include <string>

#include "remove_extension.h"
#include "str_starts_with.h"

using namespace std;

namespace tools::str {
    string replace_extension(const string& fname, const string& newext);
}