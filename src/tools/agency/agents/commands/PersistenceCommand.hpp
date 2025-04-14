#pragma once

#include <string>
#include <vector>
#include <map>

#include "../../../cmd/Command.hpp"
#include "../../../cmd/Usage.hpp"
#include "../../../cmd/Parameter.hpp"
#include "../../../utils/ERROR.hpp" // For NULLCHK and ERROR macros
#include "../../../str/implode.hpp" // For implode
#include "../../../str/strtolower.hpp"
#include "../../../str/ucfirst.hpp"
#include "../../../utils/foreach.hpp"
#include "../../../containers/array_key_exists.hpp"
// #include "../../Agency.hpp"
// #include "../../Agent.hpp"

using namespace std;
using namespace tools::cmd;
using namespace tools::utils;
using namespace tools::str; // For implode
using namespace tools::containers;
// using namespace tools::agency;

namespace tools::agency::agents::commands {

    template<typename T>
    class PersistenceCommand : public Command {
    protected:
        enum Persistor { LOAD, SAVE };

        Persistor persistor;

        string filenameParamName;
        string filenameParamDesc;


        // ===============================================================
        // Register types here...

        enum Type { AGENT, AGENCY };

        const map<Type, string> typeNameMap = {
            { AGENT, "agent" },
            { AGENCY, "agency" },
        };

        const vector<Type> types = array_keys(typeNameMap);
        const vector<string> typeNames = getTypeNames();

        // ===============================================================



        struct PersistorNameWord {
            string commandName;
            string directorWord;
        };

        const map<Persistor, string> commandNameMap = {
            { LOAD , "load" },
            { SAVE , "save" },
        };

        const map<Persistor, string> directorWordMap = {
            { LOAD , "from" },
            { SAVE , "to" },
        };

    public:

        PersistenceCommand(
            const string& prefix,
            Persistor persistor,
            const string& filenameParamName = "filename", // Default value
            const string& filenameParamDesc = "file"      // Default value
        ): 
            Command(prefix),
            persistor(persistor),
            filenameParamName(filenameParamName),
            filenameParamDesc(filenameParamDesc)
        {}
        
        virtual ~PersistenceCommand() {}

        vector<string> getPatterns() const override {
            vector<string> patterns;
            foreach (typeNameMap, [&](const string& typeName) {
                patterns.push_back(getName() + " " + typeName + " {string}");
                patterns.push_back(getName() + " " + typeName + " {string} [{string}]");
            });
            return patterns;
        }

        string getName() const override {
            return this->prefix + getCommandName(persistor);
        }

        string getDescription() const override {
            return getActionVerbCapitalized(persistor) + " an " 
                + implode(" or ", typeNames) + " " 
                + getDirectorWord(persistor) 
                + " a " + filenameParamDesc + ".";
        }

        string getUsage() const override {
            string defaultFilenameNote = 
                "If " + filenameParamName + " is not provided, it will " 
                    + getActionVerbLowercase(persistor) + " the " + implode(" or ", typeNames) 
                    + " " + getDirectorWord(persistor) + " a " + filenameParamDesc 
                    + " named after the " + implode(" or ", typeNames) + " name.";

            vector<pair<string, string>> examples;
            foreach (typeNameMap, [&](const string& typeName) {
                examples.push_back(make_pair(
                    getName() + " " + typeName + " my_" + typeName,
                    getActionVerbCapitalized(persistor) + " the " + typeName + " 'my_" 
                        + typeName + "' " + getDirectorWord(persistor) + " " 
                        + filenameParamDesc + " 'my_" + typeName + "'"
                ));
                examples.push_back(make_pair(
                    getName() + " " + typeName + " my_" + typeName 
                        + " my_" + typeName + "_file",
                    getActionVerbCapitalized(persistor) + " the " + typeName + " 'my_" 
                        + typeName + "' " + getDirectorWord(persistor) + " " 
                        + filenameParamDesc + " '" + filenameParamName + "'"
                ));
            });

            return implode("\n", vector<string>({
                Usage({
                    getName(), // string("/" + getCommandName(persistor)), // command
                    getDescription(),
                    // string(
                    //     getActionVerbCapitalized(persistor) + " an " 
                    //     + implode(" or ", typeNames) + " " 
                    //     + getDirectorWord(persistor) 
                    //     + " a " + filenameParamDesc + "."
                    // ), // help
                    vector<Parameter>({ // parameters
                        {
                            string("type"), // name
                            bool(false), // optional
                            string(
                                "Type of the " + implode(" or ", typeNames) 
                                + " to " + getActionVerbLowercase(persistor) + "."
                            ) // help
                        },
                        {
                            string("name"), // name
                            bool(false), // optional
                            string(
                                "Name of the " + implode(" or ", typeNames) 
                                + " to " + getActionVerbLowercase(persistor) + "."
                            ) // help
                        },
                        {
                            filenameParamName, // name
                            bool(true), // optional
                            string(
                                "Name of the " + filenameParamDesc 
                                + ". Defaults to " + implode(" or ", typeNames) 
                                + " name."
                            ) // help
                        }
                    }),
                    examples, // examples
                    vector<string>({ // notes
                        defaultFilenameNote
                    })
                }).to_string()
            }));
        }

