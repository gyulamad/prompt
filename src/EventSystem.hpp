#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <vector>
#include <mutex>
#include <any>
#include <typeindex>
#include <thread>
#include <condition_variable>
#include <queue>

#include "tools/Logger.hpp"
#include "tools/Tasks.hpp"
#include "tools/RingBuffer.hpp"  // Include RingBuffer for the queue implementation

using namespace std;
using namespace tools;

namespace EventSystem {

    // Forward declarations
    class Event;
    class EventBus;
    class EventQueue;  // New abstract queue interface

    // Unique identifier for components in the system
    using ComponentId = string;

    /**
     * Base class for all events in the system
     */
    class Event {
    public:
        virtual ~Event() = default;
        
        // Source component that created this event
        ComponentId sourceId;
        
        // Optional target component (empty string means broadcast)
        ComponentId targetId;
        
        // Timestamp when the event was created
        chrono::time_point<chrono::system_clock> timestamp;
        
        // Type information for runtime type checking
        virtual type_index getType() const = 0;
    };

    /**
     * Interface for components that can produce events
     */
    class EventProducer {
    public:
        virtual ~EventProducer() = default;
        
        // Register this producer with the event bus
        virtual void registerWithEventBus(shared_ptr<EventBus> bus) = 0;
        
        // Get unique identifier for this producer
        virtual ComponentId getId() const = 0;
    };

    /**
     * Interface for components that can consume events
     */
    class EventConsumer {
    public:
        virtual ~EventConsumer() = default;
        
        // Handle an incoming event
        virtual void handleEvent(shared_ptr<Event> event) = 0;
        
        // Register this consumer with the event bus
        virtual void registerWithEventBus(shared_ptr<EventBus> bus) = 0;
        
        // Get unique identifier for this consumer
        virtual ComponentId getId() const = 0;
        
        // Check if this consumer can handle a specific event type
        virtual bool canHandle(type_index eventType) const = 0;
    };

    /**
     * Combined interface for components that can both produce and consume events
     */
    class EventAgent : public EventProducer, public EventConsumer {
    public:
        virtual ~EventAgent() = default;
    };

    /**
     * Abstract interface for event queues
     */
    class EventQueue {
    public:
        virtual ~EventQueue() = default;
        virtual bool write(shared_ptr<Event> event) = 0;  // Add an event to the queue
        virtual size_t read(shared_ptr<Event>& event, bool blocking, int timeoutMs) = 0; // Retrieve an event
        virtual size_t available() const = 0;  // Check number of available events
    };

    /**
     * Implementation of EventQueue using RingBuffer
     */
    class RingBufferEventQueue : public EventQueue {
    public:
        RingBufferEventQueue(size_t capacity, shared_ptr<Logger> logger)
            : m_ringBuffer(capacity, RingBuffer<shared_ptr<Event>>::WritePolicy::Rotate),
              m_logger(logger) {
            m_ringBuffer.set_drop_callback([this](size_t count) {
                if (m_logger) {
                    m_logger->warn("Dropped " + to_string(count) + " events due to full queue");
                }
            });
        }

        bool write(shared_ptr<Event> event) override {
            return m_ringBuffer.write(&event, 1);
        }

        size_t read(shared_ptr<Event>& event, bool blocking, int timeoutMs) override {
            return m_ringBuffer.read(&event, 1, blocking, timeoutMs);
        }

        size_t available() const override {
            return m_ringBuffer.available();
        }

    private:
        RingBuffer<shared_ptr<Event>> m_ringBuffer;
        shared_ptr<Logger> m_logger;
    };

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

    /**
     * Base template for concrete event types
     */
    template<typename T>
    class TypedEvent : public Event {
    public:
        type_index getType() const override {
            return type_index(typeid(T));
        }
    };

    /**
     * Base implementation of an event consumer with helper methods
     */
    class BaseEventConsumer : public EventConsumer, public enable_shared_from_this<BaseEventConsumer> {
    public:
        BaseEventConsumer(const ComponentId& id) : m_id(id) {}
        
