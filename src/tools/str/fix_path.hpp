#pragma once

#include <string>
#include <algorithm>
#include <filesystem>

using namespace std;

namespace fs = filesystem;

namespace tools::str {

    string fix_path(const string& pname) {
        // Convert to filesystem path and normalize it
        fs::path path = fs::path(pname).lexically_normal();
        
        // Convert to string and ensure forward slashes
        string result = path.string();
        replace(result.begin(), result.end(), '\\', '/');
        
        // If the path is empty or just "/", return it as is
        if (result.empty() || result == "/") return result;
    
        // Remove trailing slash if present (unless it's the root)
        if (result.length() > 1 && result.back() == '/') result.pop_back();
    
        return result;
    }
    
}

#ifdef TEST

// #include "../utils/Test.hpp" // Removed include - handled by build system

using namespace tools::str;

void test_fix_path_basic() {
    string actual = fix_path("some/path/./file.txt");
    assert(actual == "some/path/file.txt" && "Basic normalization");
}

void test_fix_path_backslashes() {
    string actual = fix_path("windows\\path\\to\\file.txt");
    assert(actual == "windows/path/to/file.txt" && "Backslashes should be converted to forward slashes");
}

void test_fix_path_mixed_slashes() {
    string actual = fix_path("mixed/path\\to/../file.txt");
    assert(actual == "mixed/file.txt" && "Mixed slashes and parent dir should be handled");
}

void test_fix_path_trailing_slash() {
    string actual = fix_path("some/directory/");
    assert(actual == "some/directory" && "Trailing slash should be removed");
}

void test_fix_path_multiple_trailing_slashes() {
    // Note: fs::path normalization might handle this implicitly
    string actual = fix_path("some/directory///");
    assert(actual == "some/directory" && "Multiple trailing slashes should be removed");
}

void test_fix_path_root_with_trailing_slash() {
    string actual = fix_path("/");
    assert(actual == "/" && "Root path '/' should remain unchanged");
}

void test_fix_path_empty_string() {
    string actual = fix_path("");
    assert(actual == "" && "Empty string should remain empty");
}

void test_fix_path_dot() {
    string actual = fix_path(".");
    assert(actual == "." && "'.' should remain '.'");
}

void test_fix_path_double_dot() {
    string actual = fix_path("..");
    assert(actual == ".." && "'..' should remain '..'");
}

void test_fix_path_complex_dots() {
    string actual = fix_path("/a/b/../../c/./d");
    assert(actual == "/c/d" && "Complex dot navigation should be normalized");
}

// Register tests
TEST(test_fix_path_basic);
TEST(test_fix_path_backslashes);
TEST(test_fix_path_mixed_slashes);
TEST(test_fix_path_trailing_slash);
TEST(test_fix_path_multiple_trailing_slashes);
TEST(test_fix_path_root_with_trailing_slash);
TEST(test_fix_path_empty_string);
TEST(test_fix_path_dot);
TEST(test_fix_path_double_dot);
TEST(test_fix_path_complex_dots);


#endif
