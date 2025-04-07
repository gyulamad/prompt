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
    
        void run(void* worker_void, const vector<string>& args) override {
            Worker<T>& worker = *safe((Worker<T>*)worker_void);
            Agency<T>& agency = *safe((Agency<T>*)worker.getAgencyPtr());

            string typeName = args[1]; // "agent" or "agency"
            string thingName = args[2]; // agentName or agencyName
            string filename = (args.size() > 3) ? args[3] : thingName;

            if (!file_exists(filename)) filename = filename + ".json";
            if (!file_exists(filename)) throw ERROR("File not found: " + filename);

            performAction(agency, this->getType(typeName), thingName, filename);
        }

    protected: 
    
        void performAction(Agency<T>& agency, PersistenceCommand<T>::Type type, const string& name, const string& filename) {
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