        const vector<string>& validate(const vector<string>& args) override {
            // args[0] is the command itself (e.g., "/load")
            // args[1] is the type (e.g., "agent" or "agency")
            // args[2] is the name (e.g., agent/agency name)
            // args[3] is the optional filename
            if (args.size() < 3) {
                 // Provide more specific usage based on the actual command invoked if possible
                throw ERROR(
                    string("Usage: ") + getName() + " " 
                    + implode("|", typeNames) + " {name} [filename]"
                );
            }
            return args;
        }

    protected:

        // void run(void* worker_void, const vector<string>& args) override {
        //     NULLCHK(agency_void);

        //     string typeName = args[1]; // "agent" or "agency"
        //     string thingName = args[2]; // agentName or agencyName
        //     string filename = (args.size() > 3) ? args[3] : thingName;

        //     performAction(*(Agency<T>*)agency_void, getType(typeName), thingName, filename);
        // }

        // virtual void performAction(Agency<T>& agency, Type type, const string& name, const string& filename) = 0;


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

        runtime_error getInvalidTypeError(string typeName) const {
            return ERROR(
                "Invalid type given (" + typeName + ").  possible types are: " 
                + implode("|", getTypeNames())
            );
        }


        string getCommandName(Persistor persistor) const {
            if (!array_key_exists(persistor, commandNameMap))
                throw getInvalidPersistorError(persistor);
            return commandNameMap.at(persistor);
        }

        string getDirectorWord(Persistor persistor) const {
            if (!array_key_exists(persistor, directorWordMap))
                throw getInvalidPersistorError(persistor);
            return directorWordMap.at(persistor);
        }

        runtime_error getInvalidPersistorError(Persistor persistor) const {
            return ERROR("Invalid persistor: " + to_string((int)persistor));
        }


        string getActionVerbCapitalized(Persistor persistor) const {
            return ucfirst(getCommandName(persistor));
        }

        string getActionVerbLowercase(Persistor persistor) const {
            return strtolower(getCommandName(persistor));
        }
    };

}


#ifdef TEST

#include "../../../utils/Test.hpp" // Include your testing framework header
#include "../../../utils/str_contains.hpp" // For checking exception messages

using namespace tools::agency::agents::commands;
using namespace tools::utils; // For Test.hpp helpers like str_contains

// Define a dummy type for the template
using DummyType = int; 

// --- Test Cases ---

void test_PersistenceCommand_Constructor_Load() {
    PersistenceCommand<DummyType> cmd("myprefix", PersistenceCommand<DummyType>::LOAD, "filearg", "File Description");
    assert(cmd.getName() == "myprefixload" && "Constructor Load: Name check failed");
    // Add more checks if internal state needs verification, though members are protected
}

void test_PersistenceCommand_Constructor_Save() {
    PersistenceCommand<DummyType> cmd("myprefix", PersistenceCommand<DummyType>::SAVE, "outputfile", "Output File");
    assert(cmd.getName() == "myprefixsave" && "Constructor Save: Name check failed");
}

void test_PersistenceCommand_GetName_Load() {
    PersistenceCommand<DummyType> cmd("", PersistenceCommand<DummyType>::LOAD);
    string actual = cmd.getName();
    assert(actual == "load" && "GetName Load: Name mismatch");
}

