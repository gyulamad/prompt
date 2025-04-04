#pragma once

#include <string>
#include <vector>

#include "../../../cmd/Usage.hpp"
#include "../../../cmd/Parameter.hpp"
#include "../../../cmd/Command.hpp"
#include "../../Agency.hpp"
// #include "../Agent.hpp"

using namespace std;
using namespace tools::cmd;
using namespace tools::agency;
using namespace tools::agency::agents;

namespace tools::agency::agents::commands {

    template<typename T>
    class LoadAgentCommand: public Command {
    public:

        vector<string> getPatterns() const override {
            return {
                "/load agent {string} [{string}]"
            };
        }

        string getUsage() const override {
            return implode("\n", vector<string>({
                Usage({
                    string("/load agent"), // command
                    string("Loads an agent from a file."), // help
                    vector<Parameter>({ // parameters
                        {
                            string("agent-name"), // name
                            bool(false), // optional
                            string("Name of the agent to load.") // help
                        },
                        {
                            string("input-name"), // name
                            bool(true), // optional
                            string("Name of the input file. Defaults to agent-name.json.") // help
                        }
                    }),
                    vector<pair<string, string>>({ // examples
                        make_pair("/load agent my_agent", "Loads the agent 'my_agent' from my_agent.json"),
                        make_pair("/load agent my_agent input.json", "Loads the agent 'my_agent' from input.json")
                    }),
                    vector<string>({ // notes
                        string("If input-name is not provided, the agent will be loaded from a file named agent-name.json.")
                    })
                }).to_string()
            }));
        }

        void run(void* agency_void, const vector<string>& args) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;

            if (args.size() < 3) {
                throw ERROR("Usage: " + getPatterns()[0]);
            }

            string agentName = args[2];
            string inputName = (args.size() > 3) ? args[3] : agentName + ".json";

            loadAgent(agentName, inputName);
        }

    private:
        void loadAgent(const string& agentName, const string& inputName) {
            // TODO: needs to be implemented
        }
    };

}
