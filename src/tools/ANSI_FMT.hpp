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

#define ANSI_FMT_FNAME ANSI_FMT_T_BOLD ANSI_FMT_C_WHITE
#define ANSI_FMT_ERROR ANSI_FMT_T_BOLD ANSI_FMT_C_RED
#define ANSI_FMT_DEBUG ANSI_FMT_C_BLACK

#define ANSI_FMT(fmt, text) (string(fmt) + text + ANSI_FMT_RESET)

using namespace std;

namespace tools {
    // string ansi_fmt(const string& fmt, const string& text) {
    //     return fmt + text + ANSI_FMT_RESET;
    // }
}