void test_PersistenceCommand_GetName_Save() {
    PersistenceCommand<DummyType> cmd("cmd_", PersistenceCommand<DummyType>::SAVE);
    string actual = cmd.getName();
    assert(actual == "cmd_save" && "GetName Save: Name mismatch");
}

void test_PersistenceCommand_GetPatterns() {
    PersistenceCommand<DummyType> cmd("", PersistenceCommand<DummyType>::LOAD);
    vector<string> expected = {
        "load agent {string}",
        "load agent {string} [{string}]",
        "load agency {string}",
        "load agency {string} [{string}]"
    };
    vector<string> actual = cmd.getPatterns();
    assert(vector_equal(actual, expected) && "GetPatterns: Pattern mismatch");
}

void test_PersistenceCommand_GetDescription_Load() {
    PersistenceCommand<DummyType> cmd("", PersistenceCommand<DummyType>::LOAD, "filename", "file");
    string expected = "Load an agent or agency from a file.";
    string actual = cmd.getDescription();
    assert(actual == expected && "GetDescription Load: Description mismatch");
}

void test_PersistenceCommand_GetDescription_Save() {
    PersistenceCommand<DummyType> cmd("", PersistenceCommand<DummyType>::SAVE, "filename", "file");
    string expected = "Save an agent or agency to a file.";
    string actual = cmd.getDescription();
    assert(actual == expected && "GetDescription Save: Description mismatch");
}

// Usage test might be complex, maybe just check for key parts
void test_PersistenceCommand_GetUsage_Load() {
    PersistenceCommand<DummyType> cmd("", PersistenceCommand<DummyType>::LOAD, "inputfile", "input file");
    string actual = cmd.getUsage();
    assert(str_contains(actual, "load agent my_agent") && "GetUsage Load: Missing agent example");
    assert(str_contains(actual, "load agency my_agency my_agency_file") && "GetUsage Load: Missing agency example with file");
    assert(str_contains(actual, "If inputfile is not provided") && "GetUsage Load: Missing default filename note");
    assert(str_contains(actual, "Type of the agent or agency to load.") && "GetUsage Load: Missing type parameter description");
}

void test_PersistenceCommand_Validate_Success_MinArgs() {
    PersistenceCommand<DummyType> cmd("", PersistenceCommand<DummyType>::LOAD);
    vector<string> args = {"load", "agent", "my_agent"};
    bool thrown = false;
    try {
        cmd.validate(args);
    } catch (const exception& e) {
        thrown = true;
    }
    assert(!thrown && "Validate Success MinArgs: Should not throw");
}

void test_PersistenceCommand_Validate_Success_WithFilename() {
    PersistenceCommand<DummyType> cmd("", PersistenceCommand<DummyType>::SAVE);
    vector<string> args = {"save", "agency", "my_agency", "my_file.json"};
     bool thrown = false;
    try {
        cmd.validate(args);
    } catch (const exception& e) {
        thrown = true;
    }
    assert(!thrown && "Validate Success WithFilename: Should not throw");
}

void test_PersistenceCommand_Validate_Fail_TooFewArgs() {
    PersistenceCommand<DummyType> cmd("", PersistenceCommand<DummyType>::LOAD);
    vector<string> args = {"load", "agent"};
    bool thrown = false;
    try {
        cmd.validate(args);
    } catch (const runtime_error& e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Usage: load agent|agency {name} [filename]") && "Validate Fail TooFewArgs: Incorrect error message");
    } catch (...) {
        // Catch any other exception type
         assert(false && "Validate Fail TooFewArgs: Caught unexpected exception type");
    }
    assert(thrown && "Validate Fail TooFewArgs: Should have thrown runtime_error");
}

void test_PersistenceCommand_GetType_Success() {
    PersistenceCommand<DummyType> cmd("", PersistenceCommand<DummyType>::LOAD);
    auto type_agent = cmd.getType("agent");
    assert(type_agent == PersistenceCommand<DummyType>::AGENT && "GetType Success: Agent type mismatch");
    auto type_agency = cmd.getType("agency");
    assert(type_agency == PersistenceCommand<DummyType>::AGENCY && "GetType Success: Agency type mismatch");
}

