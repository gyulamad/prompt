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

#include "JSON.hpp"
#include "tools.hpp"
#include "Agent.hpp"

using namespace std;
using namespace nlohmann::json_abi_v3_11_3; // TODO: use JSON wrapper class instead

namespace fs = filesystem;


// TODO: implement linux prompt style input with history (up/down) and completion (tab) - and show choises if that's possible (tab + tab => show list of possible words)
// TODO: backspace only, arrows are doesn't work + can not add new line + copy paste with multiple lines cause problems:
string input(const string& prompt, const string& defval = "") {
    // Print the prompt
    if (!prompt.empty()) {
        cout << prompt;
    }

    // Read input from stdin
    string input;
    getline(cin, input);
    
    return input.empty() ? defval : input;
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



// TODO: refineing the 'general' prompt, ideas: https://chatgpt.com/c/6785c966-84fc-8008-9e7a-9fe48c3d750e
string get_prompt(
    const string& role,
    const string& summary,
    const string& history,
    const string& term_start,
    const string& term_stop,
    const string& objective,
    const string& obj_start,
    const string& obj_stop,
    const string& datetime,
    const string& pwd
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
        { "{datetime}", datetime },
        { "{pws}", pwd },
    },  
        "Latest conversation history: {history}\n"
        // TODO: add proper log format:
        // [LOG NOTE BEGIN] [YYYY-MM-DD HH:MM:SS.mmm] [sender] [recipien]  
        // ....
        // [LOG NOTE END]
        // Explain the AI that: The log history can be large an we don't know the context window size and that latest log note may overflows at the top of the latest history, so we have to summarize it time to time. if the AI doesn't see any log note that is summarization sent by the summarizer or it's already almost in the top few log notes then the AI should ask for summarization by using the {sum_request} token in it's output.
        "\n"
        "Summary of events preceding the conversation: {summary}\n"
        "\n"
        "Your unique name is '{agent_name}'.\n"
        "You are an AI assistant/agent as part of an AI system and while you are communicating with your owner your helper AI agents and also you have system/terminal access as well, here is how:\n"
        "Your outputs are connected to not only your owner but to a system also and you can decide who do you talking to, to your owner or to your helpers or to the system/terminal.\n"
        "Whenever you need (or want) to use the terminal, just start your response with a {term_start} marker and write your bash script command(s) you want to run.\n"
        //"Be aware and carful: The system are not intelligent, just an algorithm that reads your output and whenever it recognise the {term_start} marker, it starts execute your output as a bash script without understanding the context. To avoid accidental execution, use standard markdown code blocks (````bash ... ````) for all bash command and bash script examples.\n"
        "If you call the terminal in your response you don't need to address any explanation or specific comment(s) as it wont be seen by your owner or asistants, but only the system.\n"
        "If possible try to avoid commands, that waits for user interaction, however the commands are running with a timeout, so the output always will returned for you, even if the command waits for a user input."
        "Once you finished the bash script, use {term_stop} marker. So the correct usages for example: {term_start}ls -l /example/path {term_stop}\n"
        // "Runing multiple lines at once:\n"
        // "{term_start}echo \"Place your script here...\"\n"
        // "{term_start}echo \"It will be run line by line\"\n"
        // "{term_start}echo \"as a temporary bash script.\"\n"
        // "{term_stop}\n"
        "\n"
        "and then the system will send you back what the terminal outputs so that you will know the command(s) results. - WARNING: whenever you want to show these markers to your owner or other agents (or the user) you should escape it, otherwise you will call the system bash accidentaly!\n"
        "Note if you intended to talk to your owner (or user), do not include the terminal caller magic markers without escapes in your output because the user wont see it, only the system (or maybe the guards).\n"
        "And never refer to the system terminal output directly to the user because they don't see it. If you want to inform the user about the terminal output, you have to tell/summarise them directly.\n" // TODO: <- this line maybe not true anymore
        "Important to notice, your terminal interaction may hanging the terminal command if it's waiting for input before giving back the output to you so timeout error may occures. To resolve it try to run commands that outputs without internal interaction or pipe something it likely expects and exits, so you see the correct response.\n"
        "\n"
        "If you need internet or outside world access you can use curl or other command line programs to interact websites or APIs or any other available information sources.\n"
        "\n"
        "Your main role is: {role}\n"
        "\n"
        "Your current objective: {objective}\n"
        "Note that you can modify your objective based on the user (or your owner) request combimed with your own thought using the {obj_start} place your new/updated objective here {obj_stop} magic placeholders.\n"
        "Once the objective is done you can delete it by setting to empty: {obj_start}{obj_stop} (nothing in between the markers).\n"
        "\n"

        // TODO:
        // How you will act now? If your current task (or objective) seems too complex, you can turn down smaller step by step instuctions of a plan to describe how you would solve it and then you can spawn your own AI asistants or agents to help you out with those steps (see below how...),
        // however if your current task is super easy then you can try to solve it yourself right now.
        // If you need to spawn your own helper AI agents you can ask the system by using the following markers: {spawn_start}{"name":"helper-name","role":"describe your agent role in the system here. It also should contains the main task you want the AI to achive...","objective":"here you can give an initial objective them if you need to...","prompt":"And here is their initial prompt that you can give them."}{spawn_stop} - note that to spawn them, you have to use valid JSON format.
        // After you have your own helper(s) spawned you can message them to help or prompt them again in the future anytime with the following format: {prompt_start}{"name":"helper-name","message":"add your message here..."}{prompt_stop}
        // If your agent(s) are keep failing to achive the goal or they are having trouble you can help them but also you can restart them if they are continuisly failing by using the followings: {respawn_start}{"name":"helper-name","role":"...","objective":"...","prompt":"..."}{respawn_stop} it will recreate them,
        // Or if your agent seems totally useless or you don't need the agent anymore, you can kill them by using the {kill_start}helper-name{kill_stop} marker. (to kill, no need JSON format, only the unique name)
        // Please note that the helper-name should be unique and should only contains letters, numbers and '-' charater. When you create them, you have to generate their unique name and the convention for name generation is using (your name) following by a '-' charater and then the agent own name part. So for example: {agent_name}-helper123
        // Note: if you keep encounter into an implossible issue that you can not solve even with the help of your asistants AI agents (for example a system error or permission issues) and you have to give up on your objective you can do so but you have to inform your owner or user.
        // AI agents you already own (and their roles):
        // {agents}

        // TODO: check if the ai understand the concept of ai hierarhy, ask, if they can recognise it's an ownership tree and they are one element in it?
        "Current date and time: {datetime}\b"
        "Current working directory (pwd): {pwd}\n"
        "Now it's really your turn to continue the conversation.\n"
        "\n"
    );
    return prompt;
}

