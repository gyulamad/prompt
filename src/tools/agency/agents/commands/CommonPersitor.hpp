#pragma once

#include <map>
#include <string>
#include <vector>

#include "../../../utils/ERROR.hpp"
#include "../../../utils/foreach.hpp"
#include "../../../containers/array_key_exists.hpp"
#include "../../../str/str_ends_with.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::containers;

namespace tools::agency::agents::commands {

    // Load/save name/filename, it should work like:
    //  /save => name + (filename - optional, comes from name + ".json" if not set)
    //  /load => filename + (name - optional, comes from the file data if not set)
    // - in both cases if file not found by name it should retry with filename + ".json" if the extension is not set alread
    // + help usages in the parent class have to be updatede

    class CommonPersistor {
    protected:

        enum Persistor { LOAD, SAVE };

        CommonPersistor(AgentRoleMap& roles): roles(roles) {}
        virtual ~CommonPersistor() {}

        template<typename T>
        void proceed(Persistor persistor, const vector<string>& args) {

            Typw type = getType(args[1]); // "agent" or "agency"
            string thingName = args[2]; // agentName or agencyName
            string filename = (args.size() > 3) ? args[3] : thingName;
            if (!str_ends_with(filename, ".json")) filename =+ ".json";
            
            switch (persistor) {
                case LOAD:
                    if (!file_exists(filename)) throw ERROR("File not found: " + filename);
                    performLoad(type, filename);
                    break;
                case SAVE: 
                    performSave(type, thingName, filename);
                    break;
                default:
                    throw ERROR_INVALID(persistor);
            }
        }


        enum Type { AGENT, AGENCY };
        virtual void performLoad(Type /*type*/, const string& /*filename*/) {
            throw ERROR_UNIMP;
        }

        virtual void performSave(Type /*type*/, const string& /*name*/, const string& /*filename*/) {
            throw ERROR_UNIMP;
        }

        // =============================================================
        // =======================[ PERSISTANCE ]=======================
        // =============================================================
        // Register types here...

        const map<Type, string> typeNameMap = {
            { AGENT, "agent" },
            { AGENCY, "agency" },
        };

        const vector<Type> types = array_keys(typeNameMap);
        const vector<string> typeNames = getTypeNames();

        vector<string> getTypeNames() const {
            vector<string> typeNames;
            for (Type type: types) 
                typeNames.push_back(getTypeName(type));
            return typeNames;
        }

        string getTypeName(Type type) const {
            if (!array_key_exists(type, typeNameMap)) 
                throw getInvalidTypeError(to_string((int)type));
            return typeNameMap.at(type);
        }

        runtime_error getInvalidTypeError(string typeName) const {
            return ERROR(
                "Invalid type given (" + typeName + ").  possible types are: " 
                + implode("|", getTypeNames())
            );
        }
        
        Type getType(const string& typeName) const {
            bool found = false;
            Type type;
            foreach (typeNameMap, [&](const string& name, Type typ) {
                if (typeName == name) {
                    found = true;
                    type = typ;
                    return FE_BREAK;
                }
                return FE_CONTINUE;
            });
            if (found) return type;
            throw getInvalidTypeError(typeName);
        }

        AgentRoleMap& roles;

        // =============================================================
        // =============================================================
        // =============================================================

    };

}