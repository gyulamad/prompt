#include <string>

#include "remove_extension.h"
#include "str_starts_with.h"
#include "replace_extension.h"

using namespace std;

namespace tools::str {
    string replace_extension(const string& fname, const string& newext) {
        return remove_extension(fname) + (!newext.empty() ? str_starts_with(newext, ".") ? newext : ("." + newext) : "");
    }
}