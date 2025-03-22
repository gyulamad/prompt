#pragma once

#include "../../../cmd/Command.hpp"
#include "../../Agency.hpp"

using namespace tools::cmd;
using namespace tools::agency;

namespace tools::agency::agents::commands {

    template<typename T>
    class KillCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return { "/kill {string}" };
        }
    
        void run(void* agency_void, const vector<string>& args) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;
            
            string name = args[1];

            if (name == "user") throw ERROR("User can not be killed!");
            agency.kill(name)
                ? cout << "Agent '" + name + "' is leaving..." << endl
                : cout << "Agent '" + name + "' not found" << endl;
        }
    };
    
}