#pragma once

#include <iostream>
#include <stdexcept>

#include "ANSI_FMT.hpp"

using namespace std;

namespace tools {

    #define FILE_LINE_ERROR_MSG (ANSI_FMT_FNAME + file + ANSI_FMT_RESET ":" + to_string(line) + " - " + ANSI_FMT_ERROR + msg + ANSI_FMT_RESET)
    #define FILE_LINE_DEBUG_MSG (ANSI_FMT_FNAME + file + ANSI_FMT_RESET ":" + to_string(line) + " - " + ANSI_FMT_DEBUG + msg + ANSI_FMT_RESET)

    inline runtime_error error(const string& msg, const string& file, int line) {
        return runtime_error(FILE_LINE_ERROR_MSG.c_str());
    }

    inline void debug(const string& msg, const string& file, int line) {
        cout << FILE_LINE_DEBUG_MSG << endl;
    }

    #define ERROR(msg) error(msg, __FILE__, __LINE__)

    #define UNIMP { throw ERROR("Unimplemented"); }


    #define DEBUG(msg) debug(msg, __FILE__, __LINE__)

    #define NULLCHK(p) { if (nullptr == p) throw ERROR("nullptr"); }

};