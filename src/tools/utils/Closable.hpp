#pragma once

#include <atomic>

using namespace std;

namespace tools::utils {

    class Closable {
    public:
        void close() { closing = true; }
        bool isClosing() const { return closing; }
    protected:
        atomic<bool> closing = false;
    };

}