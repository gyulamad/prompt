#pragma once

#include "../abstracts/Closable.hpp"
#include "../str/str_contains.hpp"
#include "../str/tpl_replace.hpp"
#include "../str/implode.hpp"
#include "../utils/system.hpp"
#include "../utils/ERROR.hpp"
#include "../containers/array_merge.hpp"
#include "../containers/array_diff.hpp"

#include "PackQueueHolder.hpp"

using namespace tools::abstracts;
using namespace tools::str;
using namespace tools::utils;
using namespace tools::containers;

namespace tools::agency {
    
    template<typename T>
    class Agent: public PackQueueHolder<T>, public Closable {
        static_assert(Streamable<T>, "T must support ostream output for dump()");
    public:
        Agent(
            PackQueue<T>& queue, 
            const string& name,
            vector<string> recipients
        ):
            PackQueueHolder<T>(queue), 
            name(name),
            recipients(recipients)
        {}

        virtual ~Agent() {
            cout << "Agent (" + this->name + ") destruction..." << endl;
            this->close();
            if (t.joinable()) t.join();
        }

        void start(long ms = 10, bool run_async = true) {
            if (run_async) async(ms);
            else sync(ms);
        }

        void async(long ms = 10) {
            t = thread([this, ms]() { sync(ms); });
        }

        void sync(long ms = 10) {
            while (!closing) {
                try {
                    if (ms) sleep_ms(ms);
                    tick();
                } catch (exception &e) {
                    hoops("Agent '" + name + "' error: " + string(e.what()));
                }
            }
        }

        virtual void handle(const string& /*sender*/, const T& /*item*/) UNIMP_THROWS

        virtual void tick() {}

        void exit() {
            this->send("agency", "exit");
        }

        const string name;
        
        virtual string type() const UNIMP_THROWS


        void addRecipients(const vector<string>& recipients) {
            this->recipients = array_merge(this->recipients, recipients);
        }

        void setRecipients(const vector<string>& recipients) {
            this->recipients = recipients;
        }

        void removeRecipients(const vector<string>& recipients) {
            this->recipients = array_diff(this->recipients, recipients);
        }

        vector<string> findRecipients(const string& keyword = "") const {
            if (keyword.empty()) return recipients;
            vector<string> found;
            foreach(recipients, [&](const string& recipient) {
                if (str_contains(recipient, keyword)) 
                    found.push_back(recipient);
            });
            return found;
        }

        virtual string dump() const {
            string recipients = implode(", ", this->recipients);
            return tpl_replace({
                { "{{name}}" , name },
                { "{{type}}" , type() },
                { "{{recipients}}" , recipients.empty() ? "<nobody>" : recipients },
            }, "Agent {{name}} is a(n) {{type}} agent, talking to {{recipients}}.");
        }

    protected:
        void send(const T& item) {
            send(recipients, item);
        }

        virtual void hoops(const string& errmsg = "") {
            cerr << errmsg << endl;
        }

        thread t;
        vector<string> recipients;

    private:
        void send(const string& recipient, const T& item) {
            if (name == recipient) ERROR("Can not send for itself: " + name);
            Pack<T> pack(name, recipient, item);
            this->queue.Produce(move(pack));
        }

        void send(const vector<string>& recipients, const T& item) {
            for (const string& recipient: recipients) send(recipient, item);
        }
    };
    
}

#ifdef TEST

#include "../str/str_contains.hpp"
#include "../chat/Chatbot.hpp"
#include "../chat/ChatHistory.hpp"
#include "tests/helpers.hpp"
#include "tests/TestAgent.hpp"
#include "PackQueue.hpp" // Needed for queue operations

using namespace tools::agency;
using namespace tools::str;
using namespace tools::chat;

// Test constructor
void test_Agent_constructor_basic() {
    PackQueue<string> queue;
    Agent<string> agent(queue, "test_agent", {});
    auto actual_name = agent.name;
    assert(actual_name == "test_agent" && "Agent name should be set correctly");
    // Can't directly test queue ref, but we'll use it in send tests
}

// Test single send
void test_Agent_send_single() {
    PackQueue<string> queue;
    TestAgent<string> agent(queue, "alice", {});
    agent.testSend("bob", "hello");
    auto actual_contents = queue_to_vector(queue);
    assert(actual_contents.size() == 1 && "Send should produce one pack");
    assert(actual_contents[0].sender == "alice" && "Sender should be 'alice'");
    assert(actual_contents[0].recipient == "bob" && "Recipient should be 'bob'");
    assert(actual_contents[0].item == "hello" && "Item should be 'hello'");
}

// Test multiple sends
void test_Agent_send_multiple() {
    PackQueue<string> queue;
    TestAgent<string> agent(queue, "alice", {});
    vector<string> recipients = {"bob", "charlie"};
    agent.testSend(recipients, "hello");
    auto actual_contents = queue_to_vector(queue);
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
    PackQueue<string> queue;
    Agent<string> agent(queue, "test_agent", {});
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
    PackQueue<string> queue;
    Agent<string> agent(queue, "test_agent", {});
    // No output or state to check, just ensure it runs without crashing
    agent.tick();
    // If we reach here, it’s fine—no assert needed for empty default
}

// Test sync runs until closed
void test_Agent_sync_basic() {
    PackQueue<string> queue;
    TestAgent<string> agent(queue, "test_agent", {});
    agent.close(); // Set closing first
    agent.sync(1); // Should exit immediately
    auto actual_closed = agent.isClosing();
    assert(actual_closed && "Agent should remain closed after sync");
}

// Test async starts and stops
void test_Agent_async_basic() {
    PackQueue<string> queue;
    Agent<string> agent(queue, "test_agent", {});
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