        void registerWithEventBus(shared_ptr<EventBus> bus) override {
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
        virtual void registerEventInterests() = 0;

    private:
        ComponentId m_id;
        shared_ptr<EventBus> m_eventBus;
        unordered_map<type_index, function<void(shared_ptr<Event>)>> m_handlerMap;
    };

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

    // Event delivery filter interface
    class EventFilter {
    public:
        virtual ~EventFilter() = default;
        
        // Return true if the event should be delivered, false if it should be filtered out
        virtual bool shouldDeliverEvent(const ComponentId& consumerId, shared_ptr<Event> event) = 0;
    };

    // Self-message filter implementation that can be enabled/disabled
    class SelfMessageFilter : public EventFilter {
    public:
        SelfMessageFilter(bool filterSelfMessages = true) 
            : m_filterSelfMessages(filterSelfMessages) {}
        
        bool shouldDeliverEvent(const ComponentId& consumerId, shared_ptr<Event> event) override {
            if (!m_filterSelfMessages) {
                return true; // No filtering, deliver all events
            }
            
            // Filter out messages where the source is the same as the consumer
            return event->sourceId != consumerId;
        }
        
        // Enable or disable self-message filtering
        void setFilterSelfMessages(bool filter) {
            m_filterSelfMessages.store(filter);
        }
        
        // Check if self-message filtering is enabled
        bool isFilteringEnabled() const {
            return m_filterSelfMessages.load();
        }
        
    private:
        atomic<bool> m_filterSelfMessages;
    };

    // Enhanced event bus with filtering support
    class FilteredEventBus : public EventBus {
    public:
        FilteredEventBus(bool asyncDelivery = false, shared_ptr<Logger> logger = nullptr)
            : EventBus(asyncDelivery, logger),  // Pass logger to base class
              m_logger(logger) {
            // Create a default self-message filter
            m_selfMessageFilter = make_shared<SelfMessageFilter>(false);
        }
        
        // Add a custom event filter
        void addEventFilter(shared_ptr<EventFilter> filter) {
            lock_guard<mutex> lock(m_filterMutex);
            m_filters.push_back(filter);
        }
        
        // Remove all filters
        void clearFilters() {
            lock_guard<mutex> lock(m_filterMutex);
            m_filters.clear();
        }
        
        // Get the self-message filter (for enabling/disabling)
        shared_ptr<SelfMessageFilter> getSelfMessageFilter() {
            return m_selfMessageFilter;
        }
        
        // Set logger
        void setLogger(shared_ptr<Logger> logger) {
            m_logger = logger;
            // Logger is already set in base class via constructor
        }
        
    protected:
        // Override the deliverEvent method to apply filters
        void deliverEvent(shared_ptr<Event> event) override {
            if (m_logger) {
                m_logger->debug("Delivering event from " + event->sourceId + 
                                (event->targetId.empty() ? " (broadcast)" : " to " + event->targetId));
            }
            
            // If the event has a specific target, deliver only to that target (with filtering)
            if (!event->targetId.empty()) {
                deliverEventToTarget(event);
                return;
            }
            
            // Otherwise, deliver to all consumers that have registered interest (with filtering)
            deliverEventToInterestedConsumers(event);
        }
        
        // Access to parent class protected members
        using EventBus::m_mutex;
        using EventBus::m_consumers;
        using EventBus::m_eventInterests;
        
    private:
        // Deliver an event to a specific target with filtering
        void deliverEventToTarget(shared_ptr<Event> event) {
            lock_guard<mutex> lock(m_mutex);
            
            auto it = m_consumers.find(event->targetId);
            if (it != m_consumers.end() && it->second->canHandle(event->getType())) {
                if (shouldDeliverEvent(it->first, event)) {
                    if (m_logger) {
                        m_logger->debug("Delivering targeted event to " + it->first);
                    }
                    it->second->handleEvent(event);
                } else if (m_logger) {
                    m_logger->debug("Filtered out targeted event to " + it->first);
                }
            }
        }
        
