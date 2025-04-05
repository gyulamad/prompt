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
    class SaveCommand : public PersistenceCommand<T> {
    public:
        // Constructor calling the base class constructor
        SaveCommand() : PersistenceCommand<T>(
            "save",         // commandName
            "Saves",        // actionVerb
            "agent",       // agentTypeName
            "agency"        // agencyTypeName
        ) {}

    protected: 
    
        void performAction(void* thing, PersistenceCommand<T>::Type type, const string& name, const string& filename) override {
            // TODO needs to be implemented
        }

    private:
        // Keep the internal implementation methods
        void saveAgentInternal(Agent<T>& agent, const string& outputName) {
            // TODO: needs to be implemented
        }

        void saveAgencyInternal(Agency<T>& agency, const string& outputName) {
            // TODO: needs to be implemented
        }

        void save(T& object, const string& outputName) {
            // TODO: common save implementation for both Agent and Agency
        }
    };

}
