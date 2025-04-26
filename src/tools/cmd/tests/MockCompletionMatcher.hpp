#pragma once

#include <string>
#include <vector>

using namespace std;
using namespace tools::cmd;

class MockCompletionMatcher : public CompletionMatcher {
private:
    vector<string> _expected_completions; // Internal storage for expected completions
    bool _has_trailing_space = false;     // To track parse_input behavior

public:
    MockCompletionMatcher() : CompletionMatcher() {}

    // Configure expected completions for the next call to get_completions()
    void set_expected_completions(const vector<string>& completions) {
        _expected_completions = completions;
    }

    vector<string> get_completions(const string& /*input*/) override {
        return _expected_completions;
    }

    // Mock parse_input implementation (for testing input parsing)
    vector<string> parse_input(const string& input, bool& has_trailing_space, bool = true) override {
        has_trailing_space = _has_trailing_space;
        return CompletionMatcher::parse_input(input, has_trailing_space); // Or custom logic
    }

    // Optional: Define a method to set trailing space expectation
    void set_has_trailing_space(bool value) {
        _has_trailing_space = value;
    }
};