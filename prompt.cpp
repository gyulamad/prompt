#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <atomic>  // For std::atomic
#include <thread>  // For std::thread and std::this_thread
#include <chrono>  // For std::chrono

#include "tools.hpp"
#include "JSON.hpp"
#include "Proc.hpp"
#include "Speech.hpp"
#include "Arguments.hpp"
#include "regx.hpp"

using namespace std;

class Agent {
private:
    string name;
public:
    Agent(const string& name): name(name) {}

    string request_qwen(const string& prompt, int timeout = 30) {
        // TODO: it's hardcoded to huggingface, but it should be configurable with an adapter interface or something..
        string secret = trim(file_get_contents("hugging.key"));
        JSON json(R"({
            "model": "Qwen/Qwen2.5-Coder-32B-Instruct",
            "messages": [
                {"role": "system", "content": ""}
            ],
            "max_tokens": 500,
            "stream": false
        })");
        // json.set("messages[0].content", prompt);
        cout << json.dump(4) << endl;
        string cmd = str_replace(
            {
                { "{secret}", secret },
                { "{json}", esc(json.dump(4)) },
            },
            "curl 'https://api-inference.huggingface.co/models/Qwen/Qwen2.5-Coder-32B-Instruct/v1/chat/completions' \
                -H 'Authorization: Bearer {secret}' \
                -H 'Content-Type: application/json' \
                --data \"{json}\" -s"
        );
        cout << cmd << endl;
        string resp = bash(cmd, timeout); // TODO: use Proc
        cout << resp << endl;
        json.set(resp);
        cout << json.dump(4) << endl;
        if (!json.isDefined("choices[0].message")) 
            throw ERROR("AI error response: " + json.dump(4));
        return json.get<string>("choices[0].message.content");
    }

    string request_gemini(const string& prompt, int timeout = 30) {
        // TODO: it's hardcoded to google gemini AI, but it should be configurable with an adapter interface or something..
        string secret = trim(file_get_contents("gemini.key"));
        JSON json("{}");
        json.set("contents[0].parts[0].text", prompt);
        // cout << json.dump() << endl;
        string cmd = str_replace(
            {
                { "{secret}", secret },
                { "{json}", esc(json.dump()) },
            },
            "curl \"https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key={secret}\" -H 'Content-Type: application/json' -X POST -d \"{json}\" -s"
        );
        string resp = bash(cmd, timeout); // TODO: use Proc
        json.set(resp);
        if (!json.isDefined("candidates")) 
            throw ERROR("AI error response: " + json.dump());
        return json.get<string>("candidates[0].content.parts[0].text");
    }
};

class Log {
private:
    string log;
public:
    string note(const string& name, const string& note) {
        log += str_replace({
            { "{time}", timef() },
            { "{name}", name },
            { "{note}", note },
        }, "\n[{time}] [{name}] {note}");
        return log;
    }

    string dump() {
        return log;
    }
};


class Parser {
public:

    typedef string (*token_func_t)(void*, const string&, size_t);
    typedef unordered_map<string, token_func_t> token_func_map_t;

    string parse(
        void* context, 
        string& feed, 
        token_func_map_t token_func_map
    ) {
        string read = "";
        for (size_t i = 0; i < feed.size(); i++) {
            read += feed[i];
            for (const auto& [token, func] : token_func_map) {
                if (str_ends_with(read, token))
                    read = str_ends_replace(read, token, func(context, feed, i));
            }
        }
        return read;
    }
};

// Atomic flag to control the blinking thread
atomic<bool> blinking(true);

// Function to hide the cursor
void hide_cursor() {
    cout << "\033[?25l" << flush; // ANSI escape code to hide the cursor
}

// Function to show the cursor
void show_cursor() {
    cout << "\033[?25h" << flush; // ANSI escape code to show the cursor
}

// Function to calculate the visible length of the text (excluding ANSI escape codes)
size_t visible_length(const string& text) {
    size_t length = 0;
    bool in_escape = false;

    for (size_t i = 0; i < text.length(); i++) {
        if (text[i] == '\033') {
            in_escape = true;
        } else if (in_escape && text[i] == 'm') {
            in_escape = false;
        } else if (!in_escape) {
            // Handle multi-byte Unicode characters
            if ((text[i] & 0xC0) != 0x80) { // Count only the first byte of a multi-byte character
                length++;
            }
        }
    }

    return length;
}

