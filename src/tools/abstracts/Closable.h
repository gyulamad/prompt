#pragma once

#include <atomic>

using namespace std;

namespace tools::abstracts {

    class Closable {
    public:
        virtual ~Closable();
        void close();
        bool isClosing() const;
    
    protected:
        atomic<bool> closing = false;
    };

}