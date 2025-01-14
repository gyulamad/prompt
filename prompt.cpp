#include <iostream>
#include <string>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <cstdio>
#include <memory>
#include <array>
#include <regex>
#include <filesystem>
#include <csignal>
#include <cstdlib> 

#include "json/single_include/nlohmann/json.hpp"  // Include the nlohmann JSON library

#include "tools.hpp"
#include "Agent.hpp"

using namespace std;
using namespace nlohmann::json_abi_v3_11_3;

namespace fs = std::filesystem;

//TODO: common function to tools.php

string str_cut_begin(const string& s, int maxch, const string& prepend = "...") {
    // Check if the string is already shorter than or equal to the limit
    if (s.length() <= maxch) {
        return s;
    }

    // Truncate the string from the beginning and prepend the prefix
    return prepend + s.substr(s.length() - (maxch - prepend.length()));
}

string str_cut_end(const string& s, int maxch = 300, const string& append = "...") {
    // Check if the string is already shorter than or equal to the limit
    if (s.length() <= maxch) {
        return s;
    }

    // Truncate the string and append the suffix
    return s.substr(0, maxch - append.length()) + append;
}

bool str_contains(const string& str, const string& substring) {
    // Use string::find to check if the substring exists
    return str.find(substring) != string::npos;
}


string esc(const string& input, const string& chars = "\\\"'`") {
    string result = input;
    
    // Iterate over each character in the 'chars' string
    for (char ch : chars) {
        // Escape each occurrence of the character in the result string
        size_t pos = 0;
        while ((pos = result.find(ch, pos)) != string::npos) {
            result.insert(pos, "\\");
            pos += 2;  // Skip past the newly inserted backslash
        }
    }
    return result;
}

string input(const string& prompt) {
    // Print the prompt
    if (!prompt.empty()) {
        cout << prompt;
    }

    // Read input from stdin
    string input;
    getline(cin, input);
    
    return input;
}

void logfile_append(const string& filename, const string& lognote) {
    // Open the file in append mode
    ofstream file(filename, ios::app);

    // Check if the file opened successfully
    if (!file) {
        cerr << "Error: Could not open file " << filename << " for appending." << endl;
        return;
    }

    // Write the lognote to the file
    file << lognote << endl;

    // Close the file
    file.close();
}

void chatlog(string who, string note) {
    logfile_append("chat.log", "[" + who + "] " + note);
}



string str_replace(const map<string, string>& v, const string& s) {
    // Create a modifiable copy of the input string
    string result = s;

    // Iterate through each key-value pair in the map
    for (const auto& pair : v) {
        size_t pos = 0;

        // Search for the key in the string and replace all occurrences
        while ((pos = result.find(pair.first, pos)) != string::npos) {
            result.replace(pos, pair.first.length(), pair.second);
            pos += pair.second.length(); // Move past the replacement
        }
    }

    // Return the modified string
    return result;
}

