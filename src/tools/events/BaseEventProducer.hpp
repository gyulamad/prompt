#pragma once

#include <memory>

#include "Event.hpp"
#include "EventBus.hpp"
#include "EventProducer.hpp"

using namespace std;

namespace tools::events {

    /**
     * Base implementation of an event producer with helper methods
     */
    class BaseEventProducer : public EventProducer, public enable_shared_from_this<BaseEventProducer> {
    public:
        BaseEventProducer(const ComponentId& id) : m_id(id) {}
        
        void registerWithEventBus(EventBus* bus) override {
            NULLCHK(bus);
            lock_guard<mutex> lock(producerMutex);
            m_eventBus = bus;
            bus->registerProducer(shared_from_this());
        }
        
        ComponentId getId() const override {
            return m_id;
        }

        // Helper method to publish events
        template<typename EventType>
        void publishEvent(shared_ptr<EventType> event) {
            NULLCHK(event, "Cannot publish null event");
            if (m_eventBus) {
                event->sourceId = m_id;
                event->timestamp = chrono::system_clock::now();
                m_eventBus->publishEvent(event);
            }
        }

    private:
        ComponentId m_id;
        EventBus* m_eventBus = nullptr;
        mutex producerMutex;
    };

}

#ifdef TEST

#include "../utils/Test.hpp"
#include "../utils/tests/MockLogger.hpp"
#include "tests/TestEvent.hpp"
#include "tests/MockConsumer.hpp"

// Test registration with EventBus
void test_BaseEventProducer_registerWithEventBus_basic() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue);
    auto producer = make_shared<BaseEventProducer>("producer1");

    producer->registerWithEventBus(&bus);
    bool executedWithoutCrash = true;
    assert(executedWithoutCrash == true && "registerWithEventBus should not crash");

    // Verify producer can publish after registration (indirect test of registration)
    auto event = make_shared<TestEvent>(42);
    producer->publishEvent(event);
    assert(event->sourceId == "producer1" && "Published event should have producer's ID as source");
}

// Test getId method
void test_BaseEventProducer_getId_basic() {
    auto producer = make_shared<BaseEventProducer>("producer1");
    string id = producer->getId();

    assert(id == "producer1" && "getId should return the constructor-provided ID");
}

// Test publishEvent with EventBus
void test_BaseEventProducer_publishEvent_with_bus() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue);
    auto producer = make_shared<BaseEventProducer>("producer1");
    auto consumer = make_shared<MockConsumer>("consumer1");

    producer->registerWithEventBus(&bus);
    consumer->registerWithEventBus(&bus);
    auto event = make_shared<TestEvent>(42);
    producer->publishEvent(event);

    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 1 && "Consumer should receive event published by producer");
    auto receivedEvent = static_pointer_cast<TestEvent>(consumer->receivedEvents[0]);
    assert(receivedEvent->value == 42 && "Received event should match published event");
    assert(receivedEvent->sourceId == "producer1" && "Received event should have producer's ID as source");
    assert(receivedEvent->timestamp <= chrono::system_clock::now() && "Event timestamp should be set");
    // assert(bus.getEventQueueRef().available() == 0);
}

// Test publishEvent without EventBus
void test_BaseEventProducer_publishEvent_no_bus() {
    auto producer = make_shared<BaseEventProducer>("producer1");
    auto event = make_shared<TestEvent>(42);

    producer->publishEvent(event);  // No bus registered
    bool executedWithoutCrash = true;
    assert(executedWithoutCrash == true && "publishEvent without bus should not crash");
    assert(event->sourceId.empty() && "Event sourceId should not be set without bus");
    assert(event->timestamp == chrono::time_point<chrono::system_clock>() && "Event timestamp should not be set without bus");
}

// Test publishEvent with async EventBus
void test_BaseEventProducer_publishEvent_async_bus() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(true, logger, eventQueue);  // Async mode
    auto producer = make_shared<BaseEventProducer>("producer1");
    auto consumer = make_shared<MockConsumer>("consumer1");

    producer->registerWithEventBus(&bus);
    consumer->registerWithEventBus(&bus);

    auto event = make_shared<TestEvent>(42);
    producer->publishEvent(event);

    this_thread::sleep_for(chrono::milliseconds(200));  // Wait for async processing

    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 1 && "Consumer should receive event in async mode");
    auto receivedEvent = static_pointer_cast<TestEvent>(consumer->receivedEvents[0]);
    assert(receivedEvent->value == 42 && "Received event should match published event");
    assert(receivedEvent->sourceId == "producer1" && "Received event should have producer's ID as source");
}

