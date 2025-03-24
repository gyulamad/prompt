#pragma once

#include "../../../cmd/Command.hpp"
#include "../../Agency.hpp"

using namespace tools::cmd;
using namespace tools::agency;

namespace tools::agency::agents::commands {

    template<typename T>
    class ListCommand: public Command {
    public:
    
        vector<string> getPatterns() const override {
            return { "/list" };
        }
    
        void run(void* agency_void, const vector<string>&) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;
            
            vector<Agent<T>*> agents = agency.getAgentsCRef();
            cout << "List of agents:" << endl;
            for (const Agent<T>* agent: agents) cout << "Agent '" + agent->name + "'" << endl;
        }
    };
    
}