#pragma once

#include "../agents/IUserAgent.hpp"

using namespace prompt::agents;

namespace prompt::commands {

    class ExitCommand : public Command {
    public:
        // Define the command pattern(s) this class will match
        vector<string> get_patterns() const override {
            return {"/exit"};
        }

        // Execute the command logic
        string run(void* user_context, const vector<string>& args) override {
            dref<IUserAgent>(user_context).exit();
            return "Exiting...";
        }
    };
    
}


#ifdef TEST

#include "../../tools/utils/Test.hpp"
#include "../../tools/cmd/tests/MockCommander.hpp"

using namespace prompt::commands;

// Tests for ExitCommand
void test_ExitCommand_run_exitsCommander() {
    MockLogger logger;
    MockLineEditor editor;
    MockCommandLine cline(editor);
    MockCommander commander(cline);
    IUserAgent user("user", commander, logger);
    ExitCommand cmd;
    vector<string> args;

    string actual = cmd.run(&user, args);
    bool commanderExited = commander.is_exiting();

    assert(actual == "Exiting..." && "ExitCommand::run should return 'Exiting...' but didn't");
    assert(commanderExited == true && "ExitCommand::run should set Commander to exiting state");
}

void test_ExitCommand_getPatterns_returnsExitPattern() {
    ExitCommand cmd;
    vector<string> actual = cmd.get_patterns();
    vector<string> expected = {"/exit"};

    assert(actual == expected && "ExitCommand::get_patterns should return {'/exit'}");
}

TEST(test_ExitCommand_run_exitsCommander);
TEST(test_ExitCommand_getPatterns_returnsExitPattern);

#endif