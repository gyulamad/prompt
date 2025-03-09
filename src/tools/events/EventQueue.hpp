#pragma once

#include <memory>

#include "Event.hpp"

using namespace std;

namespace tools::events {

    /**
     * Abstract interface for event queues
     */
    class EventQueue {
    public:
        virtual ~EventQueue() = default;
        virtual bool write(shared_ptr<Event> event) = 0;  // Add an event to the queue
        virtual size_t read(shared_ptr<Event>& event, bool blocking, int timeoutMs) = 0; // Retrieve an event
        virtual size_t available() const = 0;  // Check number of available events
    };
    
}