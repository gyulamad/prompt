#include "ANSI_FMT.h"

using namespace std;

namespace tools::utils {

    string ansi_fmt(const string& fmt, const string& text) {
        return ANSI_FMT_RESET + fmt + text + ANSI_FMT_RESET;
    }

} // namespace tools::utils

#ifdef TEST

#include "../utils/Test.h"
#include "../utils/assert.hpp"

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
#endif // TEST