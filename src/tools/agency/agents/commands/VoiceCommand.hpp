#pragma once

#include <string>
#include <vector>

#include "../../../str/parse.hpp"
#include "../../../voice/STT.hpp"
#include "../../../cmd/Usage.hpp"
#include "../../../cmd/Parameter.hpp"
#include "../../../cmd/Command.hpp"
#include "../../Agency.hpp"
#include "../UserAgent.hpp"

using namespace std;
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

        using Command::Command;
        virtual ~VoiceCommand() {}
    
        vector<string> getPatterns() const override {
            return { 
                this->prefix + "voice output {switch}",
                this->prefix + "voice input {switch}",
                this->prefix + "voice input mute",
                this->prefix + "voice input unmute",
            };
        }

        string getName() const override {
            return this->prefix + "voice";
        }

        string getDescription() const override {
            return "Controls voice input (speech-to-text) and output (text-to-speech) settings.";
        }

        string getUsage() const override {
            return implode("\n", vector<string>({
                Usage({
                    getName(), // command
                    getDescription(), // help
                    vector<Parameter>({ // parameters
                        {
                            string("direction"), // name
                            bool(false), // optional
                            string("Voice direction to control (input|output)") // help
                        },
                        {
                            string("action"), // name
                            bool(false), // optional
                            string("Action to perform (on|off|mute|unmute)") // help
                        }
                    }),
                    vector<pair<string, string>>({ // examples
                        make_pair(this->prefix + "voice output on", "Enable text-to-speech"),
                        make_pair(this->prefix + "voice output off", "Disable text-to-speech"),
                        make_pair(this->prefix + "voice input on", "Enable speech-to-text"),
                        make_pair(this->prefix + "voice input off", "Disable speech-to-text"), 
                        make_pair(this->prefix + "voice input mute", "Mute voice input monitoring"),
                        make_pair(this->prefix + "voice input unmute", "Unmute voice input monitoring")
                    }),
                    vector<string>({ // notes
                        string("Requires valid user agent"),
                        string("'mute'/'unmute' actions only available for input direction"),
                        string("'on'/'off' toggle the entire voice feature"),
                        string("'mute'/'unmute' only affect input monitoring")
                    })
                }).to_string()
            }));
        }
    
        void run(void* agency_void, const vector<string>& args) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;

            // get user interface
            UserAgent<T>& user = (UserAgent<T>&)agency.getWorkerRef("user");
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
                
                // interface.setVoiceOutput(state); //  TODO ...
                interface.println("[STUB] Text to speech voice mode output is " + string(state ? "ON" : "OFF"));
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