#pragma once

#include <memory>

#include "EventBus.hpp"
#include "EventAgent.hpp"

using namespace std;

namespace tools::events {
    
    /**
     * Base implementation of an event agent (both producer and consumer)
     */
    class BaseEventAgent : public EventAgent, public enable_shared_from_this<BaseEventAgent> {
    public:
        BaseEventAgent(const ComponentId& id) : m_id(id) {}
        
        void registerWithEventBus(shared_ptr<EventBus> bus) override {
            m_eventBus = bus;
            bus->registerProducer(shared_from_this());
            bus->registerConsumer(shared_from_this());
            registerEventInterests();
        }
        
        ComponentId getId() const override {
            return m_id;
        }
        
        bool canHandle(type_index eventType) const override {
            return m_handlerMap.find(eventType) != m_handlerMap.end();
        }
        
        void handleEvent(shared_ptr<Event> event) override {
            auto it = m_handlerMap.find(event->getType());
            if (it != m_handlerMap.end()) {
                it->second(event);
            }
        }

        // Helper method to publish events
        template<typename EventType>
        void publishEvent(shared_ptr<EventType> event) {
            if (m_eventBus) {
                event->sourceId = m_id;
                event->timestamp = chrono::system_clock::now();
                m_eventBus->publishEvent(event);
            }
        }

    protected:
        // Register event handler for a specific event type
        template<typename EventType>
        void registerHandler(function<void(shared_ptr<EventType>)> handler) {
            type_index typeIdx(typeid(EventType));
            
            m_handlerMap[typeIdx] = [handler](shared_ptr<Event> baseEvent) {
                auto derivedEvent = static_pointer_cast<EventType>(baseEvent);
                handler(derivedEvent);
            };
            
            if (m_eventBus) {
                m_eventBus->registerEventInterest(m_id, typeIdx);
            }
        }
        
        // Child classes should override this to register their handlers
        virtual void registerEventInterests() = 0;

    private:
        ComponentId m_id;
        shared_ptr<EventBus> m_eventBus;
        unordered_map<type_index, function<void(shared_ptr<Event>)>> m_handlerMap;
    };    

}

#ifdef TEST

#include "../tests/MockLogger.hpp"
#include "tests/TestEvent.hpp"
#include "tests/MockConsumer.hpp"
#include "tests/TestAgent.hpp"

// Test registration with EventBus as both producer and consumer
void test_BaseEventAgent_registerWithEventBus_basic1() {
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<EventBus>(false, logger);
    auto agent = make_shared<TestAgent>("agent1");

    agent->registerWithEventBus(bus);
    bool executedWithoutCrash = true;
    assert(executedWithoutCrash == true && "registerWithEventBus should not crash");

    // Test producer role
    auto event1 = make_shared<TestEvent>(42);
    agent->publishEvent(event1);
    assert(event1->sourceId == "agent1" && "Published event should have agent's ID as source");

    // Reset receivedEvents to test consumer role independently
    agent->receivedEvents.clear();
    auto event2 = make_shared<TestEvent>(43);
    bus->publishEvent(event2);
    size_t eventCount = agent->receivedEvents.size();
    assert(eventCount == 1 && "Agent should receive event as consumer");
    auto receivedEvent = static_pointer_cast<TestEvent>(agent->receivedEvents[0]);
    assert(receivedEvent->value == 43 && "Received event should match published event");
}

void test_BaseEventAgent_registerWithEventBus_basic2() {
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<EventBus>(false, logger);
    auto agent = make_shared<TestAgent>("agent1");

    agent->registerWithEventBus(bus);
    bool executedWithoutCrash = true;
    assert(executedWithoutCrash == true && "registerWithEventBus should not crash");

    // Test consumer role only
    auto event = make_shared<TestEvent>(42);
    bus->publishEvent(event);
    size_t eventCount = agent->receivedEvents.size();
    assert(eventCount == 1 && "Agent should receive event as consumer");
    assert(static_pointer_cast<TestEvent>(agent->receivedEvents[0])->value == 42 && "Received event should match published event");
}

// Test getId method
void test_BaseEventAgent_getId_basic() {
    auto agent = make_shared<TestAgent>("agent1");
    string id = agent->getId();

    assert(id == "agent1" && "getId should return the constructor-provided ID");
}

