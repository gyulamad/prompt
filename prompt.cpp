#include <iostream>
#include <string>
#include <vector>
#include <functional>

#include "tools.hpp"
#include "JSON.hpp"
#include "Proc.hpp"

using namespace std;

class Agent {
private:
    string name;
public:
    Agent(const string& name): name(name) {}

    string request(const string& prompt, int timeout = 30) {
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

    void run(const string& agentname = "assistant") {
        Parser parser;
        Agent agent(agentname);        
        string username = "user";
        while (true) {
            term_state = str_cut_begin(term_state, 10000); // TODO: max size to parameter - roll out the old stuff

            if (!skipuser) {
                string input = readln();
                log.note(username, input);
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

            string response = agent.request(
                str_replace(
                    {
                        { "{log}", log.dump() },
                        { "{name}", agentname },
                        { "{owner}", username },
                        { "{term}", term_state },
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
                    "\nNow, it's your turn to continue the conversation from the log or use/react to your terminal latest output. (Do not simulate the log prefixes)."
                )
            );

            // dirty but we don't care what AI would say after the terminal, need to wait for the terminal outputs first...
            if (str_contains(response, "[/TERM]")) {
                response = explode("[/TERM]", response)[0];
            }
            vector<string> splits = explode("[TERM]", response);
            if (splits.size() > 0 && !splits[0].empty()) {
                cout << splits[0] << flush; // actual response to the user (rest is to the terminal)
            }
            log.note(agentname, response);

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
                        string confirm = prompt->safeguard ? readln("The following are attempt to be forwarded to the terminal:\n" + term + "\nDo you want to send? (Y/n): ", "Y") : "Y";
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

int main() {
    Prompt prompt;
    prompt.run();
    
    return 0;
}