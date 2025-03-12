#pragma once

#include "EventProducer.hpp"
#include "EventConsumer.hpp"

using namespace std;

namespace tools::events {

    /**
     * Combined interface for components that can both produce and consume events
     */
    class EventAgent : public EventProducer, public EventConsumer {
    public:
        virtual ~EventAgent() = default;
    };

}
