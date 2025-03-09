#pragma once

#include <memory>
#include <unordered_map>
#include <functional>

#include "Event.hpp"
#include "EventBus.hpp"
#include "EventConsumer.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::events {

    /**
     * Base implementation of an event consumer with helper methods
     */
    class BaseEventConsumer : public EventConsumer, public enable_shared_from_this<BaseEventConsumer> {
    public:
        BaseEventConsumer(const ComponentId& id) : m_id(id) {}
        
        void registerWithEventBus(EventBus* bus) override {
            NULLCHK(bus);
            m_eventBus = bus;
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
            NULLCHK(event, "Cannot handle null event");
            lock_guard<mutex> lock(handlerMutex);  // Protect handler execution
            auto it = m_handlerMap.find(event->getType());
            if (it != m_handlerMap.end()) {
                it->second(event);
            }
        }

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
        
    protected:
        // Child classes should override this to register their handlers
        virtual void registerEventInterests() UNIMP

    private:
        ComponentId m_id;
        EventBus* m_eventBus;
        unordered_map<type_index, function<void(shared_ptr<Event>)>> m_handlerMap;
        mutex handlerMutex;  // Added for thread safety
    };

}

#ifdef TEST

#include "../utils/Test.hpp"
#include "../utils/tests/MockLogger.hpp"
#include "tests/TestEvent.hpp"
#include "tests/TestConsumer.hpp"

// Test registration with EventBus
void test_BaseEventConsumer_registerWithEventBus_basic() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue);
    auto consumer = make_shared<TestConsumer>("consumer1");

    consumer->registerWithEventBus(&bus);
    auto event = make_shared<TestEvent>(42);
    bus.publishEvent(event);

    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 1 && "Consumer should receive event after registration");
    auto receivedEvent = static_pointer_cast<TestEvent>(consumer->receivedEvents[0]);
    assert(receivedEvent->value == 42 && "Received event should match published event");
}

// Test getId method
void test_BaseEventConsumer_getId_basic() {
    auto consumer = make_shared<TestConsumer>("consumer1");
    string id = consumer->getId();

    assert(id == "consumer1" && "getId should return the constructor-provided ID");
}

// Test canHandle with registered event type
void test_BaseEventConsumer_canHandle_registered() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue);
    auto consumer = make_shared<TestConsumer>("consumer1");
    consumer->registerWithEventBus(&bus);  // Trigger registration of event interests

    bool canHandle = consumer->canHandle(type_index(typeid(TestEvent)));
    assert(canHandle == true && "canHandle should return true for registered event type");
}

// Test canHandle with unregistered event type
void test_BaseEventConsumer_canHandle_unregistered() {
    auto consumer = make_shared<TestConsumer>("consumer1");
    bool canHandle = consumer->canHandle(type_index(typeid(int)));  // Arbitrary unrelated type

    assert(canHandle == false && "canHandle should return false for unregistered event type");
}

// Test handleEvent with registered handler
void test_BaseEventConsumer_handleEvent_registered() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue);
    auto consumer = make_shared<TestConsumer>("consumer1");
    consumer->registerWithEventBus(&bus);  // Register to populate m_handlerMap

    auto event = make_shared<TestEvent>(42);
    consumer->handleEvent(event);
    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 1 && "handleEvent should invoke registered handler");
    auto receivedEvent = static_pointer_cast<TestEvent>(consumer->receivedEvents[0]);
    assert(receivedEvent->value == 42 && "Received event should match handled event");
}

// Test handleEvent with unregistered event type
void test_BaseEventConsumer_handleEvent_unregistered() {
    class OtherEvent : public Event {
    public:
        type_index getType() const override { return type_index(typeid(OtherEvent)); }
    };
    auto consumer = make_shared<TestConsumer>("consumer1");
    auto event = make_shared<OtherEvent>();

    consumer->handleEvent(event);
    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 0 && "handleEvent should not invoke handler for unregistered event type");
}

// Test registerHandler after EventBus registration
void test_BaseEventConsumer_registerHandler_post_registration() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue);
    auto consumer = make_shared<TestConsumer>("consumer1");
    consumer->registerWithEventBus(&bus);

    // Register a second handler for the same event type
    consumer->registerHandler<TestEvent>([consumer](shared_ptr<TestEvent> event) {
        consumer->receivedEvents.push_back(event);  // Double push for this test
    });
    auto event = make_shared<TestEvent>(42);
    bus.publishEvent(event);

    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 2 && "Handler registered post-registration should also receive event");
}

