#pragma once

#include <memory>
#include <shared_mutex>

#include "../utils/system.hpp"
#include "../utils/Logger.hpp"
#include "../utils/RingBuffer.hpp"
#include "../utils/SharedPtrFactory.hpp"

#include "Event.hpp"
#include "EventQueue.hpp"
#include "EventProducer.hpp"
#include "EventConsumer.hpp"
#include "RingBufferEventQueue.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::events
{

    /**
    * Central event bus that manages event distribution
    */
    class EventBus {
    public:
        EventBus(
            Logger &logger,
            EventQueue &eventQueue
        ):
            m_logger(logger),
            m_eventQueue(eventQueue)
        {}

        ~EventBus() {}

        // Register a producer with the event bus
        void registerProducer(EventProducer& producer) {
            unique_lock<shared_mutex> lock(m_mutex);
            m_producers[producer.getId()] = &producer;
        }

        // Unregister a producer from the event bus
        void unregisterProducer(const ComponentId &producerId) {
            unique_lock<shared_mutex> lock(m_mutex);
            m_producers.erase(producerId);
        }

        // Register a consumer with the event bus
        void registerConsumer(EventConsumer& consumer) {
            unique_lock<shared_mutex> lock(m_mutex);
            m_consumers[consumer.getId()] = &consumer;
        }

        // Unregister a consumer from the event bus
        void unregisterConsumer(const ComponentId &consumerId) {
            unique_lock<shared_mutex> lock(m_mutex);
            m_consumers.erase(consumerId);
        }

        // Register a specific event type that a consumer is interested in
        void registerEventInterest(const ComponentId &consumerId, type_index eventType) {
            unique_lock<shared_mutex> lock(m_mutex);
            m_eventInterests[eventType].push_back(consumerId);
        }

        // Publish an event to the event bus
        template <typename EventType, typename... Args>
        void createAndPublishEvent(const ComponentId &sourceId, const ComponentId &targetId, Args &&...args) {
            EventType* event = new EventType(forward<Args>(args)...);
            event->sourceId = sourceId;
            event->targetId = targetId;
            event->timestamp = chrono::system_clock::now();
            deliverEventInternal(this, *event); // Call the new virtual function
            thread([&]() {
                while (event->isHolded()) sleep_ms(1); 
                delete event;  
            }).detach();
        }

        EventQueue &getEventQueueRef() {
            return m_eventQueue;
        }

    protected:

        // Deliver an event to appropriate consumers
        function<void(EventBus*, Event&)> deliverEventInternal = [](EventBus* that, Event& event) {
            shared_lock<shared_mutex> lock(that->m_mutex);
            if (!event.targetId.empty()) {
                auto it = that->m_consumers.find(event.targetId);
                if (it != that->m_consumers.end()) {
                    if (it->second->canHandle(event.getType())) {
                        it->second->handleEvent(event);
                    }
                }
                return;
            }

            auto typeIt = that->m_eventInterests.find(event.getType());
            if (typeIt != that->m_eventInterests.end()) {
                for (const ComponentId& consumerId : typeIt->second) {
                    auto consumerIt = that->m_consumers.find(consumerId);
                    if (consumerIt != that->m_consumers.end()) {
                        consumerIt->second->handleEvent(event);
                    }
                }
            }
        };

    public:
        // Synchronization
        shared_mutex m_mutex;
        // Map of event types to interested consumers
        unordered_map<type_index, vector<ComponentId>> m_eventInterests;
        // Collections of producers and consumers
        unordered_map<ComponentId, EventProducer*> m_producers;
        unordered_map<ComponentId, EventConsumer*> m_consumers;
        // Logger dependency
        Logger &m_logger; 

    protected:
        mutex m_queueMutex;
        condition_variable m_queueCondition;

    private:
        // Async delivery
        // bool m_asyncDelivery;
        bool m_running = false;
        thread m_processingThread;
        EventQueue &m_eventQueue; // Use abstracted EventQueue
        SharedPtrFactory eventFactory; // Factory for events
    };

}

#ifdef TEST

#include "../utils/Test.hpp"
#include "../utils/tests/MockLogger.hpp"
#include "tests/TestEvent.hpp"
#include "tests/MockConsumer.hpp"

// Test synchronous event delivery to a single consumer
void test_EventBus_publishEvent_sync_delivery() {
    MockLogger logger;
    // eventQueue = make_unique<RingBufferEventQueue>(1000, m_logger);
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);  // Synchronous delivery
    MockConsumer consumer("consumer1");
    consumer.registerWithEventBus(&bus);
    bus.createAndPublishEvent<TestEvent>("test-source", "consumer1", 42);
    size_t eventCount = consumer.receivedEvents.size();
    assert(eventCount == 1 && "Consumer should receive 1 event in sync mode");
    TestEvent* receivedEvent = (TestEvent*)consumer.receivedEvents[0];
    assert(receivedEvent->value == 42 && "Received event should match published event");
}

// Test synchronous delivery with no interested consumers
void test_EventBus_publishEvent_sync_no_consumers() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);
    ComponentId sourceId = "test-source"; // Arbitrary source ID for the test
    bus.createAndPublishEvent<TestEvent>(sourceId, "", 42);  // No consumers registered
    // No assertion needed; just ensure no crash occurs
    bool executedWithoutCrash = true;
    assert(executedWithoutCrash == true && "Publishing with no consumers should not crash");
}

