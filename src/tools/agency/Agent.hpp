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
    class Agent: /*public PackQueueHolder<T>,*/ public Worker<T> {
        static_assert(Streamable<T>, "T must support ostream output for dump()");
    public:

        using Worker<T>::Worker;
        
        virtual ~Agent() {}
    };
    
}

#ifdef TEST

#include "../str/str_contains.hpp"
#include "../chat/Chatbot.hpp"
#include "../chat/ChatHistory.hpp"
#include "tests/helpers.hpp"
#include "tests/TestWorker.hpp"
#include "tests/default_test_agency_setup.hpp"
#include "PackQueue.hpp" // Needed for queue operations

using namespace tools::agency;
using namespace tools::str;
using namespace tools::chat;

// Test constructor
void test_Agent_constructor_basic() {
    default_test_agency_setup setup;
    Agent<string> agent(setup.owns, setup.agency, setup.queue, string("test_agent"), setup.recipients);
    auto actual_name = agent.name;
    assert(actual_name == "test_agent" && "Agent name should be set correctly");
    // Can't directly test queue ref, but we'll use it in send tests
}

// Test single send
void test_Agent_send_single() {
    default_test_agency_setup setup;
    TestWorker<string> agent(setup.owns, setup.agency, setup.queue, string("alice"), setup.recipients);
    agent.testSend("bob", "hello");
    auto actual_contents = queue_to_vector(setup.queue);
    assert(actual_contents.size() == 1 && "Send should produce one pack");
    assert(actual_contents[0].sender == "alice" && "Sender should be 'alice'");
    assert(actual_contents[0].recipient == "bob" && "Recipient should be 'bob'");
    assert(actual_contents[0].item == "hello" && "Item should be 'hello'");
}

// Test multiple sends
void test_Agent_send_multiple() {
    default_test_agency_setup setup;
    TestWorker<string> agent(setup.owns, setup.agency, setup.queue, string("alice"), setup.recipients);
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

// Test handle throws exception
void test_Agent_handle_unimplemented() {
    default_test_agency_setup setup;
    Agent<string> agent(setup.owns, setup.agency, setup.queue, string("test_agent"), setup.recipients);
    bool thrown = false;
    try {
        agent.handle("alice", "hello");
    } catch (exception& e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Unimplemented function: handle") && "Exception message should indicate unimplemented");
    }
    assert(thrown && "Handle should throw an exception for unimplemented method");
}

// Test tick default does nothing
void test_Agent_tick_default() {
    default_test_agency_setup setup;
    Agent<string> agent(setup.owns, setup.agency, setup.queue, string("test_agent"), setup.recipients);
    // No output or state to check, just ensure it runs without crashing
    agent.tick();
    // If we reach here, it’s fine—no assert needed for empty default
}

// Test sync runs until closed
void test_Agent_sync_basic() {
    default_test_agency_setup setup;
    TestWorker<string> agent(setup.owns, setup.agency, setup.queue, string("test_agent"), setup.recipients);
    agent.close(); // Set closing first
    agent.sync(1); // Should exit immediately
    auto actual_closed = agent.isClosing();
    assert(actual_closed && "Agent should remain closed after sync");
}

// Test async starts and stops
void test_Agent_async_basic() {
    default_test_agency_setup setup;
    Agent<string> agent(setup.owns, setup.agency, setup.queue, string("test_agent"), setup.recipients);
    agent.start(1, true); // Async with 1ms sleep
    sleep_ms(10); // Let it run briefly
    agent.close(); // Signal to stop
    // Destructor joins thread, so if it exits cleanly, test passes
}

// Register tests
TEST(test_Agent_constructor_basic);
TEST(test_Agent_send_single);
TEST(test_Agent_send_multiple);
TEST(test_Agent_handle_unimplemented);
TEST(test_Agent_tick_default);
TEST(test_Agent_sync_basic);
TEST(test_Agent_async_basic);

#endif