#pragma once

#include "../utils/ERROR.hpp"

namespace tools::abstracts {

    template<typename T>
    class Stream {
    public:
        virtual void write(const T& data) = 0;
        virtual bool available() UNIMP_THROWS
        virtual T read() = 0;
        virtual T peek() UNIMP_THROWS
        virtual void flush() UNIMP_THROWS
        virtual bool eof() UNIMP_THROWS
        virtual bool error() UNIMP_THROWS
        virtual void close() UNIMP_THROWS

        virtual ~Stream() = default;
    };

}
