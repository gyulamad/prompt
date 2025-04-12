#pragma once

#include "../str/str_contains.hpp"
#include "../str/tpl_replace.hpp"
#include "../str/implode.hpp"
#include "../utils/system.hpp"
#include "../utils/ERROR.hpp"

#include "Worker.hpp"
#include "PackQueueHolder.hpp"

using namespace tools::str;
using namespace tools::utils;
using namespace tools::containers;

namespace tools::agency {

    template<typename T>
    class Agent: public Worker<T> {
        static_assert(Streamable<T>, "T must support ostream output for dump()");
    public:

        using Worker<T>::Worker;
        
        virtual ~Agent() {}
    };
    
}

#ifdef TEST

#include "../str/str_contains.hpp"
#include "chat/Chatbot.hpp"
#include "chat/ChatHistory.hpp"
#include "tests/helpers.hpp"
#include "tests/TestWorker.hpp"
#include "tests/default_test_agency_setup.hpp"
#include "PackQueue.hpp" // Needed for queue operations

using namespace tools::agency;
using namespace tools::str;
using namespace tools::agency::chat;
// TODO: these tests should be worker tests
// Test constructor
void test_Agent_constructor_basic() {
    default_test_agency_setup setup("test_agent");
    TestWorker<string> agent(setup.owns, setup.agency, setup.queue, setup.name);
    agent.fromJSON(setup.json);
    auto actual_name = agent.getName();
    assert(actual_name == "test_agent" && "Agent name should be set correctly");
    // Can't directly test queue ref, but we'll use it in send tests
}

// Test single send
void test_Agent_send_single() {
    default_test_agency_setup setup("alice");
    TestWorker<string> agent(setup.owns, setup.agency, setup.queue, setup.name);
    agent.fromJSON(setup.json);
    agent.testSend("bob", "hello");
    auto actual_contents = queue_to_vector(setup.queue);
    assert(actual_contents.size() == 1 && "Send should produce one pack");
    assert(actual_contents[0].sender == "alice" && "Sender should be 'alice'");
    assert(actual_contents[0].recipient == "bob" && "Recipient should be 'bob'");
    assert(actual_contents[0].item == "hello" && "Item should be 'hello'");
}

// Test multiple sends
void test_Agent_send_multiple() {
    default_test_agency_setup setup("alice");
    TestWorker<string> agent(setup.owns, setup.agency, setup.queue, setup.name);
    agent.fromJSON(setup.json);
    vector<string> recipients = {"bob", "charlie"};
    agent.testSend(recipients, "hello");
    auto actual_contents = queue_to_vector(setup.queue);
    assert(actual_contents.size() == 2 && "Send should produce two packs");
    assert(actual_contents[0].sender == "alice" && "First sender should be 'alice'");
    assert(actual_contents[0].recipient == "bob" && "First recipient should be 'bob'");
    assert(actual_contents[0].item == "hello" && "First item should be 'hello'");
    assert(actual_contents[1].sender == "alice" && "Second sender should be 'alice'");
    assert(actual_contents[1].recipient == "charlie" && "Second recipient should be 'charlie'");
    assert(actual_contents[1].item == "hello" && "Second item should be 'hello'");
}

// Test tick default does nothing
void test_Agent_tick_default() {
    default_test_agency_setup setup("test_agent");
    TestWorker<string> agent(setup.owns, setup.agency, setup.queue, setup.name);
    agent.fromJSON(setup.json);
    // No output or state to check, just ensure it runs without crashing
    agent.tick();
    // If we reach here, it’s fine—no assert needed for empty default
}

// Test sync runs until closed
void test_Agent_sync_basic() {
    default_test_agency_setup setup("test_agent");
    TestWorker<string> agent(setup.owns, setup.agency, setup.queue, setup.name);
    agent.fromJSON(setup.json);
    agent.close(); // Set closing first
    agent.sync(1); // Should exit immediately
    auto actual_closed = agent.isClosing();
    assert(actual_closed && "Agent should remain closed after sync");
}

// Test async starts and stops
void test_Agent_async_basic() {
    default_test_agency_setup setup("test_agent");
    TestWorker<string> agent(setup.owns, setup.agency, setup.queue, setup.name);
    agent.fromJSON(setup.json);
    agent.start(1, true); // Async with 1ms sleep
    sleep_ms(10); // Let it run briefly
    agent.close(); // Signal to stop
    // Destructor joins thread, so if it exits cleanly, test passes
}

// Register tests
TEST(test_Agent_constructor_basic);
TEST(test_Agent_send_single);
TEST(test_Agent_send_multiple);
TEST(test_Agent_tick_default);
TEST(test_Agent_sync_basic);
TEST(test_Agent_async_basic);

#endif