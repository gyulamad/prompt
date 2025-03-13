#pragma once

#include <functional>

#include "../utils/ERROR.hpp"
#include "../utils/strings.hpp"
#include "../utils/datetime.hpp"

#include "Pack.hpp"
#include "PackQueue.hpp"
#include "PackQueueHolder.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::agency {

#define AGENT_SENDER_EXIT_SIGNALER "***__EXIT_SIGNALER__***"

    template<typename T>
    class Agent {
    public:
        Agent(
            PackQueueHolder<T>& agency,
            const string& name = "",
            long ms = 0
        ):
            agency(agency),
            name(name),
            ms(ms)
        {}
    
        virtual ~Agent() {
            if (t.joinable()) t.join();
        }

        virtual void start() {
            t = thread([this]() {
                try {
                    loop(); 
                } catch (exception &e) {
                    ouch(e.what());
                }
                exited = true;
            });
        }

        [[nodiscard]]
        bool isExited() const {
            return exited;
        }

        void die() {
            dying = true;
        }

        const string name;
        
    protected:

        [[nodiscard]]
        virtual bool handle(const string& sender, const T& item) UNIMP_THROWS

        void send(T item, const string& recipient) {
            Pack<T> pack(item, name, recipient);
            agency.getPackQueueRef().Produce(move(pack));
        }

        void send(T item, const vector<string>& recipients) {
            for (const string& recipient: recipients) send(item, recipient);
        }

        void exit() {
            T item;
            Pack<T> pack(item, AGENT_SENDER_EXIT_SIGNALER);
            agency.getPackQueueRef().Produce(move(pack));
        }

        void loop() {
            Pack<T> pack;
            PackQueue<T>& queue = agency.getPackQueueRef();
            while (true) {
                try {
                    if (ms) sleep_ms(ms);
                    if (!tick()) break;

                    queue.Hold();

                    Pack<T>* packPtr = queue.Peek();
                    if (!packPtr) { // empty
                        queue.Release();
                        continue;
                    }

                    string sender = packPtr->getSender();
                    if (sender == AGENT_SENDER_EXIT_SIGNALER) { // exiting
                        queue.Release();
                        break;
                    }

                    string recipient = packPtr->getRecipient();
                    if (recipient != name) { // skip
                        queue.Release();
                        continue;
                    }

                    if (!queue.Consume(pack)) {
                        queue.Release();
                        throw ERROR("queue error");
                    }

                    queue.Release();

                    if (!handle(sender, pack.getItem())) break; // leaving 

                    if (dying) break;
                } catch (exception& e) {
                    string what(e.what());
                    throw ERROR("Error in agent '" + name + "'" + (what.empty() ? "" : ": " + what));
                }
            }
        }

        virtual void ouch(const string& what) {
            cerr << "Error in agent '" + name + "'" + (what.empty() ? "" : ": " + what) << endl;
        }

        [[nodiscard]]
        virtual bool tick() { return true; }

        atomic<bool> exited = false;
        atomic<bool> dying = false;
        PackQueueHolder<T>& agency;
        long ms;
        thread t;
    };

}


#ifdef TEST

#include "tests/TestAgent.hpp"
#include "tests/MinimalAgent.hpp"

using namespace tools::agency;

// Agent Tests
void test_Agent_Constructor_Default() {
    PackQueue<int> q;
    PackQueueHolder<int> holder(q);
    TestAgent<int> agent(holder, "test_agent");
    string actual_name = agent.name;
    string expected_name = "test_agent";
    assert(actual_name == expected_name && "Constructor should set name correctly");
    assert(!agent.isExited() && "Agent should not be exited initially");
}

void test_Agent_Start_LoopEmptyQueue() {
    PackQueue<int> q;
    PackQueueHolder<int> holder(q);
    TestAgent<int> agent(holder, "test_agent", 10); // 10ms sleep
    agent.tick_continue = false; // Exit after one tick
    agent.start();
    this_thread::sleep_for(chrono::milliseconds(20)); // Give thread time to run
    bool actual = agent.isExited();
    bool expected = true;
    assert(actual == expected && "Agent should exit when tick returns false on empty queue");
}

