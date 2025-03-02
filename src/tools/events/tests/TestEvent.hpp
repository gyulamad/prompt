
#pragma once

#include "../Event.hpp"

using namespace tools::events;

class TestEvent : public Event {
public:
    type_index getType() const override { return type_index(typeid(TestEvent)); }
    int value;
    TestEvent(int v) : value(v) {}
};