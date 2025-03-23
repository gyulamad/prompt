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
        CommandLine& command_line;
        vector<Command*> commands;
        CompletionMatcher cmatcher;
        bool exiting = false;

    public:
        Commander(CommandLine& command_line, const vector<Command*>& commands = {}): command_line(command_line) {
            set_commands(commands);
        }

        virtual ~Commander() {}

        CommandLine& get_command_line_ref() {
            return command_line;
        }

        CompletionMatcher& get_cmatcher_ref() {
            return cmatcher;
        }

        virtual bool is_exiting() const {
            return exiting || command_line.is_exited();
        }
        
        void set_commands(const vector<Command*>& commands) {
            this->commands = commands;
            
            // also update command patterns:
            cmatcher.command_patterns = {};
            for (const Command* command : commands)
                cmatcher.command_patterns = array_merge(
                    cmatcher.command_patterns, 
                    ((Command*)command)->get_patterns()
                );
            command_line.set_completion_matcher(cmatcher);
        }

        vector<Command*>& get_commands_ref() {
            return commands;
        }

        const CompletionMatcher& get_cmatcher_cref() const {
            return cmatcher;
        }

        virtual void exit() {            
            exiting = true;
        }

        virtual bool run_command(void* user_context, const string& input) {
            if (input.empty()) return false;
            
            bool trlspc;
            vector<string> input_parts = array_filter(cmatcher.parse_input(input, trlspc, false));

            bool command_found = false;
            bool command_arguments_matches = false;
            for (void* command_void : commands) {
                NULLCHK(command_void);
                Command& command = *(Command*)command_void;
                for (const string& command_pattern : command.get_patterns()) {
                    vector<string> command_pattern_parts = array_filter(explode(" ", command_pattern));
                    if (input_parts[0] == command_pattern_parts[0]) {
                        command_found = true;
                        if (input_parts.size() == command_pattern_parts.size()) {
                            command_arguments_matches = true;
                            command.run(user_context, input_parts);
                            break;
                        }
                    }
                }
                if (command_found) break;
            }
            if (!command_found) cerr << "Command not found: " << input_parts[0] << endl;
            else if (!command_arguments_matches) cerr << "Invalid argument(s)." << endl;
            return command_arguments_matches;
        }
    };

} // namespace tools::cmd

#ifdef TEST

#include "../utils/Test.hpp"

#include "tests/MockCommand.hpp"

void test_Commander_is_exiting_initial_state() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    Commander commander(cl);
    bool actual = commander.is_exiting();
    assert(actual == false && "Commander should not be exiting initially");
}

void test_Commander_is_exiting_after_exit() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    Commander commander(cl);
    commander.exit();
    bool actual = commander.is_exiting();
    assert(actual == true && "Commander should be exiting after exit() is called");
}

void test_Commander_is_exiting_after_commandline_exit() {
    MockLineEditor mock_editor;
    mock_editor.should_exit = true;
    CommandLine cl(mock_editor);
    Commander commander(cl); // Move the CommandLine into Commander
    commander.get_command_line_ref().readln(); // Call readln() on the Commander's CommandLine
    bool actual = commander.is_exiting();
    assert(actual == true && "Commander should be exiting after CommandLine exits");
}

void test_Commander_get_command_line_ref_returns_reference() {
    MockLineEditor mock_editor;
    MockCommandLine cl(mock_editor);
    Commander commander(cl);
    CommandLine& actual = commander.get_command_line_ref();
    // Modify the CommandLine via the reference
    actual.set_prompt_suffix("modified> ");
    // Check if the modification is reflected in Commander's CommandLine
    string prompt_suffix_from_commander = commander.get_command_line_ref().get_prompt_suffix();
    assert(prompt_suffix_from_commander == "modified> " && 
           "get_command_line_ref should return a reference that modifies the internal CommandLine");
}

void test_Commander_set_commands_updates_patterns() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    Commander commander(cl);
    MockCommand command;
    command.patterns = {"test cmd", "test {param}"};
    vector<Command*> commands = { &command };
    commander.set_commands(commands);
    const CompletionMatcher& cm = commander.get_cmatcher_ref();
    vector<string> actual = cm.command_patterns;
    assert(actual.size() == 2 && "set_commands should update command patterns");
    assert(actual[0] == "test cmd" && "set_commands should include first pattern");
    assert(actual[1] == "test {param}" && "set_commands should include second pattern");
}

void test_Commander_run_command_empty_input() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    Commander commander(cl);
    bool actual = commander.run_command(nullptr, "");
    assert(actual == false && "run_command should return false for empty input");
}

void test_Commander_run_command_unknown_command() {
    string err = capture_cerr([&]() {
        MockLineEditor editor;
        MockCommandLine cl(editor);
        Commander commander(cl);
        MockCommand command;
        command.patterns = {"known"};
        vector<Command*> commands = { &command };
        commander.set_commands(commands);
        bool actual = commander.run_command(nullptr, "unknown");
        assert(actual == false && "run_command should return false for unknown command");
    });
    assert(str_contains(err, "Command not found: unknown") && "Should show the correct error");
}

void test_Commander_run_command_invalid_arguments() {
    string err = capture_cerr([&]() {
        MockLineEditor editor;
        MockCommandLine cl(editor);
        Commander commander(cl);
        MockCommand command;
        command.patterns = {"test arg1"};
        vector<Command*> commands = { &command };
        commander.set_commands(commands);
        bool actual = commander.run_command(nullptr, "test");
        assert(actual == false && "run_command should return false for invalid argument count");
    });
    assert(str_contains(err, "Invalid argument(s).") && "Should show the correct error");
}

void test_Commander_run_command_successful_execution() {
    MockLineEditor editor;
    MockCommandLine cl(editor);
    Commander commander(cl);
    MockCommand command;
    command.patterns = {"test arg"};
    vector<Command*> commands = { &command };
    commander.set_commands(commands);
    bool actual = commander.run_command(nullptr, "test value");
    assert(actual == true && "run_command should return true for valid command");
    assert(command.last_args.size() == 2 && "run_command should pass correct number of args");
    assert(command.last_args[0] == "test" && "run_command should pass command name");
    assert(command.last_args[1] == "value" && "run_command should pass argument");
}


TEST(test_Commander_is_exiting_initial_state);
TEST(test_Commander_is_exiting_after_exit);
TEST(test_Commander_is_exiting_after_commandline_exit);
TEST(test_Commander_get_command_line_ref_returns_reference);
TEST(test_Commander_set_commands_updates_patterns);
TEST(test_Commander_run_command_empty_input);
TEST(test_Commander_run_command_unknown_command);
TEST(test_Commander_run_command_invalid_arguments);
TEST(test_Commander_run_command_successful_execution);

#endif