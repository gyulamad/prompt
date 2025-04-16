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

#ifdef TEST

#include "../../utils/Test.hpp" //  TODO: fix paths everywhere AI hardcode absulutes
#include "../../cmd/LinenoiseAdapter.hpp"

using namespace std;
using namespace tools::voice;
using namespace tools::cmd;
using namespace tools::utils;
using namespace tools::agency::agents;

void test_EchoAgent_type() {
    Owns owns;
    PackQueue<string> queue;
    string name = "test_agent";
    vector<string> recipients;
    TTS tts("", 0, 0, "", "", {});
    STTSwitch sttSwitch;
    MicView micView;
    LinenoiseAdapter lineEditor("> ");
    CommandLine commandLine(lineEditor, "", "", false, 10);
    vector<Command*> commands;
    Commander commander(commandLine, commands, "");
    InputPipeInterceptor inputPipeInterceptor;
    UserAgentInterface<string> interface(tts, sttSwitch, micView, commander, inputPipeInterceptor);

    EchoAgent<string> agent(queue, name, recipients, interface);

    assert(agent.type() == "echo" && "Agent type should be 'echo'");
}

TEST(test_EchoAgent_type);

#endif
