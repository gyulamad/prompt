#pragma once

#include "../chat/Chatbot.hpp"
#include "../chat/Talkbot.hpp"
#include "Agent.hpp"
#include "agents/TalkbotAgent.hpp"

using namespace tools::chat;
using namespace tools::agency::agents;

namespace tools::agency {
    

    template<typename T>
    class AgencyConfig: public AgentConfig<T> {
    public:
        AgencyConfig(
            Owns& owns,
            PackQueue<T>& queue, 
            const string& name,
            vector<string> recipients
        ):
            owns(owns),
            AgentConfig<T>(queue, name, recipients)

        {}

        virtual ~AgencyConfig() {}

        Owns& getOwnsRef() { return owns; }

    private:
        Owns& owns;
    };

    template<typename T>
    class Agency: public Agent<T> {
    public:

        static const string agent_list_tpl;

        // using Agent<T>::Agent;
        Agency(AgencyConfig<T>& config
            // Owns& owns,
            // PackQueue<T>& queue, 
            // const string& name,
            // vector<string> recipients
        ):
            owns(config.getOwnsRef()),
            Agent<T>(config)
        {}

        virtual ~Agency() {
            lock_guard<mutex> lock(agents_mtx);
            for (Agent<T>* agent : agents) {
                owns.release(this, agent); // delete agent;
                agent = nullptr;
            }
            agents.clear();
        }

        // void setVoiceOutput(bool state) { voice = state; }

        // bool isVoiceOutput() const { return voice; }

        void handle(const string& sender, const T& item) override {

            // TODO: these are deprecated: (commands and agents has/can have access to the agency so they can do it by themself)
            if (item == "exit") {
                cout << "Exit indicated by agent '" + sender + "'..." << endl;

                // closing agents and the agency itself
                for (Agent<T>* agent: agents) agent->close();
                this->close();

                // emptying package queue
                while (this->queue.Consume(pack));
            }

            if (item == "list") {
                cout << "Agents in the agency:" << endl;
                for (Agent<T>* agent: agents) cout << agent->name << endl;
            }
        }
        
        // template<typename AgentT, typename... Args> // TODO: pass only the config type and a config parameter instead Args
        // AgentT& spawn(/*const string& name, const vector<string>& recipients,*/ Args&&... args) {
        //     lock_guard<mutex> lock(agents_mtx);
        //     // AgentT* agent = new AgentT(forward<Args>(args)...); // Direct construction
        //     AgentT* agent = owns.allocate<AgentT>(/*forward<Args>(args)*/args...);
        //     owns.reserve(this, agent, FILELN);
        //     // AgentT* agent = new AgentT(/*this->queue, name, recipients,*/ forward<Args>(args)...);
        //     for (const Agent<T>* a: agents)
        //         if (agent->name == a->name) {
        //             owns.release(this, agent); // delete agent;
        //             throw ERROR("Agent '" + a->name + "' already exists.");
        //         }
        //     agents.push_back(agent);
        //     // agent->start();
        //     this->send("Agent '" + agent->name + "' created as " + agent->type());
        //     return *agent;
        // }
        template<typename AgentT, typename ConfigT>
        AgentT& spawn(ConfigT& config) { // TODO: forward Args
            lock_guard<mutex> lock(agents_mtx);
            // AgentT* agent = new AgentT(forward<Args>(args)...); // Direct construction
            AgentT* agent = owns.allocate<AgentT>(config);
            owns.reserve(this, agent, FILELN);
            // AgentT* agent = new AgentT(/*this->queue, name, recipients,*/ forward<Args>(args)...);
            for (const Agent<T>* a: agents)
                if (agent->name == a->name) {
                    owns.release(this, agent); // delete agent;
                    throw ERROR("Agent '" + a->name + "' already exists.");
                }
            agents.push_back(agent);
            // agent->start();
            this->send("Agent '" + agent->name + "' created as " + agent->type());
            return *agent;
        }

