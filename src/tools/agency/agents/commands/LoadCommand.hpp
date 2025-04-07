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

    protected:

        void performAction(Agency<T>& agency, PersistenceCommand<T>::Type type, const string& name, const string& filename) override {
            switch (type) {
                
                case PersistenceCommand<T>::AGENT:
                    loadAgent(agency, name, filename);
                    break;
                
                case PersistenceCommand<T>::AGENCY:
                    loadAgency(agency, name, filename);
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

        void loadAgent(Agency<T>& /*agency*/, const string& name, const string& filename) {
            
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
            roles[role](name, json);
        }

        void loadAgency(Agency<T>&/*agency*/, const string& /*name*/, const string& /*filename*/) {
            STUB("implement this!");
            // ((JSONSerializable&)agency)
            //     .fromJSON(JSON(file_get_contents(filename)));
        }

        AgentRoleMap& roles;

    };

}
