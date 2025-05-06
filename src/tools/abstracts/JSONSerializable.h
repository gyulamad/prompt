#pragma once

#include "../utils/JSON.h"

namespace tools::abstracts {

    class JSONSerializable {
    public:
        JSONSerializable();
        virtual ~JSONSerializable();
        virtual tools::utils::JSON toJSON() const = 0;
        virtual void fromJSON(const tools::utils::JSON& json) = 0;
    };

}