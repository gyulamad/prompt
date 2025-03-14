#pragma once

#include <atomic>

using namespace std;

namespace tools::utils {

    class Closable {
    public:
        void close() {
            closing = true;
        }
    protected:
        atomic<bool> closing = false;
    };

}