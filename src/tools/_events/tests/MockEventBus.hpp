#pragma once

#include "../EventBus.hpp"
#include "../EventAgent.hpp"

using namespace tools::events;

// Mock EventBus to capture published events
class MockEventBus : public EventBus {
public:
    MockEventBus(Logger& logger, EventQueue& queue) : EventBus(logger, queue) {}
    vector<Event*> publishedEvents;

    void registerProducer(EventAgent& producer) {
        producers.push_back(&producer);
    }

    void registerConsumer(EventAgent& consumer) {
        consumers.push_back(&consumer);
    }

    void start() { } // Stub for base class

protected:

    // Override the new virtual function
    function<void(EventBus*, Event&)> deliverEventInternal = [](EventBus* that, Event& event) {
        ((MockEventBus*)that)->publishedEvents.push_back(&event);
        // Simulate immediate handling
        for (EventConsumer* consumer: ((MockEventBus*)that)->consumers) consumer->handleEvent(event);
    };

private:
    vector<EventProducer*> producers;
    vector<EventConsumer*> consumers;
};