#pragma once

#include <string>
#include <map>
#include <vector>

#include "tools.hpp"
#include <functional>
#include "Shell.hpp"

using namespace std;
namespace fs = std::filesystem;

class Agent {
public:
    string term_start = "[BASH-START->";
    string term_stop = "<-BASH-STOP]";
    int term_timeout = 5;
    string summary = "";
    string history = "";
    int history_max_length = 1000;
    string user_prompt = "\n> ";
    string objective = "";
    string obj_start = "[OBJ-START->";
    string obj_stop = "<-OBJ-STOP]";
    string notes = "";
    string notes_start = "[NOTES-START->";
    string notes_stop = "<-NOTES-STOP]";
    string current_file = ""; // Tracks the current file name

    Shell shell;
    const string agents_folder = "agents/";

    // Constructor: Load the default agent file
    Agent(const string& file = "default") {
        fs::create_directory(agents_folder); // Ensure the `agents/` folder exists
        if (!cmd_load(*this, {file})) {
            cout << "No agent file loaded. Starting with a new agent." << endl;
        }
    }

    // Command descriptions for /help
    static map<string, string> command_descriptions() {
        return {
            {"/show", "Displays information about the agent (e.g., summary, history, objective, notes)."},
            {"/save", "Saves all agent info into a file in the 'agents/' folder."},
            {"/load", "Loads all agent info from a file in the 'agents/' folder."},
            {"/list", "Lists all saved agent files in the 'agents/' folder."},
            {"/help", "Lists all available commands and their descriptions."},
            {"/exit", "Saves the current agent and exits the program."}
        };
    }

    // Static command functions
    static bool cmd_show(Agent& agent, vector<string> params) {
        const map<string, string> options = {
            {"summary", agent.summary},
            {"history", agent.history},
            {"objective", agent.objective},
            {"notes", agent.notes},
            {
                "all",
                "summary: " + agent.summary + "\n\n" +
                "objective: " + agent.objective + "\n\n" +
                "notes: " + agent.notes + "\n\n"
            }
        };

        if (params.empty()) {
            cout << "Available options:" << endl;
            for (const auto& pair : options) {
                cout << " - " << pair.first << endl;
            }
        } else {
            string key = params[0];
            if (options.find(key) != options.end()) {
                cout << options.at(key) << endl;
            } else {
                cout << "Unknown option: " << key << endl;
            }
        }
        return true;
    }

    static bool cmd_save(Agent& agent, vector<string> params) {
        string filename;
        if (!params.empty()) {
            filename = agent.agents_folder + params[0];
            agent.current_file = params[0];
        } else if (!agent.current_file.empty()) {
            filename = agent.agents_folder + agent.current_file;
        } else {
            filename = agent.agents_folder + "default";
            agent.current_file = "default";
        }

        fs::create_directory(agent.agents_folder);

        ofstream file(filename);
        if (!file) {
            cout << "Error: Unable to open file for writing." << endl;
            return false;
        }

        file << "summary: " << agent.summary << endl;
        file << "history: " << agent.history << endl;
        file << "objective: " << agent.objective << endl;
        file << "notes: " << agent.notes << endl;

        file.close();
        cout << "Agent info saved to " << filename << endl;
        return true;
    }

    static bool cmd_load(Agent& agent, vector<string> params) {
        string filename = agent.agents_folder + (params.empty() ? "default" : params[0]);
        ifstream file(filename);
        if (!file) {
            cout << "Error: Unable to open file for reading (" << filename << ")." << endl;
            return false;
        }

        string line, key, value;
        while (getline(file, line)) {
            size_t delimiter_pos = line.find(": ");
            if (delimiter_pos == string::npos) continue;

            key = line.substr(0, delimiter_pos);
            value = line.substr(delimiter_pos + 2);

            if (key == "summary") agent.summary = value;
            else if (key == "history") agent.history = value;
            else if (key == "objective") agent.objective = value;
            else if (key == "notes") agent.notes = value;
        }

        file.close();
        agent.current_file = params.empty() ? "default" : params[0];
        cout << "Agent info loaded from " << filename << endl;
        return true;
    }

    static bool cmd_list(Agent& agent, vector<string> params) {
        cout << "Listing all saved agent files in the 'agents/' folder:" << endl;
        fs::create_directory(agent.agents_folder);

        for (const auto& entry : fs::directory_iterator(agent.agents_folder)) {
            cout << " - " << entry.path().filename().string() << endl;
        }
        return true;
    }

    static bool cmd_help(Agent& agent, vector<string> params) {
        auto descriptions = command_descriptions();
        cout << "Available commands:" << endl;
        for (const auto& pair : descriptions) {
            cout << " - " << pair.first << ": " << pair.second << endl;
        }
        return true;
    }

    static bool cmd_exit(Agent& agent, vector<string> params) {
        cout << "Saving current agent and exiting..." << endl;
        cmd_save(agent, {});
        exit(0);
    }

    bool run_internal(const string& command) {
        vector<string> words = explode(command, " ");
        string func = array_shift(words);

        const map<string, function<bool(Agent&, vector<string>)>> funcs = {
            {"/show", cmd_show},
            {"/save", cmd_save},
            {"/load", cmd_load},
            {"/list", cmd_list},
            {"/help", cmd_help},
            {"/exit", cmd_exit},
        };

        auto it = funcs.find(func);
        if (it != funcs.end()) {
            return it->second(*this, words);
        }

        cout << "Unknown command: " << func << endl;
        cout << "Use /help to find out more..." << endl;
        return false;
    }
};
