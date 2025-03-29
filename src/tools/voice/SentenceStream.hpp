#pragma once

#include <string>
#include <vector>

#include "../abstracts/Stream.hpp"
#include "../utils/ERROR.hpp"
#include "BasicSentenceSeparation.hpp"

using namespace std;
using namespace tools::abstracts;
using namespace tools::utils;

namespace tools::voice {

    class SentenceStream: public Stream<string> {
    public:
        SentenceStream(
            SentenceSeparation& separator, 
            size_t max_buffer_size // = 1024 * 1024
        ):
            separator(separator), 
            buffer(), sentences(), current_pos(0),
            max_buffer_size(max_buffer_size)
        {}

        // void write(const string& data) override {
        //     if (buffer.size() + data.size() > max_buffer_size) {
        //         flush();
        //         throw ERROR("Buffer overflow detected, flushed buffer");
        //     }
        //     buffer += data;
        //     processBuffer();
        // }
        void write(const string& data) override {
            if (buffer.size() + data.size() > max_buffer_size) {
                buffer += data; // Add the data first
                flush();        // Flush the full buffer
                throw ERROR("Buffer overflow detected, flushed buffer");
            }
            buffer += data;
            processBuffer();
        }

        bool available() override {
            return !sentences.empty();
        }

        string read() override {
            if (!available()) return "";
            string sentence = sentences.front();
            sentences.erase(sentences.begin());
            return sentence;
        }

        string peek() override {
            if (!available()) return "";
            return sentences.front();
        }

        void flush() override {
            if (buffer.empty()) return;
            sentences.push_back(buffer);
            buffer.clear();
        }

        bool eof() override {
            return buffer.empty() && sentences.empty();
        }

        void close() override {
            flush();
            buffer.clear();
            sentences.clear();
        }

    private:
        SentenceSeparation& separator;
        string buffer;
        vector<string> sentences;
        size_t current_pos;
        size_t max_buffer_size;

        void processBuffer() {
            size_t last_pos = 0; // Always start from the beginning
        
            while (true) {
                size_t end_pos = separator.findSentenceEnd(buffer, last_pos);
                if (end_pos == string::npos) break;
                string sentence = buffer.substr(last_pos, end_pos - last_pos + 1);
                trim(sentence);
                if (!sentence.empty()) sentences.push_back(sentence);
                last_pos = end_pos + 1;
            }
            
            if (last_pos > 0) {
                buffer = buffer.substr(last_pos);
            }
            current_pos = 0; // Reset to start of remaining buffer
        }

        void trim(string& s) { // TODO: use the str::trim() util instead
            size_t start = s.find_first_not_of(" \t\n\r");
            size_t end = s.find_last_not_of(" \t\n\r");
            if (start == string::npos) s.clear();
            else s = s.substr(start, end - start + 1);
        }
    };

#ifdef TEST

using namespace tools::voice;

void test_SentenceStream_write_basic_sentence() {
    BasicSentenceSeparation sep({"."});
    SentenceStream stream(sep, 1024);
    stream.write("Hello world.");
    bool actual_available = stream.available();
    string actual_sentence = stream.read();
    assert(actual_available && "Should have a sentence available after write");
    assert(actual_sentence == "Hello world." && "Should read the correct sentence");
}

void test_SentenceStream_write_multiple_sentences() {
    BasicSentenceSeparation sep({"."});
    SentenceStream stream(sep, 1024);
    stream.write("First. Second.");
    string actual_first = stream.read();
    string actual_second = stream.read();
    assert(actual_first == "First." && "Should read first sentence correctly");
    assert(actual_second == "Second." && "Should read second sentence correctly");
}

void test_SentenceStream_write_partial_then_complete() {
    BasicSentenceSeparation sep({"."});
    SentenceStream stream(sep, 1024);
    stream.write("Hello ");
    bool actual_available1 = stream.available();
    stream.write("world.");
    bool actual_available2 = stream.available();
    string actual_sentence = stream.read();
    assert(!actual_available1 && "Should not have sentence before period");
    assert(actual_available2 && "Should have sentence after completing with period");
    assert(actual_sentence == "Hello world." && "Should combine partials into full sentence");
}

void test_SentenceStream_peek_does_not_remove() {
    BasicSentenceSeparation sep({"."});
    SentenceStream stream(sep, 1024);
    stream.write("Test.");
    string actual_peek = stream.peek();
    string actual_read = stream.read();
    assert(actual_peek == "Test." && "Peek should return the sentence");
    assert(actual_read == "Test." && "Read should still return the same sentence after peek");
}

void test_SentenceStream_flush_partial_buffer() {
    BasicSentenceSeparation sep({"."});
    SentenceStream stream(sep, 1024);
    stream.write("Partial text");
    stream.flush();
    string actual_sentence = stream.read();
    assert(actual_sentence == "Partial text" && "Flush should move partial buffer to sentences");
}

void test_SentenceStream_buffer_overflow() {
    BasicSentenceSeparation sep({"."});
    SentenceStream stream(sep, 10);
    bool thrown = false;
    try {
        stream.write("This is too long for buffer.");
    } catch (exception &e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Buffer overflow detected") && "Exception message should indicate overflow");
    }
    assert(thrown && "Should throw on buffer overflow");
    string actual_flushed = stream.read();
    assert(actual_flushed == "This is too long for buffer." && "Flushed content should be readable");
}

void test_SentenceStream_eof_empty() {
    BasicSentenceSeparation sep({"."});
    SentenceStream stream(sep, 1024);
    bool actual_eof = stream.eof();
    assert(actual_eof && "Should be EOF when empty");
}

void test_SentenceStream_eof_after_read() {
    BasicSentenceSeparation sep({"."});
    SentenceStream stream(sep, 1024);
    stream.write("Done.");
    stream.read();
    bool actual_eof = stream.eof();
    assert(actual_eof && "Should be EOF after reading all sentences");
}

void test_SentenceStream_close_clears_state() {
    BasicSentenceSeparation sep({"."});
    SentenceStream stream(sep, 1024);
    stream.write("Leftover text");
    stream.close();
    bool actual_available = stream.available();
    bool actual_eof = stream.eof();
    assert(!actual_available && "Should have no sentences after close");
    assert(actual_eof && "Should be EOF after close");
}

TEST(test_SentenceStream_write_basic_sentence);
TEST(test_SentenceStream_write_multiple_sentences);
TEST(test_SentenceStream_write_partial_then_complete);
TEST(test_SentenceStream_peek_does_not_remove);
TEST(test_SentenceStream_flush_partial_buffer);
TEST(test_SentenceStream_buffer_overflow);
TEST(test_SentenceStream_eof_empty);
TEST(test_SentenceStream_eof_after_read);
TEST(test_SentenceStream_close_clears_state);

#endif

}  // namespace tools::voice