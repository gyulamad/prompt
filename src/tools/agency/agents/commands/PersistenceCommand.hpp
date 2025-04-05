#pragma once

#include <string>
#include <vector>
#include <map>
#include <stdexcept> // For std::runtime_error

#include "../../../cmd/Command.hpp"
#include "../../../cmd/Usage.hpp"
#include "../../../cmd/Parameter.hpp"
#include "../../../utils/ERROR.hpp" // For NULLCHK and ERROR macros
#include "../../../str/implode.hpp" // For implode
#include "../../../str/toLower.hpp"
#include "../../../str/toUpperFirst.hpp"
#include "../../../utils/foreach.hpp"
#include "../../../containers/array_key_exists.hpp"
#include "../../Agency.hpp"
#include "../../Agent.hpp"

using namespace std;
using namespace tools::cmd;
using namespace tools::utils;
using namespace tools::str; // For implode
using namespace tools::containers;
// using namespace tools::agency;

namespace tools::agency::agents::commands {

    template<typename T>
    class PersistenceCommand : public Command {
    protected:
        string commandName;

        string actionVerbCapitalized;
        string actionVerbLowercase;

        string filenameParamName;
        string filenameParamDesc;


        // ===============================================================
        // Register types here...

        enum Type { AGENT, AGENCY };

        map<Type, string> typeNameMap = {
            { AGENT, "agent" },
            { AGENCY, "agency" },
        };

        const vector<Type> types = array_keys(typeNameMap);
        const vector<string> typeNames = getTypeNames();

        // ===============================================================

    public:
        PersistenceCommand(
            const string& commandName,
            const string& actionVerb,
            const string& filenameParamName = "filename", // Default value
            const string& filenameParamDesc = "file"      // Default value
        ): 
            commandName(commandName),
            actionVerbCapitalized(toUpperFirst(actionVerb)),
            actionVerbLowercase(toLower(actionVerb)),
            filenameParamName(filenameParamName),
            filenameParamDesc(filenameParamDesc)
        {}

        vector<string> getPatterns() const override {
            vector<string> patterns;
            foreach (typeNameMap, [&](const string& typeName) {
                patterns.push_back("/" + commandName + " " + typeName + " {string} [{string}]");
            });
            return patterns;
        }

        string getUsage() const override {
            string defaultFilenameNote = "If " + filenameParamName + " is not provided, the agent or agency will be " + actionVerbLowercase + "d from/to a file named after the agent or agency name.";

            vector<pair<string, string>> examples;
            foreach (typeNameMap, [&](const string& typeName) {
                examples.push_back(make_pair(
                    "/" + commandName + " " + typeName + " my_" + typeName,
                    actionVerbCapitalized + " the " + typeName + " 'my_" + typeName + "' from/to file 'my_" + typeName + "'"
                ));
                examples.push_back(make_pair(
                    "/" + commandName + " " + typeName + " my_" + typeName + " my_" + typeName + "_file",
                    actionVerbCapitalized + " the " + typeName + " 'my_" + typeName + "' from/to file '" + filenameParamName + "'"
                ));
            });

            return implode("\n", vector<string>({
                Usage({
                    string("/" + commandName), // command
                    string(actionVerbCapitalized + " an " + implode(" or ", typeNames) + " from/to a file."), // help
                    vector<Parameter>({ // parameters
                        {
                            string("name"), // name
                            bool(false), // optional
                            string("Name of the " + implode(" or ", typeNames) + " to " + actionVerbLowercase + ".") // help
                        },
                        {
                            filenameParamName, // name
                            bool(true), // optional
                            string("Name of the " + filenameParamDesc + ". Defaults to " + implode(" or ", typeNames) + " name.") // help
                        }
                    }),
                    examples, // examples
                    vector<string>({ // notes
                        defaultFilenameNote
                    })
                }).to_string()
            }));
        }

        const vector<string>& validate(const vector<string>& args) override {
            // args[0] is the command itself (e.g., "/load")
            // args[1] is the type (e.g., "agent" or "agency")
            // args[2] is the name (e.g., agent/agency name)
            // args[3] is the optional filename
            if (args.size() < 3) {
                 // Provide more specific usage based on the actual command invoked if possible
                string specific_pattern = "/" + commandName + " " + implode("|", typeNames) + " {name} [filename]";
                throw ERROR("Usage: " + specific_pattern);
            }
            return args;
        }

    protected:

        void run(void* thing_void, const vector<string>& args) override {
            NULLCHK(thing_void);

            string typeName = args[1]; // "agent" or "agency"
            string thingName = args[2]; // agentName or agencyName
            // Keep the .json default for now as discussed
            string filename = (args.size() > 3) ? args[3] : thingName + ".json";

            performAction(thing_void, getType(typeName), thingName, filename);
        }

        virtual void performAction(void* thing, Type type, const string& name, const string& filename) = 0;

        // Virtual destructor is important for base classes with virtual functions
        virtual ~PersistenceCommand() = default;

    private:

        vector<string> getTypeNames() {
            vector<string> typeNames;
            // if (!typeNames.empty()) return typeNames;
            for (Type type: types) 
            // foreach (types, [&](Type type) {
                typeNames.push_back(getTypeName(type));
            // });
            return typeNames;
        }

        string getTypeName(Type type) {
            if (!array_key_exists(type, typeNameMap)) 
                throw getInvalidTypeError(to_string((int)type));
            return typeNameMap[type];
        }
        
        Type getType(const string& typeName) {
            bool found = false;
            Type type;
            foreach (typeNameMap, [&](const string& name, Type typ) {
                if (typeName == name) {
                    found = true;
                    type = typ;
                    return FE_BREAK;
                }
                return FE_CONTINUE;
            });
            if (found) return type;
            throw ERROR("Invalid type: " + typeName);
        }

        runtime_error getInvalidTypeError(string typeName) {
            return ERROR("Invalid type given (" + typeName + ").  possible types are: " + implode("|", getTypeNames()));
        }

    };

}
