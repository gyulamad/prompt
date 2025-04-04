#pragma once

#include "../../voice/TTS.hpp"
#include "../Agent.hpp"
#include "UserAgent.hpp"

using namespace tools::voice;
using namespace tools::agency;

namespace tools::agency::agents {
    
    template<typename T>
    class EchoAgent: public Agent<T> {
    public:
        EchoAgent(
            PackQueue<T>& queue,
            const string& name,
            vector<string> recipients, 
            UserAgentInterface<T>& interface
        ): 
            Agent<T>(queue, name, recipients), 
            interface(interface)
        {}

        string type() const override { return "echo"; }

        void handle(const string& sender, const T& item) override {
            // sleep(2); // emulate some background work;

            string output = "echo " + sender + "> " + string(item);
            interface.clearln();
            interface.println(output);
            if (interface.isVoiceOutput()) interface.speak(item);
        }

    private:
        // Agency<T>& agency;
        UserAgentInterface<T>& interface;
    };
    
}