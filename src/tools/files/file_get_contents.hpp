#pragma once

#include <fstream>
#include "../utils/ERROR.h"

using namespace std;
using namespace tools::utils;

namespace tools::files {

    string file_get_contents(const string& filename) {
        // Open the file in binary mode and position the cursor at the end
        ifstream file(filename, ios::in | ios::binary);
        if (!file.is_open()) {
            throw ERROR("Failed to open file: " + filename);
        }

        // Seek to the end to determine file size
        file.seekg(0, ios::end);
        streamsize size = file.tellg();
        file.seekg(0, ios::beg);

        // Read file content into a string
        string content(size, '\0');
        if (!file.read(&content[0], size)) {
            throw ERROR("Failed to read file: " + filename);
        }

        return content;
    }

}