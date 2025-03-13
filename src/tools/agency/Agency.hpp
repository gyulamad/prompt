#pragma once

#include "Agent.hpp"
#include "PackQueue.hpp"

namespace tools::agency {

    template<typename T>
    class Agency {
    public:
        Agency(long ms = 100): ms(ms) {}

        void run() {
            if (agents.empty()) throw ERROR("No agents, spawn some before run the agency!");
            while(!agents.empty()) {
                vector<Agent<T>*> result;
                copy_if(agents.begin(), agents.end(), back_inserter(result), [this](const Agent<T>* agent) {
                    if (agent->isExited()) {
                        // Remove packs from queue which addressed to the agent but never will be delivered
                        queue.drop(agent->getName());
                        delete agent;
                        return false;
                    }
                    return true;
                });
                agents = result;
                sleep_ms(ms);
            }
        }
        
        template<typename AgentT, typename... Args>
        AgentT* spawn(Args&&... args) {
            Agent<T>* agent = new AgentT(queue, forward<Args>(args)...);
            // agent->name should be unique! check for agent name in agents vector first!
            string name = agent->getName();
            for (const Agent<T>* a: agents) {
                if (a->getName() == name) {
                    delete agent;
                    return nullptr;
                }
            }
            agents.push_back(agent);
            return (AgentT*)agent;
        }
        
        template<typename AgentT, typename... Args>
        void kill(AgentT* agent) {
            agent->die();
        }

        void kill(const string& name) {
            for (const Agent<T>* agent: agents)
                if (agent->getName() == name) agent->die();
        }

    protected:
        vector<Agent<T>*> agents;
        PackQueue<T> queue;
        long ms;
    };

}