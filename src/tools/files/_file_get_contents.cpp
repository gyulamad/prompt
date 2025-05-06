#include <fstream>
#include <string>

#include "../utils/ERROR.h"
#include "file_get_contents.h"

using namespace std;
using namespace tools::utils;

namespace tools::files {
    string file_get_contents(const string& filename) {
        ifstream file(filename, ios::in | ios::binary);
        if (!file.is_open()) {
            throw ERROR("Failed to open file: " + filename);
        }
        file.seekg(0, ios::end);
        streamsize size = file.tellg();
        file.seekg(0, ios::beg);
        string content(size, '\0');
        if (!file.read(&content[0], size)) {
            throw ERROR("Failed to read file: " + filename);
        }
        return content;
    }
}