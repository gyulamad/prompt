#pragma once

#include "../../../utils/vectors.hpp"
#include "../../Agency.hpp"

using namespace tools::utils;
using namespace tools::agency;

namespace tools::agency::agents::commands {

    template<typename T, typename TranscriberT>
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
            UserAgent<T, TranscriberT>& user = (UserAgent<T, TranscriberT>&)agent;

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