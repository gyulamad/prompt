#pragma once

#include <memory>

#include "../utils/ERROR.hpp"
#include "EventBus.hpp"
#include "EventAgent.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::events {
    
    /**
     * Base implementation of an event agent (both producer and consumer)
     */
    class BaseEventAgent : public EventAgent {
    public:
        BaseEventAgent(const ComponentId& id) : m_id(id) {}
        
        void registerWithEventBus(EventBus* bus) override {
            NULLCHK(bus);
            m_eventBus = bus;
            bus->registerProducer(*this);
            bus->registerConsumer(*this);
            registerEventInterests();
        }
        
        ComponentId getId() const override {
            return m_id;
        }
        
        bool canHandle(type_index eventType) const override {
            return m_handlerMap.find(eventType) != m_handlerMap.end();
        }
        
        void handleEvent(Event& event) override {
            lock_guard<mutex> lock(handlerMutex);
            auto it = m_handlerMap.find(event.getType());
            if (it != m_handlerMap.end()) {
                for (const auto& handler : it->second) {
                    handler(event); // Run all handlers
                }
            }
        }

        template<typename EventType, typename... Args>
        void publishEvent(const ComponentId& targetId = "", Args&&... args) {
            NULLCHK(m_eventBus);
            m_eventBus->createAndPublishEvent<EventType>(m_id, targetId, forward<Args>(args)...);
        }

        template<typename EventType>
        void registerHandler(function<void(EventType&)> handler) {
            type_index typeIdx(typeid(EventType));
            auto eventHandler = [handler](Event& baseEvent) {
                EventType& derivedEvent = (EventType&)baseEvent;
                handler(derivedEvent);
            };
            m_handlerMap[typeIdx].push_back(eventHandler); // Append, don’t overwrite
            if (m_eventBus) {
                m_eventBus->registerEventInterest(m_id, typeIdx);
            }
        }
        
    protected:
        // Child classes should override this to register their handlers
        virtual void registerEventInterests() { }

    private:
        ComponentId m_id;
        EventBus* m_eventBus = nullptr;
        unordered_map<type_index, vector<function<void(Event&)>>> m_handlerMap;
        mutex handlerMutex;  // Added for thread safety
    };    

}

#ifdef TEST

#include "../utils/Test.hpp"
#include "../utils/tests/MockLogger.hpp"
#include "tests/TestEvent.hpp"
#include "tests/MockConsumer.hpp"
#include "tests/TestAgent.hpp"

// Test registration with EventBus as both producer and consumer
void test_BaseEventAgent_registerWithEventBus_basic1() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);
    TestAgent agent("agent1");

    agent.registerWithEventBus(&bus);
    bool executedWithoutCrash = true;
    assert(executedWithoutCrash == true && "registerWithEventBus should not crash");

    // Test producer role
    agent.publishEvent<TestEvent>("agent1", 42); // Uses agent's publishEvent, which calls createAndPublishEvent
    TestEvent* event1 = (TestEvent*)agent.receivedEvents.back(); // Assuming TestAgent echoes to itself
    assert(event1->sourceId == "agent1" && "Published event should have agent's ID as source");

    // Reset receivedEvents to test consumer role independently
    agent.receivedEvents.clear();
    ComponentId sourceId = "test-source";
    bus.createAndPublishEvent<TestEvent>(sourceId, "agent1", 43);
    size_t eventCount = agent.receivedEvents.size();
    assert(eventCount == 1 && "Agent should receive event as consumer");
    TestEvent* receivedEvent = (TestEvent*)agent.receivedEvents[0];
    assert(receivedEvent->value == 43 && "Received event should match published event");
}

void test_BaseEventAgent_registerWithEventBus_basic2() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);
    TestAgent agent("agent1");

    agent.registerWithEventBus(&bus);
    bool executedWithoutCrash = true;
    assert(executedWithoutCrash == true && "registerWithEventBus should not crash");

    // Test consumer role only
    ComponentId sourceId = "test-source";
    bus.createAndPublishEvent<TestEvent>(sourceId, "agent1", 42);
    size_t eventCount = agent.receivedEvents.size();
    assert(eventCount == 1 && "Agent should receive event as consumer");
    assert(((TestEvent*)agent.receivedEvents[0])->value == 42 && "Received event should match published event");
}

// Test getId method
void test_BaseEventAgent_getId_basic() {
    TestAgent agent("agent1");
    string id = agent.getId();

    assert(id == "agent1" && "getId should return the constructor-provided ID");
}

// Test canHandle with registered event type
void test_BaseEventAgent_canHandle_registered() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);
    TestAgent agent("agent1");
    agent.registerWithEventBus(&bus);

    bool canHandle = agent.canHandle(type_index(typeid(TestEvent)));
    assert(canHandle == true && "canHandle should return true for registered event type");
}

