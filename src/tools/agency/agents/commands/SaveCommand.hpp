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
        SaveCommand(const string& prefix): 
            PersistenceCommand<T>(prefix, PersistenceCommand<T>::SAVE) {}
        virtual ~SaveCommand() {}

    protected: 
    
        void performAction(Agency<T>& agency, PersistenceCommand<T>::Type type, const string& name, const string& filename) override {
            switch (type) {
                
                case PersistenceCommand<T>::AGENT:
                    saveAgent(agency, name, filename);
                    break;
                
                case PersistenceCommand<T>::AGENCY:
                    saveAgency(agency, name, filename);
                    break;

                default:
                    throw ERROR("Couldn't save, invalid type given.");
            }
        }

    private:
    
        void saveAgent(Agency<T>& agency, const string& name, const string& filename) {
            file_put_contents(filename, agency.getWorkerRef(name).toJSON().dump(4), false, true);
        }

        void saveAgency(Agency<T>& /*agency*/, const string& /*name*/, const string& /*filename*/) {
            STUB("Implement this!");
            // file_put_contents(filename, ((JSONSerializable&)agency).toJSON().dump(), false, true);
        }

    };

}
