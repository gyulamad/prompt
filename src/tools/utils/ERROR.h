#pragma once

#include <string>
#include <stdexcept>
#include "ANSI_FMT.h"

using namespace std;

namespace tools::utils {

    // Macros for formatted error, debug, and stub messages
    #define FILE_LINE_ERROR_MSG (ANSI_FMT_ERROR + msg + ANSI_FMT_RESET + "\nat " + ANSI_FMT_FILE_LINE(file, line) + ANSI_FMT_RESET)
    #define FILE_LINE_DEBUG_MSG (ANSI_FMT_DEBUG + msg + ANSI_FMT_RESET + "\nat " + ANSI_FMT_FILE_LINE(file, line) + ANSI_FMT_RESET)
    #define FILE_LINE_STUB_MSG (ANSI_FMT_STUB + msg + ANSI_FMT_RESET + "\nat " + ANSI_FMT_CALL(func, file, line) + ANSI_FMT_RESET)

    // Create a runtime error with formatted message
    runtime_error error(const string& msg, const string& file, int line);

    // Output a debug message to stdout
    void debug(const string& msg, const string& file, int line);

    // Output a stub message to stdout
    void stub(const string& msg, const string& func, const string& file, int line);

    // Error macros
    #define ERROR(msg) tools::utils::error(msg, __FILE__, __LINE__)
    #define ERROR_UNIMP tools::utils::error("Unimplemented", __FILE__, __LINE__)
    #define ERROR_INVALID(var) tools::utils::error("Invalid "#var, __FILE__, __LINE__)

    // File and line formatting macro
    #define FILELN ANSI_FMT_FILE_LINE(__FILE__, __LINE__)

    // Function name macro for cross-platform compatibility
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

    // Debug and stub macros
    #define DEBUG(msg) tools::utils::debug(msg, __FILE__, __LINE__)
    #define STUB(msg) tools::utils::stub(msg, __FUNC__, __FILE__, __LINE__)
    #define STUB_UNIMP STUB("Unimplemented")
    #define STUB_VIRTUAL STUB("It should be pure virtual and implemented in derived classes according to their specific interpretation.")

    // Null pointer check macros
    #define NULLCHK_IMPL(ptr, errmsg) { if (nullptr == ptr) throw ERROR(errmsg); }
    #define NULLCHK_SELECT(_1, _2, NAME, ...) NAME
    #define NULLCHK_1(ptr) NULLCHK_IMPL(ptr, "nullptr: "#ptr)
    #define NULLCHK_2(ptr, errmsg) NULLCHK_IMPL(ptr, errmsg)
    #define NULLCHK(...) NULLCHK_SELECT(__VA_ARGS__, NULLCHK_2, NULLCHK_1)(__VA_ARGS__)


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

} // namespace tools::utils