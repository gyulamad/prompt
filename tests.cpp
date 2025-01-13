#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <vector>

#include "Agent.hpp"

using namespace std;
namespace fs = std::filesystem;

void test_Agent() {
    // Clean up previous test files
    const string agents_folder = "agents/";
    fs::remove_all(agents_folder);

    // Test 1: Initialize agent and ensure default state
    Agent agent;
    assert(agent.summary == "");
    assert(agent.history == "");
    assert(agent.objective == "");
    assert(agent.notes == "");
    assert(agent.current_file == "");

    // Test 2: Save default agent
    assert(agent.run_internal("/save"));
    assert(fs::exists(agents_folder + "default"));

    // Test 3: Add data and save
    agent.summary = "Test Summary";
    agent.history = "Test History";
    agent.objective = "Test Objective";
    agent.notes = "Test Notes";
    assert(agent.run_internal("/save"));

    // Check saved content
    ifstream file(agents_folder + "default");
    stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    string saved_content = buffer.str();
    assert(saved_content.find("summary: Test Summary") != string::npos);
    assert(saved_content.find("history: Test History") != string::npos);
    assert(saved_content.find("objective: Test Objective") != string::npos);
    assert(saved_content.find("notes: Test Notes") != string::npos);

    // Test 4: Load agent
    Agent new_agent;
    assert(new_agent.run_internal("/load default"));
    assert(new_agent.summary == "Test Summary");
    assert(new_agent.history == "Test History");
    assert(new_agent.objective == "Test Objective");
    assert(new_agent.notes == "Test Notes");
    assert(new_agent.current_file == "default");

    // Test 5: List saved agents
    stringstream list_output;
    streambuf* original_cout = cout.rdbuf(list_output.rdbuf());
    assert(agent.run_internal("/list"));
    cout.rdbuf(original_cout);
    assert(list_output.str().find("default") != string::npos);

    // Test 6: Save and load with custom file name
    assert(agent.run_internal("/save custom_agent"));
    assert(fs::exists(agents_folder + "custom_agent"));

    Agent custom_agent;
    assert(custom_agent.run_internal("/load custom_agent"));
    assert(custom_agent.summary == "Test Summary");

    // Test 7: Help command
    stringstream help_output;
    cout.rdbuf(help_output.rdbuf());
    assert(agent.run_internal("/help"));
    cout.rdbuf(original_cout);
    assert(help_output.str().find("/save") != string::npos);
    assert(help_output.str().find("/load") != string::npos);
    assert(help_output.str().find("/exit") != string::npos);

    // Test 8: Exit command
    assert(agent.run_internal("/exit"));


    //Error Handling
    Agent errorAgent;
    assert(!errorAgent.run_internal("/load nonExistentFile")); // Check for file not found error


    //Large File Test (Simplified -  replace with a larger file for a real test)
    ofstream largeFile("agents/large.txt");
    largeFile << string(1024 * 1024, 'a'); // 1MB file
    largeFile.close();
    Agent largeAgent;
    assert(largeAgent.run_internal("/load large.txt")); //Should load without crashing
    fs::remove("agents/large.txt");


    //Edge Cases in Command Handling
    assert(!agent.run_internal("")); //Empty command
    assert(!agent.run_internal("/show ")); //Command with extra space
    assert(!agent.run_internal("/invalid_command")); //Invalid Command
    assert(agent.run_internal("/show all")); //Command with a parameter

    //History Length (Simplified - Adjust for a more robust test)
    Agent historyAgent;
    string longHistory(historyAgent.history_max_length + 100, 'x');
    historyAgent.history = longHistory;
    assert(historyAgent.run_internal("/save"));
    assert(historyAgent.run_internal("/load default"));
    assert(historyAgent.history.length() <= historyAgent.history_max_length);

    //Command Combinations (Simplified example)
    Agent comboAgent;
    comboAgent.summary = "Initial Summary";
    assert(comboAgent.run_internal("/save"));
    comboAgent.summary = "Modified Summary";
    assert(comboAgent.run_internal("/save"));
    assert(comboAgent.run_internal("/load default"));
    assert(comboAgent.summary == "Modified Summary");

    //File Format Robustness (Simplified example)
    ofstream malformedFile("agents/malformed.txt");
    malformedFile << "summary Test Summary\nhistory:Test History"; //Missing colon
    malformedFile.close();
    Agent malformedAgent;
    assert(!malformedAgent.run_internal("/load malformed.txt")); //Check for error loading
    fs::remove("agents/malformed.txt");

}

int main() {
    test_Agent();
    std::cout << "All tests passed!" << std::endl;
}