        [[nodiscard]]
        bool kill(const std::string& name) {        
            lock_guard<mutex> lock(agents_mtx);
            bool found = false;   
            for (size_t i = 0; i < agents.size(); i++)
                if (agents[i]->name == name) {
                    found = true;
                    //agents[i]->close();
                    owns.release(this, agents[i]); // delete agents[i];
                    agents[i] = nullptr;
                    agents.erase(agents.begin() + i);
                    i--;  // Back up to recheck the shifted element
                }
            this->queue.drop(name);
            return found;
        }
        
        void tick() {
            while (this->queue.Consume(pack)) {
                if (this->name == pack.recipient) handle(pack.sender, pack.item);
                else {
                    lock_guard<mutex> lock(agents_mtx);
                    for (Agent<T>* agent: agents)
                        if (agent && agent->name == pack.recipient) agent->handle(pack.sender, pack.item);    
                }
            }
        }

        bool hasAgent(const string& name) const {
            for (Agent<T>* agent: agents)
                if (agent->name == name) return true;
            return false;
        }

        template<typename AgentT>
        AgentT& getAgentRef(const string& name) const {
            for (Agent<T>* agent: agents)
                if (agent->name == name) return *(AgentT*)agent;
            throw ERROR("Requested agent '" + name + "' is not found.");
        }

        // const vector<Agent<T>*>& getAgentsCRef() const { return agents; }

        vector<string> findAgents(const string& keyword = "") const {
            vector<string> found;
            for (const Agent<T>* agent: agents) {
                string name = safe(agent)->name;
                if (keyword.empty() || str_contains(name, keyword))
                    found.push_back(name);
            };
            return found;
        }

        string dumpAgents(const vector<string>& names) const {
            vector<string> dumps;
            for (const string& name: names) {
                if (hasAgent(name)) dumps.push_back(getAgentRef<Agent<T>>(name).dump());
                else dumps.push_back("Agent " + name + " is not exists!");
            };
            return implode("\n", dumps);
        }

    private:
        Owns& owns;
        // bool voice = false;
        vector<Agent<T>*> agents; // TODO: Replace vector<Agent<T>*> with unordered_map<string, Agent<T>*>, O(1) lookup vs. O(n), huge win with many agents.
        mutex agents_mtx;
        Pack<T> pack;
    };

    template<typename T>
    const string Agency<T>::agent_list_tpl = R"(List of agents:
{{agents}}
{{found}} of {{total}} agent(s) found.)";
    
}

#ifdef TEST

#include "../utils/Test.hpp"
#include "tests/TestAgent.hpp"
#include "tests/TestAgency.hpp"
#include "PackQueue.hpp"

// Previous helpers (e.g., queue_to_vector) ...

// Agency tests
void test_Agency_constructor_basic() {
    Owns owns;
    PackQueue<string> queue;
    AgencyConfig<string> config(owns, queue, "agency", {});
    Agency<string> agency(config);
    auto actual_name = agency.name;
    assert(actual_name == "agency" && "Agency name should be 'agency'");
}

void test_Agency_handle_exit() {
    Owns owns;
    PackQueue<string> queue;
    AgencyConfig<string> agency_config(owns, queue, "agency", {});
    TestAgency<string> agency(agency_config);
    AgentConfig<string> test_agent_config(agency.queue, string("test_agent"), vector<string>({}));
    TestAgent<string>& test_agent = agency.spawn<TestAgent<string>>(test_agent_config);
    auto actual_output = capture_cout([&]() { agency.handle("user", "exit"); });
    auto actual_closed = agency.isClosing();
    auto actual_agent_closed = test_agent.isClosing();
    auto actual_queue_contents = queue_to_vector(queue);
    assert(actual_closed && "Agency should be closed after exit");
    assert(actual_agent_closed && "Spawned agent should be closed after exit");
    assert(actual_queue_contents.empty() && "Queue should be empty after exit");
    assert(str_contains(actual_output, "Exit indicated by agent 'user'...") && "Exit message should be printed");
}

