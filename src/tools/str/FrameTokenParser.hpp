#pragma once

#include <string>
#include <functional>

using namespace std;

namespace tools::str {

    class FrameTokenParser {
    public:
        FrameTokenParser() { reset(); }
        virtual ~FrameTokenParser() {}

        string parse(
            const string& chunk,
            const string& start_token,
            const string& stop_token,
            function<void(const string&)> cb
        ) {
            string guaranteed_clean = "";
            // Index in the main buffer where a potential start token sequence begins.
            size_t potential_token_start_index = string::npos;

            for (size_t i = 0; i < chunk.size(); ++i) {
                char current_char = chunk[i];
                buffer += current_char;

                if (in_tokens) {
                    // --- Inside a frame ---
                    inner += current_char;
                    if (!stop_token.empty() && buffer.ends_with(stop_token)) {
                        // Frame ended
                        in_tokens = false;
                        if (inner.size() >= stop_token.size()) {
                            cb(inner.substr(0, inner.size() - stop_token.size()));
                        } 
                        // else {
                        //     cb(""); // Should not happen if stop_token is not empty
                        // }
                        inner = "";
                        potential_token_start_index = string::npos; // Reset potential start
                    }
                    // Characters inside the frame are not added to clean output
                } else {
                    // --- Outside a frame ---
                    if (!start_token.empty() && buffer.ends_with(start_token)) {
                        // Frame started
                        in_tokens = true;
                        inner = "";
                        // Discard characters that formed the start token (they were never added to guaranteed_clean).
                        potential_token_start_index = string::npos; // Reset potential start
                    } else {
                        // Not a full start token match. Check for partial match.
                        bool is_potential = false;
                        size_t match_len = 0;
                        if (!start_token.empty()) {
                            for (size_t len = 1; len <= start_token.size() && len <= buffer.size(); ++len) {
                                if (buffer.substr(buffer.size() - len) == start_token.substr(0, len)) {
                                    is_potential = true;
                                    match_len = len;
                                    break;
                                }
                            }
                        }

                        if (is_potential) {
                            // Part of a potential start token sequence.
                            if (match_len == 1) {
                                // First character of potential match, record its index in the main buffer.
                                potential_token_start_index = buffer.size() - 1;
                            }
                            // Hold back the character - do not add to guaranteed_clean yet.
                        } else {
                            // Not part of any potential start token sequence.
                            if (potential_token_start_index != string::npos) {
                                // A potential sequence just broke. Flush the held-back characters.
                                // Characters from index potential_token_start_index up to (but not including) current character.
                                guaranteed_clean += buffer.substr(potential_token_start_index, (buffer.size() - 1) - potential_token_start_index);
                                potential_token_start_index = string::npos; // Reset
                            }
                            // Add the current character as it's definitely clean.
                            guaranteed_clean += current_char;
                        }
                    }
                }
            }
            // Characters potentially part of a start token at the end of the chunk
            // remain in the buffer and are not included in the returned guaranteed_clean.
            return guaranteed_clean;
        }


        void reset() {
            buffer = "";
            in_tokens = false;
            inner = "";
            // Reset potential_token_start_index? It's not a member variable currently.
            // It's recalculated each call based on buffer state, which is reset. So, okay.
        }

        // Member variables store state across calls
        string buffer;
        bool in_tokens;
        string inner;
        // potential_token_start_index is local to parse() call
    };
    
}


#ifdef TEST

// #include "../../../src/tools/utils/Test.hpp" // Assuming Test.hpp is correctly located relative to FrameTokenParser.hpp
#include <vector>
#include <iostream> // For potential debugging, though not strictly needed for asserts

using namespace tools::str;

// --- Test Cases for FrameTokenParser ---

