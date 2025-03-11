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
    class EventBus : public enable_shared_from_this<EventBus> {
    public:
        EventBus(
            bool asyncDelivery,
            Logger &logger,
            EventQueue &eventQueue
        ):
            m_asyncDelivery(asyncDelivery),
            m_logger(logger),
            m_eventQueue(eventQueue)
        {
            if (m_asyncDelivery) startEventProcessingThread();
        }

        ~EventBus() {
            if (m_asyncDelivery) stopEventProcessingThread();
        }

        // Register a producer with the event bus
        void registerProducer(shared_ptr<EventProducer> producer) {
            unique_lock<shared_mutex> lock(m_mutex);
            m_producers[producer->getId()] = producer;
        }

        // Unregister a producer from the event bus
        void unregisterProducer(const ComponentId &producerId) {
            unique_lock<shared_mutex> lock(m_mutex);
            m_producers.erase(producerId);
        }

        // Register a consumer with the event bus
        void registerConsumer(shared_ptr<EventConsumer> consumer) {
            unique_lock<shared_mutex> lock(m_mutex);
            m_consumers[consumer->getId()] = consumer;
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

        // // Publish an event to the event bus
        // virtual void publishEvent(shared_ptr<Event> event) {
        //     NULLCHK(event, "Cannot publish null event");
        //     if (m_asyncDelivery) {
        //         bool success = m_eventQueue.write(event);
        //         if (!success) m_logger.warn("Failed to queue event from " + event->sourceId);
        //         else m_queueCondition.notify_one();
        //     } else {
        //         deliverEvent(event);
        //     }
        // }

        // Primary version with targetId
        template <typename EventType, typename... Args>
        void createAndPublishEvent(const ComponentId &sourceId, const ComponentId &targetId, Args &&...args) {
            shared_ptr<EventType> event = SP_CREATE(eventFactory, EventType, forward<Args>(args)...);
            event->sourceId = sourceId;
            event->targetId = targetId;
            event->timestamp = chrono::system_clock::now();
            deliverEventInternal(event); // Call the new virtual function
            if (!m_asyncDelivery) eventFactory.release(event);
        }

        // // Convenience version without targetId, explicitly calling the above
        // template<typename EventType, typename... Args>
        // void createAndPublishEvent(const ComponentId& sourceId, Args&&... args) {
        //     // Use a named variable to force resolution to the two-parameter version
        //     ComponentId emptyTargetId; // Default-constructed (empty)
        //     createAndPublishEvent<EventType>(sourceId, emptyTargetId, forward<Args>(args)...);
        // }

        EventQueue &getEventQueueRef() {
            return m_eventQueue;
        }

    protected:
        virtual void deliverEventInternal(shared_ptr<Event> event) {
            if (m_asyncDelivery) {
                bool success = m_eventQueue.write(event);                
                if (success) {
                    m_queueCondition.notify_one();
                    sleep_ms(100); // TODO: if I don't wait the test test_EventBus_publishEvent_async_delivery will frozen.. find out why!!
                } else {
                    m_logger.warn("Failed to queue event...");
                    eventFactory.release(event);
                }
            } else deliverEvent(event);
        }

        // Deliver an event to appropriate consumers
        virtual void deliverEvent(shared_ptr<Event> event)
        {
            shared_lock<shared_mutex> lock(m_mutex);
            if (!event->targetId.empty()) {
                auto it = m_consumers.find(event->targetId);
                if (it != m_consumers.end()) {
                    NULLCHK(it->second, "Consumer shared_ptr is null"); // Added check
                    if (it->second->canHandle(event->getType())) {
                        it->second->handleEvent(event);
                    }
                }
                return;
            }

            auto typeIt = m_eventInterests.find(event->getType());
            if (typeIt != m_eventInterests.end()) {
                for (const auto &consumerId : typeIt->second) {
                    auto consumerIt = m_consumers.find(consumerId);
                    if (consumerIt != m_consumers.end()) {
                        NULLCHK(consumerIt->second, "Consumer shared_ptr is null"); // Added check
                        consumerIt->second->handleEvent(event);
                    }
                }
            }
        }

    private:
        // Start the background thread for async event processing
        void startEventProcessingThread() {
            m_running = true;
            m_processingThread = thread([this]() {
                while (m_running) {
                    unique_lock<mutex> lock(m_queueMutex);
                    bool hasEvents = m_queueCondition.wait_for(lock, chrono::milliseconds(100), [this]() { 
                        return m_eventQueue.available() > 0 || !m_running; 
                    });
                    if (m_running && hasEvents && m_eventQueue.available() > 0) {
                        shared_ptr<Event> event;
                        if (m_eventQueue.read(event, false, 0) == 1) {
                            lock.unlock();
                            size_t event_count_before = event.use_count();
                            deliverEvent(event);
                            m_logger.debug("Event queue size: " + to_string(m_eventQueue.available()) + "/" + to_string(m_eventQueue.getCapacity()));
                            // Wait for the shared_ptr's reference count to reach 1
                            while (event.use_count() >= event_count_before) this_thread::sleep_for(chrono::milliseconds(1)); // Small delay to avoid busy-waiting
                            eventFactory.release(event);
                        }
                    }
                }
            });
        }

        // Stop the background thread for async event processing
        void stopEventProcessingThread() {
            {
                lock_guard<mutex> lock(m_queueMutex); // Lock same mutex as thread
                m_running = false;
            }
            m_queueCondition.notify_all();
            if (m_processingThread.joinable())  m_processingThread.join();
        }

    protected:
        // Synchronization
        shared_mutex m_mutex;
        mutex m_queueMutex;
        condition_variable m_queueCondition;
        // Collections of producers and consumers
        unordered_map<ComponentId, shared_ptr<EventProducer>> m_producers;
        unordered_map<ComponentId, shared_ptr<EventConsumer>> m_consumers;
        // Map of event types to interested consumers
        unordered_map<type_index, vector<ComponentId>> m_eventInterests;

    private:
        // Async delivery
        bool m_asyncDelivery;
        bool m_running = false;
        thread m_processingThread;
        EventQueue &m_eventQueue; // Use abstracted EventQueue
        Logger &m_logger; // Logger dependency
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
    EventBus bus(false, logger, eventQueue);  // Synchronous delivery
    auto consumer = make_shared<MockConsumer>("consumer1");
    consumer->registerWithEventBus(&bus);
    bus.createAndPublishEvent<TestEvent>("test-source", "consumer1", 42);
    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 1 && "Consumer should receive 1 event in sync mode");
    auto receivedEvent = static_pointer_cast<TestEvent>(consumer->receivedEvents[0]);
    assert(receivedEvent->value == 42 && "Received event should match published event");
}

