#pragma once

#include <string>
#include <iostream>
#include <stdexcept>

#include "ANSI_FMT.hpp"

using namespace std;

namespace tools::utils {

    // #define FILE_LINE_ERROR_MSG (ANSI_FMT_FILE_LINE(file, line) + " - " + ANSI_FMT_ERROR + msg + ANSI_FMT_RESET)
    // #define FILE_LINE_DEBUG_MSG (ANSI_FMT_FILE_LINE(file, line) + " - " + ANSI_FMT_DEBUG + msg + ANSI_FMT_RESET)
    #define FILE_LINE_ERROR_MSG (ANSI_FMT_ERROR + msg + ANSI_FMT_RESET + "\nat " + ANSI_FMT_FILE_LINE(file, line) + ANSI_FMT_RESET)
    #define FILE_LINE_DEBUG_MSG (ANSI_FMT_DEBUG + msg + ANSI_FMT_RESET + "\nat " + ANSI_FMT_FILE_LINE(file, line) + ANSI_FMT_RESET)
    #define FILE_LINE_STUB_MSG (ANSI_FMT_STUB + msg + ANSI_FMT_RESET + "\nat " + ANSI_FMT_CALL(func, file, line) + ANSI_FMT_RESET)

    inline runtime_error error(const string& msg, const string& file, int line) {
        return runtime_error(FILE_LINE_ERROR_MSG.c_str());
    }

    inline void debug(const string& msg, const string& file, int line) {
        cout << "[DEBUG] " << FILE_LINE_DEBUG_MSG << endl;
    }

    inline void stub(const string& msg, const string& func, const string& file, int line) {
        cout << "[STUB] " << FILE_LINE_STUB_MSG << endl;
    }

    #define ERROR(msg) tools::utils::error(msg, __FILE__, __LINE__)
    #define ERROR_UNIMP tools::utils::error("Unimplemented", __FILE__, __LINE__)
    #define ERROR_INVALID(var) tools::utils::error("Invalid "#var, __FILE__, __LINE__)

    #define FILELN ANSI_FMT_FILE_LINE(__FILE__, __LINE__)

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

    
    // #define UNIMP_NEED = 0;
    // #define UNIMP_SKIP { }
    // #define = 0;{ throw ERROR("Unimplemented function: " + string(__FUNC__)); }
    

    #define DEBUG(msg) tools::utils::debug(msg, __FILE__, __LINE__)
    #define STUB(msg) tools::utils::stub(msg, __FUNC__, __FILE__, __LINE__)
    #define STUB_UNIMP STUB("Unimplemented")
    #define STUB_VIRTUAL STUB("It should be pure virtual and implemented in derived classes according to their specific interpretation.")


    // Define a helper macro for the implementation details
    #define NULLCHK_IMPL(ptr, errmsg) { if (nullptr == ptr) throw ERROR(errmsg); }

    // Define the main macro with optional parameter
    #define NULLCHK(...) NULLCHK_SELECT(__VA_ARGS__, NULLCHK_2, NULLCHK_1)(__VA_ARGS__)

    // Helper macros to select the right implementation based on argument count
    #define NULLCHK_SELECT(_1, _2, NAME, ...) NAME
    #define NULLCHK_1(ptr) NULLCHK_IMPL(ptr, "nullptr: "#ptr)
    #define NULLCHK_2(ptr, errmsg) NULLCHK_IMPL(ptr, errmsg)

    // Dereference to a specific type of pointer with nullptr safety check
    template<typename T>
    T& dref(void* ptr) {
        NULLCHK(ptr, "Null pointer dereference");
        T& ref = (*(T*)ptr);
        return ref;
    }

    template<typename T>
    T* safe(T* ptr) {
        NULLCHK(ptr, "Null pointer reference");
        return ptr;
    }
};