        // Deliver an event to all interested consumers with filtering
        void deliverEventToInterestedConsumers(shared_ptr<Event> event) {
            lock_guard<mutex> lock(m_mutex);
            
            auto typeIt = m_eventInterests.find(event->getType());
            if (typeIt != m_eventInterests.end()) {
                for (const auto& consumerId : typeIt->second) {
                    auto consumerIt = m_consumers.find(consumerId);
                    if (consumerIt != m_consumers.end()) {
                        if (shouldDeliverEvent(consumerId, event)) {
                            if (m_logger) {
                                m_logger->debug("Delivering broadcast event to " + consumerId);
                            }
                            consumerIt->second->handleEvent(event);
                        } else if (m_logger) {
                            m_logger->debug("Filtered out broadcast event to " + consumerId);
                        }
                    }
                }
            }
        }
        
        // Check all filters to see if an event should be delivered
        bool shouldDeliverEvent(const ComponentId& consumerId, shared_ptr<Event> event) {
            lock_guard<mutex> lock(m_filterMutex);
            
            // Check if any filter rejects the event
            for (const auto& filter : m_filters) {
                if (!filter->shouldDeliverEvent(consumerId, event)) {
                    return false;
                }
            }
            
            // Also check the self-message filter
            if (m_selfMessageFilter && !m_selfMessageFilter->shouldDeliverEvent(consumerId, event)) {
                return false;
            }
            
            return true;
        }
        
        // Filtering
        mutex m_filterMutex;
        vector<shared_ptr<EventFilter>> m_filters;
        shared_ptr<SelfMessageFilter> m_selfMessageFilter;
        shared_ptr<Logger> m_logger;
    };

} // namespace EventSystem


#ifdef TEST
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>

#include "tools/tests/MockLogger.hpp"

using namespace EventSystem;

class TestEvent : public Event {
public:
    type_index getType() const override { return type_index(typeid(TestEvent)); }
    int value;
    TestEvent(int v) : value(v) {}
};

class MockConsumer : public EventConsumer, public std::enable_shared_from_this<MockConsumer> {
public:
    MockConsumer(const ComponentId& id) : id(id) {}
    vector<shared_ptr<Event>> receivedEvents;

