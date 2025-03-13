#pragma once

#include "Agent.hpp"
#include "PackQueue.hpp"
#include "PackQueueHolder.hpp"

namespace tools::agency {

    template<typename T>
    class Agency: public PackQueueHolder<T> {
    public:
        Agency(PackQueue<T>& queue, long ms = 100): PackQueueHolder<T>(queue), ms(ms) {}

        virtual void setup() {}

        virtual void loop() {}

        void run() {
            setup();
            if (agents.empty()) throw ERROR("No agents, spawn some before run the agency!");
            while(!agents.empty()) {
                vector<Agent<T>*> result;
                copy_if(agents.begin(), agents.end(), back_inserter(result), [this](const Agent<T>* agent) {
                    if (agent->isExited()) {
                        // Remove packs from queue which addressed to the agent but never will be delivered
                        this->queue.drop(agent->name);
                        delete agent;
                        return false;
                    }
                    return true;
                });
                agents = result;

                loop();
                sleep_ms(ms);
            }
        }
        
        template<typename AgentT, typename... Args>
        AgentT* spawn(Args&&... args) {
            Agent<T>* agent = new AgentT(*this, forward<Args>(args)...);
            // agent->name should be unique! check for agent name in agents vector first!
            string name = agent->name;
            for (const Agent<T>* a: agents) {
                if (a->name == name) {
                    delete agent;
                    return nullptr;
                }
            }
            agents.push_back(agent);
            agent->start();
            return (AgentT*)agent;
        }
        
        template<typename AgentT>
        void kill(AgentT* agent) {
            killByName(agent->name);
        }

        void killByName(const string& name) {
            for (Agent<T>* agent: agents)
                if (agent->name == name) agent->die();
        }

    protected:
        vector<Agent<T>*> agents;
        // PackQueue<T> queue;
        long ms;
    };

}


#ifdef TEST

#include "tests/TestAgency.hpp"

using namespace tools::agency;

// Agency Tests
void test_Agency_Constructor_Default() {
    PackQueue<int> q;
    TestAgency<int> agency(q, 50);
    int actual_ms = agency.getMs();
    int expected_ms = 50;
    assert(actual_ms == expected_ms && "Constructor should set ms correctly");
    assert(agency.setup_called == false && "Setup should not be called yet");
}

void test_Agency_Spawn_SingleAgent() {
    PackQueue<int> q;
    TestAgency<int> agency(q);
    TestAgent<int>* agent = agency.spawn<TestAgent<int>>("agent1");
    bool actual_not_null = (agent != nullptr);
    string actual_name = agent->name;
    assert(actual_not_null && actual_name == "agent1" && "Spawn should create agent with correct name");
    assert(agency.getAgentsRef().size() == 1 && "Agent should be added to agents vector");
    agent->publicExit();
    agent->getThreadRef().join();
}

void test_Agency_Spawn_DuplicateName() {
    PackQueue<int> q;
    TestAgency<int> agency(q);
    TestAgent<int>* agent1 = agency.spawn<TestAgent<int>>("agent1");
    TestAgent<int>* agent2 = agency.spawn<TestAgent<int>>("agent1");
    bool actual_null = (agent2 == nullptr);
    assert(actual_null && "Spawn should return nullptr for duplicate name");
    assert(agency.getAgentsRef().size() == 1 && "Duplicate agent should not be added");
    agent1->publicExit();
    agent1->getThreadRef().join();
}

void test_Agency_Run_NoAgents() {
    PackQueue<int> q;
    TestAgency<int> agency(q);
    bool thrown = false;
    try {
        agency.run();
    } catch (exception& e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "No agents") && "Run should throw when no agents");
    }
    assert(thrown && "Run should throw on empty agency");
}

