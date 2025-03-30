#pragma once

#include "../../../containers/array_key_exists.hpp"
#include "../../../str/parse_vector.hpp"
#include "../../../cmd/Command.hpp"
#include "../../Agency.hpp"

using namespace tools::str;
using namespace tools::cmd;
using namespace tools::agency;

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
            return R"(
/target [operation] [targets] - Manages the list of recipients for messages.

Usage:
  /target list [filter] - Lists all targets, optionally filtered.
  /target set {target1,target2,...} - Sets the targets to the specified list.
  /target add {target1,target2,...} - Adds the specified targets to the existing list.
  /target remove {target1,target2,...} - Removes the specified targets from the list.

Parameters:
  operation - (list|set|add|remove) The operation to perform.
  targets   - (optional) Comma-separated list of target names.

Examples:
  /target list             # Lists all targets.
  /target set user1,user2  # Sets targets to user1 and user2.
  /target add user3        # Adds user3 to the targets.
  /target remove user1     # Removes user1 from the targets.

Notes:
  - If no operation is specified, it defaults to 'list'.
  - Target names must be unique.
)";
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
            
            UserAgent<T>& user = agency.template getAgentRef<UserAgent<T>>("user");
            vector<string> recipients;
            if (operation != LIST && targets.empty()) throw ERROR("No target(s) provided for operation: " + (args[1]));
            switch (operation) {
            case LIST:
                recipients = user.findRecipients(asize == 3 ? args[2] : "");
                user.getInterfaceRef().println(tpl_replace({
                    { "{{agents}}", agency.dumpAgents(recipients) },
                    { "{{found}}", to_string(recipients.size()) },
                    { "{{total}}", to_string(user.findRecipients().size()) },
                }, Agency<T>::agent_list_tpl));
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