void test_Agent_Handle_Unimplemented() {
    TEST_SKIP();
    try {
    PackQueue<int> q;
    PackQueueHolder<int> holder(q);
    MinimalAgent<int> agent(holder, "test_agent"); // Use MinimalAgent to get base handle behavior
    q.Produce(Pack<int>(42, "sender", "test_agent"));
    agent.start();
    this_thread::sleep_for(chrono::milliseconds(20)); // Let thread process
    agent.getThreadRef().join(); // Wait for thread to finish
    bool actual_exited = agent.isExited();
    assert(!actual_exited && "Agent should not mark as exited due to uncaught exception in handle");
    // Note: Exception terminates thread abnormally; exited remains false
    } catch (...) {
        DEBUG("!!!!");
    }
}

void test_Agent_Send_SingleRecipient() {
    PackQueue<int> q;
    PackQueueHolder<int> holder(q);
    TestAgent<int> agent(holder, "test_agent");
    agent.publicSend(42, "bob");
    Pack<int> pack;
    bool consumed = q.Consume(pack);
    int actual_item = pack.getItem();
    string actual_recipient = pack.getRecipient();
    string actual_sender = pack.getSender();
    assert(consumed && actual_item == 42 && actual_recipient == "bob" && actual_sender == "test_agent" && 
           "Send should produce pack with correct item, recipient, and sender");
}

void test_Agent_Send_MultipleRecipients() {
    PackQueue<int> q;
    PackQueueHolder<int> holder(q);
    TestAgent<int> agent(holder, "test_agent");
    agent.publicSendMultiple(42, {"bob", "alice"});
    Pack<int> pack1, pack2;
    bool consumed1 = q.Consume(pack1);
    bool consumed2 = q.Consume(pack2);
    int actual1 = pack1.getItem();
    string recip1 = pack1.getRecipient();
    int actual2 = pack2.getItem();
    string recip2 = pack2.getRecipient();
    assert(consumed1 && consumed2 && actual1 == 42 && actual2 == 42 && 
           (recip1 == "bob" || recip1 == "alice") && (recip2 == "bob" || recip2 == "alice") && recip1 != recip2 &&
           "Send should produce packs for all recipients with correct item");
}

void test_Agent_Exit_Self() {
    TEST_SKIP();
    PackQueue<int> q;
    PackQueueHolder<int> holder(q);
    TestAgent<int> agent(holder, "test_agent");
    agent.start();
    agent.publicExit();
    this_thread::sleep_for(chrono::milliseconds(20)); // Let thread process exit signal
    bool actual = agent.isExited();
    bool expected = true;
    assert(actual == expected && "Exit should stop the agent loop");
    assert(q.Size() == 0 && "Exit pack should be consumed");
}

void test_Agent_Handle_Item() {
    TEST_SKIP();
    PackQueue<int> q;
    PackQueueHolder<int> holder(q);
    TestAgent<int> agent(holder, "test_agent");
    q.Produce(Pack<int>(42, "sender", "test_agent"));
    agent.start();
    this_thread::sleep_for(chrono::milliseconds(20)); // Let thread process
    agent.continue_running = false; // Stop after one handle
    agent.getThreadRef().join();
    int actual_item = agent.last_item;
    string actual_sender = agent.last_sender;
    assert(actual_item == 42 && actual_sender == "sender" && "Handle should process item and sender correctly");
}

void test_Agent_Die() {
    TEST_SKIP();
    PackQueue<int> q;
    PackQueueHolder<int> holder(q);
    TestAgent<int> agent(holder, "test_agent");
    agent.start();
    agent.die();
    this_thread::sleep_for(chrono::milliseconds(20)); // Let thread exit
    bool actual = agent.isExited();
    bool expected = true;
    assert(actual == expected && "Die should stop the agent loop");
}

void test_Agent_Tick_False() {
    PackQueue<int> q;
    PackQueueHolder<int> holder(q);
    TestAgent<int> agent(holder, "test_agent");
    agent.tick_continue = false;
    agent.start();
    this_thread::sleep_for(chrono::milliseconds(20)); // Let thread exit
    bool actual = agent.isExited();
    bool expected = true;
    assert(actual == expected && "Tick returning false should stop the agent loop");
}

