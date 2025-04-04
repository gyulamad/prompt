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
    class LoadCommand : public PersistenceCommand<T> {
    public:
        // Constructor calling the base class constructor
        LoadCommand() : PersistenceCommand<T>(
            "load",        // commandName
            "Loads",       // actionVerbCapitalized
            "load",        // actionVerbLowercase
            "input-name",  // filenameParamName
            "input file"   // filenameParamDesc
        ) {}

    protected: // Changed access specifier for overridden virtual methods
        // Implement the pure virtual methods from the base class
        void performAgentAction(Agency<T>& agency, const string& agentName, const string& filename) override {
            loadAgentInternal(agency, agentName, filename);
        }

        void performAgencyAction(Agency<T>& agency, const string& agencyName, const string& filename) override {
            loadAgencyInternal(agency, agencyName, filename);
        }

    private:
        // Keep the internal implementation methods
        void loadAgentInternal(Agency<T>& agency, const string& agentName, const string& inputName) {
            // TODO: needs to be implemented
        }

        void loadAgencyInternal(Agency<T>& agency, const string& agencyName, const string& inputName) {
            // TODO: needs to be implemented
        }

        T load(const string& inputName) {
            // TODO: common load implementation for both Agent and Agency
        }
    };

}