void test_PersistenceCommand_GetType_Fail_Invalid() {
    PersistenceCommand<DummyType> cmd("", PersistenceCommand<DummyType>::LOAD);
    bool thrown = false;
    try {
        cmd.getType("invalid_type");
    } catch (const runtime_error& e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Invalid type given (invalid_type)") && "GetType Fail Invalid: Incorrect error message");
        assert(str_contains(what, "possible types are: agent|agency") && "GetType Fail Invalid: Missing possible types");
    } catch (...) {
         assert(false && "GetType Fail Invalid: Caught unexpected exception type");
    }
    assert(thrown && "GetType Fail Invalid: Should have thrown runtime_error");
}

void test_PersistenceCommand_GetTypeName_Success() {
     PersistenceCommand<DummyType> cmd("", PersistenceCommand<DummyType>::LOAD);
     string name_agent = cmd.getTypeName(PersistenceCommand<DummyType>::AGENT);
     assert(name_agent == "agent" && "GetTypeName Success: Agent name mismatch");
     string name_agency = cmd.getTypeName(PersistenceCommand<DummyType>::AGENCY);
     assert(name_agency == "agency" && "GetTypeName Success: Agency name mismatch");
}

void test_PersistenceCommand_GetTypeName_Fail_Invalid() {
    PersistenceCommand<DummyType> cmd("", PersistenceCommand<DummyType>::LOAD);
    bool thrown = false;
    try {
        // Cast an invalid integer to the enum type
        cmd.getTypeName(static_cast<PersistenceCommand<DummyType>::Type>(99)); 
    } catch (const runtime_error& e) {
        thrown = true;
        string what = e.what();
        // The specific integer value might vary depending on enum underlying type, check for core message
        assert(str_contains(what, "Invalid type given (99)") && "GetTypeName Fail Invalid: Incorrect error message"); 
        assert(str_contains(what, "possible types are: agent|agency") && "GetTypeName Fail Invalid: Missing possible types");
    } catch (...) {
         assert(false && "GetTypeName Fail Invalid: Caught unexpected exception type");
    }
    assert(thrown && "GetTypeName Fail Invalid: Should have thrown runtime_error");
}

void test_PersistenceCommand_execute_SaveSuccess() {
    // Test successful save execution
    PersistenceCommand cmd;
    auto result = cmd.execute("save testdata");
    assert(result.status == CommandStatus::SUCCESS);
}

void test_PersistenceCommand_execute_SaveInvalidPath() {
    // Test save with invalid path
    PersistenceCommand cmd;
    auto result = cmd.execute("save invalid/../path");
    assert(result.status == CommandStatus::ERROR);
}

void test_PersistenceCommand_execute_LoadSuccess() {
    // Test successful load execution
    PersistenceCommand cmd;
    auto result = cmd.execute("load valid_data");
    assert(result.status == CommandStatus::SUCCESS);
}

void test_PersistenceCommand_execute_LoadFailure() {
    // Test load of non-existent data
    PersistenceCommand cmd;
    auto result = cmd.execute("load missing_data");
    assert(result.status == CommandStatus::ERROR);
}


// --- Register Tests ---
TEST(test_PersistenceCommand_Constructor_Load);
TEST(test_PersistenceCommand_Constructor_Save);
TEST(test_PersistenceCommand_GetName_Load);
TEST(test_PersistenceCommand_GetName_Save);
TEST(test_PersistenceCommand_GetPatterns);
TEST(test_PersistenceCommand_GetDescription_Load);
TEST(test_PersistenceCommand_GetDescription_Save);
TEST(test_PersistenceCommand_GetUsage_Load); // Basic check for usage
TEST(test_PersistenceCommand_Validate_Success_MinArgs);
TEST(test_PersistenceCommand_Validate_Success_WithFilename);
TEST(test_PersistenceCommand_Validate_Fail_TooFewArgs);
TEST(test_PersistenceCommand_GetType_Success);
TEST(test_PersistenceCommand_GetType_Fail_Invalid);
TEST(test_PersistenceCommand_GetTypeName_Success);
TEST(test_PersistenceCommand_GetTypeName_Fail_Invalid);
TEST(test_PersistenceCommand_execute_SaveSuccess);
TEST(test_PersistenceCommand_execute_SaveInvalidPath);
TEST(test_PersistenceCommand_execute_LoadSuccess);
TEST(test_PersistenceCommand_execute_LoadFailure);

#endif // TEST
