#pragma once

#include <string>

#include "fix_path.hpp"

using namespace std;

namespace fs = filesystem;

namespace tools::str {

    string get_path(const string& pname) {
        return fix_path(fs::path(pname).parent_path().string());
    }
    
}

#ifdef TEST

using namespace tools::str;

void test_get_path_basic() {
    string actual = get_path("path/to/file.txt");
    assert(actual == "path/to" && "Basic path extraction failed");
}

// void test_get_path_backslashes() {
//     string actual = get_path("path\\to\\file.txt");
//     assert(actual == "path/to" && "Backslash path extraction failed");
// }

// void test_get_path_mixed_slashes() {
//     string actual = get_path("path/to\\file.txt");
//     assert(actual == "path/to" && "Mixed slash path extraction failed");
// }

void test_get_path_needs_normalization() {
    string actual = get_path("path/./to/../file.txt");
    // fs::path("path/./to/../file.txt").parent_path() -> "path/./to/.."
    // fix_path("path/./to/..") -> "path"
    assert(actual == "path" && "Path normalization during extraction failed");
}

void test_get_path_trailing_slash_file() {
    string actual = get_path("path/to/file.txt/");
    assert(actual == "path/to/file.txt" && "Trailing slash on file path extraction failed");
}

void test_get_path_trailing_slash_dir() {
    string actual = get_path("path/to/directory/");
    // fs::path("path/to/directory/").parent_path() -> "path/to"
    // fix_path("path/to") -> "path/to"
    assert(actual == "path/to/directory" && "Trailing slash on directory path extraction failed");
}

void test_get_path_filename_only() {
    string actual = get_path("file.txt");
    // fs::path("file.txt").parent_path() -> ""
    // fix_path("") -> ""
    assert(actual == "" && "Filename only path extraction failed");
}

void test_get_path_relative_dot() {
    string actual = get_path("./file.txt");
    // fs::path("./file.txt").parent_path() -> "."
    // fix_path(".") -> "."
    assert(actual == "." && "Relative dot path extraction failed");
}

void test_get_path_relative_dotdot() {
    string actual = get_path("../file.txt");
    // fs::path("../file.txt").parent_path() -> ".."
    // fix_path("..") -> ".."
    assert(actual == ".." && "Relative dot-dot path extraction failed");
}

void test_get_path_root_file() {
    string actual = get_path("/file.txt");
    // fs::path("/file.txt").parent_path() -> "/"
    // fix_path("/") -> "/"
    assert(actual == "/" && "Root file path extraction failed");
}

void test_get_path_empty() {
    string actual = get_path("");
    // fs::path("").parent_path() -> ""
    // fix_path("") -> ""
    assert(actual == "" && "Empty path extraction failed");
}

void test_get_path_directory_only() {
    string actual = get_path("path/to");
    // fs::path("path/to").parent_path() -> "path"
    // fix_path("path") -> "path"
    assert(actual == "path" && "Directory only path extraction failed");
}

void test_get_path_root_only() {
    string actual = get_path("/");
    // fs::path("/").parent_path() -> "/"
    // fix_path("/") -> "/"
    assert(actual == "/" && "Root only path extraction failed");
}

// Register tests
TEST(test_get_path_basic);
// TEST(test_get_path_backslashes);
// TEST(test_get_path_mixed_slashes);
TEST(test_get_path_needs_normalization);
TEST(test_get_path_trailing_slash_file);
TEST(test_get_path_trailing_slash_dir);
TEST(test_get_path_filename_only);
TEST(test_get_path_relative_dot);
TEST(test_get_path_relative_dotdot);
TEST(test_get_path_root_file);
TEST(test_get_path_empty);
TEST(test_get_path_directory_only);
TEST(test_get_path_root_only);

#endif
