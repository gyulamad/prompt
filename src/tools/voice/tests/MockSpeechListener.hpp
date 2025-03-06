#pragma once

#include "../SpeechListener.hpp"

using namespace tools::voice;

// Mock SpeechListener
class MockSpeechListener : public SpeechListener {
public:
    MockSpeechListener(NoiseMonitor& monitor) : SpeechListener(monitor) {}
    void start(RMSCallback rms_cb, SpeechCallback speech_cb, long) override {
        this->rms_cb = rms_cb;
        this->speech_cb = speech_cb;
    }
    void stop() override {}
    void simulate_noise(float vol_pc, bool is_noisy, const vector<float>& buffer, bool muted) {
        noise_cb(this, vol_pc, 0.1f, 1.0f, vol_pc * 1.0f, is_noisy, const_cast<vector<float>&>(buffer), muted);
    }
};