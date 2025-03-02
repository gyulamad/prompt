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

// Register tests
TEST(test_SelfMessageFilter_shouldDeliverEvent_self_filtered);
TEST(test_SelfMessageFilter_shouldDeliverEvent_self_allowed);
TEST(test_SelfMessageFilter_shouldDeliverEvent_different_ids);
TEST(test_SelfMessageFilter_setFilterSelfMessages_toggle);
#endif
