#pragma once

#include <string>
#include <vector>

#include "../str/implode.hpp"

#include "Parameter.hpp"

using namespace std;
using namespace tools::str;

namespace tools::cmd {

    struct Usage {
        string command;
        string help;
        vector<Parameter> parameters;
        vector<pair<string, string>> examples; // code and comment
        vector<string> notes;

        string to_string() const {
            vector<string> prms, hlps, exmpls, nts;
            for (const Parameter& parameter: parameters) {
                prms.push_back(
                    parameter.optional 
                        ? "[" + parameter.name + "]" 
                        : "<" + parameter.name + ">"
                );
                hlps.push_back(
                    "  " + parameter.name 
                    + "\t - " + (parameter.optional ? "(optional) " : " ") + parameter.help
                );
            }
            for (const pair<string, string>& example: examples) exmpls.push_back(example.first + "\t# " + example.second);
            for (const string& note: notes) nts.push_back(note);

            return command + (prms.empty() ? "" : (" " + implode(" ", prms)))
                + "\n" + help 
                + "\nParameters:" + (hlps.empty() ? " (none)" : ("\n  " + implode("\n  ", hlps)))
                + (exmpls.empty() ? "" : ("\nExamples:\n  - " + implode("\n  - ", exmpls)))
                + (nts.empty() ? "" : ("\nNotes:\n  - " + implode("\n  - ", nts)))
            ;
        }
    };

}