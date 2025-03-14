#pragma once

#include "../../Agency.hpp"

using namespace tools::agency;

namespace tools::agency::agents::commands {

    template<typename T>
    class SpawnCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return { 
                "/spawn {string}"
            };
        }
        
        void run(void* agency_void, const vector<string>& args) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;

            string role = args[1];

            // Map of role strings to factory functions
            map<string, function<Agent<T>&(Agency<T>&)>> roles = {
                { "echo", [](Agency<T>& agency) -> Agent<T>& { return agency.template spawn<EchoAgent<T>>(agency); } },
            };
            if (!in_array(role, array_keys(roles))) 
                throw ERROR("Invalid agent role: " + role + " - available roles are [" + implode(", ", array_keys(roles)) + "]");

            // Spawn the agent
            Agent<T>& agent = roles[role](agency);
            cout << "Agent '" + agent.name + "' created as " + role << endl;
        }
    };
    
}