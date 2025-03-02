#pragma once

#include <memory>

#include "Event.hpp"
#include "EventConsumer.hpp"

using namespace std;

namespace tools::events {

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

}

#ifdef TEST

#include "../tests/MockLogger.hpp"
#include "tests/TestEvent.hpp"
#include "tests/TestConsumer.hpp"

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

// Register tests
TEST(test_BaseEventConsumer_registerWithEventBus_basic);
TEST(test_BaseEventConsumer_getId_basic);
TEST(test_BaseEventConsumer_canHandle_registered);
TEST(test_BaseEventConsumer_canHandle_unregistered);
TEST(test_BaseEventConsumer_handleEvent_registered);
TEST(test_BaseEventConsumer_handleEvent_unregistered);
TEST(test_BaseEventConsumer_registerHandler_post_registration);
#endif