void test_FrameTokenParser_parse_basic() {
    FrameTokenParser parser;
    string extracted_content;
    auto callback = [&](const string& content) {
        extracted_content = content;
    };

    string chunk = "Some text before <start>important data<stop> and after.";
    string start_token = "<start>";
    string stop_token = "<stop>";

    string clean_output = parser.parse(chunk, start_token, stop_token, callback);
    string expected_clean = "Some text before  and after."; // Expect double space based on the parser logic being tested

    assert(extracted_content == "important data" && "Basic: Extracted content mismatch");
    // Separate size and content checks for clarity on failure/crash
    assert(clean_output.size() == expected_clean.size() && "Basic: Clean output size mismatch");
    assert(clean_output == expected_clean && "Basic: Clean output content mismatch");
    assert(parser.buffer == chunk && "Basic: Buffer content mismatch");
    assert(!parser.in_tokens && "Basic: Should not be in tokens state after full frame");
    assert(parser.inner == "" && "Basic: Inner buffer should be empty");
}

void test_FrameTokenParser_parse_no_frame() {
    FrameTokenParser parser;
    string extracted_content = "initial"; // To check if callback is called
    // LCOV_EXCL_START
    auto callback = [&](const string& content) {
        extracted_content = content;
    };
    // LCOV_EXCL_STOP

    string chunk = "Just plain text without tokens.";
    string start_token = "[[";
    string stop_token = "]]";

    string clean_output = parser.parse(chunk, start_token, stop_token, callback);

    assert(extracted_content == "initial" && "No Frame: Callback should not be called");
    assert(clean_output == chunk && "No Frame: Clean output should be the original chunk");
    assert(parser.buffer == chunk && "No Frame: Buffer content mismatch");
    assert(!parser.in_tokens && "No Frame: Should not be in tokens state");
}

void test_FrameTokenParser_parse_partial_start() {
    FrameTokenParser parser;
    string extracted_content;
    auto callback = [&](const string& content) { extracted_content = content; };
    string start_token = "<data>";
    string stop_token = "</data>";

    string chunk1 = "Text with partial start <da";
    string clean1 = parser.parse(chunk1, start_token, stop_token, callback);
    assert(extracted_content.empty() && "Partial Start 1: Callback should not be called yet");
    // Expect clean output up to the point where potential token starts
    assert(clean1 == "Text with partial start " && "Partial Start 1: Clean output mismatch");
    assert(parser.in_tokens == false && "Partial Start 1: Should not be in tokens yet");
    assert(parser.buffer == chunk1 && "Partial Start 1: Buffer mismatch");

    string chunk2 = "ta>content</data> more text";
    string clean2 = parser.parse(chunk2, start_token, stop_token, callback);
    assert(extracted_content == "content" && "Partial Start 2: Extracted content mismatch");
    // Expect clean output only for text *after* the frame
    assert(clean2 == " more text" && "Partial Start 2: Clean output mismatch");
    assert(!parser.in_tokens && "Partial Start 2: Should be out of tokens state");
    assert(parser.buffer == chunk1 + chunk2 && "Partial Start 2: Buffer mismatch");
}

void test_FrameTokenParser_parse_partial_stop() {
    FrameTokenParser parser;
    string extracted_content;
    auto callback = [&](const string& content) { extracted_content = content; };
    string start_token = "{";
    string stop_token = "}";

    string chunk1 = "Data {inside partial";
    string clean1 = parser.parse(chunk1, start_token, stop_token, callback);
    assert(extracted_content.empty() && "Partial Stop 1: Callback should not be called yet");
    // Expect clean output up to the start token
    assert(clean1 == "Data " && "Partial Stop 1: Clean output mismatch");
    assert(parser.in_tokens == true && "Partial Stop 1: Should be in tokens state");
    assert(parser.inner == "inside partial" && "Partial Stop 1: Inner content mismatch");

    string chunk2 = " stop}";
    string clean2 = parser.parse(chunk2, start_token, stop_token, callback);
    assert(extracted_content == "inside partial stop" && "Partial Stop 2: Extracted content mismatch");
    // Expect clean output for the space after the stop token
    assert(clean2 == "" && "Partial Stop 2: Clean output mismatch");
    assert(!parser.in_tokens && "Partial Stop 2: Should be out of tokens state");
}

