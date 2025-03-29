#pragma once

#include <string>

#include "../../voice/TTS.hpp"
#include "../Agent.hpp"
#include "UserAgent.hpp"

using namespace tools::abstracts;
using namespace tools::voice;
using namespace tools::agency;

using namespace std;

namespace tools::agency::agents {


    class GeminiAPI {
    public:
        
    };
    
    template<typename T>
    class LLMAgent: public Agent<T> {
    public:
        LLMAgent(
            PackQueue<T>& queue,
            const string& name
        ): 
            Agent<T>(queue, name)
        {}

        string type() const override { return "llm"; }

        void handle(const string& sender, const T& item) override {
            // sleep(2); // emulate some background work;

            // // get user agent
            // Agent<T>& agent = agency.getAgentRef("user");
            // if (agent.name != "user") throw ERROR("Invalid user agent, name is '" + agent.name + "'");
            // UserAgent<T>& user = (UserAgent<T>&)agent;

            // string output = "echo " + sender + "> " + string(item);
            // // user.getInterfaceRef()
            // interface.clearln();
            // interface.println(output);
            // if (interface.isVoiceOutput()) interface.speak(item);
            
            // UserAgent<T>& user = (UserAgent<T>&)agency.getAgentRef("user");

            this->send(sender, llm.request(item));
        }

    private:
    
    };
    
}