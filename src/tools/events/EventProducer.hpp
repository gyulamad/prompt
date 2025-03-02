#pragma once

#include <memory>

#include "Event.hpp"

using namespace std;

namespace tools::events {

    class EventBus;

    /**
     * Interface for components that can produce events
     */
    class EventProducer {
    public:
        virtual ~EventProducer() = default;
        
        // Register this producer with the event bus
        virtual void registerWithEventBus(shared_ptr<EventBus> bus) = 0;
        
        // Get unique identifier for this producer
        virtual ComponentId getId() const = 0;
    };

}

#ifdef TEST

#endif