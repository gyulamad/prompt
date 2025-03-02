#pragma once

#include <memory>

#include "../Logger.hpp"

#include "Event.hpp"
#include "EventBus.hpp"
#include "SelfMessageFilter.hpp"

using namespace std;

namespace tools::events {

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

}

#ifdef TEST

#include "../tests/MockLogger.hpp"
#include "tests/TestEvent.hpp"
#include "tests/MockConsumer.hpp"

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
TEST(test_FilteredEventBus_deliverEvent_self_allowed_default);
TEST(test_FilteredEventBus_deliverEvent_self_filtered);
TEST(test_FilteredEventBus_deliverEvent_custom_filter_blocks);
TEST(test_FilteredEventBus_deliverEvent_targeted_self_filtered);
TEST(test_FilteredEventBus_clearFilters_restores_delivery);
#endif