#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include "../str/implode.hpp"

#include "LineEditor.hpp"
#include "CompletionMatcher.hpp"

using namespace std;
using namespace tools::str;

namespace tools::cmd {

    class CommandLine {
    private:
        LineEditor& line_editor;
        bool exited = false;
        string prompt_suffix;
        string history_path;
        bool multi_line;
        size_t history_max_length;

    public:
        CommandLine(
            LineEditor& editor,
            const string& prompt_suffix = "> ",
            const string& history_path = "",
            bool multi_line = true,
            size_t history_max_length = 0
        ): line_editor(editor),
            prompt_suffix(prompt_suffix),
            history_path(history_path),
            multi_line(multi_line),
            history_max_length(history_max_length)
        {}

        // // Move constructor
        // CommandLine(CommandLine&& other) noexcept
        //     : line_editor(other.line_editor),
        //     exited(other.exited),
        //     prompt_suffix(move(other.prompt_suffix)),
        //     history_path(move(other.history_path)),
        //     multi_line(other.multi_line),
        //     history_max_length(other.history_max_length) {
        //     other.exited = false; // Reset moved-from state if needed
        // }

        // // Move assignment operator (optional but good practice)
        // CommandLine& operator=(CommandLine&& other) noexcept {
        //     if (this != &other) {
        //         line_editor = move(other.line_editor);
        //         exited = other.exited;
        //         prompt_suffix = move(other.prompt_suffix);
        //         history_path = move(other.history_path);
        //         multi_line = other.multi_line;
        //         history_max_length = other.history_max_length;
        //         other.exited = false; // Reset moved-from state if needed
        //     }
        //     return *this;
        // }

        // // Deleted copy constructor and assignment to be explicit
        // CommandLine(const CommandLine&) = delete;
        // CommandLine& operator=(const CommandLine&) = delete;

        virtual ~CommandLine() = default;

        LineEditor& getEditorRef() { return line_editor; }

        bool isExited() const {
            return exited;
        }

        atomic<bool> promptVisible = true;
        string prompt;
        void setPromptVisible(bool promptVisible) {
            // if (this->promptVisible == promptVisible) return;
            this->promptVisible = promptVisible;
            if (promptVisible) setPrompt(prompt);
            else line_editor.setPrompt("");
        }

        void setPrompt(const string& prompt) {
            this->prompt = prompt;
            if (!promptVisible) return;
            line_editor.setPrompt(string(prompt + prompt_suffix).c_str());
            line_editor.refreshLine();
        }

        void setPromptSuffix(const string& prompt_suffix) {
            this->prompt_suffix = prompt_suffix;
        }

        string getPromptSuffix() const {
            return this->prompt_suffix;
        }

        // void set_keypress_callback(LineEditor::KeypressCallback cb) {
        //     line_editor.SetKeypressCallback(cb);
        // }

        void setCompletionMatcher(CompletionMatcher& completion_matcher) {
            line_editor.setCompletionCallback([&](const char* input, vector<string>& completions) {
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
                line_editor.wipeLine();
                // cout << endl;
                for (const string& elem : all_completions) {
                    cout << elem << endl;
                }
                line_editor.refreshLine();
                // show(input);
            });
        }

        // void show(const string& input = "") {
        //     cout << prompt_suffix << input << flush;
        // }

        string readln() {
            line_editor.setMultiLine(multi_line);
            if (history_max_length) {
                line_editor.setHistoryMaxLen(history_max_length); // TODO: MAKE THE HISTORY GREAT AGAIN!
            }
            if (!history_path.empty()) {
                line_editor.loadHistory(history_path.c_str());
            }
            string line = "";
            exited = line_editor.readLine(line);
            // line_editor.addHistory(line.c_str()); // TODO
            if (!history_path.empty()) {
                // line_editor.saveHistory(history_path.c_str()); TODO
            }
            return line;
        }

        void clearln() {
            line_editor.wipeLine();
        }
    };

} // namespace tools::cmd


#ifdef TEST

#include "../utils/Test.hpp"

#include "tests/MockLineEditor.hpp"

void test_CommandLine_is_exited_initial_state() {
    MockLineEditor mock;
    CommandLine cl(mock);
    bool actual = cl.isExited();
    assert(actual == false && "CommandLine should not be exited initially");
    // No fix needed: no post-move access to mock
}