    void handleEvent(shared_ptr<Event> event) override {
        receivedEvents.push_back(event);
    }
    void registerWithEventBus(shared_ptr<EventBus> bus) override {
        bus->registerConsumer(shared_from_this());
        bus->registerEventInterest(id, type_index(typeid(TestEvent)));
    }
    ComponentId getId() const override { return id; }
    bool canHandle(type_index eventType) const override {
        return eventType == type_index(typeid(TestEvent));
    }

private:
    ComponentId id;
};

class TestConsumer : public BaseEventConsumer {
public:
    TestConsumer(const ComponentId& id) : BaseEventConsumer(id) {}
    vector<shared_ptr<Event>> receivedEvents;

protected:
    void registerEventInterests() override {
        registerHandler<TestEvent>([this](shared_ptr<TestEvent> event) {
            receivedEvents.push_back(event);
        });
    }
};

class TestAgent : public BaseEventAgent {
public:
    TestAgent(const ComponentId& id) : BaseEventAgent(id) {}
    vector<shared_ptr<Event>> receivedEvents;

protected:
    void registerEventInterests() override {
        registerHandler<TestEvent>([this](shared_ptr<TestEvent> event) {
            receivedEvents.push_back(event);
        });
    }
};

// Test basic write operation
void test_RingBufferEventQueue_write_basic() {
    auto logger = make_shared<MockLogger>();
    RingBufferEventQueue queue(3, logger);
    auto event = make_shared<TestEvent>(42);

    bool success = queue.write(event);
    assert(success == true && "Write should succeed with empty queue");
    size_t available = queue.available();
    assert(available == 1 && "Queue should have 1 event after write");
}

// Test reading from queue
void test_RingBufferEventQueue_read_basic() {
    auto logger = make_shared<MockLogger>();
    RingBufferEventQueue queue(3, logger);
    auto event = make_shared<TestEvent>(42);
    queue.write(event);

    shared_ptr<Event> readEvent;
    size_t readCount = queue.read(readEvent, false, 0);
    assert(readCount == 1 && "Read should return 1 event");
    auto testEvent = static_pointer_cast<TestEvent>(readEvent);
    assert(testEvent->value == 42 && "Read event should match written event");
    size_t available = queue.available();
    assert(available == 0 && "Queue should be empty after read");
}

// Test reading from empty queue
void test_RingBufferEventQueue_read_empty() {
    auto logger = make_shared<MockLogger>();
    RingBufferEventQueue queue(3, logger);

    shared_ptr<Event> readEvent;
    size_t readCount = queue.read(readEvent, false, 0);
    assert(readCount == 0 && "Read from empty queue should return 0");
    assert(readEvent == nullptr && "Read event should be null when queue is empty");
}

// Test write when queue is full (triggers drop callback)
void test_RingBufferEventQueue_write_full() {
    auto logger = make_shared<MockLogger>();
    RingBufferEventQueue queue(2, logger);  // Capacity of 2
    queue.write(make_shared<TestEvent>(1));
    queue.write(make_shared<TestEvent>(2));
    
    bool success = queue.write(make_shared<TestEvent>(3));  // Should overwrite oldest
    assert(success == true && "Write should succeed with Rotate policy");
    size_t available = queue.available();
    assert(available == 2 && "Queue should still have 2 events after overwrite");

    bool loggedDrop = logger->hasMessageContaining("Dropped 1 events");
    assert(loggedDrop == true && "Drop callback should log event drop");
}

// Test available method
void test_RingBufferEventQueue_available_multiple() {
    auto logger = make_shared<MockLogger>();
    RingBufferEventQueue queue(3, logger);
    queue.write(make_shared<TestEvent>(1));
    queue.write(make_shared<TestEvent>(2));

    size_t available = queue.available();
    assert(available == 2 && "Available should return 2 after two writes");

    shared_ptr<Event> readEvent;
    queue.read(readEvent, false, 0);
    available = queue.available();
    assert(available == 1 && "Available should return 1 after one read");
}

// Test blocking read with timeout (no data)
void test_RingBufferEventQueue_read_blocking_timeout() {
    auto logger = make_shared<MockLogger>();
    RingBufferEventQueue queue(3, logger);

    auto start = chrono::steady_clock::now();
    shared_ptr<Event> readEvent;
    size_t readCount = queue.read(readEvent, true, 500);  // 500ms timeout
    auto end = chrono::steady_clock::now();
    auto durationMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    assert(readCount == 0 && "Blocking read should return 0 with no data");
    assert(durationMs >= 500 && "Blocking read should wait at least 500ms");
}

// Test blocking read with data available
void test_RingBufferEventQueue_read_blocking_success() {
    auto logger = make_shared<MockLogger>();
    RingBufferEventQueue queue(3, logger);
    auto event = make_shared<TestEvent>(99);
    queue.write(event);

    shared_ptr<Event> readEvent;
    size_t readCount = queue.read(readEvent, true, 500);
    assert(readCount == 1 && "Blocking read should return 1 when data is available");
    auto testEvent = static_pointer_cast<TestEvent>(readEvent);
    assert(testEvent->value == 99 && "Read event should match written event");
}

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
    class MockProducer : public EventProducer, public std::enable_shared_from_this<MockProducer> {
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

// Test registration with EventBus
void test_BaseEventConsumer_registerWithEventBus_basic() {
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<EventBus>(false, logger);
    auto consumer = make_shared<TestConsumer>("consumer1");

    consumer->registerWithEventBus(bus);
    auto event = make_shared<TestEvent>(42);
    bus->publishEvent(event);

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
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<EventBus>(false, logger);
    auto consumer = make_shared<TestConsumer>("consumer1");
    consumer->registerWithEventBus(bus);  // Trigger registration of event interests

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
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<EventBus>(false, logger);
    auto consumer = make_shared<TestConsumer>("consumer1");
    consumer->registerWithEventBus(bus);  // Register to populate m_handlerMap

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
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<EventBus>(false, logger);
    auto consumer = make_shared<TestConsumer>("consumer1");
    consumer->registerWithEventBus(bus);

    // Register a second handler for the same event type
    consumer->registerHandler<TestEvent>([consumer](shared_ptr<TestEvent> event) {
        consumer->receivedEvents.push_back(event);  // Double push for this test
    });
    auto event = make_shared<TestEvent>(42);
    bus->publishEvent(event);

    size_t eventCount = consumer->receivedEvents.size();
    assert(eventCount == 2 && "Handler registered post-registration should also receive event");
}

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

// Test SelfMessageFilter with filtering enabled
void test_SelfMessageFilter_shouldDeliverEvent_self_filtered() {
    auto filter = make_shared<SelfMessageFilter>(true);  // Filtering enabled
    auto event = make_shared<TestEvent>(42);
    event->sourceId = "agent1";

    bool shouldDeliver = filter->shouldDeliverEvent("agent1", event);
    assert(shouldDeliver == false && "SelfMessageFilter should block event when source matches consumer");
}

// Test SelfMessageFilter with filtering disabled
void test_SelfMessageFilter_shouldDeliverEvent_self_allowed() {
    auto filter = make_shared<SelfMessageFilter>(false);  // Filtering disabled
    auto event = make_shared<TestEvent>(42);
    event->sourceId = "agent1";

    bool shouldDeliver = filter->shouldDeliverEvent("agent1", event);
    assert(shouldDeliver == true && "SelfMessageFilter should allow event when filtering is disabled");
}

// Test SelfMessageFilter with different source and consumer
void test_SelfMessageFilter_shouldDeliverEvent_different_ids() {
    auto filter = make_shared<SelfMessageFilter>(true);  // Filtering enabled
    auto event = make_shared<TestEvent>(42);
    event->sourceId = "agent1";

    bool shouldDeliver = filter->shouldDeliverEvent("consumer1", event);
    assert(shouldDeliver == true && "SelfMessageFilter should allow event when source differs from consumer");
}

// Test SelfMessageFilter toggle functionality
void test_SelfMessageFilter_setFilterSelfMessages_toggle() {
    auto filter = make_shared<SelfMessageFilter>(true);
    auto event = make_shared<TestEvent>(42);
    event->sourceId = "agent1";

    bool initial = filter->shouldDeliverEvent("agent1", event);
    assert(initial == false && "SelfMessageFilter should block initially");

    filter->setFilterSelfMessages(false);
    bool afterDisable = filter->shouldDeliverEvent("agent1", event);
    assert(afterDisable == true && "SelfMessageFilter should allow after disabling");

    filter->setFilterSelfMessages(true);
    bool afterEnable = filter->shouldDeliverEvent("agent1", event);
    assert(afterEnable == false && "SelfMessageFilter should block after re-enabling");
}

// Test FilteredEventBus default behavior (self-filter off)
void test_FilteredEventBus_deliverEvent_self_allowed_default() {
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<FilteredEventBus>(false, logger);
    auto agent = make_shared<TestAgent>("agent1");
    agent->registerWithEventBus(bus);

    auto event = make_shared<TestEvent>(42);
    agent->publishEvent(event);
    size_t eventCount = agent->receivedEvents.size();
    assert(eventCount == 1 && "Agent should receive its own event with default self-filter off");
}

// Test FilteredEventBus with self-filter enabled
void test_FilteredEventBus_deliverEvent_self_filtered() {
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<FilteredEventBus>(false, logger);
    auto agent = make_shared<TestAgent>("agent1");
    agent->registerWithEventBus(bus);

    bus->getSelfMessageFilter()->setFilterSelfMessages(true);
    auto event = make_shared<TestEvent>(42);
    agent->publishEvent(event);
    size_t eventCount = agent->receivedEvents.size();
    assert(eventCount == 0 && "Agent should not receive its own event with self-filter enabled");
}

// Test FilteredEventBus with custom filter blocking all events
void test_FilteredEventBus_deliverEvent_custom_filter_blocks() {
    class BlockAllFilter : public EventFilter {
    public:
        bool shouldDeliverEvent(const ComponentId&, shared_ptr<Event>) override { return false; }
    };
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<FilteredEventBus>(false, logger);
    auto agent = make_shared<TestAgent>("agent1");
    auto consumer = make_shared<MockConsumer>("consumer1");
    agent->registerWithEventBus(bus);
    consumer->registerWithEventBus(bus);

    bus->addEventFilter(make_shared<BlockAllFilter>());
    auto event = make_shared<TestEvent>(42);
    agent->publishEvent(event);
    size_t agentCount = agent->receivedEvents.size();
    size_t consumerCount = consumer->receivedEvents.size();
    assert(agentCount == 0 && "Agent should not receive event with blocking filter");
    assert(consumerCount == 0 && "Consumer should not receive event with blocking filter");
}

// Test FilteredEventBus with targeted event and self-filter
void test_FilteredEventBus_deliverEvent_targeted_self_filtered() {
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<FilteredEventBus>(false, logger);
    auto agent = make_shared<TestAgent>("agent1");
    agent->registerWithEventBus(bus);

    bus->getSelfMessageFilter()->setFilterSelfMessages(true);
    auto event = make_shared<TestEvent>(42);
    event->targetId = "agent1";  // Target itself
    agent->publishEvent(event);
    size_t eventCount = agent->receivedEvents.size();
    assert(eventCount == 0 && "Agent should not receive its own targeted event with self-filter enabled");
}

// Test FilteredEventBus clearFilters
void test_FilteredEventBus_clearFilters_restores_delivery() {
    auto logger = make_shared<MockLogger>();
    auto bus = make_shared<FilteredEventBus>(false, logger);
    auto agent = make_shared<TestAgent>("agent1");
    agent->registerWithEventBus(bus);

    bus->getSelfMessageFilter()->setFilterSelfMessages(true);
    bus->addEventFilter(make_shared<SelfMessageFilter>(true));
    auto event = make_shared<TestEvent>(42);
    agent->publishEvent(event);
    size_t initialCount = agent->receivedEvents.size();
    assert(initialCount == 0 && "Agent should not receive event with filters enabled");

    bus->clearFilters();
    bus->getSelfMessageFilter()->setFilterSelfMessages(false);  // Reset to default
    agent->receivedEvents.clear();
    agent->publishEvent(event);
    size_t afterClearCount = agent->receivedEvents.size();
    assert(afterClearCount == 1 && "Agent should receive event after clearing filters and resetting self-filter");
}

// Register tests
TEST(test_RingBufferEventQueue_write_basic);
TEST(test_RingBufferEventQueue_read_basic);
TEST(test_RingBufferEventQueue_read_empty);
TEST(test_RingBufferEventQueue_write_full);
TEST(test_RingBufferEventQueue_available_multiple);
TEST(test_RingBufferEventQueue_read_blocking_timeout);
TEST(test_RingBufferEventQueue_read_blocking_success);

TEST(test_EventBus_publishEvent_sync_delivery);
TEST(test_EventBus_publishEvent_sync_no_consumers);
TEST(test_EventBus_publishEvent_targeted_delivery);
TEST(test_EventBus_publishEvent_async_delivery);
TEST(test_EventBus_publishEvent_async_queue_full);
TEST(test_EventBus_registerProducer_basic);
TEST(test_EventBus_unregisterConsumer_basic);
TEST(test_EventBus_registerEventInterest_basic);

TEST(test_BaseEventConsumer_registerWithEventBus_basic);
TEST(test_BaseEventConsumer_getId_basic);
TEST(test_BaseEventConsumer_canHandle_registered);
TEST(test_BaseEventConsumer_canHandle_unregistered);
TEST(test_BaseEventConsumer_handleEvent_registered);
TEST(test_BaseEventConsumer_handleEvent_unregistered);
TEST(test_BaseEventConsumer_registerHandler_post_registration);

TEST(test_BaseEventProducer_registerWithEventBus_basic);
TEST(test_BaseEventProducer_getId_basic);
TEST(test_BaseEventProducer_publishEvent_with_bus);
TEST(test_BaseEventProducer_publishEvent_no_bus);
TEST(test_BaseEventProducer_publishEvent_async_bus);

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

TEST(test_SelfMessageFilter_shouldDeliverEvent_self_filtered);
TEST(test_SelfMessageFilter_shouldDeliverEvent_self_allowed);
TEST(test_SelfMessageFilter_shouldDeliverEvent_different_ids);
TEST(test_SelfMessageFilter_setFilterSelfMessages_toggle);
TEST(test_FilteredEventBus_deliverEvent_self_allowed_default);
TEST(test_FilteredEventBus_deliverEvent_self_filtered);
TEST(test_FilteredEventBus_deliverEvent_custom_filter_blocks);
TEST(test_FilteredEventBus_deliverEvent_targeted_self_filtered);
TEST(test_FilteredEventBus_clearFilters_restores_delivery);
#endif

/*
TODO:
-----------------------------------
test_RingBufferEventQueue_*
They don’t cover:

Exception cases (e.g., invalid capacity), as the RingBuffer constructor already throws for capacity < 2, and we assume this is tested in the RingBuffer suite.
Multi-threaded scenarios (e.g., concurrent writes/reads), which could be added if needed but would require more complex setup.

---------------------------------------
test_EventBus_*
Not covered (potential additions):

Multi-threaded scenarios: Testing concurrent publish and delivery could be complex and might require a different framework.
Exception cases: No explicit exceptions are thrown by EventBus methods, so none are tested.
Destructor behavior: Testing thread cleanup in ~EventBus() would require mocking or observing thread state, which is tricky with cassert.

---------------------------------------------------
test_BaseEventConsumer_*
Not covered (potential additions):

Exception Cases: BaseEventConsumer doesn’t throw exceptions explicitly, so none are tested.
Multi-threaded Access: Could test concurrent event handling, but this requires a more complex setup beyond cassert.
Edge Cases: Like registering the same handler multiple times (currently, it just adds duplicates, which is fine but could be tested for specific behavior).

----------------------------------------
test_BaseEventProducer_*
Not covered (potential additions):

Exception Cases: BaseEventProducer doesn’t throw exceptions explicitly, so none are tested.
Concurrent Publishing: Multi-threaded publishing could be tested but requires a more complex setup.
Multiple Event Types: Tests only use TestEvent; adding another type could verify template flexibility.

----------------------------------------
test_BaseEventAgent_*
Not covered (potential additions):

Exception Cases: No explicit exceptions are thrown, so none are tested.
Concurrent Operations: Multi-threaded publish/consume scenarios could be added but require more setup.
Multiple Handlers: Could test registering multiple handlers for the same event type.

-----------------------------------
test_SelfMessageFilter_* and test_FilteredEventBus_*
Not covered (potential additions):

Async Delivery: Could test filtering with asyncDelivery = true, but it’s orthogonal to filtering logic.
Concurrent Filter Changes: Multi-threaded filter toggling could be tested but requires more setup.
Multiple Filters: Could test interactions between multiple custom filters.
*/