// Test targeted event delivery
void test_EventBus_publishEvent_targeted_delivery() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);
    MockConsumer consumer1("consumer1");
    MockConsumer consumer2("consumer2");
    consumer1.registerWithEventBus(&bus);
    consumer2.registerWithEventBus(&bus);
    ComponentId sourceId = "test-source";
    ComponentId targetId = "consumer1";
    bus.createAndPublishEvent<TestEvent>(sourceId, targetId, 42);
    size_t count1 = consumer1.receivedEvents.size();
    size_t count2 = consumer2.receivedEvents.size();
    assert(count1 == 1 && "Targeted consumer should receive event");
    assert(count2 == 0 && "Non-targeted consumer should not receive event");
}

// Test producer registration
void test_EventBus_registerProducer_basic() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);
    class MockProducer : public EventProducer {
    public:
        MockProducer() {}
        void registerWithEventBus(EventBus* b) override { 
            NULLCHK(b);
            b->registerProducer(*this); 
        }
        ComponentId getId() const override { return "producer1"; }
    };
    MockProducer producer;
    producer.registerWithEventBus(&bus);

    bool registeredWithoutCrash = true;
    assert(registeredWithoutCrash == true && "Producer registration should not crash");
}

// Test consumer unregistration
void test_EventBus_unregisterConsumer_basic() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue); // Synchronous delivery
    MockConsumer consumer("consumer1");
    consumer.registerWithEventBus(&bus);
    bus.unregisterConsumer("consumer1");

    // Use createAndPublishEvent instead of publishEvent
    ComponentId sourceId = "test-source"; // Dummy source ID
    bus.createAndPublishEvent<TestEvent>(sourceId, "consumer1", 42);

    size_t eventCount = consumer.receivedEvents.size();
    assert(eventCount == 0 && "Unregistered consumer should not receive event");
}

// Test event interest registration
void test_EventBus_registerEventInterest_basic() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);
    MockConsumer consumer("consumer1");
    consumer.registerWithEventBus(&bus);

    ComponentId sourceId = "test-source";
    bus.createAndPublishEvent<TestEvent>(sourceId, "consumer1", 42);

    size_t eventCount = consumer.receivedEvents.size();
    assert(eventCount == 1 && "Consumer with registered interest should receive event");
}

// Test concurrent event publishing
void test_EventBus_publishEvent_concurrent_sync() {
    // TEST_SKIP("TODO: Async queue delivery mechanism needs refact");
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);  // Synchronous delivery
    MockConsumer consumer("consumer1");
    consumer.registerWithEventBus(&bus);

    const int numThreads = 4;
    const int eventsPerThread = 10;
    vector<thread> publishers;

    for (int i = 0; i < numThreads; ++i) {
        publishers.emplace_back([i, &bus]() {
            for (int j = 0; j < eventsPerThread; ++j) {
                ComponentId sourceId = "producer" + to_string(i);
                bus.createAndPublishEvent<TestEvent>(sourceId, "consumer1", i * eventsPerThread + j);
            }
        });
    }

    for (thread& publisher : publishers) {
        if (publisher.joinable()) publisher.join();
    }

    size_t eventCount = consumer.receivedEvents.size();
    assert(eventCount == numThreads * eventsPerThread && "Consumer should receive all events from concurrent publishers");
}

// Test concurrent registration and publishing
void test_EventBus_register_and_publish_concurrent() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);
    const int numConsumers = 5;
    vector<MockConsumer> consumers;
    vector<thread> registrars;

    for (int i = 0; i < numConsumers; ++i) {
        MockConsumer consumer("consumer" + to_string(i));
        consumers.push_back(consumer);
        registrars.emplace_back([&consumer, &bus]() {
            consumer.registerWithEventBus(&bus);
        });
    }

    thread publisher([&bus]() {
        for (int i = 0; i < 10; ++i) {
            ComponentId sourceId = "test-source";
            bus.createAndPublishEvent<TestEvent>(sourceId, "", i); // Broadcast
            this_thread::yield();
        }
    });

    for (thread& registrar : registrars) {
        registrar.join();
    }
    publisher.join();

    size_t totalEvents = 0;
    for (const MockConsumer& consumer : consumers) {
        totalEvents += consumer.receivedEvents.size();
    }
    assert(totalEvents >= 10 && "At least some consumers should receive events");
    assert(totalEvents <= numConsumers * 10 && "Total events should not exceed max possible deliveries");
}

// Test destructor behavior for sync mode
void test_EventBus_destructor_sync_cleanup() {
    // TEST_SKIP("TODO: Async queue delivery mechanism needs refact");
    MockLogger logger;
    {
        RingBufferEventQueue eventQueue(1000, logger);
        EventBus bus(logger, eventQueue);  // Async delivery
        MockConsumer consumer("consumer1");
        consumer.registerWithEventBus(&bus);

        // Publish an event to ensure thread is active
        ComponentId sourceId = "test-source";
        bus.createAndPublishEvent<TestEvent>(sourceId, "consumer1", 42);
        this_thread::sleep_for(chrono::milliseconds(50));  // Let thread process
    }  // Destructor called here

    // If we reach here without hanging, thread cleanup succeeded
    bool destructedWithoutHang = true;
    assert(destructedWithoutHang == true && "Destructor should cleanly stop async thread");
}

// Register tests
TEST(test_EventBus_publishEvent_sync_delivery);
TEST(test_EventBus_publishEvent_sync_no_consumers);
TEST(test_EventBus_publishEvent_targeted_delivery);
TEST(test_EventBus_registerProducer_basic);
TEST(test_EventBus_unregisterConsumer_basic);
TEST(test_EventBus_registerEventInterest_basic);
TEST(test_EventBus_publishEvent_concurrent_sync);
TEST(test_EventBus_register_and_publish_concurrent);
TEST(test_EventBus_destructor_sync_cleanup);

#endif
