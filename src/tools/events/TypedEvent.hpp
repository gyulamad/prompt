#pragma once

#include "Event.hpp"

using namespace std;

namespace tools::events {

    /**
     * Base template for concrete event types
     */
    template<typename T>
    class TypedEvent : public Event {
    public:
        type_index getType() const override {
            return type_index(typeid(T));
        }
    };

}

#ifdef TEST

#endif
