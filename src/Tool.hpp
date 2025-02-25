#pragma once

#include <string>
#include <vector>

#include "tools/strings.hpp"
#include "tools/JSON.hpp"
#include "tools/system.hpp"

#include "Parameter.hpp"

using namespace std;
using namespace tools;

namespace prompt {

    typedef function<string(void* tool_void, void* model_void, void* user_void, const JSON& args, const JSON& conf)> tool_cb;
   
    class Tool {
    private:
        string name;
        vector<Parameter> parameters;
        string description;
        tool_cb callback;

    protected:
        void error_to_user(const string& errmsg) {
            if (errmsg.empty()) return;
            throw ERROR("Error in tool '" + name + "': " + errmsg);
        }

    public:
        
        Tool(
            const string& name,
            const vector<Parameter>& parameters,
            tool_cb callback,
            const string& description = ""
        ):
            name(name),
            parameters(parameters),
            callback(callback),
            description(description)
        {}

        string get_name() const { return name; }
        string get_description() const { return description; }
        const vector<Parameter>& get_parameters_cref() const { return parameters; }

        string call(void* model_void, void* user_void, const JSON& args, const JSON& conf) {
            return callback(this, model_void, user_void, args, conf);
        };

        template<typename T> // TODO: to JSON
        static string get_required_error(const JSON& args, const string& key) {
            if (!args.has(key)) return "Parameter '" + key + "' is missing!";
            T value = args.get<T>(key);
            if (value.empty()) return "Parameter '" + key + "' can not be empty!";
            return "";
        }

        template<typename T> // TODO: to JSON
        static string get_required_errors(const JSON& args, const vector<string>& keys) {
            string error;
            vector<string> errors;
            for (const string& key: keys) {
                error = get_required_error<T>(args, key);
                if (!error.empty()) errors.push_back(error);
            }
            return errors.empty() ? "" : ("Invalid parameter(s):" + implode("\n", errors));
        }

        typedef function<bool(const string& message)> confirm_func_t;

        static string get_user_confirm_error(
            confirm_func_t confirm_func, 
            const string& question, 
            const string& errmsg,
            const string& expln,
            bool show = true
        ) {
            if (confirm_func(question)) return "";
            if (show) cout << errmsg + "." << endl;
            return errmsg + (expln.empty() ? "." : (":\n" + expln));
        }

        static string confirm_execute_ssh(confirm_func_t confirm_func, const JSON& conf, const string& reason, const string& command, int timeout = 10) {
            string error = get_user_confirm_error(
                confirm_func, 
                "The following bash command is about to executed with a time window of " + tools::to_string(timeout) + "s:\n" 
                    + command + "\n" + (reason.empty() ? "" : ("Reason: " + reason)) + "\nDo you want to proceed?",
                "User intercepted the command execution", command
            );
            if (!error.empty()) return error;

            string ssh_user = conf.get<string>("ssh_user");
            string ssh_host = conf.get<string>("ssh_host");
            string ssh_command = "timeout " + ::to_string(timeout) + "s ssh " + ssh_user + '@' + ssh_host 
                + " " + quote_cmd(command + " 2>&1");

            auto start_time = chrono::steady_clock::now();
            string output = "";
            try {
                output = execute(ssh_command.c_str());
            } catch (const runtime_error& e) {
                output = "Error: " + string(e.what());
            }
            auto current_time = chrono::steady_clock::now();
            auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(current_time - start_time).count();
            

            string elapsed = "Command execution time was: " + tools::to_string(elapsed_time) + "ms";
            cout << "\nOutput:\n" << output + "\n" + elapsed << endl;
            return 
                "Results from command execution:\n" + trim(command) + "\n" + elapsed +
                "\nOutput:\n" + (output.empty() ? "<empty>" : output);
        }
    };
    
    string to_string(const Tool& tool) {
        return "Function name: " + tool.get_name()
            + (tool.get_description().empty() ? "" : ("\nDescription: " + tool.get_description()))
            + (tool.get_parameters_cref().empty() ? "" : ("\nParameters:\n" + to_string(tool.get_parameters_cref())));
    }


    string to_string(const vector<Tool>& tools) {
        vector<string> results;
        for (const Tool& tool: tools) 
            results.push_back(to_string(tool));
        return implode("\n\n", results);
    }

}