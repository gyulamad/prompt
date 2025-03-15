#pragma once

#include "../Transcriber.hpp"

using namespace tools::voice;

// Mock Transcriber
class MockTranscriber: public Transcriber {
public:
    MockTranscriber(): Transcriber() {}
    string transcribe(const vector<float>& audio_data) override {
        return audio_data.empty() ? "" : "transcribed_text";
    }
};