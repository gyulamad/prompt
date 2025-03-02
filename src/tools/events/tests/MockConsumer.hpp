#pragma once

#include <memory>

#include "../EventConsumer.hpp"

#include "TestEvent.hpp"

using namespace std;
using namespace tools::events;

class MockConsumer : public EventConsumer, public enable_shared_from_this<MockConsumer> {
public:
    MockConsumer(const ComponentId& id) : id(id) {}
    vector<shared_ptr<Event>> receivedEvents;

    void handleEvent(shared_ptr<Event> event) override {
        receivedEvents.push_back(event);
    }
    void registerWithEventBus(shared_ptr<EventBus> bus) override {
        bus->registerConsumer(shared_from_this());
        bus->registerEventInterest(id, type_index(typeid(TestEvent)));
    }
    ComponentId getId() const override { return id; }
    bool canHandle(type_index eventType) const override {
        return eventType == type_index(typeid(TestEvent));
    }

private:
    ComponentId id;
};