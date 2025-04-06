#pragma once

#include <string>
#include <vector>
#include <map>

#include "../../../cmd/Command.hpp"
#include "../../../cmd/Usage.hpp"
#include "../../../cmd/Parameter.hpp"
#include "../../../utils/ERROR.hpp" // For NULLCHK and ERROR macros
#include "../../../str/implode.hpp" // For implode
#include "../../../str/strtolower.hpp"
#include "../../../str/ucfirst.hpp"
#include "../../../utils/foreach.hpp"
#include "../../../containers/array_key_exists.hpp"
// #include "../../Agency.hpp"
// #include "../../Agent.hpp"

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
        enum Persistor { LOAD, SAVE };

        Persistor persistor;

        string filenameParamName;
        string filenameParamDesc;


        // ===============================================================
        // Register types here...

        enum Type { AGENT, AGENCY };

        const map<Type, string> typeNameMap = {
            { AGENT, "agent" },
            { AGENCY, "agency" },
        };

        const vector<Type> types = array_keys(typeNameMap);
        const vector<string> typeNames = getTypeNames();

        // ===============================================================



        struct PersistorNameWord {
            string commandName;
            string directorWord;
        };

        const map<Persistor, string> commandNameMap = {
            { LOAD , "load" },
            { SAVE , "save" },
        };

        const map<Persistor, string> directorWordMap = {
            { LOAD , "from" },
            { SAVE , "to" },
        };

    public:

        PersistenceCommand(
            const string& prefix,
            Persistor persistor,
            const string& filenameParamName = "filename", // Default value
            const string& filenameParamDesc = "file"      // Default value
        ): 
            Command(prefix),
            persistor(persistor),
            filenameParamName(filenameParamName),
            filenameParamDesc(filenameParamDesc)
        {}
        
        virtual ~PersistenceCommand() {}

        vector<string> getPatterns() const override {
            vector<string> patterns;
            foreach (typeNameMap, [&](const string& typeName) {
                patterns.push_back(getName() + " " + typeName + " {string}");
                patterns.push_back(getName() + " " + typeName + " {string} [{string}]");
            });
            return patterns;
        }

        string getName() const override {
            return this->prefix + getCommandName(persistor);
        }

        string getDescription() const override {
            return getActionVerbCapitalized(persistor) + " an " 
                + implode(" or ", typeNames) + " " 
                + getDirectorWord(persistor) 
                + " a " + filenameParamDesc + ".";
        }

        string getUsage() const override {
            string defaultFilenameNote = 
                "If " + filenameParamName + " is not provided, it will " 
                    + getActionVerbLowercase(persistor) + " the " + implode(" or ", typeNames) 
                    + " " + getDirectorWord(persistor) + " a " + filenameParamDesc 
                    + " named after the " + implode(" or ", typeNames) + " name.";

            vector<pair<string, string>> examples;
            foreach (typeNameMap, [&](const string& typeName) {
                examples.push_back(make_pair(
                    getName() + " " + typeName + " my_" + typeName,
                    getActionVerbCapitalized(persistor) + " the " + typeName + " 'my_" 
                        + typeName + "' " + getDirectorWord(persistor) + " " 
                        + filenameParamDesc + " 'my_" + typeName + "'"
                ));
                examples.push_back(make_pair(
                    getName() + " " + typeName + " my_" + typeName 
                        + " my_" + typeName + "_file",
                    getActionVerbCapitalized(persistor) + " the " + typeName + " 'my_" 
                        + typeName + "' " + getDirectorWord(persistor) + " " 
                        + filenameParamDesc + " '" + filenameParamName + "'"
                ));
            });

            return implode("\n", vector<string>({
                Usage({
                    getName(), // string("/" + getCommandName(persistor)), // command
                    getDescription(),
                    // string(
                    //     getActionVerbCapitalized(persistor) + " an " 
                    //     + implode(" or ", typeNames) + " " 
                    //     + getDirectorWord(persistor) 
                    //     + " a " + filenameParamDesc + "."
                    // ), // help
                    vector<Parameter>({ // parameters
                        {
                            string("type"), // name
                            bool(false), // optional
                            string(
                                "Type of the " + implode(" or ", typeNames) 
                                + " to " + getActionVerbLowercase(persistor) + "."
                            ) // help
                        },
                        {
                            string("name"), // name
                            bool(false), // optional
                            string(
                                "Name of the " + implode(" or ", typeNames) 
                                + " to " + getActionVerbLowercase(persistor) + "."
                            ) // help
                        },
                        {
                            filenameParamName, // name
                            bool(true), // optional
                            string(
                                "Name of the " + filenameParamDesc 
                                + ". Defaults to " + implode(" or ", typeNames) 
                                + " name."
                            ) // help
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
                throw ERROR(
                    string("Usage: ") + getName() + " " 
                    + implode("|", typeNames) + " {name} [filename]"
                );
            }
            return args;
        }

    protected:

        void run(void* agency_void, const vector<string>& args) override {
            NULLCHK(agency_void);

            string typeName = args[1]; // "agent" or "agency"
            string thingName = args[2]; // agentName or agencyName
            string filename = (args.size() > 3) ? args[3] : thingName;

            performAction(*(Agency<T>*)agency_void, getType(typeName), thingName, filename);
        }

        virtual void performAction(Agency<T>& agency, Type type, const string& name, const string& filename) = 0;


        vector<string> getTypeNames() const {
            vector<string> typeNames;
            for (Type type: types) 
                typeNames.push_back(getTypeName(type));
            return typeNames;
        }

        string getTypeName(Type type) const {
            if (!array_key_exists(type, typeNameMap)) 
                throw getInvalidTypeError(to_string((int)type));
            return typeNameMap.at(type);
        }
        
        Type getType(const string& typeName) const {
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
            throw getInvalidTypeError(typeName);
        }

        runtime_error getInvalidTypeError(string typeName) const {
            return ERROR(
                "Invalid type given (" + typeName + ").  possible types are: " 
                + implode("|", getTypeNames())
            );
        }


        string getCommandName(Persistor persistor) const {
            if (!array_key_exists(persistor, commandNameMap))
                throw getInvalidPersistorError(persistor);
            return commandNameMap.at(persistor);
        }

        string getDirectorWord(Persistor persistor) const {
            if (!array_key_exists(persistor, directorWordMap))
                throw getInvalidPersistorError(persistor);
            return directorWordMap.at(persistor);
        }

        runtime_error getInvalidPersistorError(Persistor persistor) const {
            return ERROR("Invalid persistor: " + to_string((int)persistor));
        }


        string getActionVerbCapitalized(Persistor persistor) const {
            return ucfirst(getCommandName(persistor));
        }

        string getActionVerbLowercase(Persistor persistor) const {
            return strtolower(getCommandName(persistor));
        }
    };

}
