#pragma once

#include <vector>

#include "../utils/ERROR.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::voice {

    class Transcriber {
    private:
        bool inProgress = false;
        
    protected:

        void preTranscribe() {
            inProgress = true;
        }

        void postTranscribe() {
            inProgress = false;
        }

    public:
        Transcriber() {}

        virtual ~Transcriber() {}

        bool isInProgress() const {
            return inProgress;
        }    

        virtual string transcribe(const vector<float>& /*audio_data*/) = 0;
    };

}

#ifdef TEST

// #include "../utils/Test.hpp"
#include "../utils/Suppressor.hpp"

#include "tests/MockTranscriber.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::voice;

// Transcriber Tests
void test_Transcriber_constructor_valid() {
    Suppressor suppressor(stderr);
    MockTranscriber transcriber;
    assert(true && "Transcriber constructor should initialize without crashing");
}

void test_Transcriber_isInProgress_initial_state() {
    bool expected = false;
    bool actual = !expected;
    {
        Suppressor suppressor(stderr);
        MockTranscriber transcriber;
        actual = transcriber.isInProgress();
    }
    assert(actual == expected && "isInProgress should be false initially");
}

void test_Transcriber_isInProgress_during_transcription() {
    bool in_progress_before = true;
    bool in_progress_after = true;
    {
        Suppressor suppressor(stderr);
        MockTranscriber transcriber;
        in_progress_before = transcriber.isInProgress();

        vector<float> audio_data(1024, 0.5f);
        transcriber.transcribe(audio_data); // Triggers pre/postTranscribe
        in_progress_after = transcriber.isInProgress();

        // Note: Can't test in-progress state directly without modifying transcribe implementation
    }
    assert(!in_progress_before && "isInProgress should be false before transcription");
    assert(!in_progress_after && "isInProgress should be false after transcription");
}

void test_Transcriber_transcribe_empty_audio() {
    string expected = "";
    string actual = "not expected";
    {
        Suppressor suppressor(stderr);
        MockTranscriber transcriber;
        vector<float> empty_audio;
        actual = transcriber.transcribe(empty_audio);
    }
    assert(actual == expected && "Empty audio should yield empty transcription");
}

void test_Transcriber_transcribe_non_empty_audio() {
    string expected = "transcribed_text";
    string actual = "not expected";
    {
        Suppressor suppressor(stderr);
        MockTranscriber transcriber;
        vector<float> audio_data(1024, 0.5f);
        actual = transcriber.transcribe(audio_data);
    }
    assert(actual == expected && "Non-empty audio should yield mock transcription");
}

void test_Transcriber_transcribe_edge_case_zero_values() {
    string expected = "transcribed_text";
    string actual = "not expected";
    {
        Suppressor suppressor(stderr);
        MockTranscriber transcriber;
        vector<float> zero_audio(1024, 0.0f);
        actual = transcriber.transcribe(zero_audio);
    }
    assert(actual == expected && "Audio with all zeros should still yield transcription");
}

void test_Transcriber_transcribe_extreme_values() {
    string expected = "transcribed_text";
    string actual = "not expected";
    {
        Suppressor suppressor(stderr);
        MockTranscriber transcriber;
        vector<float> extreme_audio(1024);
        for (size_t i = 0; i < extreme_audio.size(); ++i) {
            extreme_audio[i] = (i % 2 == 0) ? numeric_limits<float>::max() : numeric_limits<float>::lowest();
        }
        actual = transcriber.transcribe(extreme_audio);
    }
    assert(actual == expected && "Extreme audio values should yield transcription");
}

// Register tests
TEST(test_Transcriber_constructor_valid);
TEST(test_Transcriber_isInProgress_initial_state);
TEST(test_Transcriber_isInProgress_during_transcription);
TEST(test_Transcriber_transcribe_empty_audio);
TEST(test_Transcriber_transcribe_non_empty_audio);
TEST(test_Transcriber_transcribe_edge_case_zero_values);
TEST(test_Transcriber_transcribe_extreme_values);

#endif