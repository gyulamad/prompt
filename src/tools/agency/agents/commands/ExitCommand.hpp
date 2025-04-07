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

        using Command::Command;
        virtual ~ExitCommand() {}
    
        vector<string> getPatterns() const override {
            return { this->prefix + "exit" };
        }

        string getName() const override {
            return this->prefix + "exit";
        }

        string getDescription() const override {
            return "Terminates the user agent's operation and exits the system.";
        }
        
        string getUsage() const override {
            return implode("\n", vector<string>({
                Usage({
                    getName(), // command
                    getDescription(), // help
                    vector<Parameter>(), // parameters (empty)
                    vector<pair<string, string>>({ // examples
                        make_pair(this->prefix + "exit", "Terminates the user agent's operation")
                    }),
                    vector<string>({ // notes
                        string("Only affects the user agent"),
                        string("Requires a valid user agent to be registered")
                    })
                }).to_string()
            }));
        }
    
        void run(void* worker_void, const vector<string>&) override {
            Worker<T>& worker = *safe((Worker<T>*)worker_void);

            worker.exit();
        }
    };
    
}