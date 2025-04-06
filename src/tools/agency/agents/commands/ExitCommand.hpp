#pragma once

#include <string>
#include <vector>

#include "../../../cmd/Usage.hpp"
#include "../../../cmd/Parameter.hpp"
#include "../../../cmd/Command.hpp"
#include "../../Agency.hpp"

using namespace std;
using namespace tools::cmd;
using namespace tools::agency;
using namespace tools::agency::agents;

namespace tools::agency::agents::commands {

    template<typename T>
    class ExitCommand: public Command {
    public:
    
        vector<string> getPatterns() const override {
            return { "/exit" };
        }
        
        string getUsage() const override {
            return implode("\n", vector<string>({
                Usage({
                    string("/exit"), // command
                    string("Terminates the user agent's operation and exits the system."), // help
                    vector<Parameter>(), // parameters (empty)
                    vector<pair<string, string>>({ // examples
                        make_pair("/exit", "Terminates the user agent's operation")
                    }),
                    vector<string>({ // notes
                        string("Only affects the user agent"),
                        string("Requires a valid user agent to be registered")
                    })
                }).to_string()
            }));
        }
    
        void run(void* agency_void, const vector<string>&) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;

            agency.exit();
        }
    };
    
}