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
    class HelpCommand: public Command { // TODO: help/exit can go back to cmd namespace?
    public:

        using Command::Command;
        virtual ~HelpCommand() {}
    
        vector<string> getPatterns() const override {
            return {
                this->prefix + "help",
                this->prefix + "help {string}"
            };
        }

        string getName() const override {
            return this->prefix + "help";
        }

        string getDescription() const override {
            return "Displays available commands or detailed help for a specific command.";
        }

        string getUsage() const override {
            vector<string> notes = { // notes
                "Without arguments, lists all command patterns alphabetically",
                "With a command name, shows that command's detailed usage"
            };
            if (!this->prefix.empty())
                notes.push_back("All command should start with a prefix: '" + this->prefix + "'");
            return implode("\n", vector<string>({
                Usage({
                    getName(), // command
                    getDescription(), // help
                    vector<Parameter>({ // parameters
                        {
                            string("command"), // name
                            bool(true), // optional
                            string("Name of the command to get help for") // help
                        }
                    }),
                    vector<pair<string, string>>({ // examples
                        make_pair(this->prefix + "help", "Shows all command patterns"), 
                        make_pair(this->prefix + "help list", "Shows detailed help for the list command")
                    }),
                    vector<string>()
                }).to_string()
            }));
        }
    
        void run(void* worker_void, const vector<string>& args) override {
            Worker<T>& worker = *safe((Worker<T>*)worker_void);
            Agency<T>& agency = *safe((Agency<T>*)worker.getAgencyPtr());

            // get user agent
            UserAgent<T>& user = (UserAgent<T>&)agency.getWorkerRef("user");

            // get commander
            Commander& commander = user.getInterfaceRef().getCommanderRef();

            vector<Command*>& commands = commander.getCommandsRef();

            // vector<string> patterns;
            vector<const Command*> cmds;
            string q = this->prefix + (args.size() >= 2 ? args[1] : "");
            for (const Command* command: commands) {
                NULLCHK(command);
                vector<string> pttrns = command->getPatterns();
                // patterns = array_merge(patterns, pttrns);

                for (const string& pttrn: pttrns)
                    if (str_starts_with(pttrn, q) && !in_array(command, cmds))
                        cmds.push_back(command);
            }
            // sort(patterns);

            // if (args.size() == 1)
            //     for (const string& pattern: patterns)
            //         cout << pattern << endl;
                
            vector<string> outputs;
            if (q == this->prefix) {
                outputs.push_back(getUsage());
                outputs.push_back("Available command(s):");
            }
            foreach (cmds, [&](const Command* cmd) {
                if (q == this->prefix)
                    outputs.push_back(safe(cmd)->getName() + "\n" + cmd->getDescription());
                else 
                    outputs.push_back(safe(cmd)->getUsage());
            });
            user.getInterfaceRef().println(implode("\n\n", outputs));
        }
    };
    
}

#ifdef TEST

// #include "../../../tools/utils/Test.hpp"
// #include "../../../src/tools/agency/AgentRoleMap.hpp"
// #include "../../../src/tools/containers/vector_equal.hpp"

using namespace tools::agency::agents::commands;
using namespace tools::utils;
using namespace tools::cmd;

void test_HelpCommand_GetName_Help() {
    AgentRoleMap dummyRoles;
    HelpCommand<string> cmd("");
    string actual = cmd.getName();
    assert(actual == "help" && "GetName Help: Name mismatch");
}

void test_HelpCommand_GetPatterns() {
    AgentRoleMap dummyRoles;
    HelpCommand<string> cmd("");
    vector<string> expected = {
        "help",
        "help {string}"
    };
    vector<string> actual = cmd.getPatterns();
    assert(vector_equal(actual, expected) && "GetPatterns: Pattern mismatch");
}

void test_HelpCommand_GetDescription_Help() {
    AgentRoleMap dummyRoles;
    HelpCommand<string> cmd("");
    string actual = cmd.getDescription();
    assert(actual == "Displays available commands or detailed help for a specific command." && "GetDescription Help: Description mismatch");
}

// --- Register Tests ---
TEST(test_HelpCommand_GetName_Help);
TEST(test_HelpCommand_GetDescription_Help);
TEST(test_HelpCommand_GetPatterns);

#endif
