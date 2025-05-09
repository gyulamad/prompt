#pragma once

#include <memory>

#include "../utils/Logger.hpp"

#include "Event.hpp"
#include "EventBus.hpp"
#include "SelfMessageFilter.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::events {

    // Enhanced event bus with filtering support (thread safe)
    class FilteredEventBus : public EventBus {
    public:
        FilteredEventBus(
            Logger& logger,
            EventQueue& eventQueue
        ): 
            EventBus(logger, eventQueue),  // Pass logger to base class
            m_logger(logger),
            m_selfMessageFilter(false)
        {}
        
        // Add a custom event filter
        void addEventFilter(EventFilter& filter) {
            lock_guard<mutex> lock(m_filterMutex);
            m_filters.push_back(&filter);
        }
        
        // Remove all filters
        void clearFilters() {
            lock_guard<mutex> lock(m_filterMutex);
            m_filters.clear();
        }
        
        // Get the self-message filter (for enabling/disabling)
        SelfMessageFilter& getSelfMessageFilter() {
            return m_selfMessageFilter;
        }
        
    protected:
        // Override the deliverEvent method to apply filters
        function<void(EventBus*, Event&)> deliverEventInternal = [](EventBus* that, Event& event) {
            that->m_logger.debug("Delivering event from " + event.sourceId + (event.targetId.empty() ? " (broadcast)" : " to " + event.targetId));
            ((FilteredEventBus*)that)->deliverEventToInterestedConsumers(event);
        };
        
        // Access to parent class protected members
        using EventBus::m_mutex;
        using EventBus::m_consumers;
        using EventBus::m_eventInterests;
        
    private:
        // Deliver an event to a specific target with filtering
        bool shouldDeliverEvent(const ComponentId& consumerId, Event& event) {
            // Get a copy of filters to avoid holding the lock during filter evaluation
            vector<EventFilter*> filtersCopy;
            {
                lock_guard<mutex> lock(m_filterMutex);
                filtersCopy = m_filters;
            }
            
            // Check if any filter rejects the event
            for (EventFilter* filter: filtersCopy) {
                if (!filter->shouldDeliverEvent(consumerId, event)) {
                    return false;
                }
            }
            
            // Also check the self-message filter
            return m_selfMessageFilter.shouldDeliverEvent(consumerId, event);
        }
        
        // Deliver an event to all interested consumers with filtering
        void deliverEventToInterestedConsumers(Event& event) {
            unique_lock<shared_mutex> lock(m_mutex);
        
            auto typeIt = m_eventInterests.find(event.getType());
            if (typeIt != m_eventInterests.end()) {
                for (const ComponentId& consumerId : typeIt->second) {
                    auto consumerIt = m_consumers.find(consumerId);
                    if (consumerIt != m_consumers.end()) {
                        if (shouldDeliverEvent(consumerId, event)) {
                            m_logger.debug("Delivering broadcast event to " + consumerId);
                            consumerIt->second->handleEvent(event);
                        } else {
                            m_logger.debug("Filtered out broadcast event to " + consumerId);
                        }
                    }
                }
            }
        }
        
        // Filtering
        mutex m_filterMutex;
        vector<EventFilter*> m_filters;
        SelfMessageFilter m_selfMessageFilter;
        Logger& m_logger;
    };    

}

#ifdef TEST

#include "../utils/Test.hpp"
#include "../utils/tests/MockLogger.hpp"
#include "tests/TestEvent.hpp"
#include "tests/MockConsumer.hpp"

// Test FilteredEventBus default behavior (self-filter off)
void test_FilteredEventBus_deliverEvent_self_allowed_default() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    FilteredEventBus bus(logger, eventQueue);
    TestAgent agent("agent1");
    agent.registerWithEventBus(&bus);

    agent.publishEvent<TestEvent>("", 42);
    size_t eventCount = agent.receivedEvents.size();
    assert(eventCount == 1 && "Agent should receive its own event with default self-filter off");
}

// Test FilteredEventBus with self-filter enabled
void test_FilteredEventBus_deliverEvent_self_filtered() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    FilteredEventBus bus(logger, eventQueue);
    TestAgent agent("agent1");
    agent.registerWithEventBus(&bus);

    bus.getSelfMessageFilter().setFilterSelfMessages(true);
    agent.publishEvent<TestEvent>("", 42);
    size_t eventCount = agent.receivedEvents.size();
    assert(eventCount == 0 && "Agent should not receive its own event with self-filter enabled");
}

// Test FilteredEventBus with custom filter blocking all events
void test_FilteredEventBus_deliverEvent_custom_filter_blocks() {
    class BlockAllFilter : public EventFilter {
    public:
        bool shouldDeliverEvent(const ComponentId&, Event&) override { return false; }
    };
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    FilteredEventBus bus(logger, eventQueue);
    TestAgent agent("agent1");
    MockConsumer consumer("consumer1");
    agent.registerWithEventBus(&bus);
    consumer.registerWithEventBus(&bus);

    BlockAllFilter blocker;
    bus.addEventFilter(blocker);
    agent.publishEvent<TestEvent>("", 42);
    size_t agentCount = agent.receivedEvents.size();
    size_t consumerCount = consumer.receivedEvents.size();
    assert(agentCount == 0 && "Agent should not receive event with blocking filter");
    assert(consumerCount == 0 && "Consumer should not receive event with blocking filter");
}

// Test FilteredEventBus with targeted event and self-filter
void test_FilteredEventBus_deliverEvent_targeted_self_filtered() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    FilteredEventBus bus(logger, eventQueue);
    TestAgent agent("agent1");
    agent.registerWithEventBus(&bus);

    bus.getSelfMessageFilter().setFilterSelfMessages(true);
    agent.publishEvent<TestEvent>("", 42);
    size_t eventCount = agent.receivedEvents.size();
    assert(eventCount == 0 && "Agent should not receive its own targeted event with self-filter enabled");
}

// Test FilteredEventBus clearFilters
void test_FilteredEventBus_clearFilters_restores_delivery() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    FilteredEventBus bus(logger, eventQueue);
    TestAgent agent("agent1");
    agent.registerWithEventBus(&bus);

    bus.getSelfMessageFilter().setFilterSelfMessages(true);
    SelfMessageFilter filter(true);
    bus.addEventFilter(filter);
    agent.publishEvent<TestEvent>("", 42);
    size_t initialCount = agent.receivedEvents.size();
    assert(initialCount == 0 && "Agent should not receive event with filters enabled");

    bus.clearFilters();
    bus.getSelfMessageFilter().setFilterSelfMessages(false);  // Reset to default
    agent.receivedEvents.clear();
    agent.publishEvent<TestEvent>("", 42);
    size_t afterClearCount = agent.receivedEvents.size();
    assert(afterClearCount == 1 && "Agent should receive event after clearing filters and resetting self-filter");
}

// Register tests
TEST(test_FilteredEventBus_deliverEvent_self_allowed_default);
TEST(test_FilteredEventBus_deliverEvent_self_filtered);
TEST(test_FilteredEventBus_deliverEvent_custom_filter_blocks);
TEST(test_FilteredEventBus_deliverEvent_targeted_self_filtered);
TEST(test_FilteredEventBus_clearFilters_restores_delivery);
#endif