#pragma once

#include "../utils/JSON.hpp"

using namespace tools::utils;

namespace tools::abstracts {

    class JSONSerializable {
    public:
        JSONSerializable() {}
        virtual ~JSONSerializable() {}
        virtual JSON toJSON() const UNIMP_THROWS
        virtual void fromJSON(const JSON& json) UNIMP_THROWS
    };

}
