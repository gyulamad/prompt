#pragma once

#include "../../../utils/ERROR.hpp"    // Ensure ERROR macro is available if needed by internal methods
#include "../../AgentRoleMap.hpp"
#include "PersistenceCommand.hpp" // Include the base class

// Keep other necessary includes like string, vector, Agency, Agent if needed by internal methods

using namespace std;
using namespace tools::cmd;
using namespace tools::agency;
using namespace tools::agency::agents;
using namespace tools::utils; // For ERROR

namespace tools::agency::agents::commands {

    template<typename T>
    class LoadCommand: public PersistenceCommand<T> {
    public:
        // Constructor calling the base class constructor
        LoadCommand(const string& prefix, AgentRoleMap& roles): 
            PersistenceCommand<T>(prefix, PersistenceCommand<T>::LOAD), roles(roles) {}
        virtual ~LoadCommand() {}

        void run(void* worker_void, const vector<string>& args) override {
            Worker<T>& worker = *safe((Worker<T>*)worker_void);
            Agency<T>& agency = *safe((Agency<T>*)worker.getAgencyPtr());

            string typeName = args[1]; // "agent" or "agency"
            string filename = args[2]; // agentName or agencyName
            // string filename = (args.size() > 3) ? args[3] : thingName;

            if (!file_exists(filename)) filename = filename + ".json";
            if (!file_exists(filename)) throw ERROR("File not found: " + filename);

            performAction(agency, this->getType(typeName), filename);
        }

    protected:

        void performAction(Agency<T>& agency, PersistenceCommand<T>::Type type, const string& filename) {
            switch (type) {
                
                case PersistenceCommand<T>::AGENT:
                    loadAgent(agency, filename);
                    break;
                
                case PersistenceCommand<T>::AGENCY:
                    loadAgency(agency, filename);
                    break;

                default:
                    throw ERROR("Couldn't save, invalid type given.");
            }
        }

    private:    // TODO: !@# Load/save name/filename inconsistency, it should work like:
                //  /save => name + (filename - optional, comes from name + ".json" if not set)
                //  /load => filename + (name - optional, comes from the file data if not set)
                // - in both cases if file not found by name it should retry with filename + ".json" if the extension is not set alread
                // + help usages in the parent class have to be updatede

        void loadAgent(Agency<T>& /*agency*/, const string& filename) {
            
            JSON json(file_get_contents(filename));

            // typename PersistenceCommand<T>::Type type = this->getType(json.get<string>("type"));
            // if (type != PersistenceCommand<T>::AGENT)
            //     throw ERROR("Agent type missmatch: " + json.get<string>("type"));

            // typename PersistenceCommand<T>::Type type = PersistenceCommand<T>::AGENT;

            string role = json.get<string>("role");
            if (!array_key_exists(role, roles))
                throw ERROR("Role type does not exist: '" + role + "'");

            // vector<string> recipients = json.get<vector<string>>("recipients");

            // Agent<T>& agent = 
            // roles[role](json);
            // agent.fromJSON(json);
            roles[role](json.get<string>("name"), json);
        }

        void loadAgency(Agency<T>&/*agency*/, const string& /*filename*/) {
            STUB("implement this!");
            // ((JSONSerializable&)agency)
            //     .fromJSON(JSON(file_get_contents(filename)));
        }

        AgentRoleMap& roles;

    };

}
