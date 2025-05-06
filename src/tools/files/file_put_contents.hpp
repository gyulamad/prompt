#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include "../utils/ERROR.h"

using namespace std;
using namespace tools::utils;

namespace tools::files {

    bool file_put_contents(const string& filename, const string& content, bool append = false, bool throws = false) {
        // Open the file in the appropriate mode
        ios_base::openmode mode = ios::out | ios::binary;
        if (append) {
            mode |= ios::app; // Append to the file if it exists
        }

        ofstream file(filename, mode);
        if (!file.is_open()) {
            if (throws) throw ERROR("Failed to open file: " + filename);
            else return false;
        }

        // Write the content to the file
        file.write(content.data(), content.size());

        // Check if the write operation failed
        if (file.fail()) {
            file.close(); // Close the file before throwing the exception
            if (throws) throw ERROR("Failed to write to file: " + filename);
            else return false;
        }

        // Flush the stream to ensure the data is written to the file
        file.flush();

        // Close the file
        file.close();

        return true;
    }

}