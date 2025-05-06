#pragma once

#include <string>
#include <vector>

#include "../../../str/parse.h"
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
                this->prefix + "voice output {switch} {agent_name}", // Added agent_name
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
                        },
                        {
                            string("agent_name"), // name
                            bool(true), // optional
                            string("Optional agent name to target (only for 'output' direction). If omitted, applies to all agents.") // help
                        }
                    }),
                    vector<pair<string, string>>({ // examples
                        make_pair(this->prefix + "voice output on", "Enable text-to-speech for all agents"),
                        make_pair(this->prefix + "voice output off", "Disable text-to-speech for all agents"),
                        make_pair(this->prefix + "voice output on assistant", "Enable text-to-speech for 'assistant' agent"),
                        make_pair(this->prefix + "voice input on", "Enable speech-to-text"),
                        make_pair(this->prefix + "voice input off", "Disable speech-to-text"), 
                        make_pair(this->prefix + "voice input mute", "Mute voice input monitoring"),
                        make_pair(this->prefix + "voice input unmute", "Unmute voice input monitoring")
                    }),
                    vector<string>({ // notes
                        string("Requires valid user agent"),
                        string("'mute'/'unmute' actions only available for input direction"),
                        string("'on'/'off' toggle the entire voice feature or for a specific agent (output only)"),
                        string("'mute'/'unmute' only affect input monitoring"),
                        string("If {agent_name} is omitted for 'output', the setting applies to all agents.")
                    })
                }).to_string()
            }));
        }
    
        void run(void* worker_void, const vector<string>& args) override {
            Worker<T>& worker = *safe((Worker<T>*)worker_void);
            Agency<T>& agency = *safe((Agency<T>*)worker.getAgencyPtr());

            // get user interface
            UserAgent<T>& user = (UserAgent<T>&)agency.getWorkerRef("user");
            UserAgentInterface<T>& interface = user.getInterfaceRef();

            string thru = args[1];
            if (thru == "output") {
                string action = args[2];
                if (action == "mute" || action == "unmute") {
                    interface.println("Mute/unmute actions are not supported for voice output.");
                    return;
                }

                bool state = parse<bool>(action);
                string agent_name = ""; // Default: apply to all

                if (args.size() > 3) {
                    agent_name = args[3]; // Agent name provided
                }

                if (!agent_name.empty()) {
                    // Apply to specific agent
                    try {
                        Worker<T>& target_worker = agency.getWorkerRef(agent_name);
                        if (target_worker.type() == "chat") {
                            ChatbotAgent<T>& chat_agent = dynamic_cast<ChatbotAgent<T>&>(target_worker);
                            chat_agent.setTalks(state);
                            interface.println("Voice output for agent '" + agent_name + "' set to " + string(state ? "ON" : "OFF") + ".");
                        } else {
                            interface.println("Agent '" + agent_name + "' is not a chat agent and does not support voice output control.");
                        }
                    } catch (const exception& e) {
                        interface.println("Error: Agent '" + agent_name + "' not found.");
                    }
                } else {
                    // Apply to all chat agents
                    vector<string> worker_names = agency.findWorkers();
                    int applied_count = 0;
                    for (const string& name : worker_names) {
                        try {
                            Worker<T>& target_worker = agency.getWorkerRef(name);
                            if (target_worker.type() == "chat") {
                                ChatbotAgent<T>& chat_agent = dynamic_cast<ChatbotAgent<T>&>(target_worker);
                                chat_agent.setTalks(state);
                                applied_count++;
                            }
                        } catch (const exception& e) {
                            // Should not happen if findWorkers is correct, but handle defensively
                            interface.println("Error accessing agent '" + name + "': " + e.what());
                        }
                    }
                    interface.println("Voice output set to " + string(state ? "ON" : "OFF") + " for " + to_string(applied_count) + " chat agent(s).");
                }
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
