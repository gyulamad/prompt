#pragma once

#include <memory>
#include <vector>

#include "../Event.hpp"
#include "../BaseEventAgent.hpp"

#include "TestEvent.hpp"

using namespace tools::events;

class TestAgent : public BaseEventAgent {
public:
    TestAgent(const ComponentId& id) : BaseEventAgent(id) {}
    vector<shared_ptr<Event>> receivedEvents;

protected:
    void registerEventInterests() override {
        registerHandler<TestEvent>([this](shared_ptr<TestEvent> event) {
            receivedEvents.push_back(event);
        });
    }
};