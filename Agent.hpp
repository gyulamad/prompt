#include <filesystem>
#pragma once

#include <string>
#include <map>
#include <vector>

#include "json/single_include/nlohmann/json.hpp"  // Include the nlohmann JSON library

#include "tools.hpp"
#include <functional>
// #include "Shell.hpp"

using namespace std;
using namespace nlohmann::json_abi_v3_11_3; // TODO: use the JSON wrapper

namespace fs = std::filesystem;

class Agent {
public:
    // TODO build up the ownership tree:
    string name; 
    string owner;

    string role = "";
    string term_start = "[BASH-START]:";
    string term_stop = ":[BASH-STOP]";
    int term_timeout = 5;
    string summary = "";
    string history = "";
    int history_max_length = 1000000;
    string user_prompt = "\n> ";
    string objective = "";
    string obj_start = "[OBJ-START]:";
    string obj_stop = ":[OBJ-STOP]";
    string current_file = ""; // Tracks the current file name

    // Shell shell;
    const string agents_folder = ".agents/";

    // Constructor: Load the default agent file
    Agent(const string& file = "default") {
        fs::create_directory(agents_folder); // Ensure the agents_folder exists
        if (!cmd_load(*this, {file})) {
            cout << "No agent file loaded. Starting with a new agent." << endl;
        }
    }

    // Command descriptions for /help
    const map<string, string> command_descriptions() {
        return {
            {"/show", "Displays information about the agent (e.g., summary, history, objective, etc.)."},
            {"/save", str_replace("{agents_folder}", agents_folder, "Saves all agent info into a file in the '{agents_folder}' folder.")},
            {"/load", str_replace("{agents_folder}", agents_folder, "Loads all agent info from a file in the '{agents_folder}' folder.")},
            {"/list", str_replace("{agents_folder}", agents_folder, "Lists all saved agent files in the '{agents_folder}' folder.")},
            {"/help", "Lists all available commands and their descriptions."},
            {"/exit", "Saves the current agent and exits the program."}
        };
    }

    // Static command functions
    static bool cmd_show(Agent& agent, vector<string> params) {
        const map<string, string> options = {
            {"role", agent.role},
            {"summary", agent.summary},
            {"history", agent.history},
            {"objective", agent.objective},
            {
                "all",
                "role: " + agent.role + "\n" +
                "summary: " + agent.summary + "\n" +
                "objective: " + agent.objective + "\n"
            }
        };

        if (params.empty()) {
            cout << "Current Agent: " << agent.current_file << endl;
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


    // Modified cmd_save function to use JSON
    static bool cmd_save(Agent& agent, vector<string> params) {
        std::string filename;
        if (!params.empty()) {
            filename = agent.agents_folder + params[0] + ".json";
            agent.current_file = params[0] + ".json";
        } else if (!agent.current_file.empty()) {
            filename = agent.agents_folder + agent.current_file;
        } else {
            filename = agent.agents_folder + "default.json";
            agent.current_file = "default.json";
        }

        // Ensure the agents folder exists
        fs::create_directory(agent.agents_folder);

        // Truncate history to history_max_length before saving
        if (agent.history.length() > agent.history_max_length) {
            agent.history = agent.history.substr(0, agent.history_max_length);
        }

        json j = {
            {"role", agent.role},
            {"summary", agent.summary},
            {"history", agent.history},
            {"objective", agent.objective},
        };

        try {
            file_put_contents(filename, j.dump(4)); // Pretty print JSON
            std::cout << "Agent info saved to " << filename << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cout << "Error saving file: " << e.what() << std::endl;
            return false;
        }
    }


    // Modified cmd_load function to use JSON
    static bool cmd_load(Agent& agent, vector<string> params) {
        std::string filename = agent.agents_folder + (params.empty() ? "default" : params[0]) + ".json";

        try {
            std::string content = file_get_contents(filename);
            json j = json::parse(content);

            agent.role = j.value("role", "");
            agent.summary = j.value("summary", "");
            agent.history = j.value("history", "");
            agent.objective = j.value("objective", "");

            // Truncate history to history_max_length after loading
            if (agent.history.length() > agent.history_max_length) {
                agent.history = agent.history.substr(0, agent.history_max_length);
            }

            agent.current_file = (params.empty() ? "default" : params[0]) + ".json";
            std::cout << "Agent info loaded from " << filename << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cout << "Error loading file: " << e.what() << std::endl;
            return false;
        }
    }


    static bool cmd_list(Agent& agent, vector<string> params) {
        cout << "Listing all saved agent files in the '" << agent.agents_folder << "' folder:" << endl;
        fs::create_directory(agent.agents_folder);

        for (const auto& entry : fs::directory_iterator(agent.agents_folder)) {
            cout << " - " << entry.path().filename().string() << endl;
        }
        return true;
    }

    static bool cmd_help(Agent& agent, vector<string> params) {
        auto descriptions = agent.command_descriptions();
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

    static bool cmd_role(Agent& agent, vector<string> params) {
        if (params.empty()) {
            // If no role is specified, display the current role or an error if none exists
            if (!agent.role.empty()) {
                std::cout << "Current role: " << agent.role << std::endl;
            } else {
                std::cout << "No role specified. Use '/role <role>' to set one." << std::endl;
            }
            return true;
        }

        // Concatenate the provided parameters into a single role string
        std::string new_role = implode(" ", params);
        agent.role = new_role;
        std::cout << "Role updated to: " << agent.role << std::endl;
        return true;
    }

    bool run_internal(const std::string& command) {
        // Trim the command to remove leading/trailing whitespace
        std::string trimmed_command = trim(command);

        if (trimmed_command.empty()) {
            std::cout << "Error: Command is empty or whitespace only." << std::endl;
            return false;
        }

        // Split the command into words
        std::vector<std::string> words = explode(" ", trimmed_command);
        std::string func = array_shift(words);

        // Command-function map
        const std::map<std::string, std::function<bool(Agent&, std::vector<std::string>)>> funcs = {
            {"/show", cmd_show},
            {"/save", cmd_save},
            {"/load", cmd_load},
            {"/list", cmd_list},
            {"/help", cmd_help},
            {"/exit", cmd_exit},
            {"/role", cmd_role},
        };

        // Find and execute the corresponding function
        auto it = funcs.find(func);
        if (it != funcs.end()) {
            return it->second(*this, words);
        }

        std::cout << "Unknown command: " << func << std::endl;
        std::cout << "Use /help to find out more..." << std::endl;
        return false;
    }

};
