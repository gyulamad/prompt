
#include "../src/tools/CommandLine.hpp"

using namespace std;
using namespace tools;


int main() {
    CompletionMatcher cmatcher;
    cmatcher.command_patterns = {
        "/help",
        "/exit",
        "/set volume {number}",
        "/set voice-input {switch}",
        "/cat {filename}",
        "/print {string}",
        "/print {string} {filename}"
    };

    // Simulate user input
    string user_input;
    cout << "Enter a command (press Tab for suggestions): " << endl;

    while (true) {
        cout << "> ";
        getline(cin, user_input);

        // Check if the user pressed Tab (simulated by an empty input)
        if (user_input.empty()) {
            cout << "Tab pressed. Generating completions..." << endl;
            user_input = ""; // Reset input for demonstration
        }

        // Get completions based on the current input
        vector<string> completions = cmatcher.get_completions(user_input);

        if (!completions.empty()) {
            cout << "Possible completions:" << endl;
            for (const string& completion : completions) {
                cout << "  " << completion << endl;
            }
        } else {
            cout << "No completions found." << endl;
        }

        // Simulate command execution (for demonstration purposes)
        if (user_input == "/exit") {
            cout << "Exiting..." << endl;
            break;
        } else if (user_input == "/help") {
            cout << "Available commands:" << endl;
            for (const string& pattern : cmatcher.command_patterns) {
                cout << "  " << pattern << endl;
            }
        } else if (user_input.find("/set volume") == 0) {
            cout << "Setting volume..." << endl;
        } else if (user_input.find("/set voice-input") == 0) {
            cout << "Setting voice input..." << endl;
        } else if (user_input.find("/cat") == 0) {
            cout << "Displaying file content..." << endl;
        } else {
            cout << "Unknown command. Type /help for a list of commands." << endl;
        }
    }

    return 0;
}