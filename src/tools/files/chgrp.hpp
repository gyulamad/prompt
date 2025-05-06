#pragma once

#include <string>
#include <cstring>     // Added for strerror
#include <grp.h>
#include <unistd.h>
#include "../utils/ERROR.h"
#include "../str/to_string.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::str;

namespace tools::files {

    void chgrp(const string& filename, const string& groupname) {
        // 1. Get the GID of the shared group.
        struct group *grp = getgrnam(groupname.c_str());
        if (grp == nullptr)
            throw ERROR("Error: Could not find group " + groupname);
            
        gid_t shared_gid = grp->gr_gid;
    
        // 2. Change the group ownership of the file.
        if (::chown(filename.c_str(), -1, shared_gid) == -1) // -1 means don't change the user
            throw ERROR("Error changing group of " + filename + ": " + to_string(strerror(errno)));
    }

}