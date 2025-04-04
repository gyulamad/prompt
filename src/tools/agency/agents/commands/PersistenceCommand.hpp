#pragma once

#include <string>
#include <vector>
#include <stdexcept> // For std::runtime_error

#include "../../../cmd/Command.hpp"
#include "../../../cmd/Usage.hpp"
#include "../../../cmd/Parameter.hpp"
#include "../../Agency.hpp" // TODO: Agent and Agency is not predeterminated and could be extended for more types, I don't think we need these here
#include "../../Agent.hpp" // Assuming Agent might be needed indirectly or by derived classes
#include "../../../utils/ERROR.hpp" // For NULLCHK and ERROR macros
#include "../../../str/implode.hpp" // For implode

using namespace std;
using namespace tools::cmd;
using namespace tools::agency;
using namespace tools::str; // For implode

namespace tools::agency::agents::commands {

    template<typename T>
    class PersistenceCommand : public Command {
    protected:
        string commandName;

        // TODO: I think action verbs can be converted to lover case and capitalized automatically for more convinience to the caller, note: we have php-like implementation for "generic" functions in the src/tools/str/ folder, if the php-like version does not exists, feel free to add it there, you can use other commands as examples.
        string actionVerbCapitalized; // e.g., "Loads", "Saves"
        string actionVerbLowercase;   // e.g., "load", "save"

        string filenameParamName;     // e.g., "input-name", "output-name"
        string filenameParamDesc;     // e.g., "input file", "output file"

        // Pure virtual methods to be implemented by derived classes
        virtual void performAgentAction(Agency<T>& agency, const string& agentName, const string& filename) = 0;
        virtual void performAgencyAction(Agency<T>& agency, const string& agencyName, const string& filename) = 0;

    public:
        PersistenceCommand(
            const string& commandName,
            const string& actionVerbCapitalized,
            const string& actionVerbLowercase,

            // TODO I think input/output file/name can have default values passed here as these are almost always the same and the caller may oke to use the default version or overrides
            const string& filenameParamName,
            const string& filenameParamDesc

        ) : commandName(commandName),
            actionVerbCapitalized(actionVerbCapitalized),
            actionVerbLowercase(actionVerbLowercase),
            filenameParamName(filenameParamName),
            filenameParamDesc(filenameParamDesc) {}

        vector<string> getPatterns() const override {
            return { // TODO: I think agent/agency etc can be passed as parameter to this class
                "/" + commandName + " agent {string} [{string}]",
                "/" + commandName + " agency {string} [{string}]"
            };
        }

        string getUsage() const override {
            
            // TODO: you can be mode generic with these strings here by applying what I just wrote about by passing the type names as a parameter to the class
            string defaultFilenameNote = "If " + filenameParamName + " is not provided, the agent or agency will be " + actionVerbLowercase + "d from/to a file named after the agent or agency name.";
            string agentExampleDefault = "/" + commandName + " agent my_agent";
            string agentExampleDefaultDesc = actionVerbCapitalized + " the agent 'my_agent' from/to file 'my_agent'";
            string agentExampleSpecific = "/" + commandName + " agent my_agent my_agent_file";
            string agentExampleSpecificDesc = actionVerbCapitalized + " the agent 'my_agent' from/to file 'my_agent_file'";
            string agencyExampleDefault = "/" + commandName + " agency my_agency";
            string agencyExampleDefaultDesc = actionVerbCapitalized + " the agency 'my_agency' from/to file 'my_agency'";
            string agencyExampleSpecific = "/" + commandName + " agency my_agency my_agency_file";
            string agencyExampleSpecificDesc = actionVerbCapitalized + " the agency 'my_agency' from/to file 'my_agency_file'";


            return implode("\n", vector<string>({
                Usage({
                    string("/" + commandName), // command
                    string(actionVerbCapitalized + " an agent or agency from/to a file."), // help
                    vector<Parameter>({ // parameters
                        {
                            string("name"), // name
                            bool(false), // optional
                            string("Name of the agent or agency to " + actionVerbLowercase + ".") // help
                        },
                        {
                            filenameParamName, // name
                            bool(true), // optional
                            string("Name of the " + filenameParamDesc + ". Defaults to agent/agency name.") // help
                        }
                    }),
                    vector<pair<string, string>>({ // examples
                        make_pair(agentExampleDefault, agentExampleDefaultDesc),
                        make_pair(agentExampleSpecific, agentExampleSpecificDesc),
                        make_pair(agencyExampleDefault, agencyExampleDefaultDesc),
                        make_pair(agencyExampleSpecific, agencyExampleSpecificDesc)
                    }),
                    vector<string>({ // notes
                        defaultFilenameNote
                    })
                }).to_string()
            }));
        }

        template<typename ThingT>
        void run(void* thing_void, const vector<string>& args) override {
            NULLCHK(thing_void);
            ThingT<T>& thing = *(ThingT<T>*)thing_void;

            // args[0] is the command itself (e.g., "/load")
            // args[1] is the type ("agent" or "agency")
            // args[2] is the name (agent/agency name)
            // args[3] is the optional filename
            if (args.size() < 3) {
                 // Provide more specific usage based on the actual command invoked if possible
                string specific_pattern = "/" + commandName + " agent|agency {name} [filename]";
                throw ERROR("Usage: " + specific_pattern);
            }

            string type = args[1]; // "agent" or "agency"
            string name = args[2]; // agentName or agencyName
            // Keep the .json default for now as discussed
            string filename = (args.size() > 3) ? args[3] : name + ".json";

            // TODO: we can make this as a switch..case based on the given type names
            if (type == "agent") {
                // Perform validation specific to agent if needed (e.g., check if agent exists before saving?)
                // Note: Load might create the agent, Save might check existence.
                // This specific validation might need to stay in derived classes or be passed via callbacks/flags if made generic.
                // For now, calling the specific action directly.
                performAgentAction(thing, name, filename);
            } else if (type == "agency") {
                // Perform validation specific to agency if needed
                performAgencyAction(thing, name, filename);
            } else {
                throw ERROR("Invalid type: '" + type + "'. Must be 'agent' or 'agency'.");
            }
        }

        // Virtual destructor is important for base classes with virtual functions
        virtual ~PersistenceCommand() = default;
    };

}