// Multi-Threaded Agent Tests
void test_Agent_ConcurrentSend_MultipleAgents() {
    TEST_SKIP();
    PackQueue<string> q;
    PackQueueHolder<string> holder(q);
    TestAgent<string> agent1(holder, "agent1");
    TestAgent<string> agent2(holder, "agent2");

    // Start agents (they won't process yet due to empty queue)
    agent1.start();
    agent2.start();

    // Concurrent sending
    vector<thread> senders;
    senders.emplace_back([&agent1]() {
        for (int i = 0; i < 10; i++) {
            agent1.publicSend("msg" + to_string(i), "agent2");
        }
    });
    senders.emplace_back([&agent2]() {
        for (int i = 10; i < 20; i++) {
            agent2.publicSend("msg" + to_string(i), "agent1");
        }
    });

    // Wait for sending to complete
    for (auto& t : senders) t.join();

    // Check queue size
    int actual_size = q.Size();
    int expected_size = 20; // 10 from agent1 + 10 from agent2
    assert(actual_size == expected_size && "Concurrent sends should add all messages to queue");

    // Cleanup
    agent1.publicExit();
    agent2.publicExit();
    agent1.getThreadRef().join();
    agent2.getThreadRef().join();
}

void test_Agent_ConcurrentHandle_Send() {
    TEST_SKIP();
    PackQueue<string> q;
    PackQueueHolder<string> holder(q);
    TestAgent<string> receiver(holder, "receiver", 10); // 10ms sleep
    TestAgent<string> sender(holder, "sender");

    // Start receiver
    receiver.start();

    // Sender thread
    thread sender_thread([&sender]() {
        for (int i = 0; i < 5; i++) {
            sender.publicSend("msg" + to_string(i), "receiver");
            this_thread::sleep_for(chrono::milliseconds(5)); // Stagger sends
        }
    });

    // Wait and stop receiver
    this_thread::sleep_for(chrono::milliseconds(100)); // Let receiver process
    receiver.continue_running = false; // Stop after handling some
    sender_thread.join();
    receiver.getThreadRef().join();

    // Check handling
    lock_guard<mutex> lock(receiver.data_mtx);
    int actual_count = receiver.handled_count;
    assert(actual_count >= 1 && actual_count <= 5 && "Receiver should handle some messages");
    assert(receiver.last_sender == "sender" && "Last sender should be correct");
    assert(str_contains(receiver.last_item, "msg") && "Last item should be a sent message");
}

void test_Agent_ConcurrentExit_WhileSending() {
    PackQueue<string> q;
    PackQueueHolder<string> holder(q);
    TestAgent<string> agent1(holder, "agent1", 10);
    TestAgent<string> agent2(holder, "agent2");

    // Start agents
    agent1.start();
    agent2.start();

    // Sender thread
    thread sender([&agent2]() {
        for (int i = 0; i < 10; i++) {
            agent2.publicSend("msg" + to_string(i), "agent1");
            this_thread::sleep_for(chrono::milliseconds(5));
        }
    });

    // Exit agent1 while agent2 sends
    this_thread::sleep_for(chrono::milliseconds(25)); // Let some sends happen
    agent1.publicExit();
    agent1.getThreadRef().join();

    // Wait for sender to finish
    sender.join();

    // Check agent1 exited, agent2 still running
    bool actual_exit1 = agent1.isExited();
    bool actual_exit2 = agent2.isExited();
    assert(actual_exit1 && !actual_exit2 && "Agent1 should exit, Agent2 should continue");

    // Cleanup
    agent2.publicExit();
    agent2.getThreadRef().join();
}

// Register Tests
TEST(test_Agent_Constructor_Default);
TEST(test_Agent_Start_LoopEmptyQueue);
TEST(test_Agent_Handle_Unimplemented);
TEST(test_Agent_Send_SingleRecipient);
TEST(test_Agent_Send_MultipleRecipients);
TEST(test_Agent_Exit_Self);
TEST(test_Agent_Handle_Item);
TEST(test_Agent_Die);
TEST(test_Agent_Tick_False);

// Register Multi-Threaded Tests
TEST(test_Agent_ConcurrentSend_MultipleAgents);
TEST(test_Agent_ConcurrentHandle_Send);
TEST(test_Agent_ConcurrentExit_WhileSending);

#endif