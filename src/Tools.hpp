#pragma once

#include <iostream>
#include <string>
#include <cstdio>
#include <memory>
#include <stdexcept>

#include "tools/Process.hpp"
#include "tools/io.hpp"
#include "tools/strings.hpp"
#include "tools/JSON.hpp"
#include "tools/datetime.hpp"

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

        static string callback(void* tool_void, void* model_void, void* user_void, const JSON& args, const JSON& conf) { return ""; }

    } nothingTool;


    class DateTimeTool: public Tool {
    public:
        
        DateTimeTool(): Tool(
            "datetime", 
            { }, 
            callback,
            "This tools shows the systems current date and time "
            "as the AI does not have access to the real time data but sometimes it's needed."
        ) {}

        static string callback(void* tool_void, void* model_void, void* user_void, const JSON& args, const JSON& conf) { 
            return ms_to_datetime(get_time_ms()); 
        }

    } dateTimeTool;


    // TODO: add https://www.firecrawl.dev
    
    class GoogleSearchTool: public Tool {
    public:
        
        GoogleSearchTool(): Tool(
            "google_search", 
            { 
                { "query", PARAMETER_TYPE_STRING, true }, 
                { "max", PARAMETER_TYPE_INTEGER, false }, 
            }, 
            callback,
            "Performs a google search and shows the result list."
        ) {}

        static string callback(void* tool_void, void* model_void, void* user_void, const JSON& args, const JSON& conf) {
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

        static string callback(void* tool_void, void* model_void, void* user_void, const JSON& args, const JSON& conf) {
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
        //const int read_timeout_ms = 1000; // TODO: config?

        static string exec(const char* cmd) { // TODO: to common
            string result = "";

            char buffer[128];
            shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
            if (!pipe) throw runtime_error("popen() failed!");
            while (!feof(pipe.get())) {
                if (fgets(buffer, 128, pipe.get()) != nullptr)
                    result += buffer;
            }
        
            return result;
        }

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
            "Notes:"
            "Do not call long running or blocking command that waits for user input etc., otherwise it may timeouts. "
            "Use the (optional) 'reason' parameter to explane to the user before they confirm or reject the command. "
            "You need to avoid long running or blocking commands that waits for user input to prevent them from being prematurely terminated when timeouts. " 
            "Keep in mind, your command will proceed as a bash command as the following: timeout <timeout>s ssh you@host \"<command>\""
            // + ::to_string(read_timeout_ms) + "ms to prevent them from being prematurely terminated. "
        ) {}

        static string callback(void* tool_void, void* model_void, void* user_void, const JSON& args, const JSON& conf) {
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
            // Process proc("bash"); // TODO: to config
            // proc.writeln("echo 'Command execution start...'");
            string ssh_user = conf.get<string>("ssh_user"); //"bot1"; // TODO: to config
            string ssh_host = conf.get<string>("ssh_host"); //"localhost"; // TODO: to config
            string ssh_command = "timeout " + ::to_string(timeout) + "s ssh " + ssh_user + '@' + ssh_host + " \"" + escape(command, "$\\\"") + " 2>&1\"";
            // proc.writeln(ssh_command);
            // proc.writeln("echo 'Command execution end.'");
            // string output = "";

            auto start_time = chrono::steady_clock::now();
            // while (true) {

            //     string outp = proc.read(); //proc.read(((BashCommandTool*)tool_void)->read_timeout_ms);
            //     if (outp.empty()) break;
            //     output += outp;

            //     // Check if the timeout has been reached
            //     auto current_time = chrono::steady_clock::now();
            //     auto elapsed_time = chrono::duration_cast<chrono::seconds>(current_time - start_time).count();
            //     if (elapsed_time >= timeout) {
            //         output += "\nExecution timed out after " + tools::to_string(timeout) + "s";
            //         break; // Timeout reached
            //     }
            // }
            // proc.kill();
            string output = "";
            try {
                output = exec(ssh_command.c_str());
                //cout << "Output:\n" << output << endl;
            } catch (const runtime_error& e) {
                output = "Error: " + string(e.what());
                // cerr << "Error: " << e.what() << endl;
                // return 1;
            }
            auto current_time = chrono::steady_clock::now();
            auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(current_time - start_time).count();
            

            string elapsed = "Command execution time was: " + tools::to_string(elapsed_time) + "ms";
            cout << output + "\n" + elapsed << endl;
            return 
                "Results from command execution:\n" + trim(command) + "\n" + elapsed +
                "\nOutput:\n" + (output.empty() ? "<empty>" : output);
        }

    } bashCommandTool;


    // TODO: add time/date callback
}