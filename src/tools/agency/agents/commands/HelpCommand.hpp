#pragma once

#include "../../../utils/vectors.hpp"
#include "../../../cmd/Command.hpp"
#include "../../../cmd/Commander.hpp"
#include "../../Agency.hpp"
#include "../../agents/UserAgent.hpp"

using namespace tools::utils;
using namespace tools::cmd;
using namespace tools::agency;
using namespace tools::agency::agents;

namespace tools::agency::agents::commands {

    template<typename T>
    class HelpCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return { "/help" };
        }
    
        void run(void* agency_void, const vector<string>&) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;

            // get user agent
            Agent<T>& agent = agency.getAgentRef("user");
            if (agent.name != "user") throw ERROR("Invalid user agent, name is '" + agent.name + "'");
            UserAgent<T>& user = (UserAgent<T>&)agent;

            // get commander
            Commander* commander = user.getCommanderPtr();
            NULLCHK(commander);

            vector<Command*>& commands = commander->get_commands_ref();
            vector<string> all_patterns;
            for (const Command* command: commands) {
                NULLCHK(command);
                all_patterns = array_merge(all_patterns, command->get_patterns());
            }
            sort(all_patterns);
            for (const string& pattern: all_patterns) cout << pattern << endl;
        }
    };
    
}