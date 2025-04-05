#pragma once

#include <string>
#include "../utils/JSON.hpp"

using namespace tools::utils;

namespace tools::abstracts {

    class JSONSerializable {
    public:
        virtual ~JSONSerializable() {}
        virtual JSON toJSON() const UNIMP_THROWS
        virtual void fromJSON(const JSON& j) UNIMP_THROWS
    };

}
