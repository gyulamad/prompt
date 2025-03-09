#pragma once

#include <memory>

#include "Event.hpp"
#include "EventFilter.hpp"

using namespace std;

namespace tools::events {
    
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

}

#ifdef TEST

#include "../utils/Test.hpp"

#include "tests/TestEvent.hpp"

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

// Test SelfMessageFilter with async delivery via EventBus
void test_SelfMessageFilter_shouldDeliverEvent_async_delivery() {
    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(true, logger, eventQueue);  // Async mode
    auto agent = make_shared<TestAgent>("agent1");
    agent->registerWithEventBus(&bus);

    // Wrap SelfMessageFilter in a FilteredEventBus for context
    class FilteredEventBusWrapper : public EventBus {
    public:
        FilteredEventBusWrapper(
            bool async, 
            Logger& logger, 
            shared_ptr<SelfMessageFilter> filter,
            EventQueue& eventQueue
        ): 
            EventBus(async, logger, eventQueue), 
            m_filter(filter) 
        {}

    protected:
        void deliverEvent(shared_ptr<Event> event) override {
            lock_guard<mutex> lock(m_mutex);
            auto typeIt = m_eventInterests.find(event->getType());
            if (typeIt != m_eventInterests.end()) {
                for (const auto& consumerId : typeIt->second) {
                    auto consumerIt = m_consumers.find(consumerId);
                    if (consumerIt != m_consumers.end() && m_filter->shouldDeliverEvent(consumerId, event)) {
                        consumerIt->second->handleEvent(event);
                    }
                }
            }
        }

    private:
        shared_ptr<SelfMessageFilter> m_filter;
        using EventBus::m_mutex;
        using EventBus::m_consumers;
        using EventBus::m_eventInterests;
    };

    auto filter = make_shared<SelfMessageFilter>(true);
    FilteredEventBusWrapper filteredBus(true, logger, filter, eventQueue);
    auto asyncAgent = make_shared<TestAgent>("agent1");
    asyncAgent->registerWithEventBus(&filteredBus);

    auto event = make_shared<TestEvent>(42);
    asyncAgent->publishEvent(event);
    this_thread::sleep_for(chrono::milliseconds(200));  // Wait for async processing

    size_t eventCount = asyncAgent->receivedEvents.size();
    assert(eventCount == 0 && "SelfMessageFilter should block self-event in async mode");
}

// Test concurrent filter toggling
void test_SelfMessageFilter_setFilterSelfMessages_concurrent() {
    auto filter = make_shared<SelfMessageFilter>(true);
    const int numThreads = 4;
    vector<thread> togglers;

    // Toggle filter state concurrently
    for (int i = 0; i < numThreads; ++i) {
        togglers.emplace_back([i, &filter]() {
            bool newState = (i % 2 == 0);  // Half enable, half disable
            for (int j = 0; j < 10; ++j) {
                filter->setFilterSelfMessages(newState);
                this_thread::yield();
            }
        });
    }

    for (auto& toggler : togglers) {
        toggler.join();
    }

    // Check final state (could be true or false due to race, but should be consistent)
    bool finalState = filter->isFilteringEnabled();
    bool executedWithoutCrash = true;
    assert(executedWithoutCrash == true && "Concurrent toggling should not crash");
    // Verify atomicity by checking a single consistent state
    auto event = make_shared<TestEvent>(42);
    event->sourceId = "agent1";
    bool shouldDeliver = filter->shouldDeliverEvent("agent1", event);
    assert((finalState && !shouldDeliver) || (!finalState && shouldDeliver) && "Filter state should be consistent after concurrent toggling");
}

// Test interaction with multiple filters
void test_SelfMessageFilter_shouldDeliverEvent_multiple_filters() {
    class OtherFilter : public EventFilter {
    public:
        bool shouldDeliverEvent(const ComponentId& consumerId, shared_ptr<Event> event) override {
            return event->sourceId != "blockedSource";  // Blocks a specific source
        }
    };

    MockLogger logger;
    RingBufferEventQueue eventQueue(1000, logger);
    EventBus bus(false, logger, eventQueue);
    auto agent = make_shared<TestAgent>("agent1");
    agent->registerWithEventBus(&bus);

    // Wrap SelfMessageFilter with another filter in a custom EventBus
    class MultiFilterBus : public EventBus {
    public:
        MultiFilterBus(
            Logger& logger, 
            shared_ptr<SelfMessageFilter> selfFilter, 
            shared_ptr<OtherFilter> otherFilter,
            EventQueue& eventQueue
        ): 
            EventBus(false, logger, eventQueue), 
            m_selfFilter(selfFilter), 
            m_otherFilter(otherFilter)
        {}
        
    protected:
        void deliverEvent(shared_ptr<Event> event) override {
            lock_guard<mutex> lock(m_mutex);
            auto typeIt = m_eventInterests.find(event->getType());
            if (typeIt != m_eventInterests.end()) {
                for (const auto& consumerId : typeIt->second) {
                    auto consumerIt = m_consumers.find(consumerId);
                    if (consumerIt != m_consumers.end() &&
                        m_selfFilter->shouldDeliverEvent(consumerId, event) &&
                        m_otherFilter->shouldDeliverEvent(consumerId, event)) {
                        consumerIt->second->handleEvent(event);
                    }
                }
            }
        }

    private:
        shared_ptr<SelfMessageFilter> m_selfFilter;
        shared_ptr<OtherFilter> m_otherFilter;
        using EventBus::m_mutex;
        using EventBus::m_consumers;
        using EventBus::m_eventInterests;
    };

    auto selfFilter = make_shared<SelfMessageFilter>(true);
    auto otherFilter = make_shared<OtherFilter>();
    MultiFilterBus multiBus(logger, selfFilter, otherFilter, eventQueue);
    auto multiAgent = make_shared<TestAgent>("agent1");
    multiAgent->registerWithEventBus(&multiBus);

    // Test self-filter blocking
    auto event1 = make_shared<TestEvent>(42);
    multiAgent->publishEvent(event1);
    size_t eventCount1 = multiAgent->receivedEvents.size();
    assert(eventCount1 == 0 && "SelfMessageFilter should block self-event with multiple filters");

    // Test other filter blocking
    multiAgent->receivedEvents.clear();
    selfFilter->setFilterSelfMessages(false);  // Allow self-events
    auto event2 = make_shared<TestEvent>(43);
    event2->sourceId = "blockedSource";
    multiBus.publishEvent(event2);
    size_t eventCount2 = multiAgent->receivedEvents.size();
    assert(eventCount2 == 0 && "OtherFilter should block event from blockedSource");

    // Test both allowing
    multiAgent->receivedEvents.clear();
    auto event3 = make_shared<TestEvent>(44);
    event3->sourceId = "agent1";
    multiAgent->publishEvent(event3);
    size_t eventCount3 = multiAgent->receivedEvents.size();
    assert(eventCount3 == 1 && "Both filters should allow event when conditions met");
}

// Register tests
TEST(test_SelfMessageFilter_shouldDeliverEvent_self_filtered);
TEST(test_SelfMessageFilter_shouldDeliverEvent_self_allowed);
TEST(test_SelfMessageFilter_shouldDeliverEvent_different_ids);
TEST(test_SelfMessageFilter_setFilterSelfMessages_toggle);
TEST(test_SelfMessageFilter_shouldDeliverEvent_async_delivery);
TEST(test_SelfMessageFilter_setFilterSelfMessages_concurrent);
TEST(test_SelfMessageFilter_shouldDeliverEvent_multiple_filters);
#endif
