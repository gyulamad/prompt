#include <string>
#include <fstream>

#include "../utils/ERROR.h"

using namespace std;
using namespace tools::utils;

namespace tools::files {
    bool file_put_contents(const string& filename, const string& content, bool append = false, bool throws = false);
}