#pragma once

#include <memory>

#include "../utils/ERROR.hpp"

#include "Event.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::events {

    /**
     * Abstract interface for event queues
     */
    class EventQueue {
    public:
        virtual ~EventQueue() = default;
        virtual bool write(shared_ptr<Event> event) UNIMP_THROWS  // Add an event to the queue
        virtual size_t read(shared_ptr<Event>& event, bool blocking, int timeoutMs) UNIMP_THROWS // Retrieve an event
        virtual size_t available() const UNIMP_THROWS  // Check number of available events
        virtual size_t getCapacity() const UNIMP_THROWS
    };
    
}