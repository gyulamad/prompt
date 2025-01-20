#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "../Agent.hpp"

namespace fs = std::filesystem;

// Helper function to clean up test environment
void cleanup_environment(const std::string& folder) {
    fs::remove_all(folder);
}

// Test 1: Initialize agent and ensure default state
void test_initialize_agent() {
    cleanup_environment("agents/");

    Agent agent;
    assert(agent.summary == "");
    assert(agent.history == "");
    assert(agent.objective == "");
    assert(agent.notes == "");
    assert(agent.current_file == "");
}

// Test 2: Save default agent
void test_save_default_agent() {
    Agent agent;
    assert(agent.run_internal("/save"));
    assert(fs::exists("agents/default"));
}

// Test 3: Add data and save
void test_add_data_and_save() {
    Agent agent;
    agent.summary = "Test Summary";
    agent.history = "Test History";
    agent.objective = "Test Objective";
    agent.notes = "Test Notes";
    assert(agent.run_internal("/save"));

    std::string content = file_get_contents("agents/default");
    assert(content.find("Test Summary") != std::string::npos);
    assert(content.find("Test History") != std::string::npos);
    assert(content.find("Test Objective") != std::string::npos);
    assert(content.find("Test Notes") != std::string::npos);
}

// Test 4: Load agent
void test_load_agent() {
    Agent agent;
    agent.summary = "Test Summary";
    agent.history = "Test History";
    agent.objective = "Test Objective";
    agent.notes = "Test Notes";
    agent.run_internal("/save");

    Agent new_agent;
    assert(new_agent.run_internal("/load default"));
    assert(new_agent.summary == "Test Summary");
    assert(new_agent.history == "Test History");
    assert(new_agent.objective == "Test Objective");
    assert(new_agent.notes == "Test Notes");
}

// Test 5: List saved agents
void test_list_saved_agents() {
    Agent agent;
    agent.run_internal("/save");

    std::stringstream output;
    std::streambuf* original_cout = std::cout.rdbuf(output.rdbuf());
    agent.run_internal("/list");
    std::cout.rdbuf(original_cout);

    assert(output.str().find("default") != std::string::npos);
}

// Test 6: Save and load with custom file name
void test_save_load_custom_file() {
    Agent agent;
    agent.summary = "Custom Summary";
    assert(agent.run_internal("/save custom_agent"));

    Agent custom_agent;
    assert(custom_agent.run_internal("/load custom_agent"));
    assert(custom_agent.summary == "Custom Summary");
}

// Test 7: Help command
void test_help_command() {
    Agent agent;

    std::stringstream output;
    std::streambuf* original_cout = std::cout.rdbuf(output.rdbuf());
    agent.run_internal("/help");
    std::cout.rdbuf(original_cout);

    assert(output.str().find("/save") != std::string::npos);
    assert(output.str().find("/load") != std::string::npos);
    assert(output.str().find("/exit") != std::string::npos);
}

// Test 8: Exit command
// void test_exit_command() { // TODO: we can not call exit in the tests
//     Agent agent;
//     assert(agent.run_internal("/exit"));
//     assert(fs::exists("agents/default"));
// }

// Error handling tests
void test_error_handling() {
    Agent agent;
    assert(!agent.run_internal("/load nonExistentFile"));
}

// Large file handling
void test_large_file_handling() {
    // Create a large JSON file with valid content
    nlohmann::json large_data = {
        {"summary", std::string(1024 * 1024, 'a')}, // 1MB string
        {"history", "Large test history"},
        {"objective", "Large test objective"},
        {"notes", "Large test notes"}
    };

    std::ofstream largeFile("agents/large.json");
    largeFile << large_data.dump(4); // Pretty-print JSON with indentation
    largeFile.close();

    Agent agent;
    assert(agent.run_internal("/load large.json"));
    fs::remove("agents/large.json");
}

// Command edge cases
void test_command_edge_cases() {
    Agent agent;
    assert(!agent.run_internal("")); // Empty command
    assert(agent.run_internal("/show ")); // Command with extra space should succeed after trimming
    assert(!agent.run_internal("/invalid_command")); // Invalid command
    assert(agent.run_internal("/save default")); // Valid command with parameter
}

// History length validation
void test_history_length_validation() {
    Agent agent;
    std::string longHistory(agent.history_max_length + 100, 'x');
    agent.history = longHistory;
    agent.run_internal("/save");
    agent.run_internal("/load default");
    assert(agent.history.length() <= agent.history_max_length);
}

// Command combinations
void test_command_combinations() {
    Agent agent;
    agent.summary = "Initial Summary";
    agent.run_internal("/save");
    agent.summary = "Modified Summary";
    agent.run_internal("/save");
    agent.run_internal("/load default");
    assert(agent.summary == "Modified Summary");
}

// File format robustness
void test_file_format_robustness() {
    std::ofstream malformedFile("agents/malformed.txt");
    malformedFile << "summary Test Summary\nhistory:Test History"; // Missing colon
    malformedFile.close();

    Agent agent;
    assert(!agent.run_internal("/load malformed.txt"));
    fs::remove("agents/malformed.txt");
}

// Main test runner
void run_tests() {
    test_initialize_agent();
    test_save_default_agent();
    test_add_data_and_save();
    test_load_agent();
    test_list_saved_agents();
    test_save_load_custom_file();
    test_help_command();
    // test_exit_command();
    test_error_handling();
    test_large_file_handling();
    test_command_edge_cases();
    test_history_length_validation();
    test_command_combinations();
    test_file_format_robustness();

    std::cout << "All tests passed successfully!" << std::endl;
}

int main() {
    run_tests();
    return 0;
}
