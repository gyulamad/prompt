#pragma once

#include <string>

using namespace std;

namespace tools::agency::agents::plugins {

    typedef enum { PARAMETER_TYPE_INTEGER, PARAMETER_TYPE_STRING } parameter_type_t;

    string to_string(parameter_type_t type) {
        switch (type) {
            case PARAMETER_TYPE_INTEGER:
                return "integer";
            case PARAMETER_TYPE_STRING:
                return "string";
            default:
                throw ERROR("Invalid parameter type");
        }
    }

    class Parameter {
    private:
        string name;
        parameter_type_t type;
        bool required;
        string rules;
    public:
        Parameter(
            const string& name, 
            parameter_type_t type, 
            bool required = false, 
            const string& rules = ""
        ): 
            name(name),
            type(type),
            required(required),
            rules(rules)
        {}

        virtual ~Parameter() {}

        string get_name() const { return name; }
        parameter_type_t get_type() const { return type; }
        bool is_required() const { return required; }
        string get_rules() const { return rules; }

    };


    string to_string(const Parameter& parameter) {
        return 
            "Parameter name: " + parameter.get_name() + "\n" + 
            "Type: " + to_string(parameter.get_type()) + "\n" + 
            "Required: " + (parameter.is_required() ? "Yes" : "No") + (
                parameter.get_rules().empty() ? "" : ("\nRules: " + parameter.get_rules())
            );
    }

    string to_string(const vector<Parameter>& parameters) {
        vector<string> results;
        for (const Parameter& parameter: parameters)
            results.push_back(to_string(parameter));
        return implode("\n", results);
    }

}