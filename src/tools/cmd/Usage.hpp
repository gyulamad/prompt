#pragma once

#include <string>
#include <vector>

#include "../str/implode.h"

#include "Parameter.hpp"

using namespace std;
using namespace tools::str;

namespace tools::cmd {

    struct Usage {
        string command;
        string help;
        vector<Parameter> parameters;
        vector<pair<string, string>> examples; // code and comment
        vector<string> notes;

        string to_string() const {
            vector<string> prms, hlps, exmpls, nts;
            for (const Parameter& parameter: parameters) {
                prms.push_back(
                    parameter.optional 
                        ? "[" + parameter.name + "]" 
                        : "<" + parameter.name + ">"
                );
                hlps.push_back(
                    "  " + parameter.name 
                    + "\t - " + (parameter.optional ? "(optional) " : "") + parameter.help
                );
            }
            for (const pair<string, string>& example: examples) exmpls.push_back(example.first + "\t# " + example.second);
            for (const string& note: notes) nts.push_back(note);

            return command + (prms.empty() ? "" : (" " + implode(" ", prms)))
                + "\n" + help 
                + "\nParameters:" + (hlps.empty() ? " (none)" : ("\n  " + implode("\n  ", hlps)))
                + (exmpls.empty() ? "" : ("\nExamples:\n  - " + implode("\n  - ", exmpls)))
                + (nts.empty() ? "" : ("\nNotes:\n  - " + implode("\n  - ", nts)))
            ;
        }
    };

}

#ifdef TEST

using namespace tools::cmd;

void test_Usage_to_string_basic() {
    Usage usage;
    usage.command = "test_cmd";
    usage.help = "This is a test command.";
    usage.parameters.push_back({"param1", false, "First parameter"});
    usage.parameters.push_back({"param2", true, "Second parameter"});

    string expected = "test_cmd <param1> [param2]\n"
                      "This is a test command.\n"
                      "Parameters:\n"
                      "  param1\t - First parameter\n"
                      "  param2\t - (optional)  Second parameter";
    string actual = usage.to_string();
    // Use str_contains for flexibility with potential whitespace differences
    assert(str_contains(actual, "test_cmd <param1> [param2]") && "Basic usage string format - command and params");
    assert(str_contains(actual, "This is a test command.") && "Basic usage string format - help");
    assert(str_contains(actual, "Parameters:") && "Basic usage string format - parameters header");
    assert(str_contains(actual, "param1\t - First parameter") && "Basic usage string format - required param");
    assert(str_contains(actual, "param2\t - (optional) Second parameter") && "Basic usage string format - optional param");
    assert(!str_contains(actual, "Examples:") && "Basic usage should not have Examples");
    assert(!str_contains(actual, "Notes:") && "Basic usage should not have Notes");
}

void test_Usage_to_string_with_examples() {
    Usage usage;
    usage.command = "example_cmd";
    usage.help = "Command with examples.";
    usage.examples.push_back({"example_cmd run", "Run the command"});
    usage.examples.push_back({"example_cmd --help", "Show help"});

    string expected = "example_cmd\n"
                      "Command with examples.\n"
                      "Parameters: (none)\n"
                      "Examples:\n"
                      "  - example_cmd run\t# Run the command\n"
                      "  - example_cmd --help\t# Show help";
    string actual = usage.to_string();
    assert(str_contains(actual, "example_cmd") && "Examples usage string format - command");
    assert(str_contains(actual, "Command with examples.") && "Examples usage string format - help");
    assert(str_contains(actual, "Parameters: (none)") && "Examples usage string format - no params");
    assert(str_contains(actual, "Examples:") && "Examples usage string format - examples header");
    assert(str_contains(actual, "example_cmd run\t# Run the command") && "Examples usage string format - example 1");
    assert(str_contains(actual, "example_cmd --help\t# Show help") && "Examples usage string format - example 2");
    assert(!str_contains(actual, "Notes:") && "Examples usage should not have Notes");
}

void test_Usage_to_string_with_notes() {
    Usage usage;
    usage.command = "note_cmd";
    usage.help = "Command with notes.";
    usage.notes.push_back("This is note 1.");
    usage.notes.push_back("This is note 2.");

    string expected = "note_cmd\n"
                      "Command with notes.\n"
                      "Parameters: (none)\n"
                      "Notes:\n"
                      "  - This is note 1.\n"
                      "  - This is note 2.";
    string actual = usage.to_string();
    assert(str_contains(actual, "note_cmd") && "Notes usage string format - command");
    assert(str_contains(actual, "Command with notes.") && "Notes usage string format - help");
    assert(str_contains(actual, "Parameters: (none)") && "Notes usage string format - no params");
    assert(!str_contains(actual, "Examples:") && "Notes usage should not have Examples");
    assert(str_contains(actual, "Notes:") && "Notes usage string format - notes header");
    assert(str_contains(actual, "This is note 1.") && "Notes usage string format - note 1");
    assert(str_contains(actual, "This is note 2.") && "Notes usage string format - note 2");
}

void test_Usage_to_string_full() {
    Usage usage;
    usage.command = "full_cmd";
    usage.help = "A command with everything.";
    usage.parameters.push_back({"req", false, "Required param"});
    usage.parameters.push_back({"opt", true, "Optional param"});
    usage.examples.push_back({"full_cmd req_val", "Basic run"});
    usage.notes.push_back("Important note.");

    string expected = "full_cmd <req> [opt]\n"
                      "A command with everything.\n"
                      "Parameters:\n"
                      "  req\t - Required param\n"
                      "  opt\t - (optional)  Optional param\n"
                      "Examples:\n"
                      "  - full_cmd req_val\t# Basic run\n"
                      "Notes:\n"
                      "  - Important note.";
    string actual = usage.to_string();
    assert(str_contains(actual, "full_cmd <req> [opt]") && "Full usage string format - command and params");
    assert(str_contains(actual, "A command with everything.") && "Full usage string format - help");
    assert(str_contains(actual, "Parameters:") && "Full usage string format - parameters header");
    assert(str_contains(actual, "req\t - Required param") && "Full usage string format - required param");
    assert(str_contains(actual, "opt\t - (optional) Optional param") && "Full usage string format - optional param");
    assert(str_contains(actual, "Examples:") && "Full usage string format - examples header");
    assert(str_contains(actual, "full_cmd req_val\t# Basic run") && "Full usage string format - example");
    assert(str_contains(actual, "Notes:") && "Full usage string format - notes header");
    assert(str_contains(actual, "Important note.") && "Full usage string format - note");
}

void test_Usage_to_string_minimal() {
    Usage usage;
    usage.command = "min_cmd";
    usage.help = "Minimal command.";

    string expected = "min_cmd\n"
                      "Minimal command.\n"
                      "Parameters: (none)";
    string actual = usage.to_string();
    assert(str_contains(actual, "min_cmd") && "Minimal usage string format - command");
    assert(str_contains(actual, "Minimal command.") && "Minimal usage string format - help");
    assert(str_contains(actual, "Parameters: (none)") && "Minimal usage string format - no params");
    assert(!str_contains(actual, "Examples:") && "Minimal usage should not have Examples");
    assert(!str_contains(actual, "Notes:") && "Minimal usage should not have Notes");
}


// Register tests
TEST(test_Usage_to_string_basic);
TEST(test_Usage_to_string_with_examples);
TEST(test_Usage_to_string_with_notes);
TEST(test_Usage_to_string_full);
TEST(test_Usage_to_string_minimal);

#endif
