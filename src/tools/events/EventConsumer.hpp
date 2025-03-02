#pragma once

#include <memory>

#include "Event.hpp"

using namespace std;

namespace tools::events {

    class EventBus;

    /**
     * Interface for components that can consume events
     */
    class EventConsumer {
    public:
        virtual ~EventConsumer() = default;
        
        // Handle an incoming event
        virtual void handleEvent(shared_ptr<Event> event) = 0;
        
        // Register this consumer with the event bus
        virtual void registerWithEventBus(shared_ptr<EventBus> bus) = 0;
        
        // Get unique identifier for this consumer
        virtual ComponentId getId() const = 0;
        
        // Check if this consumer can handle a specific event type
        virtual bool canHandle(type_index eventType) const = 0;
    };
    
}

#ifdef TEST

#endif