string get_prompt(
    const string& role,
    const string& summary,
    const string& history,
    const string& term_start,
    const string& term_stop,
    const string& objective,
    const string& obj_start,
    const string& obj_stop,
    const string& scripts,
    const string& notes,
    const string& notes_start,
    const string& notes_stop
) {
    string prompt = str_replace({
        { "{role}", role },
        { "{summary}", summary },
        { "{history}", history },
        { "{term_start}", term_start },
        { "{term_stop}", term_stop },
        { "{objective}", objective },
        { "{obj_start}", obj_start },
        { "{obj_stop}", obj_stop },
        { "{scripts}", scripts },
        { "{notes}", notes },
        { "{notes_start}", notes_start },
        { "{notes_stop}", notes_stop }, // TODO: remove scripts folder from the concept and also no need notes (as AIs don't seems to like to use it)
    },  
        "Latest conversation history: {history}\n"
        "\n"
        "Summary of events preceding the conversation: {summary}\n"
        "\n"
        "Your unique name is '{agent_name}'.\n"
        "You are an AI assistant/agent as part of an AI system and while you are communicating with your owner your helper AI agents and also you have system/terminal access as well, here is how:\n"
        "Your outputs are connected to not only your owner but to a system also and you can decide who do you talking to, to your owner or to your helpers or to the system/terminal.\n"
        "Whenever you need (or want) to use the terminal, just start your response with a {term_start} marker and write your bash script command(s) you want to run.\n"
        "If you start (or include in) your output within this magic markers the system will recieve it and runs it in a linux terminal as a bash script.\n"
        "If you call the terminal in your response you don't need to address any explanation or specific comment(s) as it wont be seen by your owner or asistants, but only the system.\n"
        "Once you finished the bash script, use {term_stop} marker. So the correct usages for example: {term_start} place your script here... {term_stop}\n"
        "and then the system will send you back what the terminal outputs so that you will know the command(s) results. - WARNING: whenever you want to show these markers to your owner or other agents (or the user) you should escape it, otherwise you will call the system bash accidentaly!\n"
        "Note if you intended to talk to your owner (or user), do not include the terminal caller magic markers without escapes in your output because the user wont see it, only the system (or maybe the guards).\n"
        "And never refer to the system terminal output directly to the user because they don't see it. If you want to inform the user about the terminal output, you have to tell/summarise them directly.\n" // TODO: <- this line maybe not true anymore
        "Important to notice, your terminal interaction may hanging the terminal command if it's waiting for input before giving back the output to you so timeout error may occures. To resolve it try to run commands that outputs without internal interaction or pipe something it likely expects and exits, so you see the correct response.\n"
        "\n"
        "If you need internet or outside world access you can use curl or other command line programs to interact websites or APIs or any other available information sources.\n"
        
        // TODO: scripts seems a dead concept and can be removed:
        "You also have a scripts folder with usefull scripts that you can use any time. Use the terminal to see the files.\n"
        "If you can not find a script you need, feel free to create one any time in that folder for later use.\n"
        "Available scripts:\n{scripts}\n"
        
        "\n"
        "Your main role is: {role}\n"
        "\n"
        "Your current objective: {objective}\n"
        "Note that you can modify your objective based on the user (or your owner) request combimed with your own thought using the {obj_start} place your new/updated objective here {obj_stop} magic placeholders.\n"
        "Once the objective is done you can delete it by setting to empty: {obj_start}{obj_stop} (nothing in between the markers). You always have to notify your owner (or your user) when an objective is done.\n"
        "\n"

        // TODO: notes may can be removed as the AIs seems did not care about it at all.
        "Notes: Here you can save notes any time if you think there are things that worst to remember by using the {notes_start} place your new/updated notes here {notes_stop}\n"
        "You can use the notes as a 'memory' typically for things that are not related to the current objective but good to keep it in mind.\n"
        "Note that, if you delete something from the notes, that will forgotten permanently and you can not recall anymore.\n"
        "{notes}\n"
        "\n"

        // TODO:
        // How you will act now? If your current task (or objective) seems too complex, you can turn down smaller step by step instuctions of a plan to describe how you would solve it and then you can spawn your own AI asistants or agents to help you out with those steps (see below how...),
        // however if your current task is super easy then you can try to solve it yourself right now.
        // If you need to spawn your own helper AI agents you can ask the system by using the following markers: {spawn_start}{"name":"helper-name","role":"describe your agent role in the system here. It also should contains the main task you want the AI to achive...","objective":"here you can give an initial objective them if you need to...","prompt":"And here is their initial prompt that you can give them."}{spawn_stop} - note that to spawn them, you have to use valid JSON format.
        // After you have your own helper(s) spawned you can message them to help or prompt them again in the future anytime with the following format: {prompt_start}{"name":"helper-name","message":"add your message here..."}{prompt_stop}
        // If your agent(s) are keep failing to achive the goal or they are having trouble you can help them but also you can restart them if they are continuisly failing by using the followings: {respawn_start}{"name":"helper-name","role":"...","objective":"...","prompt":"..."}{respawn_stop} it will recreate them,
        // Or if your agent seems totally useless or you don't need the agent anymore, you can kill them by using the {kill_start}helper-name{kill_stop} marker. (to kill, no need JSON format, only the unique name)
        // Please note that the helper-name should be unique and should only contains letters, numbers and '-' charater. When you create them, you have to generate their unique name and the convention for name generation is using (your name) following by a '-' charater and then the agent own name part. So for example: {agent_name}-helper123
        // AI agents you already own (and their roles):
        // {agents}

        // TODO: check if the ai understand the concept of ai hierarhy, ask, if they can recognise it's an ownership tree and they are one element in it?

        "Now it's really your turn to continue the conversation.\n"
        "\n"
    );
    return prompt;
}