// Test exception case: null event pointer in publishEvent throws an exception
void test_BaseEventProducer_publishEvent_null_event() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue);
    auto producer = make_shared<BaseEventProducer>("producer1");
    producer->registerWithEventBus(&bus);

    shared_ptr<TestEvent> nullEvent = nullptr;
    bool thrown = false;

    try {
        producer->publishEvent(nullEvent);
    } catch (const exception& e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Cannot publish null event") && "Exception message should indicate null event error");
    }

    assert(thrown == true && "publishEvent should throw an exception for null event");
}

// Test concurrent publishing
void test_BaseEventProducer_publishEvent_concurrent() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue);
    auto producer = make_shared<BaseEventProducer>("producer1");
    auto consumer = make_shared<MockConsumer>("consumer1");
    producer->registerWithEventBus(&bus);
    consumer->registerWithEventBus(&bus);

    const int numThreads = 4;
    const int eventsPerThread = 10;
    vector<thread> publishers;

    for (int i = 0; i < numThreads; ++i) {
        publishers.emplace_back([i, &producer]() {
            for (int j = 0; j < eventsPerThread; ++j) {
                auto event = make_shared<TestEvent>(i * eventsPerThread + j);
                producer->publishEvent(event);
            }
        });
    }

    for (auto& publisher : publishers) {
        publisher.join();
    }

    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == numThreads * eventsPerThread && "Consumer should receive all events from concurrent publishing");
}

// Test multiple event types
void test_BaseEventProducer_publishEvent_multiple_types() {
    class OtherEvent : public Event {
    public:
        type_index getType() const override { return type_index(typeid(OtherEvent)); }
        int data;
        OtherEvent(int d) : data(d) {}
    };

    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue);
    auto producer = make_shared<BaseEventProducer>("producer1");

    // Custom consumer for multiple event types
    class MultiTypeConsumer : public EventConsumer, public enable_shared_from_this<MultiTypeConsumer> {
    public:
        MultiTypeConsumer(const ComponentId& id) : id(id) {}
        vector<shared_ptr<Event>> receivedEvents;

        void handleEvent(shared_ptr<Event> event) override { receivedEvents.push_back(event); }
        void registerWithEventBus(EventBus* bus) override {
            NULLCHK(bus);
            bus->registerConsumer(shared_from_this());
            bus->registerEventInterest(id, type_index(typeid(TestEvent)));
            bus->registerEventInterest(id, type_index(typeid(OtherEvent)));
        }
        ComponentId getId() const override { return id; }
        bool canHandle(type_index eventType) const override {
            return eventType == type_index(typeid(TestEvent)) || eventType == type_index(typeid(OtherEvent));
        }

    private:
        ComponentId id;
    };

    auto consumer = make_shared<MultiTypeConsumer>("consumer1");
    producer->registerWithEventBus(&bus);
    consumer->registerWithEventBus(&bus);

    auto testEvent = make_shared<TestEvent>(42);
    auto otherEvent = make_shared<OtherEvent>(99);
    producer->publishEvent(testEvent);
    producer->publishEvent(otherEvent);

    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 2 && "Consumer should receive both event types");
    auto receivedTest = static_pointer_cast<TestEvent>(consumer->receivedEvents[0]);
    auto receivedOther = static_pointer_cast<OtherEvent>(consumer->receivedEvents[1]);
    assert(receivedTest->value == 42 && "Received TestEvent should match published event");
    assert(receivedOther->data == 99 && "Received OtherEvent should match published event");
    assert(receivedTest->sourceId == "producer1" && "TestEvent should have producer's ID as source");
    assert(receivedOther->sourceId == "producer1" && "OtherEvent should have producer's ID as source");
}

// Register tests
TEST(test_BaseEventProducer_registerWithEventBus_basic);
TEST(test_BaseEventProducer_getId_basic);
TEST(test_BaseEventProducer_publishEvent_with_bus);
TEST(test_BaseEventProducer_publishEvent_no_bus);
TEST(test_BaseEventProducer_publishEvent_async_bus);
TEST(test_BaseEventProducer_publishEvent_null_event);
TEST(test_BaseEventProducer_publishEvent_concurrent);
TEST(test_BaseEventProducer_publishEvent_multiple_types);
#endif
