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
        string prefix;
        CompletionMatcher completionMatcher; // TODO: send it as a reference? (&)
        bool exiting = false;

    public:
        Commander(CommandLine& commandLine, vector<Command*>& commands, const string& prefix): 
            commandLine(commandLine), commands(commands), prefix(prefix)
        {
            setupCommands();
        }

        virtual ~Commander() {}

        CommandLine& getCommandLineRef() {
            return commandLine;
        }

        string getPrefix() { return prefix; }

        CompletionMatcher& getCompletionMatcherRef() {
            return completionMatcher;
        }

        virtual bool isPrefixed(const string& input) const {
            return str_starts_with(input, prefix);
        }

        virtual bool isExiting() const {
            return exiting || commandLine.isExited();
        }
        
        void setupCommands() {
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
            if (!commandFound) cerr << "Command not found: " << cleanCommandName(inputParts[0]) << endl;
            else if (!commandArgumentsMatches) cerr << "Invalid argument(s). Use '" + prefix + "help " + cleanCommandName(inputParts[0]) + "'..." << endl;
            return commandArgumentsMatches;
        }

    // protected:

        string cleanCommandName(const string& inputParts0) const {
            if (str_starts_with(inputParts0, prefix)) return inputParts0.substr(1);
            throw ERROR("Invalid command name: " + inputParts0);
        }

    };

} // namespace tools::cmd

#ifdef TEST

// #include "../utils/Test.hpp"

#include "tests/MockCommand.hpp"

void test_Commander_isExiting_initial_state() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    vector<Command*> commands;
    Commander commander(cl, commands, "/");
    bool actual = commander.isExiting();
    assert(actual == false && "Commander should not be exiting initially");
}

void test_Commander_isExiting_after_exit() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    vector<Command*> commands;
    Commander commander(cl, commands, "/");
    commander.exit();
    bool actual = commander.isExiting();
    assert(actual == true && "Commander should be exiting after exit() is called");
}

void test_Commander_isExiting_after_commandline_exit() {
    MockLineEditor mock_editor;
    mock_editor.should_exit = true;
    CommandLine cl(mock_editor);
    vector<Command*> commands;
    Commander commander(cl, commands, "/"); // Move the CommandLine into Commander
    commander.getCommandLineRef().readln(); // Call readln() on the Commander's CommandLine
    bool actual = commander.isExiting();
    assert(actual == true && "Commander should be exiting after CommandLine exits");
}

void test_Commander_getCommandLineRef_returns_reference() {
    MockLineEditor mock_editor;
    MockCommandLine cl(mock_editor);
    vector<Command*> commands;
    Commander commander(cl, commands, "/");
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
    MockCommand command("/");
    command.patterns = {"test cmd", "test {param}"};
    vector<Command*> commands = { &command };
    Commander commander(cl, commands, "/");
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
    Commander commander(cl, commands, "/");
    bool actual = commander.runCommand(nullptr, "");
    assert(actual == false && "runCommand should return false for empty input");
}

void test_Commander_runCommand_unknown_command() {
    // LCOV_EXCL_START
    string err = capture_cerr([&]() {
        MockLineEditor editor;
        MockCommandLine cl(editor);
        MockCommand command("/");
        command.patterns = {"known"};
        vector<Command*> commands = { &command };
        Commander commander(cl, commands, "/");
        commander.setupCommands();
        bool actual = commander.runCommand(nullptr, "/unknown");
        assert(actual == false && "runCommand should return false for unknown command");
    });
    // LCOV_EXCL_STOP
    assert(str_contains(err, "Command not found: unknown") && "Should show the correct error");
}

void test_Commander_runCommand_invalid_arguments() {
    // LCOV_EXCL_START
    string err = capture_cerr([&]() {
        MockLineEditor editor;
        MockCommandLine cl(editor);
        MockCommand command("/");
        vector<Command*> commands = { &command };
        command.patterns = {"/test arg1"};
        Commander commander(cl, commands, "/");
        commander.setupCommands();
        bool actual = commander.runCommand(nullptr, "/test");
        assert(actual == false && "runCommand should return false for invalid argument count");
    });
    // LCOV_EXCL_STOP
    assert(str_contains(err, "Invalid argument(s).") && "Should show the correct error");
}

void test_Commander_runCommand_successful_execution() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    MockCommand command("/");
    command.patterns = {"test arg"};
    vector<Command*> commands = { &command };
    Commander commander(cl, commands, "/");
    commander.setupCommands();
    bool actual = commander.runCommand(nullptr, "test value");
    assert(actual == true && "runCommand should return true for valid command");
    assert(command.last_args.size() == 2 && "runCommand should pass correct number of args");
    assert(command.last_args[0] == "test" && "runCommand should pass command name");
    assert(command.last_args[1] == "value" && "runCommand should pass argument");
}

