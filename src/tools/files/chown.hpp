#pragma once

#include <string>
#include <cstring>     // Added for strerror
#include <pwd.h>
#include <unistd.h>
#include "../utils/ERROR.h"
#include "../str/to_string.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::str;

namespace tools::files {

    // Function to change the owner of a file
    void chown(const string& filename, const string& new_owner_username) {
        // 1. Get the UID of the new owner.
        struct passwd *pwd = getpwnam(new_owner_username.c_str());
        if (pwd == nullptr)
            throw ERROR("Error: Could not find user " + new_owner_username);
            
        uid_t new_owner_uid = pwd->pw_uid;

        // 2. Change the owner of the file
        if (::chown(filename.c_str(), new_owner_uid, -1) == -1)  // -1 means don't change the group
            throw ERROR("Error changing owner of " + filename + ": " + to_string(strerror(errno)));
    }

}