void test_Agency_Run_SingleAgent() {
    PackQueue<int> q;
    TestAgency<int> agency(q, 10); // 10ms sleep
    TestAgent<int>* agent = agency.spawn<TestAgent<int>>("agent1");
    agent->tick_continue = false; // Exit after one tick
    agency.run();
    bool actual_setup = agency.setup_called;
    int actual_loop = agency.loop_count;
    assert(actual_setup && "Setup should be called");
    assert(actual_loop >= 1 && "Loop should run at least once");
    assert(agency.getAgentsRef().empty() && "Exited agent should be cleaned up");
}

void test_Agency_Kill_Pointer() {
    TEST_SKIP();
    PackQueue<int> q;
    TestAgency<int> agency(q);
    TestAgent<int>* agent = agency.spawn<TestAgent<int>>("agent1");
    agency.kill(agent);
    this_thread::sleep_for(chrono::milliseconds(20)); // Let agent process die
    bool actual_dying = agent->isDying();
    assert(actual_dying && "Kill by pointer should set dying flag");
    agent->getThreadRef().join();
    agency.run(); // Clean up
}

void test_Agency_Kill_Name() {
    TEST_SKIP();
    PackQueue<int> q;
    TestAgency<int> agency(q);
    TestAgent<int>* agent = agency.spawn<TestAgent<int>>("agent1");
    agency.killByName("agent1");
    this_thread::sleep_for(chrono::milliseconds(20)); // Let agent process die
    bool actual_dying = agent->isDying();
    assert(actual_dying && "Kill by name should set dying flag");
    agent->getThreadRef().join();
    agency.run(); // Clean up
}

void test_Agency_ConcurrentSpawn_Run() {
    TEST_SKIP();
    PackQueue<int> q;
    TestAgency<int> agency(q, 10);
    vector<thread> spawners;
    vector<string> names = {"agent1", "agent2", "agent3"};
    
    // Concurrently spawn agents
    for (const auto& name : names) {
        spawners.emplace_back([&agency, name]() {
            agency.spawn<TestAgent<int>>(name);
        });
    }
    for (auto& t : spawners) t.join();

    // Run agency
    thread runner([&agency]() {
        agency.run();
    });
    
    // Let it run briefly, then exit agents
    this_thread::sleep_for(chrono::milliseconds(50));
    for (const auto& name : names) {
        agency.killByName(name);
    }
    
    runner.join();
    
    int actual_size = agency.getAgentsRef().size();
    assert(actual_size == 0 && "All agents should be cleaned up after run");
    assert(agency.loop_count > 0 && "Loop should have run multiple times");
}

void test_Agency_ConcurrentSend_Cleanup() {
    TEST_SKIP();
    PackQueue<int> q;
    TestAgency<int> agency(q, 10);
    TestAgent<int>* sender = agency.spawn<TestAgent<int>>("sender");
    TestAgent<int>* receiver = agency.spawn<TestAgent<int>>("receiver");

    // Sender sends messages
    thread sender_thread([&sender]() {
        for (int i = 0; i < 5; i++) {
            sender->publicSend(i, "receiver");
            this_thread::sleep_for(chrono::milliseconds(5));
        }
    });

    // Run agency and kill receiver
    thread runner([&agency, receiver]() {
        this_thread::sleep_for(chrono::milliseconds(20)); // Let some messages queue
        agency.kill(receiver);
        agency.run();
    });

    sender_thread.join();
    runner.join();

    int actual_queue_size = q.Size();
    assert(actual_queue_size == 0 && "Queue should be cleared of receiver's messages");
    assert(agency.getAgentsRef().size() == 1 && "Only sender should remain");
    sender->publicExit();
    sender->getThreadRef().join();
}

// Register Tests
TEST(test_Agency_Constructor_Default);
TEST(test_Agency_Spawn_SingleAgent);
TEST(test_Agency_Spawn_DuplicateName);
TEST(test_Agency_Run_NoAgents);
TEST(test_Agency_Run_SingleAgent);
TEST(test_Agency_Kill_Pointer);
TEST(test_Agency_Kill_Name);
TEST(test_Agency_ConcurrentSpawn_Run);
TEST(test_Agency_ConcurrentSend_Cleanup);
#endif