#pragma once

#include <memory>
#include <vector>

#include "../Event.hpp"
#include "../BaseEventConsumer.hpp"

#include "TestEvent.hpp"

using namespace tools::events;

class TestConsumer : public BaseEventConsumer {
public:
    TestConsumer(const ComponentId& id) : BaseEventConsumer(id) {}
    vector<shared_ptr<Event>> receivedEvents;

protected:
    void registerEventInterests() override {
        registerHandler<TestEvent>([this](shared_ptr<TestEvent> event) {
            receivedEvents.push_back(event);
        });
    }
};