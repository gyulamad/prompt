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

namespace tools::agency::agents::commands {

    template<typename T>
    class LoadAgencyCommand: public Command {
    public:

        vector<string> getPatterns() const override {
            return {
                "/load agency {string} [{string}]"
            };
        }

        string getUsage() const override {
            return implode("\n", vector<string>({
                Usage({
                    string("/load agency"), // command
                    string("Loads an agency from a file."), // help
                    vector<Parameter>({ // parameters
                        {
                            string("agency-name"), // name
                            bool(false), // optional
                            string("Name of the agency to load.") // help
                        },
                        {
                            string("input-name"), // name
                            bool(true), // optional
                            string("Name of the input file. Defaults to agency-name.json.") // help
                        }
                    }),
                    vector<pair<string, string>>({ // examples
                        make_pair("/load agency my_agency", "Loads the agency 'my_agency' from my_agency.json"),
                        make_pair("/load agency my_agency input.json", "Loads the agency 'my_agency' from input.json")
                    }),
                    vector<string>({ // notes
                        string("If input-name is not provided, the agency will be loaded from a file named agency-name.json.")
                    })
                }).to_string()
            }));
        }

        void run(void* agency_void, const vector<string>& args) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;

            if (args.size() < 3) {
                throw ERROR("Usage: " + getPatterns()[0]);
            }

            string agencyName = args[2];
            string inputName = (args.size() > 3) ? args[3] : agencyName + ".json";

            loadAgency(agency, inputName);
        }

    private:
        void loadAgency(Agency<T>& agency, const string& inputName) {
            // TODO: needs to be implemented
        }
    };

}
