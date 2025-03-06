#pragma once

#include "../Transcriber.hpp"

using namespace tools::voice;

// Mock Transcriber
class MockTranscriber : public Transcriber {
public:
    MockTranscriber(const string& conf = "", const char* lang = nullptr) : Transcriber(conf, lang) {}
    string transcribe(const vector<float>& audio_data) override {
        return audio_data.empty() ? "" : "transcribed_text";
    }
};