void blink_text(const string& text, int delay_ms = 400) {
    hide_cursor(); // Hide the cursor when blinking starts

    size_t text_length = visible_length(text); // Calculate visible length

    while (blinking) {
        // Print the text in red
        cout << "\033[91m" << text << "\033[0m" << flush;
        this_thread::sleep_for(chrono::milliseconds(delay_ms));

        // Clear the text
        cout << "\r" << string(text_length, ' ') << "\r" << flush;
        this_thread::sleep_for(chrono::milliseconds(delay_ms));
    }

    // Print the text one last time
    cout << "\r\033[91m" << text << "\033[0m" << flush;

    show_cursor(); // Restore the cursor when blinking stops
}


class Prompt {
private:
    Proc proc = Proc("bash");
    Log log;
    bool safeguard = true;
    bool skipuser = false;
    string term_state = "";
public:

    Prompt() {}
    ~Prompt() {
        // if (proc) {
        //     proc.kill();
        //     delete proc;
        // }
    }

    void run(bool voice_input, bool voice_output) {
        const string agentname = "assistant";     
        const string username = "user";
        const string userlang = "Hungarian";
        int voice_output_speed = 200;

        Speech speech;
        Parser parser;
        Agent agent(agentname);  

        while (true) {
            term_state = str_cut_begin(term_state, 10000); // TODO: max size to parameter - roll out the old stuff

            if (!skipuser) {
                string input = "";
                while (trim(input).empty()) {
                    if (voice_input) { 
                        cout << endl << " rec>\r" << flush;

    // Start the blinking thread
    blinking = true;
    thread blink_thread(blink_text, "\033[91m\033[5m●\033[0m", 400);

                        input = speech.rec(); 
                        if (input.empty() || speech.is_interrupted()) {                    
                            voice_input = false;
                            cout << "\r     " << flush;
                        }

    // Stop the blinking thread
    blinking = false;
    blink_thread.join(); // Wait for the thread to finish

                        cout << "\r> " << flush; 
                        cout << input << endl; 
                    }
                    else input = readln();
                }
                if (!str_starts_with(input, "/")) log.note(username, input);
                else {
                    if (input == "/voice") {
                        voice_input = true;
                        voice_output = true;
                    }
                    if (input == "/voice input on") voice_input = true;
                    if (input == "/voice input off") voice_input = false;
                    if (input == "/voice output on") voice_output = true;
                    if (input == "/voice output off") voice_output = false;
                    if (str_starts_with(input, "/voice output speed")) {
                        if (input == "/voice output speed") cout << "Speed: " << voice_output_speed << endl;
                        else {
                            vector<string> matches;
                            if (regx_match("^\\/voice output speed\\s+(\\d+)s*$", input, &matches)) {
                                voice_output_speed = parse<int>(matches[1]);
                            } else {
                                cout << "Use: /voice output speed [N] " << endl;
                            }
                        }
                    }
                    continue;
                }
            }
            skipuser = false;

            // string results = "";
            // while (proc.ready()) {
            //     string output = proc.read();
            //     cout << "Terminal says: " << output << endl;                        
            //     results += output;
            // }

            string output = proc.read();
            if (!output.empty()) {
                cout << output << endl;  
                log.note("terminal", output);
                term_state += output; 
            }

            string response = agent.request_gemini(
                str_replace(
                    {
                        { "{log}", log.dump() },
                        { "{name}", agentname },
                        { "{owner}", username },
                        { "{term}", term_state },
                        { "{lang}", userlang },
                    }, 
                    "{log}"
                    "\n[LOG ENDS]"
                    "\n"
                    "\nEarlier conversasion and other events log history ends here."
                    "\n"
                    "\n***System instructions and guide-lines:***"
                    "\n"
                    "\nYour name is {name}."
                    "\nYou are an AI assistant, and you asist {owner}."
                    "\n- {owner} language is {lang}, talk to his/her on his/her own language."
                    "\n"
                    "\n*Your terminal usage abilities:*"
                    "\n"
                    "\nYou have access to a linux terminal (bash) standard input:"
                    "\nEverything you say is going to the user by default. Once you need to type something to the terminal, you outputs [TERM] token and everything afterwards will be forwarded to the terminal standard input, just like you typing in it. This way you can run and use commands. To finish your typeing use [/TERM] token and you can continue to talk to the user."
                    "\nExample: [TERM]ls -l[/TERM]"
                    "\nOr multiple lines at once example:"
                    "\n[TERM]"
                    "\ncd folder"
                    "\necho \"hello\" >> hello.txt"
                    "\nls -l"
                    "\n[/TERM]"
                    "\n"
                    "\nThe terminal can run long or blocking processes and not necessary stops the session when you using the [/TERM] token, if the process you run earlier is still runing, for eg waiting for user input (you can tell by the latest command results in terminal outputs) then you have to continue the process."
                    "\nIf you want to reset the terminal and start a new bash, user the [TERM-RESET] token. It will kill the current process session and you free to start a new workflow."
                    "\nExample for long/blocking process usages:"
                    "\n[TERM]while read -p \"Enter input: \" line; do echo \"$line\" >> input.log; done[/TERM]"
                    "\nResults would be the `Enter input:` text you should see from the terminal."
                    "\n..."
                    "\nThen later time you can type: [TERM]Test line here...[/TERM]"
                    "\nResults would be the `Enter input:` text again."
                    "\n..."
                    "\n..."
                    "\nThen restart the terminal using [TERM-RESET] token."
                    "\nThen you can validate the result: [TERM]cat input.log[/TERM]"
                    "\nResults would be the `Test line here...` that you typed into the terminal."
                    "\n..."
                    "\nBe mindful of long-running processes, notice that if you accidentally miss the [TERM-RESET] in the example above, you would continue writing the `cat input.log` line into the input.log instead running it in a new bash shell."
                    "\nWARNING: Never and ever mention literally these terminal indicator tokens in your response if it's not intended directly typed into the terminal as you can accidentally trigger the terminal handler. If you really need to talk about it just refer to them as `term` tokens (lowercase)."
                    "\n"
                    "\n*Your latest terminal output history:*"
                    "\n{term}"
                    "\n"
                    "\n*Let the user know:*"
                    "\n - if you have any confusion ar ambiguity about the system instructions"
                    "\n - if you have strugle to complete the given objective be transparent about the limitations, and explain why."
                    "\n"
                    "\nNow, it's your turn to continue the conversation from the log or use/react to your terminal latest output. (Do not simulate the log prefixes in your response, always be short and concise)."
                )
            );

            // dirty but we don't care what AI would say after the terminal, need to wait for the terminal outputs first...
            if (str_contains(response, "[/TERM]")) {
                response = explode("[/TERM]", response)[0] + "[/TERM]";
            }
            vector<string> splits = explode("[TERM]", response);
            if (splits.size() > 0 && !splits[0].empty()) {
                cout << "\033[92m\033[3m" << splits[0] << "\033[0m" << flush; // actual response to the user (rest is to the terminal)
                if (voice_output) speech.say(splits[0], voice_output_speed);
            }
            log.note(agentname, response);
            response = explode("[/TERM]", response)[0];

            // TODO: change the token parsing mechanism to read through the AI response and use state-machine like interpreter.
             
            parser.parse(this, response, {
                {
                    "[TERM]", [](void* _prompt, const string& feed, size_t i) -> string {
                        Prompt* prompt = (Prompt*)_prompt;
                        string term = str_get_rest(feed, i+1);
                        while (str_contains(term, "[TERM]")) term = str_replace(
                            {
                                { "\n[TERM]", "\n" },
                                { "[TERM]\n", "\n" },
                                { "[TERM]", "\n" },
                            }, term
                        );
                        string inputln = "$ " + term + "\nThis terminal input was blocked by the safeguard.\n";
                        string confirm = prompt->safeguard ? readln("\n\033[33mThe following are attempt to be forwarded to the terminal:\033[0m\n" + term + "\n\033[33mDo you want to send? (Y/n):\033[0m ", "Y") : "Y";
                        prompt->skipuser = false;
                        if (confirm == "Y") {
                            inputln = term;
                            prompt->log.note("terminal", inputln);
                            // cout << inputln << endl;
                            prompt->proc.writeln(inputln);
                            prompt->term_state += "$ " + inputln + "\n";
                            prompt->skipuser = true;
                        }
                        return feed;
                    }
                },
                {
                    "[TERM-RESET]", [](void* _prompt, const string& feed, size_t i) -> string {
                        Prompt* prompt = (Prompt*)_prompt;
                        prompt->proc.reset();
                        prompt->term_state = "";
                        string note = "Terminal reset sucessfully.";
                        prompt->log.note("terminal", note);
                        cout << note << endl;
                        return feed;
                    }
                },
            });
        }
        proc.kill();
    }
};

int main(int argc, char *argv[]) {
    Arguments args(argc, argv);
    const bool voice_input = args.has("voice-input") ? args.getBool("voice-input") : args.getBool("voice");
    const bool voice_output = args.has("voice-output") ? args.getBool("voice-output") : args.getBool("voice");
    Prompt prompt;
    prompt.run(voice_input, voice_output);
    
    return 0;
}