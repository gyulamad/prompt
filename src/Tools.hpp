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
        ) {}

        static string callback(void* tool_void, void* model_void, void* user_void, const JSON& args, const JSON& conf) {
            string command = args.get<string>("command");
            if (trim(command).empty()) return "Command can not be empty!";
            int timeout = args.has("timeout") ? args.get<int>("timeout") : 10;
            if (timeout <= 0) return "Parameter 'timeout' can not be less or equal to zero.";
            string reason = args.has("reason") ? args.get<string>("reason") : "";
            return confirm_execute_ssh(
                [&](const string& prmpt) { return ((User*)user_void)->confirm(prmpt); }, 
                conf, reason, command, timeout
            );
        }

    } bashCommandTool;


    // TODO:
    // 1.  **Python Script Execution Tool:** (Highest Priority - vast functionality increase) - running in the SSH environment?
    // ```json
    // simplifymanager(
    //     action: str,
    //     filename: str,
    //     content: str = "",
    //     start_line: int = 0,
    //     end_line: int = 0,
    //     permissions: str = ""
    // )
    // ```

    // Where:

    // *   `action`: Specifies the action to perform (e.g., "create", "modify", "list", "read", "delete", "execute").
    // *   `filename`: The name of the file to operate on.
    // *   `content`: (Optional) The content to write to the file (for "create" and "modify").
    // *   `start_line`: (Optional) The starting line number for "read" (0 for beginning).
    // *   `end_line`: (Optional) The ending line number for "read" (0 for end).
    // *   `permissions`: (Optional) The file permissions to set (e.g., "755") (used for "execute").

    // **Action Breakdown:**

    // *   **create:**
    //     *   `filename`: Required.
    //     *   `content`: Required.
    //     *   Example: `simplifymanager(action="create", filename="myfile.txt", content="This is the content.")`

    // *   **modify:**
    //     *   `filename`: Required.
    //     *   `content`: Required (replaces the entire file content).
    //     *   Example: `simplifymanager(action="modify", filename="myfile.txt", content="This is the new content.")`

    // *   **list:**
    //     *   `filename`: Not required. If specified, list only that file. If not, list all files in the current directory.
    //     *   Example: `simplifymanager(action="list")` or `simplifymanager(action="list", filename="myfile.txt")`

    // *   **read:**
    //     *   `filename`: Required.
    //     *   `start_line`: Optional (default 0).
    //     *   `end_line`: Optional (default 0).
    //     *   Example: `simplifymanager(action="read", filename="myfile.txt", start_line=10, end_line=20)`

    // *   **delete:**
    //     *   `filename`: Required.
    //     *   Example: `simplifymanager(action="delete", filename="myfile.txt")`

    // *   **execute:**
    //     *   `filename`: Required.
    //     *   `permissions`: Optional (e.g., "755" to make it executable). If provided, the tool changes the permissions before executing, if not, then will use it as it is.
    //     *   Example: `simplifymanager(action="execute", filename="myscript.sh", permissions="755")` or `simplifymanager(action="execute", filename="myscript.py")`
    class FileManagerTool: public Tool {
    public:
        
        FileManagerTool(): Tool(
            "file_manager", 
            { 
                { "action", PARAMETER_TYPE_STRING, true }, // TODO: use get_required_errors to validate Tools before callback()... Note, I found that it's already validated in some way, using the Parameter::is_required()...
            }, 
            callback,
            "*   `action`: Specifies the action to perform: " + actions_to_string(action_map) + "\n"
            "\n"
            "**Action Breakdown:**"
            "\n"
            "*   **create:**\n"
            "    *   `filename`: Required.\n"
            "    *   `content`: Required.\n"
            "\n"
        ) {}

        static string callback(void* tool_void, void* model_void, void* user_void, const JSON& args, const JSON& conf) {
            if (!args.has("action")) return "Parameter 'action' is missing!";
            string action = args.get<string>("action");
            if (trim(action).empty()) return "Parameter 'action' can not be empty!";
            auto it = action_map.find(action);
            if (it == action_map.end()) return "Action '" + it->first + "' does not exists!\nPossible actions are: " + actions_to_string(action_map);
            return it->second(tool_void, model_void, user_void, args, conf);
        }

    private:

        static map<string, tool_cb> action_map;

        static string actions_to_string(map<string, tool_cb> action_map) {
            return "[\"" + implode("\", \"", array_keys(action_map)) + "\"]";
        }

        string write(void* user_void, const JSON& args, const JSON& conf, bool append) {
            NULLCHK(user_void);
            error_to_user(get_required_error<string>(conf, "base_folder"));
            
            string errors = get_required_errors<string>(args, { "filename", "content" });
            if (!errors.empty()) return errors;
            string filename = args.get<string>("filename");
            string content = args.get<string>("content"); 
            errors += get_user_confirm_error(
                [&](const string& prmpt) { 
                    return ((User*)user_void)->confirm(prmpt); 
                }, 
                    string("File manager tool wants to ") 
                    + (append ? "append to" : "create a") + " file: " + filename 
                    + "\nwith contents:\n" + (content.empty() ? "<empty>" : content) 
                    + "\nDo you want to proceed?",
                "User intercepted the file operation", filename
            );
            if (!errors.empty()) return errors;

            if (!file_put_contents(conf.get<string>("base_folder") + "/" + filename, content, append))
                return "File write failed: " + filename; 

            return "File writen: " + filename;
        }

        static string create_cb(void* tool_void, void* model_void, void* user_void, const JSON& args, const JSON& conf) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->write(user_void, args, conf, false);
        }

        static string append_cb(void* tool_void, void* model_void, void* user_void, const JSON& args, const JSON& conf) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->write(user_void, args, conf, true);
        }

    } fileManagerTool;

    map<string, tool_cb> FileManagerTool::action_map = {
        { "create", FileManagerTool::create_cb },
    };


    // TODO 2.  **Bug Reporting/Feedback Tool (with Future Request Integration):** (Crucial for improvement and long-term planning)
    // TODO 3.  **Note-Taking/Memory Tool:** (Important for efficiency and avoiding repeated work) - notes/per AI AND/OR "global" notes for each AI?; categories for notes?
    // TODO 4.  **Clarifying Question Prompt:** I'll try to use existing functions and just ask directly. If I still fail to do it, it will be one function for me to ask you some questions before I even start with a task. (later when we have multiple AI but any of them need to ask a question for the subtask, they can send a message to the user directly "bypassing" its parent AI)
}