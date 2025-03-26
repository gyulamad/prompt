#pragma once

#include "../../../containers/array_keys.hpp"
#include "../../../cmd/Command.hpp"
#include "../../Agency.hpp"
#include "../../AgentRoleMap.hpp"

using namespace tools::containers;
using namespace tools::cmd;
using namespace tools::agency;

namespace tools::agency::agents::commands {

    template<typename T>
    class SpawnCommand: public Command {
    public:

        SpawnCommand(
            AgentRoleMap<T>& roles
        ): 
            Command(),
            roles(roles)
        {}
    
        vector<string> getPatterns() const override {
            return { 
                "/spawn {string} {string} {string}"
            };
        }
        
        void run(void* agency_void, const vector<string>& args) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;

            if (args.size() < 2) throw ERROR("Missing argument(s): use: /spawn <role> [<name> [<recipients>,...]]");
            string role = trim(args[1]);
            string name = args.size() >= 3 ? trim(args[2]) : role;
            vector<string> recipients = args.size() >= 4 ? explode(",", args[3]) : vector<string>({});
            foreach (recipients, [](string& recipient) { recipient = trim(recipient); });

            if (!in_array(role, array_keys(roles))) 
                throw ERROR("Invalid agent role: " + role + " - available roles are [" + implode(", ", array_keys(roles)) + "]");

            // Spawn the agent
            Agent<T>& agent = roles[role](agency, name/*TODO: , recipients*/);
            cout << "Agent '" + agent.name + "' created as " + role << endl;
        }
        
    private:
        AgentRoleMap<T>& roles;
    };
    
}