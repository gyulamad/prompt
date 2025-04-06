#pragma once

#include <string>
#include <vector>

#include "../../../containers/array_key_exists.hpp"
#include "../../../cmd/Usage.hpp"
#include "../../../cmd/Parameter.hpp"
#include "../../../cmd/Command.hpp"
#include "../../../cmd/Commander.hpp"
#include "../../Agency.hpp"
#include "../UserAgent.hpp"

using namespace std;
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
            return implode("\n", vector<string>({
                Usage({
                    string("/help"), // command
                    string("Displays available commands or detailed help for a specific command."), // help
                    vector<Parameter>({ // parameters
                        {
                            string("command"), // name
                            bool(true), // optional
                            string("Name of the command to get help for") // help
                        }
                    }),
                    vector<pair<string, string>>({ // examples
                        make_pair("/help", "Shows all command patterns"), 
                        make_pair("/help list", "Shows detailed help for the list command")
                    }),
                    vector<string>({ // notes
                        string("Without arguments, lists all command patterns alphabetically"),
                        string("With a command name, shows that command's detailed usage")
                    })
                }).to_string()
            }));
        }
    
        void run(void* agency_void, const vector<string>& args) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;

            // get user agent
            UserAgent<T>& user = (UserAgent<T>&)agency.getWorkerRef("user");

            // get commander
            Commander& commander = user.getInterfaceRef().getCommanderRef();

            vector<Command*>& commands = commander.getCommandsRef();

            // vector<string> patterns;
            vector<const Command*> cmds;
            string cmd = commander.getPrefix() + (args.size() >= 2 ? args[1] : "");
            for (const Command* command: commands) {
                NULLCHK(command);
                vector<string> pttrns = command->getPatterns();
                // patterns = array_merge(patterns, pttrns);

                for (const string& pttrn: pttrns)
                    if (str_starts_with(pttrn, cmd) && !in_array(command, cmds))
                        cmds.push_back(command);
            }
            // sort(patterns);

            // if (args.size() == 1)
            //     for (const string& pattern: patterns)
            //         cout << pattern << endl;
                
            vector<string> outputs;
            foreach (cmds, [&](const Command* cmd) {
                outputs.push_back(safe(cmd)->getUsage());
            });
            user.getInterfaceRef().println(implode("\n\n", outputs));
        }
    };
    
}