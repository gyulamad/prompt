#pragma once

#include "../../Agency.hpp"

using namespace tools::agency;

namespace tools::agency::agents::commands {

    template<typename T, typename TranscriberT>
    class VoiceCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return { 
                "/voice out {switch}",
                "/voice in {switch}",
            };
        }
    
        void run(void* agency_void, const vector<string>& args) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;

            string thru = args[1];
            bool state = parse<bool>(args[2]);
            if (thru == "out") {
                agency.setVoiceOutput(state);
                cout << "Text to speach voice mode output is " + string(state ? "ON" : "OFF") << endl;
                return;
            }
            if (thru == "in") {
                // get user agent
                Agent<T>& agent = agency.getAgentRef("user");
                if (agent.name != "user") throw ERROR("Invalid user agent, name is '" + agent.name + "'");
                UserAgent<T, TranscriberT>& user = (UserAgent<T, TranscriberT>&)agent;

                user.setVoiceInput(state);
                cout << "Speach to text voice mode input is " + string(state ? "ON" : "OFF") << endl;
                return;
            }

            throw ERROR("Invalid selection: '" + thru + "' - use [tts, stt]");
            
        }

    };
    
}