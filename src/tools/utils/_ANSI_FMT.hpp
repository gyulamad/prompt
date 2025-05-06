#pragma once

#include <string>

// Text formatting
#define ANSI_FMT_T_BOLD "\033[1m"
#define ANSI_FMT_T_DIM "\033[2m"
#define ANSI_FMT_T_UNDERLINE "\033[4m"
#define ANSI_FMT_T_BLINK "\033[5m"
#define ANSI_FMT_T_REVERSE "\033[7m"
#define ANSI_FMT_T_HIDDEN "\033[8m"

// Text colors
#define ANSI_FMT_RESET "\033[0m"
#define ANSI_FMT_C_BLACK "\033[30m"
#define ANSI_FMT_C_RED "\033[31m"
#define ANSI_FMT_C_GREEN "\033[32m"
#define ANSI_FMT_C_YELLOW "\033[33m"
#define ANSI_FMT_C_BLUE "\033[34m"
#define ANSI_FMT_C_MAGENTA "\033[35m"
#define ANSI_FMT_C_CYAN "\033[36m"
#define ANSI_FMT_C_WHITE "\033[37m"

// Background colors
#define ANSI_FMT_B_BLACK "\033[40m"
#define ANSI_FMT_B_RED "\033[41m"
#define ANSI_FMT_B_GREEN "\033[42m"
#define ANSI_FMT_B_YELLOW "\033[43m"
#define ANSI_FMT_B_BLUE "\033[44m"
#define ANSI_FMT_B_MAGENTA "\033[45m"
#define ANSI_FMT_B_CYAN "\033[46m"
#define ANSI_FMT_B_WHITE "\033[47m"

// ---------------------------------------------------------------
// User specific theme

// #define ANSI_FMT_FNAME ANSI_FMT_T_BOLD ANSI_FMT_C_WHITE
#define ANSI_FMT_SUCCESS ANSI_FMT_T_BOLD ANSI_FMT_C_GREEN
#define ANSI_FMT_WARNING ANSI_FMT_T_BOLD ANSI_FMT_C_YELLOW
#define ANSI_FMT_ERROR ANSI_FMT_T_BOLD ANSI_FMT_C_RED
#define ANSI_FMT_NOTE ANSI_FMT_T_BOLD ANSI_FMT_C_BLUE
#define ANSI_FMT_DEBUG ANSI_FMT_RESET ANSI_FMT_C_BLACK
#define ANSI_FMT_STUB ANSI_FMT_RESET ANSI_FMT_C_YELLOW
#define ANSI_FMT_FILE ANSI_FMT_RESET ANSI_FMT_C_BLACK
#define ANSI_FMT_FUNC ANSI_FMT_T_BOLD ANSI_FMT_C_WHITE
#define ANSI_FMT_HIGHLIGHT ANSI_FMT_T_BOLD ANSI_FMT_C_WHITE

#define ANSI_FMT(fmt, txt) (std::string(ANSI_FMT_RESET) + (fmt) + (txt) + ANSI_FMT_RESET)
#define ANSI_FMT_FILE_LINE(file, line) ANSI_FMT(ANSI_FMT_FILE, (std::string(file)) + ":" + std::to_string(line))
#define ANSI_FMT_CALL(func, file, line) ANSI_FMT(ANSI_FMT_FUNC, (std::string(func)) + "()" + ANSI_FMT_RESET + " at " + ANSI_FMT_FILE_LINE((file), (line)))

using namespace std;

namespace tools::utils {
    string ansi_fmt(const string& fmt, const string& text) {
        return ANSI_FMT_RESET + fmt + text + ANSI_FMT_RESET;
    }
}


#ifdef TEST

// #include "assert.hpp"
#include <cassert>
#include "Test.hpp"

using namespace tools::utils;

void test_ansi_fmt_basic_formatting() {
    string result = ansi_fmt(ANSI_FMT_C_RED, "test");
    string expected = string(ANSI_FMT_RESET) + ANSI_FMT_C_RED + "test" + ANSI_FMT_RESET;
    assert(result == expected && "Basic red color formatting failed");
}

void test_ansi_fmt_empty_text() {
    string result = ansi_fmt(ANSI_FMT_C_GREEN, "");
    string expected = string(ANSI_FMT_RESET) + ANSI_FMT_C_GREEN + "" + ANSI_FMT_RESET;
    assert(result == expected && "Empty text formatting failed");
}

void test_ansi_fmt_no_format() {
    string result = ansi_fmt("", "test");
    string expected = string(ANSI_FMT_RESET) + "test" + ANSI_FMT_RESET;
    assert(result == expected && "No format applied failed");
}

void test_ansi_fmt_multiple_formats() {
    string format = string(ANSI_FMT_T_BOLD) + ANSI_FMT_C_BLUE;
    string result = ansi_fmt(format, "boldblue");
    string expected = string(ANSI_FMT_RESET) + ANSI_FMT_T_BOLD + ANSI_FMT_C_BLUE + "boldblue" + ANSI_FMT_RESET;
    assert(result == expected && "Multiple formats (bold and blue) failed");
}

// Register tests
TEST(test_ansi_fmt_basic_formatting);
TEST(test_ansi_fmt_empty_text);
TEST(test_ansi_fmt_no_format);
TEST(test_ansi_fmt_multiple_formats);

#endif