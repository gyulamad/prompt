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

using namespace std;
using namespace tools;

namespace EventSystem {

    // Forward declarations
    class Event;
    class EventBus;

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
     * Central event bus that manages event distribution
     */
    class EventBus : public enable_shared_from_this<EventBus> {
    public:
        EventBus(bool asyncDelivery = false) : m_asyncDelivery(asyncDelivery) {
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
                {
                    lock_guard<mutex> lock(m_queueMutex);
                    m_eventQueue.push(event);
                }
                m_queueCondition.notify_one();
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
                    shared_ptr<Event> event;
                    {
                        unique_lock<mutex> lock(m_queueMutex);
                        m_queueCondition.wait(lock, [this]() { 
                            return !m_eventQueue.empty() || !m_running; 
                        });
                        
                        if (!m_running) break;
                        
                        event = m_eventQueue.front();
                        m_eventQueue.pop();
                    }
                    
                    if (event) {
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
        queue<shared_ptr<Event>> m_eventQueue;
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

    protected:
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
        
        // Helper method to publish events
        template<typename EventType>
        void publishEvent(shared_ptr<EventType> event) {
            if (m_eventBus) {
                event->sourceId = m_id;
                event->timestamp = chrono::system_clock::now();
                m_eventBus->publishEvent(event);
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
            : EventBus(asyncDelivery),
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