// string execute(Shell& shell, const string &command, int timeout = -1, bool throws = true) {
//     logfile_append("exec.log", "\n$ " + command + "\n");
//     shell.timeout(timeout); // TODO: using Shell class is now deprecated, use bash() tool from tools.hpp instead
//     string out = shell.exec(command);
//     string err = shell.error();
//     int res = shell.result();
//     if (throws && (res || !err.empty())) {
//         throw ERROR("Command error: " + to_string(res) + "\ncommand: $ " + str_cut_end(command) + "\noutput: " + str_cut_end(out) + "\nerror: " + err);
//     }
//     logfile_append("exec.log", "\n" + out + err + "\n"); // TODO: log files just for debuging, also colusion happens when multiple AI runs, need some cleanup
//     return out + err;
// }

string ai_call(const string& prompt, int timeout = 30) { // TODO: it's hardcoded to google gemini AI, but it should be configurable with an adapter interface or something..
    string secret = trim(file_get_contents("gemini.key"));
    // secret = "API_KEY";
    // $prompt = esc($prompt);    
    json js = esc(str_replace({ // TODO: it should be generated with json lib
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
    // json js = R"(
    //     {
    //         "contents": [
    //             {
    //                 "parts": [
    //                     {
    //                         "text": ""
    //                     }
    //                 ]
    //             }
    //         ]
    //     }
    // )";
    // js["contents"][0]["parts"][0]["text"] = esc(prompt);

    string cmd = str_replace(
        {
            {"{secret}", secret},
            {"{js}", js },
        },
        "curl \"https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key={secret}\" -H 'Content-Type: application/json' -X POST -d \"{js}\" -s"
    );// | jq .candidates[0].content.parts[0].text';
    // Shell shell;
    // string res = shell.exec(cmd);
    string res = bash(cmd, timeout);
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



// Test the find_placeholders function with both old and new markers
void run_tests() { // TODO: put these test to the tests.php
    // Test 1: Simple test with default markers {{}} (Backward compatibility)
    string template1 = "Hello, {{name}}! Welcome to {{place}}.";
    auto result1 = find_placeholders(template1, "{{", "}}");
    assert(result1.size() == 2); 
    assert(result1[0] == "name"); 
    assert(result1[1] == "place");

    // Test 2: No placeholders in the template (Backward compatibility)
    string template2 = "No placeholders here.";
    auto result2 = find_placeholders(template2, "{{", "}}");
    assert(result2.empty()); // Expected: (empty)

    // Test 3: Template with malformed markers (e.g., unmatched opening marker) (Backward compatibility)
    string template3 = "Some text {{placeholder1} and some text {{placeholder2}}";
    auto result3 = find_placeholders(template3, "{{", "}}");
    assert(result3.size() == 1); 
    assert(result3[0] == "placeholder1} and some text {{placeholder2");

    // Test 4: Using different markers [[START]] and [[STOP]]
    string template4 = "The placeholder is [[VALUE-START]]value[[VALUE-STOP]].";
    auto result4 = find_placeholders(template4, "[[VALUE-START]]", "[[VALUE-STOP]]");
    assert(result4.size() == 1);
    assert(result4[0] == "value");

    // Test 5: Template with placeholders containing empty strings [[START]] [[STOP]]
    string template5 = "Here is an empty [[START]][[STOP]] placeholder.";
    auto result5 = find_placeholders(template5, "[[START]]", "[[STOP]]");
    assert(result5.size() == 1);
    // for (const auto& r :result3)  cout << r << " "; cout << endl;
    // cout << "[" << result5[0] << "]" << endl;
    assert(result5[0].empty()); // Expected: (empty string)

    // Test 6: Multiple placeholders with custom markers
    string template6 = "Here are multiple values: [[START]]first[[STOP]], [[START]]second[[STOP]].";
    auto result6 = find_placeholders(template6, "[[START]]", "[[STOP]]");
    assert(result6.size() == 2);
    assert(result6[0] == "first");
    assert(result6[1] == "second");

    // Test 7: Multiple placeholders using both default and custom markers
    string template7 = "Here is a mix: {{first}} and [[START]]second[[STOP]]!";
    auto result7_1 = find_placeholders(template7, "{{", "}}");
    auto result7_2 = find_placeholders(template7, "[[START]]", "[[STOP]]");
    
    assert(result7_1.size() == 1); 
    assert(result7_1[0] == "first");
    assert(result7_2.size() == 1);
    assert(result7_2[0] == "second");

    // Test 8: Template with placeholders containing multiple lines [[START]]...[[STOP]]
    string template8 = "Here is an empty \n[[START]]\nthis is a\n multi-line text\nhere...\n[[STOP]]\n placeholder.";
    auto result8 = find_placeholders(template8, "[[START]]", "[[STOP]]");
    // cout << "[" << result5[0].size() << "]" << endl;
    // for (const auto& r :result3)  cout << r << " "; cout << endl;
    // cout << "[" << result5[0] << "]" << endl;
    assert(result8.size() == 1);
    assert(result8[0] == "\nthis is a\n multi-line text\nhere...\n");
}

bool confirm(const string& message, char def = 'y') {
    def = tolower(def); // Normalize the default to lowercase
    char choice;

    while (true) {
        // Display the prompt with the default option
        cout << message << " (" 
                  << (def == 'y' ? "Y/n" : "y/N") << "): ";
        choice = cin.get();

        // Handle Enter (newline) input for default option
        if (choice == '\n') {
            return def == 'y';
        }

        // Clear the input buffer to handle extra characters after one key press
        while (cin.get() != '\n') { }

        // Normalize choice to lowercase and evaluate
        choice = tolower(choice);
        if (choice == 'y') {
            return true;
        } else if (choice == 'n') {
            return false;
        }

        // Invalid input, prompt again
        cout << "Please press 'y' or 'n'." << endl;
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
                timef(),
                bash("pwd")
            );
            string response = ai_call(prompt);
            chatlog("ai", response);

            // handle terminal command(s)
            vector<string> terms = find_placeholders(response, agent.term_start, agent.term_stop);
            if (!terms.empty()) {
                vector<string> splits = explode(agent.term_start, response);
                terms[0] = str_replace(agent.term_start, "\n", terms[0]);
                agent.history += "\n<ai>: " + splits[0] + agent.term_start + terms[0] + agent.term_stop;

                string sysmsg = "";
                // map<string, string> results;
                for (const string& term: terms) {
                    string trimmed = trim(term);
                    sysmsg += "\n" + agent.term_start + term + agent.term_stop + "\nResults:\n";
                    if (confirm("The system wants to run the following command:\n" + trimmed + "\nDo you want to proceed?", 'y')) {  // TODO: its supervise the bash commands, but we should be able to turn it off - note unsafe! so it should be turned on by default
                        string results = bash(trimmed, 5); // TODO: add command timeout settings, it's just hardcoded here now
                        sysmsg += results;
                        cout << results << endl; // TODO: also show the inner things that AI says around the bash scripts, because AIs can not always knows who the talking to when they start there responses
                    } else {
                        sysmsg += "Execution blocked by user. Reason: " + input("Provide a reason: ", "<no reason>");
                    }
                    break; // NOTE: we need the first one only, (to do all bash terminal command, just remove this line) 
                }
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

            cout << response;
            agent.history += "\n<ai>: " + response;
            chatlog("ai", response);
            skip_user = false;
        }
    } catch (exception &e) {
        // echo exceptionChainToString($e, true);
        cerr << e.what() << endl;
        return 1;
    }

    return 0;
}