#pragma once

#include "../../../containers/array_key_exists.hpp"
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
    
        vector<string> getPatterns() const override {
            return { 
                "/help",
                "/help {string}"
            };
        }

        string getUsage() const override {
            return "/help [command]\n"
                   "Displays available commands or detailed help for a specific command.\n"
                   "Usage:\n"
                   "  /help           - List all available command patterns\n"
                   "  /help <command> - Show detailed usage for a specific command\n"
                   "Parameters:\n"
                   "  command - (optional) Name of the command to get help for\n"
                   "Examples:\n"
                   "  /help          # Shows all command patterns\n"
                   "  /help list     # Shows detailed help for the list command\n"
                   "Notes:\n"
                   "  - Without arguments, lists all command patterns alphabetically\n"
                   "  - With a command name, shows that command's detailed usage";
        }
    
        void run(void* agency_void, const vector<string>& args) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;

            // get user agent
            UserAgent<T>& user = agency.template getAgentRef<UserAgent<T>>("user");
            // Agent<T>& agent = agency.getAgentRef("user");
            // if (agent.name != "user") throw ERROR("Invalid user agent, name is '" + agent.name + "'");
            // UserAgent<T>& user = (UserAgent<T>&)agent;
            // UserAgent<T>& user = agency.getUserAgentRef();

            // get commander
            Commander& commander = user.getInterfaceRef().getCommanderRef();

            vector<Command*>& commands = commander.getCommandsRef();

            vector<string> patterns;
            vector<const Command*> cmds;
            string cmd = "/" + (args.size() >= 2 ? args[1] : "");
            for (const Command* command: commands) {
                NULLCHK(command);
                vector<string> pttrns = command->getPatterns();
                patterns = array_merge(patterns, pttrns);

                for (const string& pttrn: pttrns)
                    if (str_starts_with(pttrn, cmd) && !in_array(command, cmds))
                        cmds.push_back(command);
            }
            sort(patterns);

            if (args.size() == 1)
                for (const string& pattern: patterns)
                    cout << pattern << endl;
                    
            foreach (cmds, [](const Command* cmd) {
                cout << safe(cmd)->getUsage() << endl;
            });

        }
    };
    
}