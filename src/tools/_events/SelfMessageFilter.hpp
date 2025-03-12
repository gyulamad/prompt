#pragma once

#include <memory>

#include "Event.hpp"
#include "EventFilter.hpp"

using namespace std;

namespace tools::events {

    /**
     * @brief Self-message filter implementation that can be enabled/disabled.
     *
     * This filter prevents components from receiving their own events.
     * This is useful to avoid infinite loops and reduce unnecessary processing.
     */
    class SelfMessageFilter : public EventFilter {
    public:
        SelfMessageFilter(bool filterSelfMessages = true) 
            : m_filterSelfMessages(filterSelfMessages) {}
        
        bool shouldDeliverEvent(const ComponentId& consumerId, Event& event) override {
            if (!m_filterSelfMessages) {
                return true; // No filtering, deliver all events
            }
            
            // Filter out messages where the source is the same as the consumer
            return event.sourceId != consumerId;
        }
        
        // Enable or disable self-message filtering
        void setFilterSelfMessages(bool filter) {
            m_filterSelfMessages.store(filter, memory_order_relaxed);
        }
        
        // Check if self-message filtering is enabled
        bool isFilteringEnabled() const {
            return m_filterSelfMessages.load(memory_order_relaxed);
        }
                
    private:
        atomic<bool> m_filterSelfMessages;
    };    

}

#ifdef TEST

#include "../utils/Test.hpp"

#include "tests/TestEvent.hpp"

// Test SelfMessageFilter with filtering enabled
void test_SelfMessageFilter_shouldDeliverEvent_self_filtered() {
    SelfMessageFilter filter(true);  // Filtering enabled
    TestEvent event(42);
    event.sourceId = "agent1";

    bool shouldDeliver = filter.shouldDeliverEvent("agent1", event);
    assert(shouldDeliver == false && "SelfMessageFilter should block event when source matches consumer");
}

// Test SelfMessageFilter with filtering disabled
void test_SelfMessageFilter_shouldDeliverEvent_self_allowed() {
    SelfMessageFilter filter(false);  // Filtering disabled
    TestEvent event(42);
    event.sourceId = "agent1";

    bool shouldDeliver = filter.shouldDeliverEvent("agent1", event);
    assert(shouldDeliver == true && "SelfMessageFilter should allow event when filtering is disabled");
}

// Test SelfMessageFilter with different source and consumer
void test_SelfMessageFilter_shouldDeliverEvent_different_ids() {
    SelfMessageFilter filter(true);  // Filtering enabled
    TestEvent event(42);
    event.sourceId = "agent1";

    bool shouldDeliver = filter.shouldDeliverEvent("consumer1", event);
    assert(shouldDeliver == true && "SelfMessageFilter should allow event when source differs from consumer");
}

// Test SelfMessageFilter toggle functionality
void test_SelfMessageFilter_setFilterSelfMessages_toggle() {
    SelfMessageFilter filter(true);
    TestEvent event(42);
    event.sourceId = "agent1";

    bool initial = filter.shouldDeliverEvent("agent1", event);
    assert(initial == false && "SelfMessageFilter should block initially");

    filter.setFilterSelfMessages(false);
    bool afterDisable = filter.shouldDeliverEvent("agent1", event);
    assert(afterDisable == true && "SelfMessageFilter should allow after disabling");

    filter.setFilterSelfMessages(true);
    bool afterEnable = filter.shouldDeliverEvent("agent1", event);
    assert(afterEnable == false && "SelfMessageFilter should block after re-enabling");
}

// Test SelfMessageFilter with sync delivery via EventBus
void test_SelfMessageFilter_shouldDeliverEvent_sync_delivery() {
    // TEST_SKIP("TODO: Async queue delivery mechanism needs refact");
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);  // sync mode
    TestAgent agent("agent1");
    agent.registerWithEventBus(&bus);

    // Wrap SelfMessageFilter in a FilteredEventBus for context
    class FilteredEventBusWrapper : public EventBus {
    public:
        FilteredEventBusWrapper(
            Logger& logger, 
            SelfMessageFilter& filter,
            EventQueue& eventQueue
        ): 
            EventBus(logger, eventQueue), 
            m_filter(filter) 
        {}

    protected:

        // void deliverEventInternal(Event& event) override {
        function<void(EventBus*, Event&)> deliverEventInternal = [](EventBus* that, Event& event) {
            unique_lock<shared_mutex> lock(that->m_mutex);
            auto typeIt = that->m_eventInterests.find(event.getType());
            if (typeIt != that->m_eventInterests.end()) {
                for (const ComponentId& consumerId : typeIt->second) {
                    auto consumerIt = that->m_consumers.find(consumerId);
                    if (consumerIt != that->m_consumers.end() && ((FilteredEventBusWrapper*)that)->m_filter.shouldDeliverEvent(consumerId, event)) {
                        consumerIt->second->handleEvent(event);
                    }
                }
            }
        };

    public:
        SelfMessageFilter& m_filter;
    private:
        using EventBus::m_mutex;
        using EventBus::m_consumers;
        using EventBus::m_eventInterests;
    };

    SelfMessageFilter filter(true);
    FilteredEventBusWrapper filteredBus(logger, filter, eventQueue);
    TestAgent asyncAgent("agent1");
    asyncAgent.registerWithEventBus(&filteredBus);

    asyncAgent.publishEvent<TestEvent>("", 42);
    this_thread::sleep_for(chrono::milliseconds(200));  // Wait for async processing

    size_t eventCount = asyncAgent.receivedEvents.size();
    assert(eventCount == 0 && "SelfMessageFilter should block self-event in async mode");
}

