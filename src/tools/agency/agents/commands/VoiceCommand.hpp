#pragma once

#include "../../../voice/STT.hpp"
#include "../../Agency.hpp"

using namespace tools::voice;
using namespace tools::agency;

namespace tools::agency::agents::commands {

    template<typename T, typename TranscriberT>
    class VoiceCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
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
                cout << "Text to speach voice mode output is " + string(state ? "ON" : "OFF") << endl;
                return;
            }
            if (thru == "input") {
                // get user agent
                Agent<T>& agent = agency.getAgentRef("user");
                if (agent.name != "user") throw ERROR("Invalid user agent, name is '" + agent.name + "'");
                UserAgent<T, TranscriberT>& user = (UserAgent<T, TranscriberT>&)agent;

                if (args[2] == "mute") {
                    STT<TranscriberT>* stt = user.getSttPtr();
                    if (!stt) return;
                    NoiseMonitor* monitor = stt->getMonitorPtr();
                    if (!monitor) return;
                    monitor->set_muted(true);
                    return;
                }
                if (args[2] == "unmute") {
                    STT<TranscriberT>* stt = user.getSttPtr();
                    if (!stt) return;
                    NoiseMonitor* monitor = stt->getMonitorPtr();
                    if (!monitor) return;
                    monitor->set_muted(false);
                    return;
                }

                bool state = parse<bool>(args[2]);


                user.setVoiceInput(state);
                cout << "Speach to text voice mode input is " + string(state ? "ON" : "OFF") << endl;
                return;
            }

            throw ERROR("Unrecognised voice command: '" + implode(" ", args) + "'");
            
        }

    };
    
}