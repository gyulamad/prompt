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
    class BaseEventProducer : public EventProducer {
    public:
        BaseEventProducer(const ComponentId& id) : m_id(id) {}
        
        void registerWithEventBus(EventBus* bus) override {
            NULLCHK(bus);
            lock_guard<mutex> lock(producerMutex);
            m_eventBus = bus;
            bus->registerProducer(*this);
        }
        
        ComponentId getId() const override {
            return m_id;
        }

        // Helper method to publish events
        template<typename EventType, typename... Args>
        void publishEvent(const ComponentId& targetId, Args&&... args) {
            NULLCHK(m_eventBus, "Cannot publish event, event bus is nullptr.");
            m_eventBus->createAndPublishEvent<EventType>(m_id, targetId, forward<Args>(args)...);
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

// Test getId method
void test_BaseEventProducer_getId_basic() {
    BaseEventProducer producer("producer1");
    string id = producer.getId();

    assert(id == "producer1" && "getId should return the constructor-provided ID");
}

// Test publishEvent with EventBus
void test_BaseEventProducer_publishEvent_with_bus() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);
    BaseEventProducer producer("producer1");
    MockConsumer consumer("consumer1");

    producer.registerWithEventBus(&bus);
    consumer.registerWithEventBus(&bus);
    producer.publishEvent<TestEvent>("consumer1", 42);

    size_t eventCount = consumer.receivedEvents.size();
    assert(eventCount == 1 && "Consumer should receive event published by producer");
    TestEvent* receivedEvent = (TestEvent*)consumer.receivedEvents[0];
    assert(receivedEvent->value == 42 && "Received event should match published event");
    assert(receivedEvent->sourceId == "producer1" && "Received event should have producer's ID as source");
    assert(receivedEvent->timestamp <= chrono::system_clock::now() && "Event timestamp should be set");
    // assert(bus.getEventQueueRef().available() == 0);
}

// Test publishEvent without EventBus
void test_BaseEventProducer_publishEvent_no_bus() {
    BaseEventProducer producer("producer1");
    bool thrown = false;
    try {
        producer.publishEvent<TestEvent>("", 42);  // No bus registered
    } catch (exception &e) {
        thrown = true;
        assert(str_contains(e.what(), "Cannot publish event, event bus is nullptr."));
    }
    assert(thrown && "Producer withour bus should throw");
}

// Test concurrent publishing
void test_BaseEventProducer_publishEvent_concurrent() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);
    BaseEventProducer producer("producer1");
    MockConsumer consumer("consumer1");
    producer.registerWithEventBus(&bus);
    consumer.registerWithEventBus(&bus);

    const int numThreads = 4;
    const int eventsPerThread = 10;
    vector<thread> publishers;

    for (int i = 0; i < numThreads; ++i) {
        publishers.emplace_back([i, &producer]() {
            for (int j = 0; j < eventsPerThread; ++j) {
                producer.publishEvent<TestEvent>("consumer1", i * eventsPerThread + j);
            }
        });
    }

    for (thread& publisher : publishers) {
        if (publisher.joinable()) publisher.join();
    }

    size_t eventCount = consumer.receivedEvents.size();
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
    EventBus bus(logger, eventQueue);
    BaseEventProducer producer("producer1");

    // Custom consumer for multiple event types
    class MultiTypeConsumer : public EventConsumer {
    public:
        MultiTypeConsumer(const ComponentId& id) : id(id) {}
        vector<Event*> receivedEvents;

        void handleEvent(Event& event) override { receivedEvents.push_back(&event); }
        void registerWithEventBus(EventBus* bus) override {
            NULLCHK(bus);
            bus->registerConsumer(*this);
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

    MultiTypeConsumer consumer("consumer1");
    producer.registerWithEventBus(&bus);
    consumer.registerWithEventBus(&bus);

    producer.publishEvent<TestEvent>("consumer1", 42);
    producer.publishEvent<OtherEvent>("consumer1", 99);

    size_t eventCount = consumer.receivedEvents.size();
    assert(eventCount == 2 && "Consumer should receive both event types");
    TestEvent* receivedTest = (TestEvent*)consumer.receivedEvents[0];
    OtherEvent* receivedOther = (OtherEvent*)consumer.receivedEvents[1];
    assert(receivedTest->value == 42 && "Received TestEvent should match published event");
    assert(receivedOther->data == 99 && "Received OtherEvent should match published event");
    assert(receivedTest->sourceId == "producer1" && "TestEvent should have producer's ID as source");
    assert(receivedOther->sourceId == "producer1" && "OtherEvent should have producer's ID as source");
}

// Register tests
TEST(test_BaseEventProducer_getId_basic);
TEST(test_BaseEventProducer_publishEvent_with_bus);
TEST(test_BaseEventProducer_publishEvent_no_bus);
TEST(test_BaseEventProducer_publishEvent_concurrent);
TEST(test_BaseEventProducer_publishEvent_multiple_types);
#endif
