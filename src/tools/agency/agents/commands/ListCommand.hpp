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
using namespace tools::agency::agents;

namespace tools::agency::agents::commands {

    template<typename T>
    class ListCommand: public Command {
    public:

        using Command::Command;
        virtual ~ListCommand() {}
    
        vector<string> getPatterns() const override {
            return {
                this->prefix + "list",
                this->prefix + "list {string}",
            };
        }

        string getName() const override {
            return this->prefix + "list";
        }

        string getDescription() const override {
            return "Displays a list of all registered agents in the agency.";
        }

        string getUsage() const override {
            return implode("\n", vector<string>({
                Usage({
                    getName(), // command
                    getDescription(), // help
                    vector<Parameter>({ // parameters
                        {
                            string("filter"), // name
                            bool(true), // optional
                            string("Optional filter string to match agent names") // help
                        }
                    }),
                    vector<pair<string, string>>({ // examples
                        make_pair(this->prefix + "list", "Lists all agents"),
                        make_pair(this->prefix + "list bot", "Lists agents containing 'bot' in their name")
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
            vector<string> agents = agency.findWorkers(args.size() >= 2 ? args[1] : "");
            UserAgent<T>& user = (UserAgent<T>&)agency.getWorkerRef("user");
            user.getInterfaceRef()
                .println(tpl_replace({
                    { "{{agency}}", agency.getName() },
                    { "{{workers}}", agency.dumpWorkers(agents) },
                    { "{{found}}", to_string(agents.size()) },
                    { "{{total}}", to_string(agency.findWorkers().size()) },
                }, Agency<T>::worker_list_tpl));
                
            // vector<Agent<T>*> agents = agency.getAgentsCRef();
            // cout << "List of agents:" << endl;
            // for (const Agent<T>* agent: agents) cout << "Agent '" + agent->name + "'" << endl;
        }
    };
    
}