#include <filesystem> // For current_path

#include "../str/fix_path.hpp"

using namespace std;
using namespace tools::str;

namespace fs = filesystem;

namespace tools::utils {

    // Helper to get the expected base path
    string get_cwd() {
        return fix_path(fs::current_path().string());
    }

}