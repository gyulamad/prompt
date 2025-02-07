#pragma once

#include "VoiceRecorder.hpp"

using namespace std;

namespace tools::rec {

    class NoiseMonitor {
    public:
        using NoiseCallback = function<void(
            void* listener, 
            bool is_noisy, 
            float rms, 
            float threshold, 
            vector<float>& buffer
        )>;

        NoiseMonitor(VoiceRecorder& recorder, float threshold, size_t window)
            : recorder(recorder), threshold(threshold), window(window) {}

        void start(void* listener, NoiseCallback cb, long pollIntervalMs) {
            monitorThread = thread([=, this]{
                vector<float> buffer(window);
                while (running) {
                    const size_t avail = recorder.available();
                    if (avail >= window) {
                        recorder.read_audio(buffer.data(), window);
                        float rms = calculate_rms(buffer);
                        bool noisy = rms > threshold;
                        cb(listener, noisy, rms, threshold, buffer);
                    }
                    Pa_Sleep(pollIntervalMs);
                }
            });
        }

        void stop() {
            running = false;
            while (!monitorThread.joinable());
            monitorThread.join();
        }

    private:
        VoiceRecorder& recorder;
        float threshold;
        size_t window;
        atomic<bool> running{true};
        thread monitorThread;

        float calculate_rms(const vector<float>& buffer) {
            float sum = 0;
            for (float sample : buffer) sum += sample * sample;
            return sqrt(sum / buffer.size());
        }
    };

}