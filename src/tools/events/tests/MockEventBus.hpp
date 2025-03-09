#pragma once

#include "../EventBus.hpp"
#include "../EventAgent.hpp"

using namespace tools::events;

// Mock EventBus to capture published events
class MockEventBus : public EventBus {
public:
    MockEventBus(bool async, Logger& logger, EventQueue& queue) : EventBus(async, logger, queue) {}
    vector<shared_ptr<Event>> publishedEvents;

    void publishEvent(shared_ptr<Event> event) {
        publishedEvents.push_back(event);
        // Simulate immediate handling
        for (auto& consumer : consumers) {
            consumer.lock()->handleEvent(event);
        }
    }

    void registerProducer(shared_ptr<EventAgent> producer) {
        producers.push_back(producer);
    }

    void registerConsumer(shared_ptr<EventAgent> consumer) {
        consumers.push_back(weak_ptr<EventAgent>(consumer));
    }

    void start() { } // Stub for base class

private:
    vector<shared_ptr<EventAgent>> producers;
    vector<weak_ptr<EventAgent>> consumers;
};