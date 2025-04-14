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
            string clean = "";
            for (size_t i = 0; i < chunk.size(); i++) {
                buffer += chunk[i];
                if (in_tokens) inner += chunk[i];

                bool in_start_token = false;
                for (size_t i = 1; i <= start_token.size(); i++)
                    if (buffer.ends_with(start_token.substr(0, i))) {
                        in_start_token = true;
                        break;
                    }
                bool in_stop_token = false;
                for (size_t i = 1; i <= stop_token.size(); i++)
                    if (buffer.ends_with(stop_token.substr(0, i))) {
                        in_stop_token = true;
                        break;
                    }
                if (!in_start_token && !in_stop_token && !in_tokens) clean += chunk[i];

                if (buffer.ends_with(start_token)) {
                    in_tokens = true;
                    inner = ""; // Reset inner buffer when start token is found
                }
                if (buffer.ends_with(stop_token)) {
                    in_tokens = false;
                    cb(inner.substr(0, inner.size() - stop_token.size()));
                    // clean += start_token + inner;
                    inner = "";
                }
            }
            return clean;
        }

        void reset() {
            buffer = "";
            in_tokens = false;
            inner = "";
        }

        string buffer;
        bool in_tokens;
        string inner;
    };
    
}


#ifdef TEST

#include "../../../src/tools/utils/Test.hpp" // Assuming Test.hpp is correctly located relative to FrameTokenParser.hpp
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
    auto callback = [&](const string& content) {
        extracted_content = content;
    };

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


#endif // TEST
