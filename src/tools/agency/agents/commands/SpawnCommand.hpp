#pragma once

#include <string>
#include <vector>

#include "../../../containers/array_keys.hpp"
#include "../../../str/parse_vector.hpp"
#include "../../../cmd/Usage.hpp"
#include "../../../cmd/Parameter.hpp"
#include "../../../cmd/Command.hpp"
#include "../../Agency.hpp"
#include "../../AgentRoleMap.hpp"

using namespace std;
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
            return implode("\n", vector<string>({
                Usage({
                    string("/spawn"), // command
                    string("Creates a new agent with specified role, optional name, and optional recipients."), // help
                    vector<Parameter>({ // parameters
                        {
                            string("role"), // name
                            bool(false), // optional
                            string("Agent role type (available: " + implode(", ", array_keys(roles)) + ")") // help
                        },
                        {
                            string("name"), // name
                            bool(true), // optional
                            string("Custom agent name (defaults to role name)") // help
                        },
                        {
                            string("recipients"), // name
                            bool(true), // optional
                            string("Comma-separated list of recipient names") // help
                        }
                    }),
                    vector<pair<string, string>>({ // examples
                        make_pair("/spawn worker", "Creates worker agent"),
                        make_pair("/spawn bot mybot", "Creates bot agent named 'mybot'"),
                        make_pair("/spawn bot mybot user1,user2", "Creates bot agent with recipients")
                    }),
                    vector<string>({ // notes
                        string("Role must match an existing role type"),
                        string("Names must be unique within the agency"),
                        string("Default recipient is the current user")
                    })
                }).to_string()
            }));
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
