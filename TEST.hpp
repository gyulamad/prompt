#pragma once

#include <iostream>

using namespace std;

typedef void (*test_func_t)();

void __call_test_case(test_func_t func, const string& name, const string& file, int line) {
    ::cout << file << ":" << line << " " << name << ": " << flush;
    func();
    ::cout << "[ok]" << endl << flush;
}

// TODO: move it to the tools
#define TEST(name) __call_test_case(name, #name, __FILE__, __LINE__); 

