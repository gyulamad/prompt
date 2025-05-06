#include <string>
#include <iostream>
#include <fstream>

#include "../utils/ERROR.h"
#include "file_put_contents.h"

using namespace std;
using namespace tools::utils;

namespace tools::files {
    bool file_put_contents(const string& filename, const string& content, bool append, bool throws) {
        ios_base::openmode mode = ios::out | ios::binary;
        if (append) mode |= ios::app;
        ofstream file(filename, mode);
        if (!file.is_open()) {
            if (throws) throw ERROR("Failed to open file: " + filename);
            else return false;
        }
        file.write(content.data(), content.size());
        if (file.fail()) {
            file.close();
            if (throws) throw ERROR("Failed to write to file: " + filename);
            else return false;
        }
        file.flush();
        file.close();
        return true;
    }
}