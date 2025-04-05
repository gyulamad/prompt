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
        LoadCommand(AgentRoleMap<T>& roles): roles(roles), PersistenceCommand<T>(PersistenceCommand<T>::LOAD) {}

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

    private:
    
        void loadAgent(Agency<T>& agency, const string& name, const string& filename) {
            
            JSON json(file_get_contents(filename));

            // typename PersistenceCommand<T>::Type type = this->getType(json.get<string>("type"));
            // if (type != PersistenceCommand<T>::AGENT)
            //     throw ERROR("Agent type missmatch: " + json.get<string>("type"));

            typename PersistenceCommand<T>::Type type = PersistenceCommand<T>::AGENT;

            string role = json.get<string>("role");
            if (!array_key_exists(role, roles))
                throw ERROR("Agent role not exists: " + role);

            vector<string> recipients = json.get<vector<string>>("recipients");

            Agent<T>& agent = roles[role](agency, name, recipients);
            agent.fromJSON(json);
        }

        void loadAgency(Agency<T>& agency, const string& name, const string& filename) {
            // ((JSONSerializable&)agency)
            //     .fromJSON(JSON(file_get_contents(filename)));
        }

        AgentRoleMap<T>& roles;

    };

}
