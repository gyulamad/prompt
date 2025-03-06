#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include "ILineEditor.hpp"
#include "CompletionMatcher.hpp"

using namespace std;
using namespace tools;

namespace tools::cmd {

    class CommandLine {
    private:
        unique_ptr<ILineEditor> line_editor;
        bool exited = false;
        string prompt;
        string history_path;
        bool multi_line;
        size_t history_max_length;

    public:
        CommandLine(
            unique_ptr<ILineEditor> editor = nullptr,
            const string& prompt = "> ",
            const string& history_path = "",
            bool multi_line = true,
            size_t history_max_length = 0
        ) : line_editor(move(editor)),
            prompt(prompt),
            history_path(history_path),
            multi_line(multi_line),
            history_max_length(history_max_length) {
            if (!line_editor) {
                throw runtime_error("ILineEditor implementation must be provided");
            }
        }

        // Move constructor
        CommandLine(CommandLine&& other) noexcept
            : line_editor(move(other.line_editor)),
            exited(other.exited),
            prompt(move(other.prompt)),
            history_path(move(other.history_path)),
            multi_line(other.multi_line),
            history_max_length(other.history_max_length) {
            other.exited = false; // Reset moved-from state if needed
        }

        // Move assignment operator (optional but good practice)
        CommandLine& operator=(CommandLine&& other) noexcept {
            if (this != &other) {
                line_editor = move(other.line_editor);
                exited = other.exited;
                prompt = move(other.prompt);
                history_path = move(other.history_path);
                multi_line = other.multi_line;
                history_max_length = other.history_max_length;
                other.exited = false; // Reset moved-from state if needed
            }
            return *this;
        }

        // Deleted copy constructor and assignment to be explicit
        CommandLine(const CommandLine&) = delete;
        CommandLine& operator=(const CommandLine&) = delete;

        ~CommandLine() = default;

        bool is_exited() const {
            return exited;
        }

        void set_prompt(const string& prompt) {
            this->prompt = prompt;
        }

        string get_prompt() const {
            return this->prompt;
        }

        void set_completion_matcher(CompletionMatcher& completion_matcher) {
            line_editor->SetCompletionCallback([&](const char* input, vector<string>& completions) {
                vector<string> all_completions = completion_matcher.get_completions(input);
                if (all_completions.size() <= 1) {
                    string input_s(input ? input : "");
                    bool has_trailing_space;
                    vector<string> parts = completion_matcher.parse_input(input, has_trailing_space);
                    if (!has_trailing_space && !parts.empty()) {
                        parts.pop_back();
                        input_s = implode(" ", parts);
                    }
                    if (!input_s.empty()) input_s += " ";
                    for (const string& elem : all_completions) {
                        completions.push_back(input_s + elem);
                    }
                    return;
                }
                cout << endl;
                for (const string& elem : all_completions) {
                    cout << elem << endl;
                }
                show(input);
            });
        }

        void show(const string& input = "") {
            cout << prompt << input << flush;
        }

        string readln() {
            line_editor->SetMultiLine(multi_line);
            if (history_max_length) {
                line_editor->SetHistoryMaxLen(history_max_length);
            }
            if (!history_path.empty()) {
                line_editor->LoadHistory(history_path.c_str());
            }
            string line;
            exited = line_editor->Readline(prompt.c_str(), line);
            line_editor->AddHistory(line.c_str());
            if (!history_path.empty()) {
                line_editor->SaveHistory(history_path.c_str());
            }
            return line;
        }
    };

} // namespace tools::cmd


#ifdef TEST

#include "tests/MockLineEditor.hpp"

void test_CommandLine_is_exited_initial_state() {
    auto mock = make_unique<MockLineEditor>();
    CommandLine cl(move(mock));
    bool actual = cl.is_exited();
    assert(actual == false && "CommandLine should not be exited initially");
    // No fix needed: no post-move access to mock
}

void test_CommandLine_is_exited_after_readline_exit() {
    auto mock = make_unique<MockLineEditor>();
    MockLineEditor* mock_ptr = mock.get(); // Get raw pointer before moving
    mock_ptr->should_exit = true;
    mock_ptr->next_input = "exit";
    CommandLine cl(move(mock));
    cl.readln(); // Trigger exit condition
    bool actual = cl.is_exited();
    assert(actual == true && "CommandLine should be exited after Readline returns true");
    // Fixed: Added mock_ptr to set mock state before move
}

