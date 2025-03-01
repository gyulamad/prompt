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