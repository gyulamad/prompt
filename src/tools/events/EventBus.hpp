#pragma once

#include <memory>

#include "../Logger.hpp"
#include "../RingBuffer.hpp"

#include "Event.hpp"
#include "EventQueue.hpp"
#include "EventProducer.hpp"
#include "EventConsumer.hpp"
#include "RingBufferEventQueue.hpp"

using namespace std;

namespace tools::events {

    /**
     * Central event bus that manages event distribution
     */
    class EventBus : public enable_shared_from_this<EventBus> {
    public:
        EventBus(bool asyncDelivery = false, shared_ptr<Logger> logger = nullptr,
                unique_ptr<EventQueue> eventQueue = nullptr)
            : m_asyncDelivery(asyncDelivery), m_logger(logger) {
            if (!eventQueue) {
                m_eventQueue = make_unique<RingBufferEventQueue>(1000, m_logger);  // Default: RingBuffer
            } else {
                m_eventQueue = move(eventQueue);
            }
            if (m_asyncDelivery) {
                startEventProcessingThread();
            }
        }
        
        ~EventBus() {
            if (m_asyncDelivery) {
                stopEventProcessingThread();
            }
        }
        
        // Register a producer with the event bus
        void registerProducer(shared_ptr<EventProducer> producer) {
            lock_guard<mutex> lock(m_mutex);
            m_producers[producer->getId()] = producer;
        }
        
        // Unregister a producer from the event bus
        void unregisterProducer(const ComponentId& producerId) {
            lock_guard<mutex> lock(m_mutex);
            m_producers.erase(producerId);
        }
        
        // Register a consumer with the event bus
        void registerConsumer(shared_ptr<EventConsumer> consumer) {
            lock_guard<mutex> lock(m_mutex);
            m_consumers[consumer->getId()] = consumer;
        }
        
        // Unregister a consumer from the event bus
        void unregisterConsumer(const ComponentId& consumerId) {
            lock_guard<mutex> lock(m_mutex);
            m_consumers.erase(consumerId);
        }
        
        // Register a specific event type that a consumer is interested in
        void registerEventInterest(const ComponentId& consumerId, type_index eventType) {
            lock_guard<mutex> lock(m_mutex);
            m_eventInterests[eventType].push_back(consumerId);
        }
        
        // Publish an event to the event bus
        void publishEvent(shared_ptr<Event> event) {
            if (m_asyncDelivery) {
                bool success = m_eventQueue->write(event);
                if (!success && m_logger) {
                    m_logger->warn("Failed to queue event from " + event->sourceId);
                } else {
                    m_queueCondition.notify_one();
                }
            } else {
                deliverEvent(event);
            }
        }

    protected:
        // Deliver an event to appropriate consumers
        virtual void deliverEvent(shared_ptr<Event> event) {
            lock_guard<mutex> lock(m_mutex);
            
            // If the event has a specific target, deliver only to that target
            if (!event->targetId.empty()) {
                auto it = m_consumers.find(event->targetId);
                if (it != m_consumers.end() && it->second->canHandle(event->getType())) {
                    it->second->handleEvent(event);
                }
                return;
            }
            
            // Otherwise, deliver to all consumers that have registered interest
            auto typeIt = m_eventInterests.find(event->getType());
            if (typeIt != m_eventInterests.end()) {
                for (const auto& consumerId : typeIt->second) {
                    auto consumerIt = m_consumers.find(consumerId);
                    if (consumerIt != m_consumers.end()) {
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
                    m_queueCondition.wait_for(lock, chrono::milliseconds(100),
                        [this]() { return m_eventQueue->available() > 0 || !m_running; });
                    if (!m_running) break;

                    shared_ptr<Event> event;
                    if (m_eventQueue->read(event, false, 0) == 1) {
                        lock.unlock();
                        deliverEvent(event);
                    }
                }
            });
        }
        
        // Stop the background thread for async event processing
        void stopEventProcessingThread() {
            m_running = false;
            m_queueCondition.notify_all();
            if (m_processingThread.joinable()) {
                m_processingThread.join();
            }
        }

    protected:
        // Synchronization
        mutex m_mutex;
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
        unique_ptr<EventQueue> m_eventQueue;  // Use abstracted EventQueue
        shared_ptr<Logger> m_logger;          // Logger dependency
    };
    
}

#ifdef TEST

#include "../tests/MockLogger.hpp"
#include "tests/TestEvent.hpp"
#include "tests/MockConsumer.hpp"

// Test synchronous event delivery to a single consumer
void test_EventBus_publishEvent_sync_delivery() {
    auto bus = make_shared<EventBus>(false);  // Synchronous delivery
    auto consumer = make_shared<MockConsumer>("consumer1");
    consumer->registerWithEventBus(bus);
    auto event = make_shared<TestEvent>(42);

    bus->publishEvent(event);
    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 1 && "Consumer should receive 1 event in sync mode");
    auto receivedEvent = static_pointer_cast<TestEvent>(consumer->receivedEvents[0]);
    assert(receivedEvent->value == 42 && "Received event should match published event");
}

