#pragma once

#include <memory>

#include "Event.hpp"
#include "EventProducer.hpp"

using namespace std;

namespace tools::events {

    /**
     * Base implementation of an event producer with helper methods
     */
    class BaseEventProducer : public EventProducer, public enable_shared_from_this<BaseEventProducer> {
    public:
        BaseEventProducer(const ComponentId& id) : m_id(id) {}
        
        void registerWithEventBus(shared_ptr<EventBus> bus) override {
            m_eventBus = bus;
            bus->registerProducer(shared_from_this());
        }
        
        ComponentId getId() const override {
            return m_id;
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

    private:
        ComponentId m_id;
        shared_ptr<EventBus> m_eventBus;
    };

}

#ifdef TEST

#include "../tests/MockLogger.hpp"
#include "tests/TestEvent.hpp"
#include "tests/MockConsumer.hpp"

// Test registration with EventBus
void test_BaseEventProducer_registerWithEventBus_basic() {
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<EventBus>(false, logger);
    auto producer = make_shared<BaseEventProducer>("producer1");

    producer->registerWithEventBus(bus);
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
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<EventBus>(false, logger);
    auto producer = make_shared<BaseEventProducer>("producer1");
    auto consumer = make_shared<MockConsumer>("consumer1");

    producer->registerWithEventBus(bus);
    consumer->registerWithEventBus(bus);
    auto event = make_shared<TestEvent>(42);
    producer->publishEvent(event);

    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 1 && "Consumer should receive event published by producer");
    auto receivedEvent = static_pointer_cast<TestEvent>(consumer->receivedEvents[0]);
    assert(receivedEvent->value == 42 && "Received event should match published event");
    assert(receivedEvent->sourceId == "producer1" && "Received event should have producer's ID as source");
    assert(receivedEvent->timestamp <= chrono::system_clock::now() && "Event timestamp should be set");
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
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<EventBus>(true, logger);  // Async mode
    auto producer = make_shared<BaseEventProducer>("producer1");
    auto consumer = make_shared<MockConsumer>("consumer1");

    producer->registerWithEventBus(bus);
    consumer->registerWithEventBus(bus);
    auto event = make_shared<TestEvent>(42);
    producer->publishEvent(event);

    this_thread::sleep_for(chrono::milliseconds(200));  // Wait for async processing
    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 1 && "Consumer should receive event in async mode");
    auto receivedEvent = static_pointer_cast<TestEvent>(consumer->receivedEvents[0]);
    assert(receivedEvent->value == 42 && "Received event should match published event");
    assert(receivedEvent->sourceId == "producer1" && "Received event should have producer's ID as source");
}

// Register tests
TEST(test_BaseEventProducer_registerWithEventBus_basic);
TEST(test_BaseEventProducer_getId_basic);
TEST(test_BaseEventProducer_publishEvent_with_bus);
TEST(test_BaseEventProducer_publishEvent_no_bus);
TEST(test_BaseEventProducer_publishEvent_async_bus);
#endif