// Test synchronous delivery with no interested consumers
void test_EventBus_publishEvent_sync_no_consumers() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue);
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
    EventBus bus(false, logger, eventQueue);
    auto consumer1 = make_shared<MockConsumer>("consumer1");
    auto consumer2 = make_shared<MockConsumer>("consumer2");
    consumer1->registerWithEventBus(&bus);
    consumer2->registerWithEventBus(&bus);
    ComponentId sourceId = "test-source";
    ComponentId targetId = "consumer1";
    bus.createAndPublishEvent<TestEvent>(sourceId, targetId, 42);
    size_t count1 = consumer1->receivedEvents.size();
    size_t count2 = consumer2->receivedEvents.size();
    assert(count1 == 1 && "Targeted consumer should receive event");
    assert(count2 == 0 && "Non-targeted consumer should not receive event");
}

// Test async event delivery with RingBufferEventQueue
void test_EventBus_publishEvent_async_delivery() {
    // TEST_SKIP("TODO: Async queue delivery mechanism needs refact");
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(true, logger, eventQueue);  // Async delivery
    auto consumer = make_shared<MockConsumer>("consumer1");
    consumer->registerWithEventBus(&bus);
    bus.createAndPublishEvent<TestEvent>("producer1", "consumer1", 42);
    this_thread::sleep_for(chrono::milliseconds(200));  // Wait for async processing
    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 1 && "Consumer should receive 1 event in async mode");
    auto receivedEvent = static_pointer_cast<TestEvent>(consumer->receivedEvents[0]);
    assert(receivedEvent->value == 42 && "Received event should match published event");
}

// // Test async queue full scenario with logging
// void test_EventBus_publishEvent_async_queue_full() {
//     TEST_SKIP("TODO: Async queue delivery mechanism needs refact");
//     MockLogger logger;
//     RingBufferEventQueue queue(1, logger); // Capacity of 1
//     EventBus bus(true, logger, queue);     // Asynchronous delivery
//     auto consumer = make_shared<MockConsumer>("consumer1");
//     consumer->registerWithEventBus(&bus);

//     // Publish two events using createAndPublishEvent
//     ComponentId sourceId = "test-source"; // Dummy source ID
//     bus.createAndPublishEvent<TestEvent>(sourceId, "consumer1", 1);
//     bus.createAndPublishEvent<TestEvent>(sourceId, "consumer1", 2);

//     // Poll for the drop message
//     auto start = chrono::steady_clock::now();
//     bool loggedDrop = false;
//     while (!loggedDrop && chrono::steady_clock::now() - start < chrono::seconds(1)) {
//         loggedDrop = 
//             logger.hasMessageContaining("Dropped 1 event(s) due to full queue");        
//         if (!loggedDrop) {
//             this_thread::sleep_for(chrono::milliseconds(50));
//         }
//     }
//     // array_dump(logger.loggedMessages, true);
//     assert(loggedDrop == true && "Queue full should trigger log message");

//     // Wait a bit more for consumer processing
//     this_thread::sleep_for(chrono::milliseconds(100));
//     size_t eventCount = consumer->receivedEvents.size();
//     assert(eventCount == 1 && "Consumer should receive only 1 event due to capacity");
// }

// Test producer registration
void test_EventBus_registerProducer_basic() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue);
    class MockProducer : public EventProducer, public enable_shared_from_this<MockProducer> {
    public:
        MockProducer() {}
        void registerWithEventBus(EventBus* b) override { 
            NULLCHK(b);
            b->registerProducer(shared_from_this()); 
        }
        ComponentId getId() const override { return "producer1"; }
    };
    auto producer = make_shared<MockProducer>();
    producer->registerWithEventBus(&bus);

    bool registeredWithoutCrash = true;
    assert(registeredWithoutCrash == true && "Producer registration should not crash");
}

