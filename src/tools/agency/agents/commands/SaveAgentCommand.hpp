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
    class SaveAgentCommand: public Command {
    public:

        vector<string> getPatterns() const override {
            return {
                "/save agent {string} [{string}]"
            };
        }

        string getUsage() const override {
            return implode("\n", vector<string>({
                Usage({
                    string("/save agent <agent-name> [<output-name>]"), // command
                    string("Saves an agent to a file."), // help
                    vector<Parameter>({ // parameters
                        {
                            string("agent-name"), // name
                            bool(false), // optional
                            string("Name of the agent to save.") // help
                        },
                        {
                            string("output-name"), // name
                            bool(true), // optional
                            string("Name of the output file. Defaults to agent-name.json.") // help
                        }
                    }),
                    vector<pair<string, string>>({ // examples
                        make_pair("/save agent my_agent", "Saves the agent 'my_agent' to my_agent.json"),
                        make_pair("/save agent my_agent output.json", "Saves the agent 'my_agent' to output.json")
                    }),
                    vector<string>({ // notes
                        string("If output-name is not provided, the agent will be saved to a file named agent-name.json.")
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
            string outputName = (args.size() > 3) ? args[3] : agentName + ".json";

            Agent<T>& agent = agency.getWorkerRef(agentName);
            if (agent.name != agentName) throw ERROR("Invalid agent, name is '" + agent.name + "'");

            saveAgent(agent, outputName);
        }

    private:
        void saveAgent(Agent<T>& agent, const string& outputName) {
            // TODO: needs to be implemented
        }
    };

}