void test_CommandLine_set_prompt_changes_prompt() {
    auto cl = create_command_line();
    cl->set_prompt("new> ");
    string actual = cl->get_prompt();
    assert(actual == "new> " && "set_prompt should update the prompt string");
    // No fix needed: no direct mock access
}

void test_CommandLine_get_prompt_returns_initial_prompt() {
    auto cl = create_command_line();
    string actual = cl->get_prompt();
    assert(actual == "> " && "get_prompt should return the initial prompt");
    // No fix needed: no direct mock access
}

void test_CommandLine_readln_reads_input() {
    auto mock = make_unique<MockLineEditor>();
    MockLineEditor* mock_ptr = mock.get(); // Get raw pointer before moving
    mock_ptr->next_input = "test input";
    auto cl = create_command_line(move(mock));
    string actual = cl->readln();
    assert(actual == "test input" && "readln should return the input from the editor");
    // Fixed: Added mock_ptr to set next_input before move
}

void test_CommandLine_readln_sets_multi_line() {
    auto mock = make_unique<MockLineEditor>();
    MockLineEditor* mock_ptr = mock.get(); // Get raw pointer before moving
    auto cl = create_command_line(move(mock));
    cl->readln();
    bool actual = mock_ptr->multi_line_enabled; // Use raw pointer
    assert(actual == true && "readln should enable multi-line mode based on constructor arg");
    // Already fixed in your example
}

void test_CommandLine_readln_sets_history_max_length() {
    auto mock = make_unique<MockLineEditor>();
    MockLineEditor* mock_ptr = mock.get(); // Get raw pointer before moving
    auto cl = create_command_line(move(mock));
    cl->readln();
    size_t actual = mock_ptr->max_history_len;
    assert(actual == 10 && "readln should set history max length based on constructor arg");
    // Fixed: Added mock_ptr to access max_history_len after move
}

void test_CommandLine_readln_loads_history() {
    auto mock = make_unique<MockLineEditor>();
    MockLineEditor* mock_ptr = mock.get(); // Get raw pointer before moving
    auto cl = create_command_line(move(mock));
    cl->readln();
    string actual = mock_ptr->loaded_history_path;
    assert(actual == "test_history.txt" && "readln should load history from the specified path");
    // Fixed: Added mock_ptr to access loaded_history_path after move
}

void test_CommandLine_readln_adds_to_history() {
    auto mock = make_unique<MockLineEditor>();
    MockLineEditor* mock_ptr = mock.get(); // Get raw pointer before moving
    mock_ptr->next_input = "test command";
    auto cl = create_command_line(move(mock));
    cl->readln();
    size_t actual_size = mock_ptr->history.size();
    string actual_entry = mock_ptr->history[0];
    assert(actual_size == 1 && "readln should add input to history");
    assert(actual_entry == "test command" && "readln should add correct input to history");
    // Fixed: Added mock_ptr to set next_input and access history after move
}

void test_CommandLine_readln_saves_history() {
    auto mock = make_unique<MockLineEditor>();
    MockLineEditor* mock_ptr = mock.get(); // Get raw pointer before moving
    auto cl = create_command_line(move(mock));
    cl->readln();
    string actual = mock_ptr->saved_history_path;
    assert(actual == "test_history.txt" && "readln should save history to the specified path");
    // Fixed: Added mock_ptr to access saved_history_path after move
}

void test_CommandLine_constructor_throws_without_editor() {
    bool thrown = false;
    try {
        CommandLine cl(nullptr); // No editor provided
    } catch (exception& e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "ILineEditor implementation must be provided") && 
               "Constructor should throw with correct message when no editor is provided");
    }
    assert(thrown && "Constructor should throw when no editor is provided");
    // No fix needed: no mock access after move
}


TEST(test_CommandLine_is_exited_initial_state);
TEST(test_CommandLine_is_exited_after_readline_exit);
TEST(test_CommandLine_set_prompt_changes_prompt);
TEST(test_CommandLine_get_prompt_returns_initial_prompt);
TEST(test_CommandLine_readln_reads_input);
TEST(test_CommandLine_readln_sets_multi_line);
TEST(test_CommandLine_readln_sets_history_max_length);
TEST(test_CommandLine_readln_loads_history);
TEST(test_CommandLine_readln_adds_to_history);
TEST(test_CommandLine_readln_saves_history);
TEST(test_CommandLine_constructor_throws_without_editor);

#endif