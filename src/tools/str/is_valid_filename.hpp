#pragma once

#include <string>

using namespace std;

namespace tools::str {

    bool is_valid_filename(const string& filename) {
        // List of invalid characters (you can customize this)
        string invalidChars = "\\/:*?\"<>|";
      
        // Check for empty filename
        if (filename.empty()) return false;
      
        // Check if filename contains any invalid characters
        for (char c : invalidChars)
          if (filename.find(c) != string::npos) return false;
      
        // Check for reserved names (CON, PRN, AUX, NUL, COM1, COM2, etc.) - Optional
        // This is a more advanced check and might not be necessary for all systems
      
        // If all checks pass, the filename is considered valid
        return true;
    }
    
}

#ifdef TEST

using namespace tools::str;

// Test cases for is_valid_filename
void test_is_valid_filename_valid() {
    assert(is_valid_filename("file.txt") && "test_is_valid_filename_valid failed");
}

void test_is_valid_filename_invalid_characters() {
    assert(!is_valid_filename("file/name.txt") && "test_is_valid_filename_invalid_characters failed");
    assert(!is_valid_filename("file<name>.txt") && "test_is_valid_filename_invalid_characters failed");
    assert(!is_valid_filename("file:name.txt") && "test_is_valid_filename_invalid_characters failed");
    assert(!is_valid_filename("file?name.txt") && "test_is_valid_filename_invalid_characters failed");
    assert(!is_valid_filename("file*name.txt") && "test_is_valid_filename_invalid_characters failed");
    assert(!is_valid_filename("file\"name.txt") && "test_is_valid_filename_invalid_characters failed");
    assert(!is_valid_filename("file|name.txt") && "test_is_valid_filename_invalid_characters failed");
}

void test_is_valid_filename_empty_string() {
    assert(!is_valid_filename("") && "test_is_valid_filename_empty_string failed");
}

void test_is_valid_filename_with_path_separator() {
    assert(!is_valid_filename("folder/file.txt") && "test_is_valid_filename_with_path_separator failed");
}

// Register tests
TEST(test_is_valid_filename_valid);
TEST(test_is_valid_filename_invalid_characters);
TEST(test_is_valid_filename_empty_string);
TEST(test_is_valid_filename_with_path_separator);
#endif
