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


    /**
     * @brief 
     * @deprecated
     */
    class BashCommandTool: public Tool { // TODO: remove this with ssh user all together if the file_manager.exec action works
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

    private:

        // static string confirm_execute_ssh(confirm_func_t confirm_func, const JSON& conf, const string& reason, const string& command, int timeout = 10) {
        //     string error = get_user_confirm_error(
        //         confirm_func, 
        //         "The following bash command is about to executed with a time window of " + tools::to_string(timeout) + "s:\n" 
        //             + command + "\n" + (reason.empty() ? "" : ("Reason: " + reason)) + "\nDo you want to proceed?",
        //         "User intercepted the command execution", command
        //     );
        //     if (!error.empty()) return error;

        //     string ssh_user = conf.get<string>("ssh_user");
        //     string ssh_host = conf.get<string>("ssh_host");
        //     string ssh_key = "/opt/prompt/keys/id_rsa_" + ssh_user;
        //     string ssh_command = "timeout " + ::to_string(timeout) + "s ssh -i " + ssh_key + " " + ssh_user + '@' + ssh_host 
        //         + " " + quote_cmd(command + " 2>&1");

        //     auto start_time = chrono::steady_clock::now();
        //     string output = "";
        //     try {
        //         output = execute(ssh_command.c_str());
        //     } catch (const runtime_error& e) {
        //         output = "Error: " + string(e.what());
        //     }
        //     auto current_time = chrono::steady_clock::now();
        //     auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(current_time - start_time).count();
            

        //     string elapsed = "Command execution time was: " + tools::to_string(elapsed_time) + "ms";
        //     cout << "\nOutput:\n" << output + "\n" + elapsed << endl;
        //     return 
        //         "Results from command execution:\n" + trim(command) + "\n" + elapsed +
        //         "\nOutput:\n" + (output.empty() ? "<empty>" : output);
        // }
        
        static string confirm_execute_ssh(confirm_func_t confirm_func, const JSON& conf, const string& reason, const string& command, int timeout = 10) {
            // Step 1: Get user confirmation
            string error = get_user_confirm_error(
                confirm_func, 
                "The following bash command is about to be executed with a time window of " + tools::to_string(timeout) + "s:\n" 
                    + command + "\n" + (reason.empty() ? "" : ("Reason: " + reason)) + "\nDo you want to proceed?",
                "User intercepted the command execution", command
            );
            if (!error.empty()) return error;
        
            // Step 2: Extract SSH configuration
            string ssh_user = conf.get<string>("ssh_user");
            string ssh_host = conf.get<string>("ssh_host");
            string ssh_key = "/opt/prompt/keys/id_rsa_" + ssh_user;
        
            // Step 3: Build the SSH command with environment variables and BatchMode
            // string ssh_command = "SSH_ASKPASS= DISPLAY= timeout " + ::to_string(timeout) + "s ssh -o BatchMode=yes -i " + ssh_key + " " + ssh_user + '@' + ssh_host 
            //     + " " + quote_cmd(command + " 2>&1");

            string ssh_command = "timeout " + ::to_string(timeout) + "s ssh " + ssh_user + '@' + ssh_host 
                + " " + quote_cmd(command + " 2>&1");
        
            // Step 4: Execute the SSH command
            auto start_time = chrono::steady_clock::now();
            string output = "";
            try {
                output = execute(ssh_command.c_str());
            } catch (const runtime_error& e) {
                output = "Error: " + string(e.what());
            }
            auto current_time = chrono::steady_clock::now();
            auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(current_time - start_time).count();
        
            // Step 5: Display and return results
            string elapsed = "Command execution time was: " + tools::to_string(elapsed_time) + "ms";
            cout << "\nOutput:\n" << output + "\n" + elapsed << endl;
            return 
                "Results from command execution:\n" + trim(command) + "\n" + elapsed +
                "\nOutput:\n" + (output.empty() ? "<empty>" : output);
        }

    } bashCommandTool;


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
            "*   **create/append:**\n"
            "    *   `filename`: Required.\n"
            "    *   `content`: Required.\n"
            "    *Note: Create attempt overrides if the file already exists.\n"
            "\n"
            "*   **remove:**\n"
            "    *   `filename`: Required.\n"
            "\n"
            "*   **rename:**\n"
            "    *   `old_filename`: Required.\n"
            "    *   `new_filename`: Required.\n"
            "\n"
            "*   **view:**\n"
            "    *   `filename`: Required.\n"
            "    *   `start_line`: Optional (number, default: 1th line).\n"
            "    *   `end_line`: Optional (number, default: last line).\n"
            "\n"
            "*   **edit:**\n"
            "    *   `filename`: Required.\n"
            "    *   `start_line`: Optional (number, default: 1th line).\n"
            "    *   `end_line`: Optional (number, default: last line).\n"
            "    *   `content`: Required. (send empty string to remove lines)\n"
            "\n"
            "*   **exec:**\n"
            "    *   `command`: Required (bash command).\n"
            "    *   `timeout`: Optional (number, in seconds).\n"
            "    *Notes:\n"
            "       You need to avoid long running or blocking commands\n"
            "       that waits for user input to prevent them\n"
            "       from being prematurely terminated when timeouts.\n" 
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
            string base = conf.get<string>("base_folder");
            
            string errors = get_required_errors<string>(args, { "filename", "content" });
            if (!errors.empty()) return errors;
            string filepath = base + args.get<string>("filename");
            if (!is_valid_filepath(filepath)) return "Filename is invalid: " + filepath;
            string content = args.get<string>("content"); 
            errors += get_user_confirm_error(
                [&](const string& prmpt) { 
                    return ((User*)user_void)->confirm(prmpt); 
                }, 
                    string("File manager tool wants to ") 
                    + (append ? "append to" : "create a") + " file: " + filepath 
                    + "\nwith contents:\n" + (content.empty() ? "<empty>" : content) 
                    + "\nDo you want to proceed?",
                "User intercepted the file operation", filepath
            );
            if (!errors.empty()) return errors;

            if (!file_put_contents(filepath, content, append))
                return "File write failed: " + filepath; 

            // if (!append) {
            //     try {
            //         chgrp(filepath, group);
            //         chprm(filepath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
            //     } catch (exception &e) {
            //         return "File write error: " + string(e.what());
            //     }
            // }

            string outp = "File writen: " + filepath;
            cout << outp << "\nContents:\n" + (content.empty() ? "<empty>" : content) << endl;
            return outp;
        }

        string remove(void* user_void, const JSON& args, const JSON& conf) {
            NULLCHK(user_void);
            error_to_user(get_required_error<string>(conf, "base_folder"));
            string base = conf.get<string>("base_folder");
            
            string errors = get_required_error<string>(args, "filename");
            if (!errors.empty()) return errors;
            string filepath = base + args.get<string>("filename");
            if (!is_valid_filepath(filepath)) return "Filename is invalid: " + filepath;
            if (!file_exists(filepath)) return "File not exists: " + filepath;

            errors += get_user_confirm_error(
                [&](const string& prmpt) { 
                    return ((User*)user_void)->confirm(prmpt); 
                }, 
                    string("File manager tool wants to delete file:") + filepath 
                    + "\nDo you want to proceed?",
                "User intercepted the file operation", filepath
            );
            if (!errors.empty()) return errors;

            if (!tools::remove(filepath, false))
                return "File remove failed: " + filepath; 

            string ret = "File removed: " + filepath;
            cout << ret << endl;
            return ret;
        }

        string rename(void* user_void, const JSON& args, const JSON& conf) {
            NULLCHK(user_void);
            error_to_user(get_required_error<string>(conf, "base_folder"));
            string base = conf.get<string>("base_folder");
            
            string errors = get_required_errors<string>(args, { "old_filename", "new_filename" });
            if (!errors.empty()) return errors;
            string old_filepath = base + args.get<string>("old_filename");
            if (!is_valid_filepath(old_filepath)) return "Old filename is invalid: " + old_filepath;
            string new_filepath = base + args.get<string>("new_filename");
            if (!is_valid_filepath(new_filepath)) return "New filename is invalid :" + new_filepath;
            if (old_filepath == new_filepath) return "Files can not be the same: " + old_filepath;
            if (!file_exists(old_filepath)) return "File not exists: " + old_filepath;
            if (file_exists(new_filepath)) return "File already exists: " + new_filepath + "\nYou have to delete first!";

            string to = old_filepath + " to " + new_filepath;
            errors += get_user_confirm_error(
                [&](const string& prmpt) { 
                    return ((User*)user_void)->confirm(prmpt); 
                }, 
                    string("File manager tool wants to rename file: ") + to
                    + "\nDo you want to proceed?",
                "User intercepted the file operation", to
            );
            if (!errors.empty()) return errors;

            if (!tools::rename(old_filepath, new_filepath, false))
                return "File rename failed: " + to; 

            string ret = "File renamed: " + to;
            cout << ret << endl;
            return ret;
        }

        string view(void* user_void, const JSON& args, const JSON& conf) {
            error_to_user(get_required_error<string>(conf, "base_folder"));
            string base = conf.get<string>("base_folder");
            string errors = get_required_error<string>(args, "filename");
            if (!errors.empty()) return errors;
            string filepath = base + args.get<string>("filename");
            if (!is_valid_filepath(filepath)) return "Filename is invalid: " + filepath;
            if (!file_exists(filepath)) return "File not exists: " + filepath;
            vector<string> lines;
            try {
                lines = explode("\n", file_get_contents(filepath));
            } catch (ios_base::failure &e) {
                return "IO operation failed on file read '" + filepath + "': " + e.what();
            }
            if (lines.empty()) return "File '" + filepath + "' is empty.";
            int start_line = args.has("start_line") ? args.get<int>("start_line") - 1 : 0;
            int end_line = args.has("end_line") ? args.get<int>("end_line") - 1 : lines.size();
            if (start_line < 0) return "Start line can not be less than 1!";
            if (end_line < 0) return "End line can not be less than 1!";
            string outp = "";
            for (int i = start_line; i < end_line; i++)
                outp += ::to_string(i+1) + "|" + lines[i] + "\n";
            outp = "Contents from file: " + filepath + "\n" + (outp.empty() ? "<empty>" : outp) + "\n";
            cout << outp << flush;
            return outp;
        }

        string edit(void* user_void, const JSON& args, const JSON& conf) {
            error_to_user(get_required_error<string>(conf, "base_folder"));
            string base = conf.get<string>("base_folder");
            string errors = get_required_errors<string>(args, { "filename", "content" });
            if (!errors.empty()) return errors;
            string filepath = base + args.get<string>("filename");
            if (!is_valid_filepath(filepath)) return "Filename is invalid: " + filepath;
            string content = args.get<string>("content"); 
            if (!file_exists(filepath)) return "File not exists: " + filepath;
            vector<string> lines;
            try {
                lines = explode("\n", file_get_contents(filepath));
            } catch (ios_base::failure &e) {
                return "IO operation failed on file read '" + filepath + "': " + e.what();
            }
            
            errors += get_user_confirm_error(
                [&](const string& prmpt) { 
                    return ((User*)user_void)->confirm(prmpt); 
                }, 
                    string("File manager tool wants to replace in file: ") + filepath
                    + "\nDo you want to proceed?",
                "User intercepted the file operation", filepath
            );
            if (!errors.empty()) return errors;

            int start_line = args.has("start_line") ? args.get<int>("start_line") - 1 : 0;
            int end_line = args.has("end_line") ? args.get<int>("end_line") - 1 : lines.size();
            if (start_line < 0) return "Start line can not be less than 1!";
            if (end_line < 0) return "End line can not be less than 1!";
            vector<string> before;
            for (int i = 0; i < start_line; i++)
                before.push_back(lines[i]);
            vector<string> after;
            for (int i = end_line; i < lines.size(); i++)
                after.push_back(lines[i]);
            string new_content = implode("\n", before) + (content.empty() ? "" : "\n" + content + "\n") + implode("\n", after);
            if (!file_put_contents(filepath, content))
                return "File content modification failed: " + filepath; 

            string ret = "File updated: " + filepath;
            cout << ret + "\nNew contents:\n" + (new_content.empty() ? "<empty>" : new_content) << endl;
            return ret;
        }

        string exec(void* user_void, const JSON& args, const JSON& conf) {
            error_to_user(get_required_error<string>(conf, "base_folder"));
            string base = conf.get<string>("base_folder");

            string errors = get_required_error<string>(args, { "command" });
            if (!errors.empty()) return errors;

            string command = args.get<string>("command");
            int timeout = args.has("timeout") ? args.get<int>("timeout") : 10;
            
            errors += get_user_confirm_error(
                [&](const string& prmpt) { 
                    return ((User*)user_void)->confirm(prmpt); 
                }, // solve tools::to_string(..) conflict
                    string("File manager tool wants to execute a command with timeout ") + ::to_string(timeout) + "s:\n" + command
                    + "\nDo you want to proceed?",
                "User intercepted the command execution", command
            );
            if (!errors.empty()) return errors;
            
            auto start_time = chrono::steady_clock::now();
            string output = "";
            try {
                string full_command = "cd " + base + " && " + (timeout ? string("timeout ") + ::to_string(timeout) + "s " : "") + command + " 2>&1";
                output = execute(full_command.c_str());
            } catch (const runtime_error& e) {
                output = "Command execution failed: " + string(e.what());
            }
            auto current_time = chrono::steady_clock::now();
            auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(current_time - start_time).count();

            // Display and return results
            string elapsed = "Command execution time was: " + tools::to_string(elapsed_time) + "ms";
            cout << "\nOutput:\n" << output + "\n" + elapsed << endl;
            return 
                "Results from command execution:\n" + trim(command) + "\n" + elapsed +
                "\nOutput:\n" + (output.empty() ? "<empty>" : output);
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

        static string remove_cb(void* tool_void, void* model_void, void* user_void, const JSON& args, const JSON& conf) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->remove(user_void, args, conf);
        }

        static string rename_cb(void* tool_void, void* model_void, void* user_void, const JSON& args, const JSON& conf) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->rename(user_void, args, conf);
        }

        static string view_cb(void* tool_void, void* model_void, void* user_void, const JSON& args, const JSON& conf) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->view(user_void, args, conf);
        }

        static string edit_cb(void* tool_void, void* model_void, void* user_void, const JSON& args, const JSON& conf) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->edit(user_void, args, conf);
        }

        static string exec_cb(void* tool_void, void* model_void, void* user_void, const JSON& args, const JSON& conf) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->exec(user_void, args, conf);
        }

    } fileManagerTool;

    map<string, tool_cb> FileManagerTool::action_map = {
        { "create", FileManagerTool::create_cb },
        { "append", FileManagerTool::append_cb },
        { "remove", FileManagerTool::remove_cb },
        { "rename", FileManagerTool::rename_cb },
        { "view", FileManagerTool::view_cb },
        { "edit", FileManagerTool::edit_cb },
        { "exec", FileManagerTool::exec_cb },
    };


    // TODO 2.  **Bug Reporting/Feedback Tool (with Future Request Integration):** (Crucial for improvement and long-term planning)
    // TODO 3.  **Note-Taking/Memory Tool:** (Important for efficiency and avoiding repeated work) - notes/per AI AND/OR "global" notes for each AI?; categories for notes?
    // TODO 4.  **Clarifying Question Prompt:** I'll try to use existing functions and just ask directly. If I still fail to do it, it will be one function for me to ask you some questions before I even start with a task. (later when we have multiple AI but any of them need to ask a question for the subtask, they can send a message to the user directly "bypassing" its parent AI)
}