// Test canHandle with unregistered event type
void test_BaseEventAgent_canHandle_unregistered() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);
    TestAgent agent("agent1");
    agent.registerWithEventBus(&bus);

    bool canHandle = agent.canHandle(type_index(typeid(int)));
    assert(canHandle == false && "canHandle should return false for unregistered event type");
}

// Test handleEvent with registered handler
void test_BaseEventAgent_handleEvent_registered() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);
    TestAgent agent("agent1");
    agent.registerWithEventBus(&bus);

    TestEvent event(42);
    agent.handleEvent(event);
    size_t eventCount = agent.receivedEvents.size();
    assert(eventCount == 1 && "handleEvent should invoke registered handler");
    TestEvent* receivedEvent = (TestEvent*)agent.receivedEvents[0];
    assert(receivedEvent->value == 42 && "Received event should match handled event");
}

// Test handleEvent with unregistered event type
void test_BaseEventAgent_handleEvent_unregistered() {
    class OtherEvent : public Event {
    public:
        type_index getType() const override { return type_index(typeid(OtherEvent)); }
    };
    TestAgent agent("agent1");
    OtherEvent event;

    agent.handleEvent(event);
    size_t eventCount = agent.receivedEvents.size();
    assert(eventCount == 0 && "handleEvent should not invoke handler for unregistered event type");
}

// Test publishEvent with EventBus
void test_BaseEventAgent_publishEvent_with_bus() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);
    TestAgent agent("agent1");
    MockConsumer consumer("consumer1");

    agent.registerWithEventBus(&bus);
    consumer.registerWithEventBus(&bus);
    agent.publishEvent<TestEvent>("consumer1", 42);

    size_t eventCount = consumer.receivedEvents.size();
    assert(eventCount == 1 && "Consumer should receive event published by agent");
    TestEvent* receivedEvent = (TestEvent*)consumer.receivedEvents[0];
    assert(receivedEvent->value == 42 && "Received event should match published event");
    assert(receivedEvent->sourceId == "agent1" && "Received event should have agent's ID as source");
}

// Test self-communication (agent publishes and consumes its own event)
void test_BaseEventAgent_self_communication() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);
    TestAgent agent("agent1");
    agent.registerWithEventBus(&bus);

    agent.publishEvent<TestEvent>("", 42);
    size_t eventCount = agent.receivedEvents.size();
    assert(eventCount == 1 && "Agent should receive its own published event");
    TestEvent* receivedEvent = (TestEvent*)agent.receivedEvents[0];
    assert(receivedEvent->value == 42 && "Received event should match published event");
    assert(receivedEvent->sourceId == "agent1" && "Received event should have agent's ID as source");
}

// Test concurrent publish and consume
void test_BaseEventAgent_publish_and_consume_concurrent() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);
    TestAgent agent("agent1");
    agent.registerWithEventBus(&bus);

    const int numThreads = 4;
    const int eventsPerThread = 10;
    vector<thread> publishers;

    for (int i = 0; i < numThreads; ++i) {
        publishers.emplace_back([i, &agent]() {
            for (int j = 0; j < eventsPerThread; ++j) {
                agent.publishEvent<TestEvent>("", i * eventsPerThread + j);
            }
        });
    }

    for (auto& publisher : publishers) {
        publisher.join();
    }

    size_t eventCount = agent.receivedEvents.size();
    assert(eventCount == numThreads * eventsPerThread && "Agent should receive all events from concurrent publishing");
}

// Test multiple handlers for the same event type
void test_BaseEventAgent_registerHandler_multiple_handlers() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);
    TestAgent agent("agent1");
    agent.registerWithEventBus(&bus);

    vector<int> values;
    // Register the same handler twice
    auto handler = [&values](TestEvent& event) {
        values.push_back(event.value + 1);
    };
    agent.registerHandler<TestEvent>(handler);
    agent.registerHandler<TestEvent>(handler);

    TestEvent event = TestEvent(42);
    agent.handleEvent(event);
    size_t eventCount = agent.receivedEvents.size();
    assert(eventCount == 1 && "Original handler should push to receivedEvents"); // Update intent
    assert(values.size() == 2 && "Second handler should record twice due to duplicate registration");
    assert(values[0] == 43 && "First call should record event.value + 1");
    assert(values[1] == 43 && "Second call should record event.value + 1");
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
TEST(test_BaseEventAgent_self_communication);
TEST(test_BaseEventAgent_publish_and_consume_concurrent);
TEST(test_BaseEventAgent_registerHandler_multiple_handlers);
#endif
