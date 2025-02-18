#pragma once

#include "tools/Process.hpp"
#include "tools/io.hpp"
#include "tools/strings.hpp"

#include "User.hpp"
#include "Tool.hpp"
#include "Parameter.hpp"

using namespace std;
using namespace tools;

namespace prompt {

    class NothingTool: public Tool {
    public:
        
        NothingTool(): Tool(
            "do_nothing", 
            { }, 
            callback,
            "Performs nothing. Litterally nothing. It's just a placeholder stuff..."
        ) {}

        static string callback(void* tool_void, void* model_void, void* user_void, const JSON& args) { return ""; }

    } nothingTool;


    class GoogleSearchTool: public Tool {
    public:
        
        GoogleSearchTool(): Tool(
            "google_search", 
            { 
                { "query", PARAMETER_TYPE_STRING, true }, 
                { "max", PARAMETER_TYPE_INTEGER, true }, 
            }, 
            callback,
            "Performs a google search and shows the result list."
        ) {}

        static string callback(void* tool_void, void* model_void, void* user_void, const JSON& args) {
            string query = args.get<string>("query");
            int max = args.get<int>("max");
            cout << "Google search: '" + escape(query) + "'" << endl;
            string results = Process::execute(
                "node browse/google-search.js"
                " --query \"" + escape(query) + "\""
                " --max " + tools::to_string(max));
            cout << results << endl;
            return results;
        }

    } googleSearchTool;
    

    class WebBrowserTool: public Tool {
    public:
        
        WebBrowserTool(): Tool(
            "web_browse", 
            { 
                { "url", PARAMETER_TYPE_STRING, true }, 
                { "method", PARAMETER_TYPE_STRING },
                { "data", PARAMETER_TYPE_STRING },
                { "cookies", PARAMETER_TYPE_STRING }, 
                { "script", PARAMETER_TYPE_STRING }, 
            }, 
            callback,
            "Load the source-code of a web page from the given URL. "
            "Use methods GET/POST/PUT etc. with data and cookies. "
            "With the script parameter you can inject javascript."
        ) {}

        static string callback(void* tool_void, void* model_void, void* user_void, const JSON& args) {
            string url = args.get<string>("url");
            string method = args.has("method") ? args.get<string>("method") : "";
            string data = args.has("data") ? args.get<string>("data") : "";
            string cookies = args.has("cookies") ? args.get<string>("cookies") : "";
            string script = args.has("script") ? args.get<string>("script") : "";
            
            cout << "Loading: " << (method.empty() ? "" : "[" + method + "] ") << url << endl;
            return Process::execute(
                "node browse/web_scraper.js"
                " --url \"" + escape(url) + "\""
                + (method.empty() ? "" : " --method " + escape(method))
                + (data.empty() ? "" : " --data \"" + escape(data) + "\"")
                + (cookies.empty() ? "" : " --cookies \"" + escape(cookies) + "\"")
                + (script.empty() ? "" : " --script \"" + escape(script) + "\"") // TODO: escaping mess up!
            );
        }

    } webBrowserTool;


    class BashCommandTool: public Tool {
    private:
        const int read_timeout_ms = 300;
    public:
        
        BashCommandTool(): Tool(
            "bash_command", 
            { 
                { "command", PARAMETER_TYPE_STRING, true },
                { "timeout", PARAMETER_TYPE_INTEGER, false },
                { "reason", PARAMETER_TYPE_STRING, false },
            }, 
            callback,
            "Runs a bash command and shows the output. "
            "Note: Do not call long running or blocking command that waits for user input etc., otherwise it may timeouts. "
            "Note: Use the (optional) 'reason' parameter to explane to the user before they confirm or reject the command. "
            "Note: You need to avoid commands that don't produce output for more than " 
            + to_string(read_timeout_ms) + "ms to prevent them from being prematurely terminated. "
        ) {}

        static string callback(void* tool_void, void* model_void, void* user_void, const JSON& args) {
            string command = args.get<string>("command");
            if (trim(command).empty()) return "Command can not be empty!";
            int timeout = args.has("timeout") ? args.get<int>("timeout") : 10;
            if (timeout <= 0) return "Parameter 'timeout' can not be less or equal to zero.";
            string reason = args.has("reason") ? args.get<string>("reason") : "";
            //string fullcmd = "(timeout " + to_string(timeout) + "s " + command + ") || echo 'Command timed out after " + to_string(timeout) + "s!'";
            // cout << "The following bash command is about to executed with a time window of " + to_string(timeout) + "s:\n" + command + "\n"
            //     + (reason.empty() ? "" : ("Reason: " + reason)) << endl;
            
            if (!user_void) throw ERROR("No user?!");
            if (!((User*)user_void)->confirm(
                "The following bash command is about to executed with a time window of " + tools::to_string(timeout) + "s:\n" 
                + command + "\n" + (reason.empty() ? "" : ("Reason: " + reason)) + "\nDo you want to proceed?"
            )) {
                string msg = "User intercepted the command execution";
                cout << msg + "." << endl;
                return msg + ":\n" + command;
            }

            // TODO: it could go into common:
            Process proc;
            proc.writeln("echo 'Command execution start...'");
            proc.writeln(command);
            proc.writeln("echo 'Command execution end.'");
            string output = "";

            auto start_time = chrono::steady_clock::now();
            while (true) {

                string outp = proc.read(read_timeout_ms);
                if (outp.empty()) break;
                output += outp;

                // Check if the timeout has been reached
                auto current_time = chrono::steady_clock::now();
                auto elapsed_time = chrono::duration_cast<chrono::seconds>(current_time - start_time).count();
                if (elapsed_time >= timeout) {
                    output += "\nExecution timed out after " + tools::to_string(timeout) + "s";
                    break; // Timeout reached
                }
            }
            auto current_time = chrono::steady_clock::now();
            auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(current_time - start_time).count();
            proc.kill();

            string elapsed = "Command execution time was: " + tools::to_string(elapsed_time) + "ms";
            cout << output + "\n" + elapsed << endl;
            return 
                "Results from command execution:\n" + trim(command) + "\n" + elapsed +
                "\nOutput:\n" + (output.empty() ? "<empty>" : output);
        }

    } bashCommandTool;

}