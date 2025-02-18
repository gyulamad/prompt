#pragma once

#include <string>
#include <vector>

#include "tools/strings.hpp"
#include "tools/JSON.hpp"

#include "Parameter.hpp"
#include "Tool.hpp"

using namespace std;
using namespace tools;

namespace prompt {
   
    class Tool {
    private:
        string name;
        vector<Parameter> parameters;
        string description;
        function<string(void* tool_void, void* model_void, void* user_void, const JSON& args)> callback;

    public:
        
        Tool(
            const string& name,
            const vector<Parameter>& parameters,
            function<string(void* tool_void, void* model_void, void* user_void, const JSON& args)> callback,
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

        string call(void* model_void, void* user_void, const JSON& args) {
            return callback(this, model_void, user_void, args);
        };
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