#pragma once

#include <iostream>
#include <string>
#include <cstring>     // Added for strerror
#include <fstream>
#include <filesystem> // Requires C++17 or later
#include <stdexcept>
#include <chrono>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <sys/stat.h> // For chmod (permissions)
#include <sys/types.h>

#include "../str/str_cut_begin.hpp"
#include "../str/to_string.hpp"
#include "ERROR.h"
#include "time.hpp"

using namespace std;
using namespace tools::str;

namespace fs = filesystem; // Alias for convenience

namespace tools::utils { //  TODO: break down this functions into a tools::files namespace



}