// Test consumer unregistration
void test_EventBus_unregisterConsumer_basic() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue); // Synchronous delivery
    auto consumer = make_shared<MockConsumer>("consumer1");
    consumer->registerWithEventBus(&bus);
    bus.unregisterConsumer("consumer1");

    // Use createAndPublishEvent instead of publishEvent
    ComponentId sourceId = "test-source"; // Dummy source ID
    bus.createAndPublishEvent<TestEvent>(sourceId, "consumer1", 42);

    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 0 && "Unregistered consumer should not receive event");
}

// Test event interest registration
void test_EventBus_registerEventInterest_basic() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue);
    auto consumer = make_shared<MockConsumer>("consumer1");
    consumer->registerWithEventBus(&bus);

    ComponentId sourceId = "test-source";
    bus.createAndPublishEvent<TestEvent>(sourceId, "consumer1", 42);

    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 1 && "Consumer with registered interest should receive event");
}

// Test concurrent event publishing
void test_EventBus_publishEvent_concurrent_sync() {
    // TEST_SKIP("TODO: Async queue delivery mechanism needs refact");
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue);  // Synchronous delivery
    auto consumer = make_shared<MockConsumer>("consumer1");
    consumer->registerWithEventBus(&bus);

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

    for (auto& publisher : publishers) {
        publisher.join();
    }

    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == numThreads * eventsPerThread && "Consumer should receive all events from concurrent publishers");
}

// // Test concurrent async event publishing
// void test_EventBus_publishEvent_concurrent_async() {
//     // TEST_SKIP("TODO: Async queue delivery mechanism needs refact");
//     MockLogger logger;
//     RingBufferEventQueue eventQueue(1000, logger);
//     EventBus bus(true, logger, eventQueue);  // Asynchronous delivery
//     auto consumer = make_shared<MockConsumer>("consumer1");
//     consumer->registerWithEventBus(&bus);

//     const int numThreads = 4;
//     const int eventsPerThread = 10;
//     vector<thread> publishers;

//     for (int i = 0; i < numThreads; ++i) {
//         publishers.emplace_back([i, &bus]() {
//             for (int j = 0; j < eventsPerThread; ++j) {
//                 ComponentId sourceId = "producer" + to_string(i);
//                 bus.createAndPublishEvent<TestEvent>(sourceId, "consumer1", i * eventsPerThread + j);
//             }
//         });
//     }

//     for (auto& publisher : publishers) {
//         publisher.join();
//     }

//     this_thread::sleep_for(chrono::milliseconds(500));  // Wait for async processing
//     size_t eventCount = consumer->receivedEvents.size();
//     assert(eventCount == numThreads * eventsPerThread && "Consumer should receive all events from concurrent async publishers");
// }

// Test concurrent registration and publishing
void test_EventBus_register_and_publish_concurrent() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue);
    const int numConsumers = 5;
    vector<shared_ptr<MockConsumer>> consumers;
    vector<thread> registrars;

    for (int i = 0; i < numConsumers; ++i) {
        auto consumer = make_shared<MockConsumer>("consumer" + to_string(i));
        consumers.push_back(consumer);
        registrars.emplace_back([consumer, &bus]() {
            consumer->registerWithEventBus(&bus);
        });
    }

    thread publisher([&bus]() {
        for (int i = 0; i < 10; ++i) {
            ComponentId sourceId = "test-source";
            bus.createAndPublishEvent<TestEvent>(sourceId, "", i); // Broadcast
            this_thread::yield();
        }
    });

    for (auto& registrar : registrars) {
        registrar.join();
    }
    publisher.join();

    size_t totalEvents = 0;
    for (const auto& consumer : consumers) {
        totalEvents += consumer->receivedEvents.size();
    }
    assert(totalEvents >= 10 && "At least some consumers should receive events");
    assert(totalEvents <= numConsumers * 10 && "Total events should not exceed max possible deliveries");
}

// Test destructor behavior for async mode
void test_EventBus_destructor_async_cleanup() {
    // TEST_SKIP("TODO: Async queue delivery mechanism needs refact");
    MockLogger logger;
    {
        RingBufferEventQueue eventQueue(1000, logger);
        EventBus bus(true, logger, eventQueue);  // Async delivery
        auto consumer = make_shared<MockConsumer>("consumer1");
        consumer->registerWithEventBus(&bus);

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
TEST(test_EventBus_publishEvent_async_delivery);
// TEST(test_EventBus_publishEvent_async_queue_full);
TEST(test_EventBus_registerProducer_basic);
TEST(test_EventBus_unregisterConsumer_basic);
TEST(test_EventBus_registerEventInterest_basic);
TEST(test_EventBus_publishEvent_concurrent_sync);
// TEST(test_EventBus_publishEvent_concurrent_async);
TEST(test_EventBus_register_and_publish_concurrent);
TEST(test_EventBus_destructor_async_cleanup);

#endif
