#pragma once

#include <memory>

#include "Event.hpp"

using namespace std;

namespace tools::events {

    // Event delivery filter interface
    class EventFilter {
    public:
        virtual ~EventFilter() = default;
        
        // Return true if the event should be delivered, false if it should be filtered out
        virtual bool shouldDeliverEvent(const ComponentId& consumerId, Event& event) = 0;
    };

}
