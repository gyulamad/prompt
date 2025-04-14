#pragma once

#include <string>
#include <vector>

#include "SentenceSeparation.hpp"

using namespace std;

namespace tools::voice {

    class BasicSentenceSeparation: public SentenceSeparation {
    public:
        BasicSentenceSeparation(const vector<string>& separators): SentenceSeparation(separators) {}

        virtual ~BasicSentenceSeparation() {}

        // size_t findSentenceEnd(const string& text, size_t start_pos) const override {
        //     size_t len = text.length();
        //     for (size_t i = start_pos; i < len; ++i) {
        //         char c = text[i];
        //         for (const string& sep : separators) {
        //             if (c == sep[0] && (i + 1 >= len || isspace(text[i + 1]))) {
        //                 // Skip if it looks like an abbreviation (e.g., "Mr." followed by space)
        //                 if (c == '.' && i > 0 && isalpha(text[i - 1]) && 
        //                     i + 1 < len && !ispunct(text[i + 1])) {
        //                     continue;
        //                 }
        //                 return i;
        //             }
        //         }
        //     }
        //     return string::npos;
        // }
        size_t findSentenceEnd(const string& text, size_t start_pos) const override {
            size_t len = text.length();
            for (size_t i = start_pos; i < len; ++i) {
                char c = text[i];
                for (const string& sep : separators) {
                    if (c == sep[0] && (i + 1 >= len || isspace(text[i + 1]))) {
                        if (c == '.' && i > 0 && isalpha(text[i - 1])) {
                            // Look back to find the start of the word
                            size_t word_start = i - 1;
                            while (word_start > 0 && isalpha(text[word_start - 1])) {
                                word_start--;
                            }
                            size_t word_len = i - word_start;
                            // Skip if it’s a short word (likely an abbreviation, e.g., "Mr.")
                            if (word_len <= 3 && i + 1 < len) {
                                // Skip spaces to check the next character
                                size_t next_char_pos = i + 1;
                                while (next_char_pos < len && isspace(text[next_char_pos])) {
                                    next_char_pos++;
                                }
                                // If next word starts with uppercase, don’t treat as sentence end
                                if (next_char_pos < len && isupper(text[next_char_pos])) {
                                    continue;
                                }
                            }
                        }
                        return i; // Sentence end (not an abbreviation or end of string)
                    }
                }
            }
            return string::npos;
        }
    };
    
}

#ifdef TEST

using namespace tools::voice;

void test_BasicSentenceSeparation_findSentenceEnd_basic_period() {
    BasicSentenceSeparation sep({"."});
    string text = "Hello world.";
    size_t actual = sep.findSentenceEnd(text, 0);
    size_t expected = 11;  // Position of '.'
    assert(actual == expected && "Should find period at end of sentence");
}

void test_BasicSentenceSeparation_findSentenceEnd_abbreviation() {
    BasicSentenceSeparation sep({"."});
    string text = "Mr. Smith is here.";
    size_t actual = sep.findSentenceEnd(text, 0);
    size_t expected = 17;  // Position of '.' after "here"
    assert(actual == expected && "Should skip abbreviation 'Mr.' and find sentence end");
}

void test_BasicSentenceSeparation_findSentenceEnd_exclamation() {
    BasicSentenceSeparation sep({"!"});
    string text = "Wow! What a day.";
    size_t actual = sep.findSentenceEnd(text, 0);
    size_t expected = 3;  // Position of '!'
    assert(actual == expected && "Should find exclamation mark as sentence end");
}

void test_BasicSentenceSeparation_findSentenceEnd_no_separator() {
    BasicSentenceSeparation sep({"."});
    string text = "Hello world";
    size_t actual = sep.findSentenceEnd(text, 0);
    size_t expected = string::npos;
    assert(actual == expected && "Should return npos when no separator found");
}

void test_BasicSentenceSeparation_findSentenceEnd_empty_string() {
    BasicSentenceSeparation sep({"."});
    string text = "";
    size_t actual = sep.findSentenceEnd(text, 0);
    size_t expected = string::npos;
    assert(actual == expected && "Should return npos for empty string");
}

void test_BasicSentenceSeparation_findSentenceEnd_start_pos() {
    BasicSentenceSeparation sep({"."});
    string text = "First. Second.";
    size_t actual = sep.findSentenceEnd(text, 6);  // Start after "First."
    size_t expected = 13;  // Position of '.' in "Second."
    assert(actual == expected && "Should find next sentence end from start position");
}

void test_BasicSentenceSeparation_findSentenceEnd_multiple_separators() {
    BasicSentenceSeparation sep({".", "!", "?"});
    string text = "Yes? No! Done.";
    size_t actual = sep.findSentenceEnd(text, 0);
    size_t expected = 3;  // Position of '?'
    assert(actual == expected && "Should find first separator '?' among multiple options");
}

// Register tests
TEST(test_BasicSentenceSeparation_findSentenceEnd_basic_period);
TEST(test_BasicSentenceSeparation_findSentenceEnd_abbreviation);
TEST(test_BasicSentenceSeparation_findSentenceEnd_exclamation);
TEST(test_BasicSentenceSeparation_findSentenceEnd_no_separator);
TEST(test_BasicSentenceSeparation_findSentenceEnd_empty_string);
TEST(test_BasicSentenceSeparation_findSentenceEnd_start_pos);
TEST(test_BasicSentenceSeparation_findSentenceEnd_multiple_separators);

#endif