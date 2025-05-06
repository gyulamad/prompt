#pragma once

#include <atomic>

using namespace std;

namespace tools::abstracts {

    class Closable {
    public:
        virtual ~Closable() { if (!isClosing()) close(); }
        void close() { closing = true; }
        bool isClosing() const { return closing; }        
    protected:
        atomic<bool> closing = false;
    };

}