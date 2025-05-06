#pragma once

#include "../utils/ERROR.hpp"

namespace tools::abstracts {

    template<typename T>
    class Stream {
    public:

        virtual ~Stream() {};

        virtual void write(const T& data) = 0;
        virtual bool available() = 0;
        virtual T read() = 0;
        virtual T peek() = 0;
        virtual void flush() = 0;
        virtual bool eof() = 0;
        virtual bool error() = 0;
        virtual void close() = 0;
    };

}
