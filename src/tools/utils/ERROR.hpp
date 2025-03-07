#pragma once

#include <string>
#include <iostream>
#include <stdexcept>

#include "ANSI_FMT.hpp"

using namespace std;

namespace tools::utils {

    #define FILE_LINE_ERROR_MSG (ANSI_FMT_FNAME + file + ANSI_FMT_RESET ":" + to_string(line) + " - " + ANSI_FMT_ERROR + msg + ANSI_FMT_RESET)
    #define FILE_LINE_DEBUG_MSG (ANSI_FMT_FNAME + file + ANSI_FMT_RESET ":" + to_string(line) + " - " + ANSI_FMT_DEBUG + msg + ANSI_FMT_RESET)

    inline runtime_error error(const string& msg, const string& file, int line) {
        return runtime_error(FILE_LINE_ERROR_MSG.c_str());
    }

    inline void debug(const string& msg, const string& file, int line) {
        cout << "[DEBUG] " <<  FILE_LINE_DEBUG_MSG << endl;
    }

    #define ERROR(msg) error(msg, __FILE__, __LINE__)

    #ifdef __PRETTY_FUNCTION__
    #define __FUNC__ __PRETTY_FUNCTION__
    #else
    #ifdef __FUNCSIG__
    #define __FUNC__ __FUNCSIG__
    #else
    #ifdef __FUNCTION__
    #define __FUNC__ __FUNCTION__
    #else
    #define __FUNC__ __func__
    #endif
    #endif
    #endif

    #define UNIMP { throw ERROR("Unimplemented: " + string(__FUNC__)); }


    #define DEBUG(msg) debug(msg, __FILE__, __LINE__)

    // Define a helper macro for the implementation details
    #define NULLCHK_IMPL(p, errmsg) { if (nullptr == p) throw ERROR(errmsg); }

    // Define the main macro with optional parameter
    #define NULLCHK(...) NULLCHK_SELECT(__VA_ARGS__, NULLCHK_2, NULLCHK_1)(__VA_ARGS__)

    // Helper macros to select the right implementation based on argument count
    #define NULLCHK_SELECT(_1, _2, NAME, ...) NAME
    #define NULLCHK_1(p) NULLCHK_IMPL(p, "nullptr: "#p)
    #define NULLCHK_2(p, errmsg) NULLCHK_IMPL(p, errmsg)

};