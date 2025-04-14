#pragma once

#include <string>
#include <vector>

#include "../abstracts/Stream.hpp"
#include "../utils/ERROR.hpp"
#include "../str/trim.hpp"
#include "BasicSentenceSeparation.hpp"

using namespace std;
using namespace tools::abstracts;
using namespace tools::utils;
using namespace tools::str;

namespace tools::voice {

    class SentenceStream: public Stream<string> {
    public:
        SentenceStream(
            Owns& owns,
            // SentenceSeparation& separator, 
            void* separator,
            size_t max_buffer_size // = 1024 * 1024
        ):
            owns(owns),
            // separator(separator), 
            separator(owns.reserve<SentenceSeparation>(this, separator, FILELN)),
            buffer(), sentences(), current_pos(0),
            max_buffer_size(max_buffer_size)
        {}

        virtual ~SentenceStream() {
            owns.release(this, separator);
        }
        
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

        bool error() override { // TODO
            STUB("Need to be implemented");
            throw ERROR("Unimplemented");
        }

    private:
        Owns& owns;
        SentenceSeparation* separator = nullptr;
        string buffer;
        vector<string> sentences;
        size_t current_pos;
        size_t max_buffer_size;

        void processBuffer() {
            size_t last_pos = 0; // Always start from the beginning
        
            while (true) {
                size_t end_pos = safe(separator)->findSentenceEnd(buffer, last_pos);
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
    assert(actual_second == " Second." && "Should read second sentence correctly");
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

void test_SentenceStream_write_no_separator() {
    BasicSentenceSeparation sep({"."});
    SentenceStream stream(sep, 1024);
    stream.write("Hello world");
    bool actual_available = stream.available();
    string actual_sentence = stream.read();
    assert(!actual_available && "Should not have sentence without separator");
    assert(actual_sentence == "" && "Should return empty string when no sentence available");
}

void test_SentenceStream_write_not_ending_with_separator() {
    BasicSentenceSeparation sep({"."});
    SentenceStream stream(sep, 1024);
    stream.write("First. Second");
    string actual_first = stream.read();
    bool actual_available_after = stream.available();
    string actual_second = stream.read();
    assert(actual_first == "First." && "Should read complete sentence");
    assert(!actual_available_after && "Should not have sentence without ending separator");
    assert(actual_second == "" && "Should return empty string for remaining partial");
    stream.write(".");
    bool final_available = stream.available();
    string final_sentence = stream.read();
    assert(final_available && "Should have sentence after adding separator");
    assert(final_sentence == " Second." && "Should complete partial sentence");
}

void test_SentenceStream_flush_last_chunk_unfinished() {
    BasicSentenceSeparation sep({"."});
    SentenceStream stream(sep, 1024);
    stream.write("First. Last chunk");
    string actual_first = stream.read();
    assert(actual_first == "First." && "Should read complete sentence");
    assert(!stream.available() && "No sentence available without separator");
    stream.flush(); // Sender finishes, TTS should get the last chunk
    bool actual_available = stream.available();
    string actual_last = stream.read();
    assert(actual_available && "Should have last chunk available after flush");
    assert(actual_last == " Last chunk" && "Should read unfinished last chunk after flush");
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
TEST(test_SentenceStream_write_no_separator);
TEST(test_SentenceStream_write_not_ending_with_separator);
TEST(test_SentenceStream_flush_last_chunk_unfinished);

#endif

}  // namespace tools::voice