void test_Commander_getPrefix_returns_correct_value() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    vector<Command*> commands;
    string expected_prefix = "/test";
    Commander commander(cl, commands, expected_prefix);
    string actual = commander.getPrefix();
    assert(actual == expected_prefix && "getPrefix should return the set prefix");
}

void test_Commander_isPrefixed_returns_true() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    vector<Command*> commands;
    string prefix = "/";
    Commander commander(cl, commands, prefix);
    assert(commander.isPrefixed("/test") && "Input with prefix should return true");
    assert(commander.isPrefixed(prefix) && "Exact prefix match should return true");
}

void test_Commander_isPrefixed_returns_false() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    vector<Command*> commands;
    Commander commander(cl, commands, "/");
    assert(!commander.isPrefixed("test") && "Input without prefix should return false");
}

void test_Commander_isPrefixed_empty_input() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    vector<Command*> commands;
    Commander commander(cl, commands, "/");
    assert(!commander.isPrefixed("") && "Empty input should return false");
}

void test_Commander_cleanCommandName_success() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    vector<Command*> commands;
    Commander commander(cl, commands, "/");
    
    string input = "/test";
    string expected = "test";
    string actual = commander.cleanCommandName(input);
    
    assert(actual == expected && "cleanCommandName should return the substring after prefix");
}

void test_Commander_cleanCommandName_error_case() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    vector<Command*> commands;
    Commander commander(cl, commands, "/");
    
    string input = "test";
    string expected_error = "Invalid command name: test";
    bool thrown = false;
    string what;
    
    try {
        commander.cleanCommandName(input);
    } catch (exception &e) {
        thrown = true;
        what = e.what();
    }
    
    assert(thrown && "cleanCommandName should throw an exception when input doesn't start with prefix");
    assert(str_contains(what, expected_error) && "Exception message should match expected error");
}

void test_Commander_cleanCommandName_empty_input() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    vector<Command*> commands;
    Commander commander(cl, commands, "/");
    
    string input = "";
    string expected_error = "Invalid command name: ";
    bool thrown = false;
    string what;
    
    try {
        commander.cleanCommandName(input);
    } catch (exception &e) {
        thrown = true;
        what = e.what();
    }
    
    assert(thrown && "cleanCommandName should throw an exception for empty input");
    assert(str_contains(what, expected_error) && "Exception message should match expected error");
}

void test_Commander_getCommandsRef_success() {
    // Setup
    MockLineEditor mock_editor;
    MockCommandLine mock_cl(mock_editor);
    MockCommand cmd1("/"); // Assuming MockCommand constructor takes prefix or similar
    MockCommand cmd2("/");
    cmd1.patterns = {"cmd1"}; // Give them distinct patterns for clarity if needed
    cmd2.patterns = {"cmd2"};
    vector<Command*> initial_commands = {&cmd1, &cmd2};
    Commander commander(mock_cl, initial_commands, "/");

    // Get Reference
    vector<Command*>& commands_ref = commander.getCommandsRef();

    // Assertions
    size_t initial_size = initial_commands.size();
    assert(commands_ref.size() == initial_size && "Reference should have the same size as the original vector initially");
    assert(commands_ref[0] == &cmd1 && "Reference should contain the pointer to cmd1");
    assert(commands_ref[1] == &cmd2 && "Reference should contain the pointer to cmd2");

    // Test Reference Behavior (Modifying original vector)
    MockCommand cmd3("/");
    cmd3.patterns = {"cmd3"};
    // LCOV_EXCL_START
    initial_commands.push_back(&cmd3); // Modify the original vector
    // LCOV_EXCL_STOP

    size_t new_size = initial_commands.size();
    assert(commands_ref.size() == new_size && "Change in original vector size should reflect in the reference");
    assert(commands_ref.back() == &cmd3 && "New command added to original vector should be accessible via reference");
    assert(new_size == initial_size + 1 && "Size should have increased by 1");
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
TEST(test_Commander_getPrefix_returns_correct_value);
TEST(test_Commander_isPrefixed_returns_true);
TEST(test_Commander_isPrefixed_returns_false);
TEST(test_Commander_isPrefixed_empty_input);
TEST(test_Commander_cleanCommandName_success);
TEST(test_Commander_cleanCommandName_error_case);
TEST(test_Commander_cleanCommandName_empty_input);
TEST(test_Commander_getCommandsRef_success);

#endif