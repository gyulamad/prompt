#pragma once

#include <string>
#include <vector>

#include "../../../containers/array_key_exists.hpp"
#include "../../../str/parse_vector.hpp"
#include "../../../cmd/Usage.hpp"
#include "../../../cmd/Parameter.hpp"
#include "../../../cmd/Command.hpp"
#include "../../Agency.hpp"

using namespace std;
using namespace tools::str;
using namespace tools::cmd;
using namespace tools::agency;
using namespace tools::agency::agents;

namespace tools::agency::agents::commands {
    
    template<typename T>
    class TargetCommand: public Command {
    public:
    
        vector<string> getPatterns() const override {
            return { 
                "/target", // default is "list"
                "/target list",
                "/target set {string}",
                "/target add {string}",
                "/target remove {string}"
            };
        }
        
        string getUsage() const override {
            return implode("\n", vector<string>({
                Usage({
                    string("/target"), // command
                    string("Manages the list of recipients for messages."), // help
                    vector<Parameter>({ // parameters
                        {
                            string("operation"), // name
                            bool(true), // optional
                            string("Operation to perform (list|set|add|remove), defaults to 'list'") // help
                        },
                        {
                            string("targets"), // name
                            bool(true), // optional
                            string("Comma-separated list of target names (required for set/add/remove)") // help
                        }
                    }),
                    vector<pair<string, string>>({ // examples
                        make_pair("/target list", "Lists all targets"),
                        make_pair("/target set user1,user2", "Sets targets to user1 and user2"),
                        make_pair("/target add user3", "Adds user3 to the targets"),
                        make_pair("/target remove user1", "Removes user1 from the targets") 
                    }),
                    vector<string>({ // notes
                        string("If no operation is specified, it defaults to 'list'"),
                        string("Target names must be unique"),
                        string("For set/add/remove operations, targets parameter is required")
                    })
                }).to_string()
            }));
        }
    
        void run(void* agency_void, const vector<string>& args) override {
            NULLCHK(agency_void);
            Agency<T>& agency = *(Agency<T>*)agency_void;

            vector<string> targets = {};
            size_t asize = args.size();
            enum Operation { LIST, SET, ADD, REMOVE } operation = LIST;
            map<string, Operation> opnames = {
                {"list", LIST},
                {"set", SET},
                {"add", ADD},
                {"remove", REMOVE},
            };
            if (asize == 2) targets = explode(",", args[1]);
            else if (asize == 3) {
                if (!array_key_exists(args[1], opnames)) throw ERROR("Invalid operation: " + args[1]);
                operation = opnames[args[1]];
                targets = explode(",", args[2]);
            } if (asize > 3) throw ERROR("Invalid argument(s).");
            
            UserAgent<T>& user = agency.template getWorkerRef<UserAgent<T>>("user");
            vector<string> recipients;
            if (operation != LIST && targets.empty()) throw ERROR("No target(s) provided for operation: " + (args[1]));
            switch (operation) {
            case LIST:
                recipients = user.findRecipients(asize == 3 ? args[2] : "");
                user.getInterfaceRef().println(tpl_replace({
                    { "{{workers}}", agency.dumpWorkers(recipients) },
                    { "{{found}}", to_string(recipients.size()) },
                    { "{{total}}", to_string(user.findRecipients().size()) },
                }, Agency<T>::worker_list_tpl));
                break;
            case SET:
                user.setRecipients(targets);
                break;
            case ADD:
                user.addRecipients(targets);
                break;
            case REMOVE:
                user.removeRecipients(targets);
                break;
            default:
                throw ERROR("Invalid target operation provided");
            }

        }

    };
    
}
