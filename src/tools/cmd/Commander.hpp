#pragma once

#include "Command.hpp"
#include "CommandLine.hpp"
#include "CompletionMatcher.hpp"

#include "../containers/array_filter.hpp"
#include "../str/explode.hpp"

using namespace std;
using namespace tools::containers;
using namespace tools::str;

namespace tools::cmd {

    class Commander {
    private:
        CommandLine& commandLine;
        vector<Command*>& commands;
        CompletionMatcher completionMatcher; // TODO: send it as a reference? (&)
        bool exiting = false;

    public:
        Commander(CommandLine& commandLine, vector<Command*>& commands): 
            commandLine(commandLine), commands(commands)
        {
            setupCommands(/*commands*/);
        }

        virtual ~Commander() {}

        CommandLine& getCommandLineRef() {
            return commandLine;
        }

        CompletionMatcher& getCompletionMatcherRef() {
            return completionMatcher;
        }

        virtual bool isExiting() const {
            return exiting || commandLine.isExited();
        }
        
        void setupCommands(/*const vector<Command*>& commands*/) {
            // this->commands = commands;
            
            // update command patterns:
            completionMatcher.commandPatterns = {};
            for (const Command* command : commands)
                completionMatcher.commandPatterns = array_merge(
                    completionMatcher.commandPatterns, 
                    ((Command*)command)->getPatterns()
                );
            commandLine.setCompletionMatcher(completionMatcher);
        }

        vector<Command*>& getCommandsRef() {
            return commands;
        }

        const CompletionMatcher& getCompletionMatcherCRef() const {
            return completionMatcher;
        }

        virtual void exit() {            
            exiting = true;
        }

        virtual bool runCommand(void* user_context, const string& input) {
            if (input.empty()) return false;
            
            bool trlspc;
            vector<string> inputParts = array_filter(completionMatcher.parse_input(input, trlspc, false));

            bool commandFound = false;
            bool commandArgumentsMatches = false;
            for (void* command_void : commands) {
                NULLCHK(command_void);
                Command& command = *(Command*)command_void;
                for (const string& commandPattern : command.getPatterns()) {
                    vector<string> commandPatternParts = array_filter(explode(" ", commandPattern));
                    if (inputParts[0] == commandPatternParts[0]) {
                        commandFound = true;
                        if (inputParts.size() == commandPatternParts.size()) {
                            commandArgumentsMatches = true;
                            command.run(user_context, command.validate(inputParts));
                            break;
                        }
                    }
                }
                if (commandFound) break;
            }
            if (!commandFound) cerr << "Command not found: " << inputParts[0] << endl;
            else if (!commandArgumentsMatches) cerr << "Invalid argument(s)." << endl;
            return commandArgumentsMatches;
        }
    };

} // namespace tools::cmd

#ifdef TEST

#include "../utils/Test.hpp"

#include "tests/MockCommand.hpp"

void test_Commander_isExiting_initial_state() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    vector<Command*> commands;
    Commander commander(cl, commands);
    bool actual = commander.isExiting();
    assert(actual == false && "Commander should not be exiting initially");
}

void test_Commander_isExiting_after_exit() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    vector<Command*> commands;
    Commander commander(cl, commands);
    commander.exit();
    bool actual = commander.isExiting();
    assert(actual == true && "Commander should be exiting after exit() is called");
}

void test_Commander_isExiting_after_commandline_exit() {
    MockLineEditor mock_editor;
    mock_editor.should_exit = true;
    CommandLine cl(mock_editor);
    vector<Command*> commands;
    Commander commander(cl, commands); // Move the CommandLine into Commander
    commander.getCommandLineRef().readln(); // Call readln() on the Commander's CommandLine
    bool actual = commander.isExiting();
    assert(actual == true && "Commander should be exiting after CommandLine exits");
}

void test_Commander_getCommandLineRef_returns_reference() {
    MockLineEditor mock_editor;
    MockCommandLine cl(mock_editor);
    vector<Command*> commands;
    Commander commander(cl, commands);
    CommandLine& actual = commander.getCommandLineRef();
    // Modify the CommandLine via the reference
    actual.setPromptSuffix("modified> ");
    // Check if the modification is reflected in Commander's CommandLine
    string prompt_suffix_from_commander = commander.getCommandLineRef().getPromptSuffix();
    assert(prompt_suffix_from_commander == "modified> " && 
           "getCommandLineRef should return a reference that modifies the internal CommandLine");
}

void test_Commander_set_commands_updates_patterns() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    MockCommand command;
    command.patterns = {"test cmd", "test {param}"};
    vector<Command*> commands = { &command };
    Commander commander(cl, commands);
    commander.setupCommands();
    const CompletionMatcher& cm = commander.getCompletionMatcherRef();
    vector<string> actual = cm.commandPatterns;
    assert(actual.size() == 2 && "setCommands should update command patterns");
    assert(actual[0] == "test cmd" && "set_commands should include first pattern");
    assert(actual[1] == "test {param}" && "set_commands should include second pattern");
}

void test_Commander_runCommand_empty_input() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    vector<Command*> commands;
    Commander commander(cl, commands);
    bool actual = commander.runCommand(nullptr, "");
    assert(actual == false && "runCommand should return false for empty input");
}

void test_Commander_runCommand_unknown_command() {
    string err = capture_cerr([&]() {
        MockLineEditor editor;
        MockCommandLine cl(editor);
        MockCommand command;
        command.patterns = {"known"};
        vector<Command*> commands = { &command };
        Commander commander(cl, commands);
        commander.setupCommands();
        bool actual = commander.runCommand(nullptr, "unknown");
        assert(actual == false && "runCommand should return false for unknown command");
    });
    assert(str_contains(err, "Command not found: unknown") && "Should show the correct error");
}

void test_Commander_runCommand_invalid_arguments() {
    string err = capture_cerr([&]() {
        MockLineEditor editor;
        MockCommandLine cl(editor);
        MockCommand command;
        vector<Command*> commands = { &command };
        command.patterns = {"test arg1"};
        Commander commander(cl, commands);
        commander.setupCommands();
        bool actual = commander.runCommand(nullptr, "test");
        assert(actual == false && "runCommand should return false for invalid argument count");
    });
    assert(str_contains(err, "Invalid argument(s).") && "Should show the correct error");
}

void test_Commander_runCommand_successful_execution() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    MockCommand command;
    command.patterns = {"test arg"};
    vector<Command*> commands = { &command };
    Commander commander(cl, commands);
    commander.setupCommands();
    bool actual = commander.runCommand(nullptr, "test value");
    assert(actual == true && "runCommand should return true for valid command");
    assert(command.last_args.size() == 2 && "runCommand should pass correct number of args");
    assert(command.last_args[0] == "test" && "runCommand should pass command name");
    assert(command.last_args[1] == "value" && "runCommand should pass argument");
}


TEST(test_Commander_isExiting_initial_state);
TEST(test_Commander_isExiting_after_exit);
TEST(test_Commander_isExiting_after_commandline_exit);
TEST(test_Commander_getCommandLineRef_returns_reference);
TEST(test_Commander_set_commands_updates_patterns);
TEST(test_Commander_runCommand_empty_input);
TEST(test_Commander_runCommand_unknown_command);
TEST(test_Commander_runCommand_invalid_arguments);
TEST(test_Commander_runCommand_successful_execution);

#endif