#pragma once

#include "../SpeechRecogniser.hpp"

using namespace tools::voice;

// Mock SpeechRecogniser
class MockSpeechRecogniser : public SpeechRecogniser {
public:
    MockSpeechRecogniser(VoiceRecorder& recorder, NoiseMonitor& monitor, SpeechListener& listener, Transcriber& transcriber)
        : SpeechRecogniser(recorder, monitor, listener, transcriber, 10) {}
    
    void start() override {
        running = true;
        listener.start(
            [this](float vol_pc, float threshold_pc, float rmax, float rms, bool loud, bool muted) {
                if (rms_cb) rms_cb(vol_pc, threshold_pc, rmax, rms, loud, muted);
            },
            [this](vector<float>& record) {
                if (speech_cb) speech_cb(record);
                if (transcribe_cb) transcribe_cb(record, "mock_transcription");
            },
            10 // pollIntervalMs (not used in mock, but kept for consistency)
        );
    }
    
    void stop() override { running = false; }
    
    void simulate_speech(vector<float>& record) { // Optional, not used here
        if (speech_cb) speech_cb(record);
        if (transcribe_cb) transcribe_cb(record, "mock_transcription");
    }
};