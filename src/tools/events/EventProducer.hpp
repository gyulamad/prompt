#pragma once

#include <memory>

#include "../utils/ERROR.hpp"
#include "Event.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::events {

    class EventBus;

    /**
     * Interface for components that can produce events
     */
    class EventProducer {
    public:
        virtual ~EventProducer() = default;
        
        // Register this producer with the event bus
        virtual void registerWithEventBus(EventBus* bus) UNIMP_THROWS
        
        // Get unique identifier for this producer
        virtual ComponentId getId() const UNIMP_THROWS
    };

}