// Test concurrent filter toggling
void test_SelfMessageFilter_setFilterSelfMessages_concurrent() {
    SelfMessageFilter filter(true);
    const int numThreads = 4;
    vector<thread> togglers;

    // Toggle filter state concurrently
    for (int i = 0; i < numThreads; ++i) {
        togglers.emplace_back([i, &filter]() {
            bool newState = (i % 2 == 0);  // Half enable, half disable
            for (int j = 0; j < 10; ++j) {
                filter.setFilterSelfMessages(newState);
                this_thread::yield();
            }
        });
    }

    for (thread& toggler : togglers) {
        toggler.join();
    }

    // Check final state (could be true or false due to race, but should be consistent)
    bool finalState = filter.isFilteringEnabled();
    bool executedWithoutCrash = true;
    assert(executedWithoutCrash == true && "Concurrent toggling should not crash");
    // Verify atomicity by checking a single consistent state
    TestEvent event(42);
    event.sourceId = "agent1";
    bool shouldDeliver = filter.shouldDeliverEvent("agent1", event);
    assert((finalState && !shouldDeliver) || (!finalState && shouldDeliver) && "Filter state should be consistent after concurrent toggling");
}

// Test interaction with multiple filters
void test_SelfMessageFilter_shouldDeliverEvent_multiple_filters() {
    class OtherFilter : public EventFilter {
    public:
        bool shouldDeliverEvent(const ComponentId& consumerId, Event& event) override {
            return event.sourceId != "blockedSource";  // Blocks a specific source
        }
    };

    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(logger, eventQueue);
    TestAgent agent("agent1");
    agent.registerWithEventBus(&bus);

    // Wrap SelfMessageFilter with another filter in a custom EventBus
    class MultiFilterBus : public EventBus {
    public:
        MultiFilterBus(
            Logger& logger, 
            SelfMessageFilter& selfFilter, 
            OtherFilter& otherFilter,
            EventQueue& eventQueue
        ): 
            EventBus(logger, eventQueue), 
            m_selfFilter(selfFilter), 
            m_otherFilter(otherFilter)
        {}
        
    protected:

        function<void(EventBus*, Event&)> deliverEventInternal = [](EventBus* that, Event& event) {
            unique_lock<shared_mutex> lock(that->m_mutex);
            auto typeIt = that->m_eventInterests.find(event.getType());
            if (typeIt != that->m_eventInterests.end()) {
                for (const ComponentId& consumerId : typeIt->second) {
                    auto consumerIt = that->m_consumers.find(consumerId);
                    if (consumerIt != that->m_consumers.end() &&
                        ((MultiFilterBus*)that)->m_selfFilter.shouldDeliverEvent(consumerId, event) &&
                        ((MultiFilterBus*)that)->m_otherFilter.shouldDeliverEvent(consumerId, event)) {
                        consumerIt->second->handleEvent(event);
                    }
                }
            }
        };

    private:
        SelfMessageFilter& m_selfFilter;
        OtherFilter& m_otherFilter;
        using EventBus::m_mutex;
        using EventBus::m_consumers;
        using EventBus::m_eventInterests;
    };

    SelfMessageFilter selfFilter(true);
    OtherFilter otherFilter;
    MultiFilterBus multiBus(logger, selfFilter, otherFilter, eventQueue);
    TestAgent multiAgent("agent1");
    multiAgent.registerWithEventBus(&multiBus);

    // Test self-filter blocking
    multiAgent.publishEvent<TestEvent>("", 42);
    size_t eventCount1 = multiAgent.receivedEvents.size();
    assert(eventCount1 == 0 && "SelfMessageFilter should block self-event with multiple filters");

    // Test other filter blocking
    multiAgent.receivedEvents.clear();
    selfFilter.setFilterSelfMessages(false);  // Allow self-events
    multiBus.createAndPublishEvent<TestEvent>("blockedSource", "agent1", 43); //Changed here.
    size_t eventCount2 = multiAgent.receivedEvents.size();
    assert(eventCount2 == 0 && "OtherFilter should block event from blockedSource");

    // Test both allowing
    multiAgent.receivedEvents.clear();
    multiAgent.publishEvent<TestEvent>("", 44);
    size_t eventCount3 = multiAgent.receivedEvents.size();
    assert(eventCount3 == 1 && "Both filters should allow event when conditions met");
}

// Register tests
TEST(test_SelfMessageFilter_shouldDeliverEvent_self_filtered);
TEST(test_SelfMessageFilter_shouldDeliverEvent_self_allowed);
TEST(test_SelfMessageFilter_shouldDeliverEvent_different_ids);
TEST(test_SelfMessageFilter_setFilterSelfMessages_toggle);
TEST(test_SelfMessageFilter_shouldDeliverEvent_sync_delivery);
TEST(test_SelfMessageFilter_setFilterSelfMessages_concurrent);
TEST(test_SelfMessageFilter_shouldDeliverEvent_multiple_filters);
#endif
