#pragma once

#include "../../../containers/array_keys.hpp"
#include "../../../str/parse_vector.hpp"
#include "../../../cmd/Command.hpp"
#include "../../Agency.hpp"
#include "../../AgentRoleMap.hpp"

using namespace tools::containers;
using namespace tools::str;
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
                "/spawn {string}",
                "/spawn {string} {string}",
                "/spawn {string} {string} {string}"
            };
        }

        string getUsage() const override {
            return "/spawn <role> [name] [recipients]\n"
                   "Creates a new agent with specified role, optional name, and optional recipients.\n"
                   "Usage:\n"
                   "  /spawn <role>              - Spawn agent with default name and no recipients\n"
                   "  /spawn <role> <name>       - Spawn agent with custom name and no recipients\n"
                   "  /spawn <role> <name> <recipients> - Spawn agent with custom name and recipients\n"
                   "Parameters:\n"
                   "  role       - Required agent role type\n"
                   "  name       - Optional custom agent name (defaults to role)\n"
                   "  recipients - Optional comma-separated list of recipient names\n"
                   "Examples:\n"
                   "  /spawn worker              # Creates worker agent\n"
                   "  /spawn bot mybot           # Creates bot agent named 'mybot'\n"
                   "  /spawn bot mybot user1,user2 # Creates bot agent named 'mybot' with recipients user1 and user2\n"
                   "Available roles: " + implode(", ", array_keys(roles)) + "\n"
                   "Notes:\n"
                   "  - Role must match an existing role type\n"
                   "  - Names must be unique within the agency";
        }
        
        void run(void* agency_void, const vector<string>& args) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;

            if (args.size() < 2) throw ERROR("Missing argument(s): use: /spawn <role> [<name>] [<recipients>]");
            string role = trim(args[1]);
            string name = args.size() >= 3 ? trim(args[2]) : role;
            vector<string> recipients = args.size() >= 4 
                ? parse_vector<string>(args[3]) 
                : vector<string>({ agency.template getAgentRef<UserAgent<T>>("user").name });

            if (!in_array(role, array_keys(roles))) {
                throw ERROR("Invalid agent role: " + role + " - available roles are [" + implode(", ", array_keys(roles)) + "]");
            }

            Agent<T>& agent = roles[role](agency, name, recipients);
        }
        
        
    private:
        AgentRoleMap<T>& roles;
    };
    
}