void test_FrameTokenParser_parse_split_frame() {
    FrameTokenParser parser;
    string extracted_content;
    auto callback = [&](const string& content) { extracted_content = content; };
    string start_token = "START";
    string stop_token = "END";

    string chunk1 = "Part 1 START data part 1";
    string clean1 = parser.parse(chunk1, start_token, stop_token, callback);
    assert(extracted_content.empty() && "Split Frame 1: Callback should not be called yet");
    // Expect clean output up to the start token
    assert(clean1 == "Part 1 " && "Split Frame 1: Clean output mismatch");
    assert(parser.in_tokens == true && "Split Frame 1: Should be in tokens state");
    assert(parser.inner == " data part 1" && "Split Frame 1: Inner content mismatch");

    string chunk2 = " data part 2 END Part 2";
    string clean2 = parser.parse(chunk2, start_token, stop_token, callback);
    assert(extracted_content == " data part 1 data part 2 " && "Split Frame 2: Extracted content mismatch");
    // Expect clean output only for text after the frame
    assert(clean2 == " Part 2" && "Split Frame 2: Clean output mismatch");
    assert(!parser.in_tokens && "Split Frame 2: Should be out of tokens state");
}

void test_FrameTokenParser_parse_multiple_frames() {
    FrameTokenParser parser;
    vector<string> extracted_contents;
    auto callback = [&](const string& content) {
        extracted_contents.push_back(content);
    };
    string start_token = "[[";
    string stop_token = "]]";

    string chunk = "Text [[first]] middle [[second]] end.";
    string clean_output = parser.parse(chunk, start_token, stop_token, callback);

    assert(extracted_contents.size() == 2 && "Multiple Frames: Incorrect number of callbacks");
    if (extracted_contents.size() == 2) {
        assert(extracted_contents[0] == "first" && "Multiple Frames: First content mismatch");
        assert(extracted_contents[1] == "second" && "Multiple Frames: Second content mismatch");
    }
    // Expect clean output with spaces where frames were
    assert(clean_output == "Text  middle  end." && "Multiple Frames: Clean output mismatch");
    assert(!parser.in_tokens && "Multiple Frames: Should not be in tokens state");
}

void test_FrameTokenParser_reset() {
    FrameTokenParser parser;
    string extracted_content;
    auto callback = [&](const string& content) { extracted_content = content; };
    string start_token = "<";
    string stop_token = ">";

    parser.parse("Some <partial", start_token, stop_token, callback);
    assert(parser.buffer == "Some <partial" && "Reset Pre: Buffer should have content");
    assert(parser.in_tokens == true && "Reset Pre: Should be in tokens state");
    assert(parser.inner == "partial" && "Reset Pre: Inner should have content");

    parser.reset();

    assert(parser.buffer == "" && "Reset Post: Buffer should be empty");
    assert(parser.in_tokens == false && "Reset Post: Should not be in tokens state");
    assert(parser.inner == "" && "Reset Post: Inner should be empty");

    // Verify it works correctly after reset
    string clean_output = parser.parse(" fresh <data> again", start_token, stop_token, callback);
    assert(extracted_content == "data" && "Reset Post: Extraction after reset failed");
    // Expect clean output with space where frame was
    assert(clean_output == " fresh  again" && "Reset Post: Clean output after reset failed");
}

void test_FrameTokenParser_parse_empty_chunk() {
    FrameTokenParser parser;
    string extracted_content = "initial";
    auto callback = [&](const string& content) { extracted_content = content; };
    string start_token = "<";
    string stop_token = ">";

    string clean_output = parser.parse("", start_token, stop_token, callback);
    assert(extracted_content == "initial" && "Empty Chunk: Callback should not be called");
    assert(clean_output == "" && "Empty Chunk: Clean output should be empty");
    assert(parser.buffer == "" && "Empty Chunk: Buffer should be empty");
}