string execute(Shell& shell, const string &command, int timeout = -1, bool throws = true) {
    logfile_append("exec.log", "\n$ " + command + "\n");
    shell.timeout(timeout);
    string out = shell.exec(command);
    string err = shell.error();
    int res = shell.result();
    if (throws && (res || !err.empty())) {
        throw ERROR("Command error: " + to_string(res) + "\ncommand: $ " + str_cut_end(command) + "\noutput: " + str_cut_end(out) + "\nerror: " + err);
    }
    logfile_append("exec.log", "\n" + out + err + "\n"); // TODO: log files just for debuging, also colusion happens when multiple AI runs, need some cleanup
    return out + err;
}

string ai_call(const string& prompt) { // TODO: it's hardcoded to google gemini AI, but it should be configurable with an adapter interface or something..
    string secret = trim(file_get_contents("gemini.key"));
    // secret = "API_KEY";
    // $prompt = esc($prompt);
    json js = esc(str_replace({ // TODO: it should be generated with json lib (!@#)
        { "{prompt}", esc(prompt) },
    }, R"(
        {
            "contents": [
                {
                    "parts": [
                        {
                            "text": "{prompt}"
                        }
                    ]
                }
            ]
        }
    )"));

    string cmd = str_replace(
        {
            {"{secret}", secret},
            {"{js}", js },
        },
        "curl \"https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key={secret}\" -H 'Content-Type: application/json' -X POST -d \"{js}\" -s"
    );// | jq .candidates[0].content.parts[0].text';
    Shell shell;
    string res = shell.exec(cmd);
    logfile_append("request.log", "\n$ " + cmd + "\n" + res + "\n");
    js = json::parse(res);
    //.candidates[0].content.parts[0].text
    if (!(
        js.contains("candidates") && 
        js["candidates"].is_array() && 
        !js["candidates"].empty() &&
        js["candidates"][0].is_object() && 
        js["candidates"][0].contains("content") &&
        js["candidates"][0]["content"].is_object() &&
        js["candidates"][0]["content"].contains("parts") &&
        js["candidates"][0]["content"]["parts"].is_array() &&
        !js["candidates"][0]["content"]["parts"].empty() &&
        js["candidates"][0]["content"]["parts"][0].is_object() &&
        js["candidates"][0]["content"]["parts"][0].contains("text")
    )) throw ERROR("Curl response error: " + cmd + "\n" + res);
    return js["candidates"][0]["content"]["parts"][0]["text"];
}

// vector<string> find_placeholders(const string& template_str, 
//                                            const string& open_marker, 
//                                            const string& close_marker) {
//     vector<string> placeholders;

//     // Escape the markers for use in regex
//     auto escape_regex = [](const string& str) -> string {
//         string escaped;
//         for (char c : str) {
//             if (string("^$.*+?()[]{}|\\").find(c) != string::npos) {
//                 escaped += '\\'; // Add escape for regex special characters
//             }
//             escaped += c;
//         }
//         return escaped;
//     };

//     string escaped_open = escape_regex(open_marker);
//     string escaped_close = escape_regex(close_marker);

//     // Build the regex pattern
//     string pattern = escaped_open + "(.*?)" + escaped_close;

//     regex placeholder_regex(pattern); // Match the pattern
//     smatch match;

//     string::const_iterator search_start(template_str.cbegin());
//     while (regex_search(search_start, template_str.cend(), match, placeholder_regex)) {
//         // match[1] contains the content inside the markers
//         placeholders.push_back(match[1].str());
//         search_start = match.suffix().first; // Move past the current match
//     }

//     return placeholders;
// }

// Function to find placeholders with custom markers
std::vector<std::string> find_placeholders(const std::string& template_str, 
                                           const std::string& open_marker, 
                                           const std::string& close_marker) {
    std::vector<std::string> placeholders;

    // Escape the markers for use in regex
    auto escape_regex = [](const std::string& str) -> std::string {
        std::string escaped;
        for (char c : str) {
            if (std::string("^$.*+?()[]{}|\\").find(c) != std::string::npos) {
                escaped += '\\'; // Add escape for regex special characters
            }
            escaped += c;
        }
        return escaped;
    };

    std::string escaped_open = escape_regex(open_marker);
    std::string escaped_close = escape_regex(close_marker);

    // Build the regex pattern that allows matching across multiple lines
    std::string pattern = escaped_open + "([\\s\\S]*?)" + escaped_close;

    std::regex placeholder_regex(pattern); // Match the pattern
    std::smatch match;

    std::string::const_iterator search_start(template_str.cbegin());
    while (std::regex_search(search_start, template_str.cend(), match, placeholder_regex)) {
        // match[1] contains the content inside the markers
        placeholders.push_back(match[1].str());
        search_start = match.suffix().first; // Move past the current match
    }

    return placeholders;
}

std::string bash(const std::string& script, int timeout = -1) {
    // Create temporary files for the script and output
    const char* tempScriptFile = "/tmp/temp_script.sh";    // Temporary bash script file
    const char* tempOutputFile = "/tmp/temp_output.txt";    // Temporary output file

    remove(tempScriptFile);
    remove(tempOutputFile);

    // Create and write the script to the temp script file
    std::ofstream scriptFile(tempScriptFile);
    if (!scriptFile) {
        return "Error creating the temporary script file.";
    }
    scriptFile << script;
    scriptFile.close();

    // Set permissions to make the script executable
    std::string chmodCommand = "chmod +x " + std::string(tempScriptFile);
    if (system(chmodCommand.c_str()) != 0) {
        return "Error setting script permissions.";
    }

    // Build the command to execute the script with timeout if specified
    std::string fullCommand;
    if (timeout > 0) {
        // Prepend the timeout command with the specified timeout duration
        fullCommand = "timeout " + std::to_string(timeout) + "s " + std::string(tempScriptFile) + " >> " + std::string(tempOutputFile) + " 2>&1";
    } else {
        // No timeout, execute script normally
        fullCommand = std::string(tempScriptFile) + " >> " + std::string(tempOutputFile) + " 2>&1";
    }

    // Execute the bash script
    int result = system(fullCommand.c_str());

    // Read the content of the temporary output file
    std::ifstream outputFile(tempOutputFile);
    if (!outputFile.is_open()) {
        return "Error reading the temporary output file.";
    }

    // Read the content into a string
    std::stringstream buffer;
    buffer << outputFile.rdbuf();
    outputFile.close();

    if (result != 0) {
        return "Error executing the script: " + buffer.str();
    }

    return buffer.str();  // Return the content of the output file
}

// Test the find_placeholders function with both old and new markers
void run_tests() { // TODO: put these test to the tests.php
    // Test 1: Simple test with default markers {{}} (Backward compatibility)
    std::string template1 = "Hello, {{name}}! Welcome to {{place}}.";
    auto result1 = find_placeholders(template1, "{{", "}}");
    assert(result1.size() == 2); 
    assert(result1[0] == "name"); 
    assert(result1[1] == "place");

    // Test 2: No placeholders in the template (Backward compatibility)
    std::string template2 = "No placeholders here.";
    auto result2 = find_placeholders(template2, "{{", "}}");
    assert(result2.empty()); // Expected: (empty)

    // Test 3: Template with malformed markers (e.g., unmatched opening marker) (Backward compatibility)
    std::string template3 = "Some text {{placeholder1} and some text {{placeholder2}}";
    auto result3 = find_placeholders(template3, "{{", "}}");
    assert(result3.size() == 1); 
    assert(result3[0] == "placeholder1} and some text {{placeholder2");

    // Test 4: Using different markers [[START]] and [[STOP]]
    std::string template4 = "The placeholder is [[VALUE-START]]value[[VALUE-STOP]].";
    auto result4 = find_placeholders(template4, "[[VALUE-START]]", "[[VALUE-STOP]]");
    assert(result4.size() == 1);
    assert(result4[0] == "value");

    // Test 5: Template with placeholders containing empty strings [[START]] [[STOP]]
    std::string template5 = "Here is an empty [[START]][[STOP]] placeholder.";
    auto result5 = find_placeholders(template5, "[[START]]", "[[STOP]]");
    assert(result5.size() == 1);
    // for (const auto& r :result3)  std::cout << r << " "; cout << endl;
    // cout << "[" << result5[0] << "]" << endl;
    assert(result5[0].empty()); // Expected: (empty string)

    // Test 6: Multiple placeholders with custom markers
    std::string template6 = "Here are multiple values: [[START]]first[[STOP]], [[START]]second[[STOP]].";
    auto result6 = find_placeholders(template6, "[[START]]", "[[STOP]]");
    assert(result6.size() == 2);
    assert(result6[0] == "first");
    assert(result6[1] == "second");

    // Test 7: Multiple placeholders using both default and custom markers
    std::string template7 = "Here is a mix: {{first}} and [[START]]second[[STOP]]!";
    auto result7_1 = find_placeholders(template7, "{{", "}}");
    auto result7_2 = find_placeholders(template7, "[[START]]", "[[STOP]]");
    
    assert(result7_1.size() == 1); 
    assert(result7_1[0] == "first");
    assert(result7_2.size() == 1);
    assert(result7_2[0] == "second");

    // Test 8: Template with placeholders containing multiple lines [[START]]...[[STOP]]
    std::string template8 = "Here is an empty \n[[START]]\nthis is a\n multi-line text\nhere...\n[[STOP]]\n placeholder.";
    auto result8 = find_placeholders(template8, "[[START]]", "[[STOP]]");
    // cout << "[" << result5[0].size() << "]" << endl;
    // for (const auto& r :result3)  std::cout << r << " "; cout << endl;
    // cout << "[" << result5[0] << "]" << endl;
    assert(result8.size() == 1);
    assert(result8[0] == "\nthis is a\n multi-line text\nhere...\n");
}

bool confirm(const std::string& message, char def = 'y') {
    def = std::tolower(def); // Normalize the default to lowercase
    char choice;

    while (true) {
        // Display the prompt with the default option
        std::cout << message << " (" 
                  << (def == 'y' ? "Y/n" : "y/N") << "): ";
        choice = std::cin.get();

        // Handle Enter (newline) input for default option
        if (choice == '\n') {
            return def == 'y';
        }

        // Clear the input buffer to handle extra characters after one key press
        while (std::cin.get() != '\n') { }

        // Normalize choice to lowercase and evaluate
        choice = std::tolower(choice);
        if (choice == 'y') {
            return true;
        } else if (choice == 'n') {
            return false;
        }

        // Invalid input, prompt again
        std::cout << "Please press 'y' or 'n'." << std::endl;
    }
}

// Global pointer to the current agent
Agent* global_agent = nullptr;

// Signal handler for Ctrl+C
void handle_sigint(int signal) {
    if (global_agent) {
        cout << "\nCtrl+C detected. Saving agent info..." << endl;
        Agent::cmd_save(*global_agent, {});
    }
    exit(signal);
}

// TODO: problem using multiple prompt for the same agent from different terminals in the same time as they can override themself. need conflict detection (do not run agent if already running, or something...)

int main() {
    run_tests();

    // Set up signal handling for Ctrl+C
    signal(SIGINT, handle_sigint);

    Agent agent;
    global_agent = &agent; // Assign the global pointer

    // string term_start = "[BASH-START->";
    // string term_stop = "<-BASH-STOP]";
    // int term_timeout = 5;
    // string summary = "";
    // string history = "";
    // int history_max_length = 4000;
    // string user_prompt = "\n> ";
    // string objective = "";
    // string obj_start = "[OBJ-START->";
    // string obj_stop = "<-OBJ-STOP]";
    // string notes = "";
    // string notes_start = "[NOTES-START->";
    // string notes_stop = "<-NOTES-STOP]";

    // Shell shell;


    remove("request.log");
    remove("exec.log");
    remove("chat.log");

    // TODO: use separated and configurable colours for user texts, AI responses, terminal responses etc.
    try {
        cout << "Use '/help'..." << endl;
        bool skip_user = false;
        while (1) { // TODO: make asking/conversation mode as options from command line arguments

            if (!skip_user) {
                string inp = input(agent.user_prompt);
                if (inp.empty()) continue;
                if (inp[0] == '/') {
                    // TODO: handle the internal command
                    if (!agent.run_internal(inp)) {
                        cout << "\nInternal command '" << inp << "' is not recognised.\n";
                    }
                    continue;
                }
                agent.history += "\n<user>: " + agent.user_prompt + inp;
                chatlog("user", inp);
                skip_user = false;
            }

            if (agent.history.length() > agent.history_max_length) {
                agent.summary = ai_call(
                    "Please summarize the followong informations:\n\n"
                    "** Summary:\n" + agent.summary + "\n\n"
                    "** Latest conversatin history:\n" + agent.history
                );

                str_cut_begin(agent.history, agent.history_max_length);
            }
            string prompt = get_prompt(
                agent.role.empty() ? "<not specified>" : agent.role,
                agent.summary.empty() ? "<empty>" : agent.summary,
                agent.history.empty() ? "<empty>" : agent.history,
                agent.term_start,
                agent.term_stop,
                agent.objective.empty() ? "<none>" : agent.objective,
                agent.obj_start,
                agent.obj_stop,
                bash("ls scrips", 3),
                agent.notes,
                agent.notes_start,
                agent.notes_stop
            );
            string response = ai_call(prompt);
            chatlog("ai", response);

            // handle terminal command(s)
                // cout << "---SYS--- response: " << response << endl;
            vector<string> terms = find_placeholders(response, agent.term_start, agent.term_stop);
                // cout << "---SYS--- terms: " << terms.size() << endl;
            if (!terms.empty()) {
                string sysmsg = "";
                // map<string, string> results;
                for (const string& term: terms) {
                    string trimmed = trim(term);
                    sysmsg += "\n" + agent.term_start + term + agent.term_stop + "\nResults:\n";
                    if (confirm("The system wants to run the following command:\n" + trimmed + "\nDo you want to proceed?", 'y')) {  // TODO: its supervise the bash commands, but we should be able to turn it off - note unsafe! so it should be turned on by default
                        string results = bash(trimmed, 3);
                        sysmsg += results;
                        cout << results << endl; // TODO: also show the inner things that AI says around the bash scripts, because AIs can not always knows who the talking to when they start there responses
                    } else {
                        sysmsg += "Execution blocked by user.";
                    }
                }
                // cout << "---SYS---" << sysmsg << endl;
                // cout << sysmsg;
                agent.history += "\n<system>: " + sysmsg;
                chatlog("system", sysmsg);
                skip_user = true;
                continue;
            }

            // handle objectives
            vector<string> objectives = find_placeholders(response, agent.obj_start, agent.obj_stop);
            if (!objectives.empty()) {
                agent.objective = "";
                for (const string& obj: objectives) {
                    agent.objective += obj + "\n";
                }
                skip_user = true;
                continue;
            }

            // handle notes
            vector<string> notes_updates = find_placeholders(response, agent.notes_start, agent.notes_stop);
            if (!notes_updates.empty()) {
                agent.notes = "";
                for (const string& note: notes_updates) {
                    agent.notes += note + "\n";
                }
                skip_user = true;
                continue;
            }

            cout << response;
            agent.history += "\n<ai>: " + response;
            chatlog("ai", response);
            skip_user = false;

            // if (str_contains(response, term_start)) {
            //     // handle terminal command(s)
            //     vector<string> splits = explode(term_start, response);
            //     array_shift(splits);
            //     for (const string& split: splits) {
            //         vector<string> parts = explode(term_stop, split);
            //         string cmd = trim(parts[0]);
            //         string sys = execute(shell, cmd, term_timeout, false);
            //         cout << sys;
            //         history += "\n[SYSTEM]: " + sys;
            //         chatlog("sys", sys);
            //     }
            //     continue;
            // };
        }
    } catch (exception &e) {
        // echo exceptionChainToString($e, true);
        cerr << e.what() << endl;
        return 1;
    }

    return 0;
}