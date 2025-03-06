#pragma once

#include "Command.hpp"
#include "CommandLine.hpp"
#include "CompletionMatcher.hpp"

using namespace std;
using namespace tools;

namespace tools::cmd {

    class Commander {
    private:
        CommandLine command_line; // Still owns the CommandLine
        CompletionMatcher cmatcher;
        bool exiting = false;
        vector<void*> commands;

    public:
        // Updated constructor: Take CommandLine by value and move it
        Commander(CommandLine command_line) : command_line(move(command_line)) {}

        virtual ~Commander() {}

        CommandLine& get_command_line_ref() {
            return command_line;
        }

        CompletionMatcher& get_cmatcher_ref() {
            return cmatcher;
        }

        bool is_exiting() const {
            return exiting || command_line.is_exited();
        }
        
        void set_commands(const vector<void*>& commands) {
            this->commands = commands;
            
            // also update command patterns:
            cmatcher.command_patterns = {};
            for (const void* command : commands)
                cmatcher.command_patterns = array_merge(
                    cmatcher.command_patterns, 
                    ((Command*)command)->get_patterns()
                );
            command_line.set_completion_matcher(cmatcher);
        }

        vector<void*> get_commands_ref() {
            return commands;
        }

        const CompletionMatcher& get_cmatcher_ref() const {
            return cmatcher;
        }

        void exit() {            
            exiting = true;
        }

        bool run_command(void* user_context, const string& input) {
            if (input.empty()) return false;
            
            bool trlspc;
            vector<string> input_parts = array_filter(cmatcher.parse_input(input, trlspc, false));

            bool command_found = false;
            bool command_arguments_matches = false;
            for (void* command_void : commands) {
                Command* command = (Command*)command_void;
                for (const string& command_pattern : command->get_patterns()) {
                    vector<string> command_pattern_parts = array_filter(explode(" ", command_pattern));
                    if (input_parts[0] == command_pattern_parts[0]) {
                        command_found = true;
                        if (input_parts.size() == command_pattern_parts.size()) {
                            command_arguments_matches = true;
                            cout << command->run(user_context, input_parts) << endl;
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

#include "tests/MockCommand.hpp"

void test_Commander_is_exiting_initial_state() {
    auto commander = create_commander();
    bool actual = commander->is_exiting();
    assert(actual == false && "Commander should not be exiting initially");
}

void test_Commander_is_exiting_after_exit() {
    auto commander = create_commander();
    commander->exit();
    bool actual = commander->is_exiting();
    assert(actual == true && "Commander should be exiting after exit() is called");
}

void test_Commander_is_exiting_after_commandline_exit() {
    auto mock_editor = make_unique<MockLineEditor>();
    mock_editor->should_exit = true;
    auto cl = make_unique<CommandLine>(move(mock_editor));
    Commander commander(move(*cl)); // Move the CommandLine into Commander
    commander.get_command_line_ref().readln(); // Call readln() on the Commander's CommandLine
    bool actual = commander.is_exiting();
    assert(actual == true && "Commander should be exiting after CommandLine exits");
}

void test_Commander_get_command_line_ref_returns_reference() {
    auto cl = make_unique<MockCommandLine>();
    Commander commander(move(*cl));
    CommandLine& actual = commander.get_command_line_ref();
    // Modify the CommandLine via the reference
    actual.set_prompt("modified> ");
    // Check if the modification is reflected in Commander's CommandLine
    string prompt_from_commander = commander.get_command_line_ref().get_prompt();
    assert(prompt_from_commander == "modified> " && 
           "get_command_line_ref should return a reference that modifies the internal CommandLine");
}

void test_Commander_set_commands_updates_patterns() {
    auto commander = create_commander();
    auto command = new MockCommand();
    command->patterns = {"test cmd", "test {param}"};
    vector<void*> commands = {command};
    commander->set_commands(commands);
    const CompletionMatcher& cm = commander->get_cmatcher_ref();
    vector<string> actual = cm.command_patterns;
    assert(actual.size() == 2 && "set_commands should update command patterns");
    assert(actual[0] == "test cmd" && "set_commands should include first pattern");
    assert(actual[1] == "test {param}" && "set_commands should include second pattern");
    delete command;
}

void test_Commander_run_command_empty_input() {
    auto commander = create_commander();
    bool actual = commander->run_command(nullptr, "");
    assert(actual == false && "run_command should return false for empty input");
}

void test_Commander_run_command_unknown_command() {
    string err = capture_stderr([&]() {
        auto commander = create_commander();
        auto command = new MockCommand();
        command->patterns = {"known"};
        vector<void*> commands = {command};
        commander->set_commands(commands);
        bool actual = commander->run_command(nullptr, "unknown");
        assert(actual == false && "run_command should return false for unknown command");
        delete command;
    });
    assert(str_contains(err, "Command not found: unknown") && "Should show the correct error");
}

void test_Commander_run_command_invalid_arguments() {
    string err = capture_stderr([&]() {
        auto commander = create_commander();
        auto command = new MockCommand();
        command->patterns = {"test arg1"};
        vector<void*> commands = {command};
        commander->set_commands(commands);
        bool actual = commander->run_command(nullptr, "test");
        assert(actual == false && "run_command should return false for invalid argument count");
        delete command;
    });
    assert(str_contains(err, "Invalid argument(s).") && "Should show the correct error");
}

void test_Commander_run_command_successful_execution() {
    string outp = capture_output([&]() {
        auto commander = create_commander();
        auto command = new MockCommand();
        command->patterns = {"test arg"};
        command->run_result = "Command executed";
        vector<void*> commands = {command};
        commander->set_commands(commands);
        bool actual = commander->run_command(nullptr, "test value");
        assert(actual == true && "run_command should return true for valid command");
        assert(command->last_args.size() == 2 && "run_command should pass correct number of args");
        assert(command->last_args[0] == "test" && "run_command should pass command name");
        assert(command->last_args[1] == "value" && "run_command should pass argument");
        delete command;
    });
    assert(str_contains(outp, "Command executed") && "Should show the correct output");
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