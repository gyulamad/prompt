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
    class SaveAgencyCommand: public Command {
    public:

        vector<string> getPatterns() const override {
            return {
                "/save agency {string} [{string}]"
            };
        }

        string getUsage() const override {
            return implode("\n", vector<string>({
                Usage({
                    string("/save agency <agency-name> [<output-name>]"), // command
                    string("Saves an agency to a file."), // help
                    vector<Parameter>({ // parameters
                        {
                            string("agency-name"), // name
                            bool(false), // optional
                            string("Name of the agency to save.") // help
                        },
                        {
                            string("output-name"), // name
                            bool(true), // optional
                            string("Name of the output file. Defaults to agency-name.json.") // help
                        }
                    }),
                    vector<pair<string, string>>({ // examples
                        make_pair("/save agency my_agency", "Saves the agency 'my_agency' to my_agency.json"),
                        make_pair("/save agency my_agency output.json", "Saves the agency 'my_agency' to output.json")
                    }),
                    vector<string>({ // notes
                        string("If output-name is not provided, the agency will be saved to a file named agency-name.json.")
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
            string outputName = (args.size() > 3) ? args[3] : agencyName + ".json";

            saveAgency(agency, outputName);
        }

    private:
        void saveAgency(Agency<T>& agency, const string& outputName) {
            // TODO: needs to be implemented
        }
    };

}
