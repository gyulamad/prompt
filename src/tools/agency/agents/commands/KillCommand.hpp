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
    class KillCommand: public Command {
    public:

        using Command::Command;
        virtual ~KillCommand() {}
    
        vector<string> getPatterns() const override {
            return { this->prefix + "kill {string}" };
        }

        string getName() const override {
            return this->prefix + "kill";
        }

        string getDescription() const override {
            return "Terminates a specified agent by name.";
        }

        string getUsage() const override {
            return implode("\n", vector<string>({
                Usage({
                    getName(), // command
                    getDescription(), // help
                    vector<Parameter>({ // parameters
                        {
                            string("agent_name"), // name
                            bool(false), // optional
                            string("The name of the agent to terminate") // help
                        }
                    }),
                    vector<pair<string, string>>({ // examples
                        make_pair(this->prefix + "kill bot1", "Terminates the agent named 'bot1'")
                    }),
                    vector<string>({ // notes
                        string("Cannot kill the 'user' agent"),
                        string("Returns success/failure message")
                    })
                }).to_string()
            }));
        }
    
        void run(void* agency_void, const vector<string>& args) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;
            
            string name = args[1];

            if (name == "user") throw ERROR("User can not be killed!");
            agency.kill(name)
                ? cout << "Agent '" + name + "' is leaving..." << endl
                : cout << "Agent '" + name + "' not found" << endl;
        }
    };
    
}