#pragma once

#include <string>
#include <mutex>
#include <functional>
#include <iostream>

#include "ERROR.h"

using namespace std;

namespace tools::utils {

    // Mutexes for thread-safe I/O operations
    extern mutex io_readln_mtx;
    extern mutex io_kbhit_mtx;
    extern mutex io_confirm_mtx;

    // Write to stdout without newline
    void write(const string& output = "");

    // Write to stdout with newline
    void writeln(const string& output = "");

    // Read a line from stdin with optional prompt
    string readln(const string& prompt = "");

    // Check if a key has been pressed (non-blocking)
    bool kbhit();

    // Check for a specific keypress (e.g., ESC) non-blocking
    bool kbhit_chk(int key = 27);

    // Prompt for confirmation with default option
    bool confirm(const string& message, int def = 'y');

    // Capture cout output
    string capture_cout(function<void()> func);

    // Capture cerr output
    string capture_cerr(function<void()> func);

    // Capture both cout and cerr output
    string capture_cout_cerr(function<void()> func);

} // namespace tools::utils

#ifdef TEST
// Test function declarations (in global namespace)
void test_capture_cout_basic();
void test_capture_cout_empty();
void test_capture_cerr_basic();
void test_capture_cerr_empty();
void test_capture_cout_cerr_basic();
void test_capture_cout_cerr_empty();
#endif