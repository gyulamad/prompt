#pragma once

#include "../../../cmd/Command.hpp"
#include "../../Agency.hpp"

using namespace tools::cmd;
using namespace tools::agency;

namespace tools::agency::agents::commands {

    template<typename T>
    class KillCommand: public Command {
    public:
    
        vector<string> getPatterns() const override {
            return { "/kill {string}" };
        }

        string getUsage() const override {
            return "/kill {agent_name}\n"
                   "Terminates a specified agent by name.\n"
                   "Usage: /kill <agent_name>\n"
                   "Parameters:\n"
                   "  agent_name - The name of the agent to terminate\n"
                   "Examples:\n"
                   "  /kill bot1\n"
                   "Notes:\n"
                   "  - Cannot kill the 'user' agent\n"
                   "  - Returns success/failure message";
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