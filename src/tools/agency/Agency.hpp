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
                        this->queue.drop(agent->getName());
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
        // PackQueue<T> queue;
        long ms;
    };

}