#pragma once

#include "CommonPersitor.hpp"

namespace tools::agency::agents::commands {

    template<typename T>
    class LoadCommand: public CommonPersistor, public Command {
    public:
    
        LoadCommand(const string& prefix, AgentRoleMap& roles): 
            CommonPersistor(roles), Command(prefix) {}

        virtual ~LoadCommand() {}

        vector<string> getPatterns() const override {
            vector<string> patterns;
            foreach (typeNameMap, [&](const string& typeName) {
                patterns.push_back(getName() + " " + typeName + " {string}");
            });
            return patterns;
        }

        string getName() const override {
            return this->prefix + "load";
        }

        string getDescription() const override {
            return "Load an " + implode(" or ", typeNames) + " from a file.";
        }

        string getUsage() const override {
            vector<pair<string, string>> examples;
            foreach (typeNameMap, [&](const string& typeName) {
                examples.push_back(make_pair(
                    getName() + " " + typeName + " my_" + typeName,
                    "Load the " + typeName + " 'my_" + typeName 
                        + "' from file 'my_" + typeName + "'"
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
                                + " to load."
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
                        "If file is not provided, it will load the " 
                            + implode(" or ", typeNames) + " from a file named after the " 
                            + implode(" or ", typeNames) + " name."
                    })
                }).to_string()
            }));
        }

        void run(void* worker_void, const vector<string>& args) override {
        //     Worker<T>& worker = *safe((Worker<T>*)worker_void);
        //     Agency<T>& agency = *safe((Agency<T>*)worker.getAgencyPtr());

        //     string typeName = args[1]; // "agent" or "agency"
        //     string filename = args[2]; // agentName or agencyName
        //     // string filename = (args.size() > 3) ? args[3] : thingName;

        //     if (!file_exists(filename)) filename = filename + ".json";
        //     if (!file_exists(filename)) throw ERROR("File not found: " + filename);

        //     performAction(agency, getType(typeName), filename);
            worker = safe((Worker<T>*)worker_void);
            proceed<T>(LOAD, args);
        }

    protected:

        Worker<T>* worker = nullptr;

        void performLoad(Type type, const string& filename) override {
            Agency<T>& agency = *(Agency<T>*)safe(worker->getAgencyPtr());

            switch (type) {
                
                case AGENT:
                    loadAgent(filename);
                    break;
                
                case AGENCY:
                    loadAgency(agency, filename);
                    break;

                default:
                    throw ERROR("Couldn't load, invalid type given.");
            }
        }

    private:

        void loadAgent(const string& filename) {
            JSON json(file_get_contents(filename));
            
            string role = json.get<string>("role");
            if (!array_key_exists(role, roles))
                throw ERROR("Role type does not exist: '" + role + "'");
                
            roles[role](json.get<string>("name"), json);
        }

        void loadAgency(Agency<T>& agency, const string& filename) {
            JSON json(file_get_contents(filename));
            vector<string> fworkers = json.get<vector<string>>("workers");
            vector<JSON> jworkers;
            for (const string& fworker: fworkers) {
                JSON jworker(file_get_contents(fworker));
                jworkers.push_back(jworker);
            }
            json.set("workers", jworkers);
            agency.fromJSON(json);
        }

    };

}