void test_Agency_handle_list() {
    Owns owns;
    PackQueue<string> queue;
    AgencyConfig<string> agency_config(owns, queue, "agency", {});
    Agency<string> agency(agency_config);
    AgentConfig<string> agent_config_1(agency.queue, string("agent1"), vector<string>({}));
    AgentConfig<string> agent_config_2(agency.queue, string("agent2"), vector<string>({}));
    agency.spawn<TestAgent<string>>(agent_config_1);
    agency.spawn<TestAgent<string>>(agent_config_2);
    auto actual_output = capture_cout([&]() { agency.handle("user", "list"); });
    auto expected_output = "Agents in the agency:\nagent1\nagent2\n";
    assert(actual_output == expected_output && "List should output all agent names");
}

void test_Agency_spawn_success() {
    Owns owns;
    PackQueue<string> queue;
    AgencyConfig<string> agency_config(owns, queue, "agency", {});
    Agency<string> agency(agency_config);
    AgentConfig<string> agent_config(agency.queue, string("test_agent"), vector<string>({}));
    auto& agent = agency.spawn<TestAgent<string>>(agent_config);
    auto actual_name = agent.name;
    assert(actual_name == "test_agent" && "Spawned agent should have correct name");
    auto actual_output = capture_cout([&]() { agency.handle("user", "list"); });
    assert(str_contains(actual_output, "test_agent") && "Spawned agent should appear in list");
}

void test_Agency_spawn_duplicate() {
    Owns owns;
    PackQueue<string> queue;
    AgencyConfig<string> agency_config(owns, queue, "agency", {});
    Agency<string> agency(agency_config);
    AgentConfig<string> agent_config_1(agency.queue, string("test_agent"), vector<string>({}));
    agency.spawn<TestAgent<string>>(agent_config_1);
    bool thrown = false;
    try {
        AgentConfig<string> agent_config_2(agency.queue, string("test_agent"), vector<string>({}));
        agency.spawn<TestAgent<string>>(agent_config_2);
    } catch (exception& e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Agent 'test_agent' already exists") && "Exception should indicate duplicate name");
    }
    assert(thrown && "Spawn with duplicate name should throw");
}

void test_Agency_kill_basic() {
    Owns owns;
    PackQueue<string> queue;
    AgencyConfig<string> agency_config(owns, queue, "agency", {});
    Agency<string> agency(agency_config);
    AgentConfig<string> agent_config(agency.queue, string("test_agent"), vector<string>({}));
    agency.spawn<TestAgent<string>>(agent_config);
    queue.Produce(Pack<string>("user", "test_agent", "hello"));
    agency.tick();  // Process the queue before killing
    assert(agency.kill("test_agent") && "Agent should be found");
    auto actual_output = capture_cout([&]() { agency.handle("user", "/list"); });
    auto actual_queue = queue_to_vector(queue);
    assert(!str_contains(actual_output, "test_agent") && "Killed agent should not appear in list");
    assert(actual_queue.empty() && "Queue should have no items for killed agent");
}

void test_Agency_tick_dispatch() {
    Owns owns;
    PackQueue<string> queue;
    AgencyConfig<string> config(owns, queue, "agency", {});
    Agency<string> agency(config);
    AgentConfig<string> agent_config(agency.queue, string("test_agent"), vector<string>({}));
    auto& test_agent = agency.spawn<TestAgent<string>>(agent_config);
    queue.Produce(Pack<string>("user", "agency", "list"));
    queue.Produce(Pack<string>("user", "test_agent", "hello"));
    auto actual_output = capture_cout([&]() { agency.tick(); });
    assert(str_contains(actual_output, "test_agent") && "Tick should process agency message");
    assert(test_agent.handled && "Tick should dispatch to agent");
}

// Register tests
TEST(test_Agency_constructor_basic);
TEST(test_Agency_handle_exit);
TEST(test_Agency_handle_list);
TEST(test_Agency_spawn_success);
TEST(test_Agency_spawn_duplicate);
TEST(test_Agency_kill_basic);
TEST(test_Agency_tick_dispatch);

// TODO:
// Memory Management: Ensure ~Agency doesn’t double-delete if kill is called before destruction (current code is safe, but worth a double-check).
// Deadlock Prevention: Verify no deadlocks if handle calls back into queue (e.g., via send).

#endif