#pragma once

#include <string>
#include <thread>
#include <typeindex>

using namespace std;

namespace tools::events {

    // Unique identifier for components in the system
    using ComponentId = string;

    /**
     * Base class for all events in the system
     */
    class Event {
    public:
        virtual ~Event() = default;
        
        // Source component that created this event
        ComponentId sourceId = "";
        
        // Optional target component (empty string means broadcast)
        ComponentId targetId = "";
        
        // Timestamp when the event was created
        chrono::time_point<chrono::system_clock> timestamp;
        
        // Type information for runtime type checking
        virtual type_index getType() const = 0;
    };

}
