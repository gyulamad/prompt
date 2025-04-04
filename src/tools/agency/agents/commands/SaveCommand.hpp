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
            "Saves",        // actionVerbCapitalized
            "save",         // actionVerbLowercase
            "output-name",  // filenameParamName
            "output file"   // filenameParamDesc
        ) {}

    protected: // Changed access specifier for overridden virtual methods
        // Implement the pure virtual methods from the base class
        void performAgentAction(Agency<T>& agency, const string& agentName, const string& filename) override {
            // Need to get the agent reference here, as the base run() doesn't do it anymore
            Agent<T>& agent = agency.template getWorkerRef<Agent<T>>(agentName);
            // Optional: Add validation back if needed, e.g.,
            // if (agent.name != agentName) throw ERROR("Invalid agent, name mismatch: expected '" + agentName + "', got '" + agent.name + "'");
            saveAgentInternal(agent, filename);
        }

        void performAgencyAction(Agency<T>& agency, const string& agencyName, const string& filename) override {
            // The agency passed is the one to save. The agencyName parameter might be used
            // for validation or logging if needed, but the base run() passes the name from args[2].
            // We might want to validate agency.name against agencyName if relevant.
            saveAgencyInternal(agency, filename);
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