// Test synchronous delivery with no interested consumers
void test_EventBus_publishEvent_sync_no_consumers() {
    auto bus = make_shared<EventBus>(false);
    auto event = make_shared<TestEvent>(42);

    bus->publishEvent(event);  // No consumers registered
    // No assertion needed; just ensure no crash occurs
    bool executedWithoutCrash = true;
    assert(executedWithoutCrash == true && "Publishing with no consumers should not crash");
}

// Test targeted event delivery
void test_EventBus_publishEvent_targeted_delivery() {
    auto bus = make_shared<EventBus>(false);
    auto consumer1 = make_shared<MockConsumer>("consumer1");
    auto consumer2 = make_shared<MockConsumer>("consumer2");
    consumer1->registerWithEventBus(bus);
    consumer2->registerWithEventBus(bus);
    auto event = make_shared<TestEvent>(42);
    event->targetId = "consumer1";

    bus->publishEvent(event);
    size_t count1 = consumer1->receivedEvents.size();
    size_t count2 = consumer2->receivedEvents.size();
    assert(count1 == 1 && "Targeted consumer should receive event");
    assert(count2 == 0 && "Non-targeted consumer should not receive event");
}

// Test async event delivery with RingBufferEventQueue
void test_EventBus_publishEvent_async_delivery() {
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<EventBus>(true, logger);  // Async delivery
    auto consumer = make_shared<MockConsumer>("consumer1");
    consumer->registerWithEventBus(bus);
    auto event = make_shared<TestEvent>(42);

    bus->publishEvent(event);
    this_thread::sleep_for(chrono::milliseconds(200));  // Wait for async processing
    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 1 && "Consumer should receive 1 event in async mode");
    auto receivedEvent = static_pointer_cast<TestEvent>(consumer->receivedEvents[0]);
    assert(receivedEvent->value == 42 && "Received event should match published event");
}

// Test async queue full scenario with logging
void test_EventBus_publishEvent_async_queue_full() {
    auto logger = make_shared<MockLogger>();
    auto queue = make_unique<RingBufferEventQueue>(1, logger);
    auto bus = make_shared<EventBus>(true, logger, move(queue));
    auto consumer = make_shared<MockConsumer>("consumer1");
    consumer->registerWithEventBus(bus);

    bus->publishEvent(make_shared<TestEvent>(1));
    bus->publishEvent(make_shared<TestEvent>(2));
    this_thread::sleep_for(chrono::milliseconds(200));

    bool loggedDrop = logger->hasMessageContaining("Dropped 1 events");
    assert(loggedDrop == true && "Queue full should trigger drop callback and log");
    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 1 && "Consumer should receive only 1 event due to capacity");
}

// Test producer registration
void test_EventBus_registerProducer_basic() {
    auto bus = make_shared<EventBus>(false);
    class MockProducer : public EventProducer, public enable_shared_from_this<MockProducer> {
    public:
        MockProducer() {}
        void registerWithEventBus(shared_ptr<EventBus> b) override { b->registerProducer(shared_from_this()); }
        ComponentId getId() const override { return "producer1"; }
    };
    auto producer = make_shared<MockProducer>();
    producer->registerWithEventBus(bus);

    bool registeredWithoutCrash = true;
    assert(registeredWithoutCrash == true && "Producer registration should not crash");
}

// Test consumer unregistration
void test_EventBus_unregisterConsumer_basic() {
    auto bus = make_shared<EventBus>(false);
    auto consumer = make_shared<MockConsumer>("consumer1");
    consumer->registerWithEventBus(bus);
    bus->unregisterConsumer("consumer1");

    auto event = make_shared<TestEvent>(42);
    bus->publishEvent(event);
    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 0 && "Unregistered consumer should not receive event");
}

// Test event interest registration
void test_EventBus_registerEventInterest_basic() {
    auto bus = make_shared<EventBus>(false);
    auto consumer = make_shared<MockConsumer>("consumer1");
    consumer->registerWithEventBus(bus);  // Registers interest in TestEvent

    auto event = make_shared<TestEvent>(42);
    bus->publishEvent(event);
    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 1 && "Consumer with registered interest should receive event");
}

// Register tests
TEST(test_EventBus_publishEvent_sync_delivery);
TEST(test_EventBus_publishEvent_sync_no_consumers);
TEST(test_EventBus_publishEvent_targeted_delivery);
TEST(test_EventBus_publishEvent_async_delivery);
TEST(test_EventBus_publishEvent_async_queue_full);
TEST(test_EventBus_registerProducer_basic);
TEST(test_EventBus_unregisterConsumer_basic);
TEST(test_EventBus_registerEventInterest_basic);
#endif