#pragma once

#include <memory>
#include <vector>
#include <functional>

#include "../EventConsumer.hpp"
#include "TestEvent.hpp"

using namespace std;
using namespace tools::events;

class MockConsumer : public EventConsumer {
public:
    MockConsumer(const ComponentId& id) : id(id) {}
    vector<Event*> receivedEvents;
    // Callback type for event handling notification
    using Callback = function<void(Event&)>;

    void handleEvent(Event& event) override {
        receivedEvents.push_back(&event);
        if (callback) {
            callback(event); // Invoke callback if set
        }
    }

    void registerWithEventBus(EventBus* bus) override {
        NULLCHK(bus);
        bus->registerConsumer(*this);
        bus->registerEventInterest(id, type_index(typeid(TestEvent)));
    }

    ComponentId getId() const override { return id; }

    bool canHandle(type_index eventType) const override {
        return eventType == type_index(typeid(TestEvent));
    }

private:
    ComponentId id;
    Callback callback; // Store the callback
};