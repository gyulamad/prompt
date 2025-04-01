#pragma once

#include <string>
#include <vector>

#include "../../../cmd/Usage.hpp"
#include "../../../cmd/Parameter.hpp"
#include "../../../cmd/Command.hpp"
#include "../../Agency.hpp"

using namespace std;
using namespace tools::cmd;
using namespace tools::agency;

namespace tools::agency::agents::commands {

    template<typename T>
    class ListCommand: public Command {
    public:
    
        vector<string> getPatterns() const override {
            return {
                "/list",
                "/list {string}",
            };
        }

        string getUsage() const override {
            return implode("\n", vector<string>({
                Usage({
                    string("/list"), // command
                    string("Displays a list of all registered agents in the agency."), // help
                    vector<Parameter>({ // parameters
                        {
                            string("filter"), // name
                            bool(true), // optional
                            string("Optional filter string to match agent names") // help
                        }
                    }),
                    vector<pair<string, string>>({ // examples
                        make_pair("/list", "Lists all agents"),
                        make_pair("/list bot", "Lists agents containing 'bot' in their name")
                    }),
                    vector<string>({ // notes
                        string("Outputs each agent's name on a new line"),
                        string("Shows total count of matching agents")
                    })
                }).to_string()
            }));
        }
    
        void run(void* agency_void, const vector<string>& args) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;
            vector<string> agents = agency.findAgents(args.size() >= 2 ? args[1] : "");
            agency.template getAgentRef<UserAgent<T>>("user")
                .getInterfaceRef()
                .println(tpl_replace({
                    { "{{agents}}", agency.dumpAgents(agents) },
                    { "{{found}}", to_string(agents.size()) },
                    { "{{total}}", to_string(agency.findAgents().size()) },
                }, Agency<T>::agent_list_tpl));
                
            // vector<Agent<T>*> agents = agency.getAgentsCRef();
            // cout << "List of agents:" << endl;
            // for (const Agent<T>* agent: agents) cout << "Agent '" + agent->name + "'" << endl;
        }
    };
    
}