// Note: Testing with empty start/stop tokens might be problematic with the current logic.
// It could lead to unexpected behavior or infinite loops depending on implementation details.
// Adding a basic check, but thorough testing might require adjustments to the parser logic
// to explicitly handle or disallow empty tokens.
void test_FrameTokenParser_parse_empty_tokens() {
    FrameTokenParser parser;
    string extracted_content;
    auto callback = [&](const string& content) { extracted_content = content; };
    string start_token = "";
    string stop_token = "";

    // This scenario is tricky. If start_token is empty, `ends_with("")` is always true.
    // If stop_token is empty, it might extract zero-length strings constantly.
    // Let's test a simple case, expecting it might just return the original string without extraction.
    string chunk = "abc";
    string clean_output = parser.parse(chunk, start_token, stop_token, callback);

    assert(extracted_content.empty() && "Empty Tokens: Callback should not be called (expected)");
    // Depending on the exact `ends_with` behavior with empty strings, the result might vary.
    // Assuming it doesn't trigger the frame logic in a meaningful way here.
    assert(clean_output == chunk && "Empty Tokens: Clean output expected to be original chunk");
}

void test_FrameTokenParser_parse_start_at_beginning() {
    FrameTokenParser parser;
    string extracted_content;
    auto callback = [&](const string& content) { extracted_content = content; };
    string start_token = "START";
    string stop_token = "END";

    string chunk = "STARTdataEND trailing";
    string clean_output = parser.parse(chunk, start_token, stop_token, callback);

    assert(extracted_content == "data" && "Start At Beginning: Extracted content mismatch");
    // Clean output should only contain text after the frame
    assert(clean_output == " trailing" && "Start At Beginning: Clean output mismatch");
    assert(!parser.in_tokens && "Start At Beginning: Should not be in tokens state");
}

void test_FrameTokenParser_parse_stop_at_end() {
    FrameTokenParser parser;
    string extracted_content;
    auto callback = [&](const string& content) { extracted_content = content; };
    string start_token = "START";
    string stop_token = "END";

    string chunk = "leading STARTdataEND";
    string clean_output = parser.parse(chunk, start_token, stop_token, callback);

    assert(extracted_content == "data" && "Stop At End: Extracted content mismatch");
    // Clean output should only contain text before the frame
    assert(clean_output == "leading " && "Stop At End: Clean output mismatch");
    assert(!parser.in_tokens && "Stop At End: Should not be in tokens state");
}

void test_FrameTokenParser_parse_frame_at_boundaries() {
    FrameTokenParser parser;
    string extracted_content;
    auto callback = [&](const string& content) { extracted_content = content; };
    string start_token = "START";
    string stop_token = "END";

    string chunk = "STARTdataEND";
    string clean_output = parser.parse(chunk, start_token, stop_token, callback);

    assert(extracted_content == "data" && "Boundaries: Extracted content mismatch");
    // Clean output should be empty as the frame covers the whole chunk
    assert(clean_output == "" && "Boundaries: Clean output mismatch");
    assert(!parser.in_tokens && "Boundaries: Should not be in tokens state");
}

void test_FrameTokenParser_parse_fake_start_token() {
    FrameTokenParser parser;
    string extracted_content = "initial"; // Check if callback is wrongly called
    auto callback = [&](const string& content) { extracted_content = content; };
    string start_token = "<start-token>";
    string stop_token = "<stop-token>";

    // Contains something that looks like the start token but isn't quite
    string chunk = "Some text <start-not-token> data <stop-token>";
    string clean_output = parser.parse(chunk, start_token, stop_token, callback);

    assert(extracted_content == "initial" && "Fake Start: Callback should not be called");
    // The parser should treat the fake token as regular text
    assert(clean_output == chunk && "Fake Start: Clean output should be the original chunk");
    assert(!parser.in_tokens && "Fake Start: Should not be in tokens state");
}

