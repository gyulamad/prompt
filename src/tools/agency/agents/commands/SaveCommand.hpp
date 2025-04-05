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
    class SaveCommand: public PersistenceCommand<T> {
    public:
        // Constructor calling the base class constructor
        SaveCommand(): PersistenceCommand<T>(PersistenceCommand<T>::SAVE) {}

    protected: 
    
        void performAction(Agency<T>& agency, PersistenceCommand<T>::Type type, const string& name, const string& filename) override {
            switch (type) {
                
                case PersistenceCommand<T>::AGENT:
                    saveAgent(name, filename);
                    break;
                
                case PersistenceCommand<T>::AGENCY:
                    saveAgency(name, filename);
                    break;

                default:
                    throw ERROR("Couldn't save, invalid type given.");
            }
        }

    private:
    
        void saveAgent(Agent<T>& agent, const string& filename) {
            // TODO: needs to be implemented
        }

        void saveAgency(Agency<T>& agency, const string& filename) {
            // TODO: needs to be implemented
        }

    };

}
