#pragma once

#include "../../../utils/ERROR.hpp"

using namespace std;
using namespace tools::cmd;
using namespace tools::agency;
using namespace tools::agency::agents;
using namespace tools::utils;

namespace tools::agency::agents::commands {

    template<typename T>
    class SaveCommand: public CommonPersistor, public Command {
    public:
    
        SaveCommand(const string& prefix, AgentRoleMap& roles): 
            CommonPersistor(roles), Command(prefix) {}

        virtual ~SaveCommand() {}
    
        vector<string> getPatterns() const override {
            vector<string> patterns;
            foreach (typeNameMap, [&](const string& typeName) {
                patterns.push_back(getName() + " " + typeName + " {string}");
                patterns.push_back(getName() + " " + typeName + " {string} [{string}]");
            });
            return patterns;
        }

        string getName() const override {
            return this->prefix + "save";
        }

        string getDescription() const override {
            return "Save an " + implode(" or ", typeNames) + " to a file.";
        }

        string getUsage() const override {
            vector<pair<string, string>> examples;
            foreach (typeNameMap, [&](const string& typeName) {
                examples.push_back(make_pair(
                    getName() + " " + typeName + " my_" + typeName,
                    "Save the " + typeName + " 'my_" + typeName 
                        + "' to file 'my_" + typeName + "'"
                ));
                examples.push_back(make_pair(
                    getName() + " " + typeName + " my_" + typeName 
                        + " my_" + typeName + "_file",
                    "Save the " + typeName + " 'my_" + typeName + "' to file."
                ));
            });

            return implode("\n", vector<string>({
                Usage({
                    getName(), // command
                    getDescription(), // help
                    vector<Parameter>({ // parameters
                        {
                            string("type"), // name
                            bool(false), // optional
                            string(
                                "Type of the " + implode(" or ", typeNames) 
                                    + " to save."
                            ) // help
                        },
                        {
                            string("name"), // name
                            bool(false), // optional
                            string(
                                "Name of the " + implode(" or ", typeNames) 
                                    + " to save."
                            ) // help
                        },
                        {
                            string("file"), // name
                            bool(true), // optional
                            string(
                                "Name of the file. Defaults to " 
                                    + implode(" or ", typeNames) + " name."
                            ) // help
                        }
                    }),
                    examples, // examples
                    vector<string>({ // notes
                        "If file is not provided, it will save the " 
                            + implode(" or ", typeNames) + " to a file named after the "
                            + implode(" or ", typeNames) + " name."
                    })
                }).to_string()
            }));
        }

        void run(void* worker_void, const vector<string>& args) override {
        //     Worker<T>& worker = *safe((Worker<T>*)worker_void);
        //     Agency<T>& agency = *safe((Agency<T>*)worker.getAgencyPtr());

        //     string typeName = args[1]; // "agent" or "agency"
        //     string thingName = args[2]; // agentName or agencyName
        //     string filename = (args.size() > 3) ? args[3] : thingName;

        //     if (!file_exists(filename)) filename = filename + ".json";
        //     if (!file_exists(filename)) throw ERROR("File not found: " + filename);

        //     performAction(agency, this->getType(typeName), thingName, filename);
            worker = safe((Worker<T>*)worker_void);
            proceed<T>(SAVE, args);
        }

    protected:

        Worker<T>* worker = nullptr;
    
        void performSave(Type type, const string& name, const string& filename) override {
            Agency<T>& agency = *(Agency<T>*)safe(worker->getAgencyPtr());

            switch (type) {
                
                case AGENT:
                    saveAgent(agency, name, filename);
                    break;
                
                case AGENCY:
                    saveAgency(agency, filename);
                    break;

                default:
                    throw ERROR("Couldn't save, invalid type given.");
            }
        }

    private:
    
        void saveAgent(Agency<T>& agency, const string& name, const string& filename) {
            file_put_contents(filename, agency.getWorkerRef(name).toJSON().dump(4), false, true);
        }

        void saveAgency(Agency<T>& agency, const string& filename) {
            JSON jagency = agency.toJSON();
            vector<JSON> jworkers = jagency.get<vector<JSON>>("workers");
            vector<string> fworkers;
            for (const JSON& jworker: jworkers) {
                string fworker = jworker.get<string>("name") + ".json";
                file_put_contents(fworker, jworker.dump(4), false, true);
                fworkers.push_back(fworker);
            }
            jagency.set("workers", fworkers);
            file_put_contents(filename, jagency.dump(4), false, true);
        }

    };

}