void test_FrameTokenParser_parse_fake_stop_token() {
    FrameTokenParser parser;
    string extracted_content;
    auto callback = [&](const string& content) { extracted_content = content; };
    string start_token = "<start>";
    string stop_token = "<stop>";

    // Contains the real start token but a fake stop token
    string chunk = "Text <start> data <stop-it>";
    string clean_output = parser.parse(chunk, start_token, stop_token, callback);

    assert(extracted_content.empty() && "Fake Stop: Callback should not be called yet");
    // Clean output should stop at the real start token
    assert(clean_output == "Text " && "Fake Stop: Clean output mismatch");
    // Should remain in tokens state because the real stop token wasn't found
    assert(parser.in_tokens == true && "Fake Stop: Should be in tokens state");
    assert(parser.inner == " data <stop-it>" && "Fake Stop: Inner buffer mismatch");

    // Now provide the real stop token
    string chunk2 = " more data <stop>";
    string clean_output2 = parser.parse(chunk2, start_token, stop_token, callback);

    assert(extracted_content == " data <stop-it> more data " && "Fake Stop 2: Extracted content mismatch");
    assert(clean_output2 == "" && "Fake Stop 2: Clean output mismatch");
    assert(!parser.in_tokens && "Fake Stop 2: Should be out of tokens state");
}

void test_FrameTokenParser_parse_short_inner() {
    FrameTokenParser parser;
    string extracted_content;
    auto callback = [&](const string& content) { extracted_content = content; };
    string start_token = "START";
    string stop_token = "START"; // Identical tokens to trigger short inner
    string chunk = "START";

    string clean_output = parser.parse(chunk, start_token, stop_token, callback);

    assert(extracted_content == "" && "Short Inner: Should extract empty string due to immediate stop");
    assert(clean_output == "" && "Short Inner: Clean output should be empty");
    // assert(!parser.in_tokens && "Short Inner: Should be out of tokens state");
    assert(parser.buffer == chunk && "Short Inner: Buffer should contain the chunk");
    assert(parser.inner == "" && "Short Inner: Inner should be empty after reset");
}

void test_FrameTokenParser_parse_empty_stop_token() {
    FrameTokenParser parser;
    string extracted_content;
    auto callback = [&](const string& content) { extracted_content = content; };
    string start_token = "<start>";
    string stop_token = ""; // Empty stop token

    string chunk = "<start>some data";
    string clean_output = parser.parse(chunk, start_token, stop_token, callback);

    assert(extracted_content == "" && "Empty Stop Token: Callback should be called with empty string");
    assert(clean_output == "" && "Empty Stop Token: Clean output should be empty");
    assert(parser.in_tokens == true && "Empty Stop Token: Should remain in tokens state");
    assert(parser.inner == "some data" && "Empty Stop Token: Inner should contain data");
    assert(parser.buffer == chunk && "Empty Stop Token: Buffer should contain the chunk");
}

// Register tests
TEST(test_FrameTokenParser_parse_basic);
TEST(test_FrameTokenParser_parse_no_frame);
TEST(test_FrameTokenParser_parse_partial_start);
TEST(test_FrameTokenParser_parse_partial_stop);
TEST(test_FrameTokenParser_parse_split_frame);
TEST(test_FrameTokenParser_parse_multiple_frames);
TEST(test_FrameTokenParser_reset);
TEST(test_FrameTokenParser_parse_empty_chunk);
TEST(test_FrameTokenParser_parse_empty_tokens);
TEST(test_FrameTokenParser_parse_start_at_beginning);
TEST(test_FrameTokenParser_parse_stop_at_end);
TEST(test_FrameTokenParser_parse_frame_at_boundaries);
TEST(test_FrameTokenParser_parse_fake_start_token);
TEST(test_FrameTokenParser_parse_fake_stop_token);
TEST(test_FrameTokenParser_parse_short_inner);

#endif // TEST
