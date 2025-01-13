#pragma once

#include <stdexcept>

using namespace std;

inline runtime_error error(const string& msg, const string& file, int line) {
    return runtime_error((file + ":" + to_string(line) + " - " + msg).c_str());
}

#define ERROR(msg) error(msg, __FILE__, __LINE__)

#define ERR_UNIMP ERROR("Unimplemented")
