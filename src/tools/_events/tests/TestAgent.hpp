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
    vector<Event*> receivedEvents;

protected:
    void registerEventInterests() override {
        registerHandler<TestEvent>([this](TestEvent& event) {
            receivedEvents.push_back(&event);
        });
    }
};