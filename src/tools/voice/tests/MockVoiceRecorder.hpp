#pragma once

#include "../VoiceRecorder.hpp"

using namespace tools::voice;

// Mock VoiceRecorder for testing
class MockVoiceRecorder : public VoiceRecorder {
public:
    MockVoiceRecorder(double sampleRate, unsigned long framesPerBuffer, size_t bufferSeconds)
        : VoiceRecorder(sampleRate, framesPerBuffer, bufferSeconds) {
        // Override the stream_thread to avoid real PortAudio
    }
    
    ~MockVoiceRecorder() {
        if (paStream) {
            Pa_StopStream(paStream);
            Pa_CloseStream(paStream);
            Pa_Terminate();
        }
    }

    // Simulate audio data availability
    void set_available(size_t samples) { available_samples = samples; }
    size_t available() const override { return available_samples; }

    // Simulate reading audio data
    void set_audio_data(const vector<float>& data) {
        audio_data = data;
        read_pos = 0;
    }
    void read_audio(float* dest, size_t maxSamples) override {
        size_t samples_to_read = min(maxSamples, audio_data.size() - read_pos);
        for (size_t i = 0; i < samples_to_read; ++i) {
            dest[i] = audio_data[read_pos + i];
        }
        read_pos += samples_to_read;
    }

private:
    size_t available_samples = 0;
    vector<float> audio_data;
    size_t read_pos = 0;
};