// Test canHandle with registered event type
void test_BaseEventAgent_canHandle_registered() {
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<EventBus>(false, logger);
    auto agent = make_shared<TestAgent>("agent1");
    agent->registerWithEventBus(bus);

    bool canHandle = agent->canHandle(type_index(typeid(TestEvent)));
    assert(canHandle == true && "canHandle should return true for registered event type");
}

// Test canHandle with unregistered event type
void test_BaseEventAgent_canHandle_unregistered() {
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<EventBus>(false, logger);
    auto agent = make_shared<TestAgent>("agent1");
    agent->registerWithEventBus(bus);

    bool canHandle = agent->canHandle(type_index(typeid(int)));
    assert(canHandle == false && "canHandle should return false for unregistered event type");
}

// Test handleEvent with registered handler
void test_BaseEventAgent_handleEvent_registered() {
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<EventBus>(false, logger);
    auto agent = make_shared<TestAgent>("agent1");
    agent->registerWithEventBus(bus);

    auto event = make_shared<TestEvent>(42);
    agent->handleEvent(event);
    size_t eventCount = agent->receivedEvents.size();
    assert(eventCount == 1 && "handleEvent should invoke registered handler");
    auto receivedEvent = static_pointer_cast<TestEvent>(agent->receivedEvents[0]);
    assert(receivedEvent->value == 42 && "Received event should match handled event");
}

// Test handleEvent with unregistered event type
void test_BaseEventAgent_handleEvent_unregistered() {
    class OtherEvent : public Event {
    public:
        type_index getType() const override { return type_index(typeid(OtherEvent)); }
    };
    auto agent = make_shared<TestAgent>("agent1");
    auto event = make_shared<OtherEvent>();

    agent->handleEvent(event);
    size_t eventCount = agent->receivedEvents.size();
    assert(eventCount == 0 && "handleEvent should not invoke handler for unregistered event type");
}

// Test publishEvent with EventBus
void test_BaseEventAgent_publishEvent_with_bus() {
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<EventBus>(false, logger);
    auto agent = make_shared<TestAgent>("agent1");
    auto consumer = make_shared<MockConsumer>("consumer1");

    agent->registerWithEventBus(bus);
    consumer->registerWithEventBus(bus);
    auto event = make_shared<TestEvent>(42);
    agent->publishEvent(event);

    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 1 && "Consumer should receive event published by agent");
    auto receivedEvent = static_pointer_cast<TestEvent>(consumer->receivedEvents[0]);
    assert(receivedEvent->value == 42 && "Received event should match published event");
    assert(receivedEvent->sourceId == "agent1" && "Received event should have agent's ID as source");
}

// Test publishEvent without EventBus
void test_BaseEventAgent_publishEvent_no_bus() {
    auto agent = make_shared<TestAgent>("agent1");
    auto event = make_shared<TestEvent>(42);

    agent->publishEvent(event);
    bool executedWithoutCrash = true;
    assert(executedWithoutCrash == true && "publishEvent without bus should not crash");
    assert(event->sourceId.empty() && "Event sourceId should not be set without bus");
    assert(event->timestamp == chrono::time_point<chrono::system_clock>() && "Event timestamp should not be set without bus");
}

// Test self-communication (agent publishes and consumes its own event)
void test_BaseEventAgent_self_communication() {
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<EventBus>(false, logger);
    auto agent = make_shared<TestAgent>("agent1");
    agent->registerWithEventBus(bus);

    auto event = make_shared<TestEvent>(42);
    agent->publishEvent(event);
    size_t eventCount = agent->receivedEvents.size();
    assert(eventCount == 1 && "Agent should receive its own published event");
    auto receivedEvent = static_pointer_cast<TestEvent>(agent->receivedEvents[0]);
    assert(receivedEvent->value == 42 && "Received event should match published event");
    assert(receivedEvent->sourceId == "agent1" && "Received event should have agent's ID as source");
}

// Register tests
TEST(test_BaseEventAgent_registerWithEventBus_basic1);
TEST(test_BaseEventAgent_registerWithEventBus_basic2);
TEST(test_BaseEventAgent_getId_basic);
TEST(test_BaseEventAgent_canHandle_registered);
TEST(test_BaseEventAgent_canHandle_unregistered);
TEST(test_BaseEventAgent_handleEvent_registered);
TEST(test_BaseEventAgent_handleEvent_unregistered);
TEST(test_BaseEventAgent_publishEvent_with_bus);
TEST(test_BaseEventAgent_publishEvent_no_bus);
TEST(test_BaseEventAgent_self_communication);
#endif
