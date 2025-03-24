#pragma once

#include "../../../str/parse.hpp"
#include "../../../voice/STT.hpp"
#include "../../../cmd/Command.hpp"
#include "../../Agency.hpp"
#include "../../agents/UserAgent.hpp"

using namespace tools::str;
using namespace tools::voice;
using namespace tools::utils;
using namespace tools::cmd;
using namespace tools::agency;
using namespace tools::agency::agents;

namespace tools::agency::agents::commands {

    template<typename T>
    class VoiceCommand: public Command {
    public:
    
        vector<string> getPatterns() const override {
            return { 
                "/voice output {switch}",
                "/voice input {switch}",
                "/voice input mute",
                "/voice input unmute",
            };
        }
    
        void run(void* agency_void, const vector<string>& args) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;

            // get user agent
            Agent<T>& agent = agency.getAgentRef("user");
            if (agent.name != "user") throw ERROR("Invalid user agent, name is '" + agent.name + "'");
            UserAgent<T>& user = (UserAgent<T>&)agent;

            string thru = args[1];
            if (thru == "output") {                
                if (args[2] == "mute") {
                    
                    return;
                }
                if (args[2] == "unmute") {
                    
                    return;
                }
                
                bool state = parse<bool>(args[2]);
                agency.setVoiceOutput(state); //  TODO ...
                user.getInterfaceRef().println("Text to speech voice mode output is " + string(state ? "ON" : "OFF"));
                // cout << "Text to speech voice mode output is " + string(state ? "ON" : "OFF") << endl;
                return;
            }
            if (thru == "input") {
                if (args[2] == "mute") {
                    STT* stt = user.getInterfaceRef().get_stt_switch_ref().get_stt_ptr(); // getSttPtr();
                    if (!stt) return;
                    NoiseMonitor* monitor = stt->getMonitorPtr();
                    if (!monitor) return;
                    monitor->set_muted(true);
                    return;
                }
                if (args[2] == "unmute") {
                    STT* stt = user.getInterfaceRef().get_stt_switch_ref().get_stt_ptr(); // getSttPtr();
                    if (!stt) return;
                    NoiseMonitor* monitor = stt->getMonitorPtr();
                    if (!monitor) return;
                    monitor->set_muted(false);
                    return;
                }

                bool state = parse<bool>(args[2]);

                user.getInterfaceRef().setVoiceInput(state);
                user.getInterfaceRef().println("Speech to text voice mode input is " + string(state ? "ON" : "OFF"));
                // cout << "Speech to text voice mode input is " + string(state ? "ON" : "OFF") << endl;
                return;
            }

            throw ERROR("Unrecognised voice command: '" + implode(" ", args) + "'");
            
        }

    };
    
}