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
        EchoAgent(PackQueue<T>& queue, Agency<T>& agency, TTS* tts = nullptr): Agent<T>(queue, "echo"), agency(agency), tts(tts) {}

        void handle(const string& sender, const T& item) override {
            // sleep(2); // emulate some background work;

            // get user agent
            Agent<T>& agent = agency.getAgentRef("user");
            if (agent.name != "user") throw ERROR("Invalid user agent, name is '" + agent.name + "'");
            UserAgent<T>& user = (UserAgent<T>&)agent;

            // get commander
            Commander& commander = user.getInterfaceRef().getCommanderRef();

            // get command line
            CommandLine& cline = commander.get_command_line_ref();

            string output = "echo " + sender + "> " + string(item);
            if (agency.isVoiceOutput()) {
                if (tts) tts->speak(output);
                else throw ERROR("Text to speech is missing");
            } else {
                cline.getEditorRef().WipeLine();
                cout << output << endl;
                cline.getEditorRef().RefreshLine();
            }
        }

    private:
        Agency<T>& agency;
        TTS* tts = nullptr;
    };
    
}