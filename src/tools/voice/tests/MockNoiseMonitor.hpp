#pragma once

#include "../NoiseMonitor.hpp"
#include "../SpeechListener.hpp"

using namespace std;
using namespace tools::voice;

// Mock NoiseMonitor for controlled input
class MockNoiseMonitor : public NoiseMonitor {
public:
    MockNoiseMonitor(VoiceRecorder& recorder)
        : NoiseMonitor(recorder, 0.1f, 0.01f, 1024) {}

    // Simulate starting the monitor with custom behavior
    void simulate_noise(SpeechListener* listener, float vol_pc, bool is_noisy, const vector<float>& buffer, bool muted = false) {
        NoiseCallback cb = get_callback();
        if (cb) {
            float rms = vol_pc * 1.0f; // Simplified RMS for testing
            float rmax = 1.0f;
            cb(listener, vol_pc, threshold_pc, rmax, rms, is_noisy, const_cast<vector<float>&>(buffer), muted);
        }
    }

    bool start(void* /*listener*/, NoiseCallback cb, long /*pollIntervalMs*/, bool /*throws*/ = false) override {
        lock_guard<mutex> lock(monitor_mutex); // Use inherited mutex
        callback = cb; // Store callback for simulation
        running = true;
        // Don’t start a real thread; we’ll simulate manually
        return true;
    }

    void stop() override {
        lock_guard<mutex> lock(monitor_mutex);
        running = false;
        callback = nullptr;
    }

private:
    NoiseCallback callback;
    mutex monitor_mutex; // Ensure thread-safety for callback access

    NoiseCallback get_callback() {
        lock_guard<mutex> lock(monitor_mutex);
        return callback;
    }
};