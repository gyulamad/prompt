#include <string>
#include <vector>

#include "explode.h"
#include "remove_path.h"

using namespace std;

namespace tools::str {
    string remove_path(const string& fname) {
        vector<string> splits = explode("/", fname);
        return splits.back();
    }
}