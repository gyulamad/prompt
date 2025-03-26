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
    
        void run(void* agency_void, const vector<string>&) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;

            agency.getAgentRef("user").exit();
            // UserAgent<T>& user = agency.getUserAgentRef().exit();
        }
    };
    
}