void test_CommandLine_is_exited_after_readline_exit() {
    MockLineEditor mock;
    mock.should_exit = true;
    mock.next_input = "exit";
    CommandLine cl(mock);
    cl.readln(); // Trigger exit condition
    bool actual = cl.isExited();
    assert(actual == true && "CommandLine should be exited after Readline returns true");
    // Fixed: Added mock_ptr to set mock state before move
}

void test_CommandLine_set_prompt_suffix_changes_prompt_suffix() {
    MockLineEditor mock;
    CommandLine cl(mock, "> ", "test_history.txt", true, 10);
    cl.setPromptSuffix("new> ");
    string actual = cl.getPromptSuffix();
    assert(actual == "new> " && "set_prompt_suffix should update the prompt_suffix string");
    // No fix needed: no direct mock access
}

void test_CommandLine_get_prompt_suffix_returns_initial_prompt_suffix() {
    MockLineEditor mock;
    CommandLine cl(mock, "> ", "test_history.txt", true, 10);
    string actual = cl.getPromptSuffix();
    assert(actual == "> " && "get_prompt_suffix should return the initial prompt_suffix");
    // No fix needed: no direct mock access
}

void test_CommandLine_readln_reads_input() {
    MockLineEditor mock;
    mock.next_input = "test input";
    CommandLine cl(mock, "> ", "test_history.txt", true, 10);
    string actual = cl.readln();
    assert(actual == "test input" && "readln should return the input from the editor");
    // Fixed: Added mock_ptr to set next_input before move
}

void test_CommandLine_readln_sets_multi_line() {
    MockLineEditor mock;
    CommandLine cl(mock, "> ", "test_history.txt", true, 10);
    cl.readln();
    bool actual = mock.multi_line_enabled; // Use raw pointer
    assert(actual == true && "readln should enable multi-line mode based on constructor arg");
    // Already fixed in your example
}

void test_CommandLine_readln_sets_history_max_length() {
    MockLineEditor mock;
    CommandLine cl(mock, "> ", "test_history.txt", true, 10);
    cl.readln();
    size_t actual = mock.max_history_len;
    assert(actual == 10 && "readln should set history max length based on constructor arg");
    // Fixed: Added mock_ptr to access max_history_len after move
}

void test_CommandLine_readln_loads_history() {
    MockLineEditor mock;
    CommandLine cl(mock, "> ", "test_history.txt", true, 10);
    cl.readln();
    string actual = mock.loaded_history_path;
    assert(actual == "test_history.txt" && "readln should load history from the specified path");
    // Fixed: Added mock_ptr to access loaded_history_path after move
}

void test_CommandLine_readln_adds_to_history() {
    MockLineEditor mock;
    mock.next_input = "test command";
    CommandLine cl(mock, "> ", "test_history.txt", true, 10);
    cl.readln();
    size_t actual_size = mock.history.size();
    string actual_entry = mock.history[0];
    assert(actual_size == 1 && "readln should add input to history");
    assert(actual_entry == "test command" && "readln should add correct input to history");
    // Fixed: Added mock_ptr to set next_input and access history after move
}

void test_CommandLine_readln_saves_history() {
    MockLineEditor mock;
    CommandLine cl(mock, "> ", "test_history.txt", true, 10);
    cl.readln();
    string actual = mock.saved_history_path;
    assert(actual == "test_history.txt" && "readln should save history to the specified path");
    // Fixed: Added mock_ptr to access saved_history_path after move
}


TEST(test_CommandLine_is_exited_initial_state);
TEST(test_CommandLine_is_exited_after_readline_exit);
TEST(test_CommandLine_set_prompt_suffix_changes_prompt_suffix);
TEST(test_CommandLine_get_prompt_suffix_returns_initial_prompt_suffix);
TEST(test_CommandLine_readln_reads_input);
TEST(test_CommandLine_readln_sets_multi_line);
TEST(test_CommandLine_readln_sets_history_max_length);
TEST(test_CommandLine_readln_loads_history);
TEST(test_CommandLine_readln_adds_to_history);
TEST(test_CommandLine_readln_saves_history);

#endif