// Test exception case: null event pointer in handleEvent throws an exception
void test_BaseEventConsumer_handleEvent_null_event() {
    auto consumer = make_shared<TestConsumer>("consumer1");
    shared_ptr<Event> nullEvent = nullptr;

    bool thrown = false;
    try {
        consumer->handleEvent(nullEvent);  // Should throw an exception
    } catch (const exception& e) {  // Generic catch for custom exception
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Cannot handle null event") && "Exception message should indicate null event error");
    }

    assert(thrown == true && "handleEvent should throw an exception for null event");
    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 0 && "No events should be recorded for null event");
}

// Test multi-threaded event handling
void test_BaseEventConsumer_handleEvent_concurrent() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue);
    auto consumer = make_shared<TestConsumer>("consumer1");
    consumer->registerWithEventBus(&bus);

    const int numThreads = 4;
    const int eventsPerThread = 10;
    vector<thread> handlers;

    for (int i = 0; i < numThreads; ++i) {
        handlers.emplace_back([i, &consumer]() {
            for (int j = 0; j < eventsPerThread; ++j) {
                auto event = make_shared<TestEvent>(i * eventsPerThread + j);
                event->sourceId = "source" + to_string(i);
                consumer->handleEvent(event);
            }
        });
    }

    for (auto& handler : handlers) {
        handler.join();
    }

    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == numThreads * eventsPerThread && "Consumer should handle all events from concurrent threads");
}

// Test edge case: registering the same handler multiple times
void test_BaseEventConsumer_registerHandler_multiple_same() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue);
    auto consumer = make_shared<TestConsumer>("consumer1");
    consumer->registerWithEventBus(&bus);  // Initial handler registered

    // Register the same handler again
    consumer->registerHandler<TestEvent>([consumer](shared_ptr<TestEvent> event) {
        consumer->receivedEvents.push_back(event);
    });

    auto event = make_shared<TestEvent>(42);
    bus.publishEvent(event);
    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 2 && "Registering same handler again should result in duplicate handling");

    // Verify both executions occurred
    auto firstEvent = static_pointer_cast<TestEvent>(consumer->receivedEvents[0]);
    auto secondEvent = static_pointer_cast<TestEvent>(consumer->receivedEvents[1]);
    assert(firstEvent->value == 42 && "First handled event should match published event");
    assert(secondEvent->value == 42 && "Second handled event should match published event");
}

// Test edge case: registering different handlers for the same event type
void test_BaseEventConsumer_registerHandler_multiple_different() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue);
    auto consumer = make_shared<TestConsumer>("consumer1");
    consumer->registerWithEventBus(&bus);  // Initial handler pushes to receivedEvents

    // Register a different handler
    vector<int> values;
    consumer->registerHandler<TestEvent>([&values](shared_ptr<TestEvent> event) {
        values.push_back(event->value + 1);  // Different action
    });

    auto event = make_shared<TestEvent>(42);
    bus.publishEvent(event);
    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 0 && "Original handler should not push to receivedEvents due to handler override");
    assert(values.size() == 2 && "New handler should record twice due to duplicate interest");
    assert(values[0] == 43 && "First call should record event.value + 1");
    assert(values[1] == 43 && "Second call should record event.value + 1");
}

// Register tests
TEST(test_BaseEventConsumer_registerWithEventBus_basic);
TEST(test_BaseEventConsumer_getId_basic);
TEST(test_BaseEventConsumer_canHandle_registered);
TEST(test_BaseEventConsumer_canHandle_unregistered);
TEST(test_BaseEventConsumer_handleEvent_registered);
TEST(test_BaseEventConsumer_handleEvent_unregistered);
TEST(test_BaseEventConsumer_registerHandler_post_registration);
TEST(test_BaseEventConsumer_handleEvent_null_event);
TEST(test_BaseEventConsumer_handleEvent_concurrent);
TEST(test_BaseEventConsumer_registerHandler_multiple_same);
TEST(test_BaseEventConsumer_registerHandler_multiple_different);
#endif

/* TODO:
Limitations:
Multi-threaded test focuses on handleEvent; concurrent registerHandler calls could race (not tested here due to complexity).
Exception coverage is limited to null events; no other explicit throws exist in BaseEventConsumer.
*/