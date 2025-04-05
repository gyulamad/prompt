#pragma once

#include "PersistenceCommand.hpp" // Include the base class
#include "../../../utils/ERROR.hpp"    // Ensure ERROR macro is available if needed by internal methods

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
        LoadCommand(): PersistenceCommand<T>(PersistenceCommand<T>::LOAD) {}

    protected:

        void performAction(Agency<T>& agency, PersistenceCommand<T>::Type type, const string& name, const string& filename) override {
            switch (type) {
                
                case PersistenceCommand<T>::AGENT:
                    loadAgent(name, filename);
                    break;
                
                case PersistenceCommand<T>::AGENCY:
                    loadAgency(name, filename);
                    break;

                default:
                    throw ERROR("Couldn't save, invalid type given.");
            }
        }

    private:
    
        void loadAgent(Agent<T>& agent, const string& agentName, const string& inputName) {
            // TODO: needs to be implemented
        }

        void loadAgency(Agency<T>& agency, const string& agencyName, const string& inputName) {
            // TODO: needs to be implemented
        }

    };

}
