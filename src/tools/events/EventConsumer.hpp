#pragma once

#include <memory>

#include "../utils/ERROR.hpp"
#include "Event.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::events {

    class EventBus;

    /**
     * Interface for components that can consume events
     */
    class EventConsumer {
    public:
        virtual ~EventConsumer() = default;
        
        // Handle an incoming event
        virtual void handleEvent(shared_ptr<Event> event) UNIMP
        
        // Register this consumer with the event bus
        virtual void registerWithEventBus(EventBus* bus) UNIMP
        
        // Get unique identifier for this consumer
        virtual ComponentId getId() const UNIMP
        
        // Check if this consumer can handle a specific event type
        virtual bool canHandle(type_index eventType) const UNIMP
    };
    
}

#ifdef TEST

#endif
