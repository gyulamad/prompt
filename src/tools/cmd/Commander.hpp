#pragma once

#include "Command.hpp"
#include "CommandLine.hpp"
#include "CompletionMatcher.hpp"

using namespace std;
using namespace tools;

namespace tools::cmd {

    class Commander {
    private:
        CommandLine command_line;
        CompletionMatcher cmatcher;
        bool exiting = false;
        vector<void*> commands;
    public:
        Commander(const CommandLine& command_line): command_line(command_line) {}

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
            for (const void* command: commands)
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
            for (void* command_void: commands) {
                Command* command = (Command*)command_void;
                for (const string& command_pattern: command->get_patterns()) {
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
            if (!command_found) cout << "Command not found: " << input_parts[0] << endl;
            else if (!command_arguments_matches) cout << "Invalid argument(s)." << endl;
            return command_arguments_matches;
        }
    };

}
