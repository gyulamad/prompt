#pragma once

#include "../../../cmd/Command.hpp"
#include "../../Agency.hpp"

using namespace tools::cmd;
using namespace tools::agency;

namespace tools::agency::agents::commands {

    template<typename T>
    class ExitCommand: public Command {
    public:
    
        vector<string> getPatterns() const override {
            return { "/exit" };
        }
        
        string getUsage() const override {
            return "/exit\n"
                   "Terminates the user agent's operation and exits the system.\n"
                   "Usage: /exit\n"
                   "Example: /exit\n"
                   "Notes:\n"
                   "  - Only affects the user agent\n"
                   "  - Requires a valid user agent to be registered";
        }
    
        void run(void* agency_void, const vector<string>&) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;

            agency.getAgentRef("user").exit();
            // UserAgent<T>& user = agency.getUserAgentRef().exit();
        }
    };
    
}