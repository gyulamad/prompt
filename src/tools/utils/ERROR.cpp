#include "ERROR.h"
#include <iostream>

using namespace std;

namespace tools::utils {

    runtime_error error(const string& msg, const string& file, int line) {
        return runtime_error(FILE_LINE_ERROR_MSG.c_str());
    }

    void debug(const string& msg, const string& file, int line) {
        cout << "[DEBUG] " << FILE_LINE_DEBUG_MSG << endl;
    }

    void stub(const string& msg, const string& func, const string& file, int line) {
        cout << "[STUB] " << FILE_LINE_STUB_MSG << endl;
    }
    
} // namespace tools::utils