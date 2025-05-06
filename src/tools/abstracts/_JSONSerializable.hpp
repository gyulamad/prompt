#pragma once

#include "../utils/JSON.hpp"

using namespace tools::utils;

namespace tools::abstracts {

    class JSONSerializable {
    public:
        JSONSerializable() {}
        virtual ~JSONSerializable() {}
        virtual JSON toJSON() const = 0;
        virtual void fromJSON(const JSON& json) = 0;
    };

}
