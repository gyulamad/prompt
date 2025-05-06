#include <string>
#include <vector>

#include "explode.h"
#include "implode.h"
#include "remove_extension.h"

using namespace std;

namespace tools::str {
    string remove_extension(const string& fname) {
        vector<string> splits = explode(".", fname);
        splits.pop_back();
        return implode(".", splits);
    }
}