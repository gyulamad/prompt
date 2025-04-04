#pragma once

#include <string>
#include "../utils/JSON.hpp"

using namespace tools::utils;

namespace tools::abstracts {

    class JsonSerializable {
    public:
        virtual JSON toJSON() const = 0;
        virtual void fromJSON(const JSON& j) = 0;
    };

}
