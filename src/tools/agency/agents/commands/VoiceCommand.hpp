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

        string getUsage() const override {
            return "/voice <input|output> <on|off|mute|unmute>\n"
                   "Controls voice input (speech-to-text) and output (text-to-speech) settings.\n"
                   "Usage:\n"
                   "  /voice output <on|off>     - Toggle text-to-speech output\n"
                   "  /voice input <on|off>      - Toggle speech-to-text input\n"
                   "  /voice input mute          - Mute speech input monitoring\n"
                   "  /voice input unmute        - Unmute speech input monitoring\n"
                   "Parameters:\n"
                   "  input|output - Select voice direction to control\n"
                   "  on|off       - Enable or disable the feature\n"
                   "  mute|unmute  - Control input monitoring (input only)\n"
                   "Examples:\n"
                   "  /voice output on          # Enable text-to-speech\n"
                   "  /voice input off          # Disable speech-to-text\n"
                   "  /voice input mute         # Mute voice input monitoring\n"
                   "Notes:\n"
                   "  - Requires valid user agent\n"
                   "  - 'mute'/'unmute' only available for input";
        }
    
        void run(void* agency_void, const vector<string>& args) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;

            // get user agent
            Agent<T>& agent = agency.getAgentRef("user");
            if (agent.name != "user") throw ERROR("Invalid user agent, name is '" + agent.name + "'");
            UserAgent<T>& user = (UserAgent<T>&)agent;
            // UserAgent<T>& user = agency.getUserAgentRef();
            UserAgentInterface<T>& interface = user.getInterfaceRef();

            string thru = args[1];
            if (thru == "output") {                
                if (args[2] == "mute") {
                    
                    return;
                }
                if (args[2] == "unmute") {
                    
                    return;
                }
                
                bool state = parse<bool>(args[2]);
                
                interface.setVoiceOutput(state); //  TODO ...
                interface.println("Text to speech voice mode output is " + string(state ? "ON" : "OFF"));
                // cout << "Text to speech voice mode output is " + string(state ? "ON" : "OFF") << endl;
                return;
            }
            if (thru == "input") {
                if (args[2] == "mute") {
                    STT* stt = interface.getSttSwitchRef().getSttPtr(); // getSttPtr();
                    if (!stt) return;
                    NoiseMonitor* monitor = stt->getMonitorPtr();
                    if (!monitor) return;
                    monitor->setMuted(true);
                    return;
                }
                if (args[2] == "unmute") {
                    STT* stt = interface.getSttSwitchRef().getSttPtr(); // getSttPtr();
                    if (!stt) return;
                    NoiseMonitor* monitor = stt->getMonitorPtr();
                    if (!monitor) return;
                    monitor->setMuted(false);
                    return;
                }

                bool state = parse<bool>(args[2]);

                interface.setVoiceInput(state);
                interface.println("Speech to text voice mode input is " + string(state ? "ON" : "OFF"));
                // cout << "Speech to text voice mode input is " + string(state ? "ON" : "OFF") << endl;
                return;
            }

            throw ERROR("Unrecognised voice command: '" + implode(" ", args) + "'");
            
        }

    };
    
}