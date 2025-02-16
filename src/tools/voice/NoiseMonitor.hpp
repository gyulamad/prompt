#pragma once

#include <functional>
#include <thread>
#include <cmath>
#include <portaudio.h>

#include "VoiceRecorder.hpp"

using namespace std;

namespace tools::voice {

    class NoiseMonitor {
    private:
        // ------ RMS (max/decay) ------
        float rmax = -INFINITY;
    public:
        using NoiseCallback = function<void(
            void* listener, 
            float vol_pc, 
            float threshold_pc,
            float rmax, 
            float rms, 
            bool is_noisy, 
            vector<float>& buffer
        )>;

        NoiseMonitor(
            VoiceRecorder& recorder, 
            float threshold_pc, 
            float rmax_decay_pc, 
            size_t window
        ): 
            recorder(recorder), 
            threshold_pc(threshold_pc), 
            rmax_decay_pc(rmax_decay_pc),
            window(window) 
        {}

        void start(void* listener, NoiseCallback cb, long pollIntervalMs) {
            // cout << "[DEBUG] NoiseMonitor monitor thread start..." << endl;
            monitorThread = thread([=, this]{
                vector<float> buffer(window);
                while (running) {
                    Pa_Sleep(pollIntervalMs);
                    // if (paused) continue;
                    const size_t avail = recorder.available();
                    if (avail >= window) {
                        recorder.read_audio(buffer.data(), window);
                        float rms = calculate_rms(buffer);
                        if (rms >= rmax) rmax = rms;
                        float vol_pc = rms / rmax;
                        bool noisy = vol_pc > threshold_pc;

                        cb(listener, vol_pc, threshold_pc, rmax, rms, noisy, buffer);

                        // const float rmax_decay_pc = 0.01;
                        rmax += ((rmax < rmax * threshold_pc * 2) ? 1 : -1) * rmax * rmax_decay_pc;
                    }
                }
            });
        }

        void stop() {
            running = false;
            //while (!monitorThread.joinable());
            monitorThread.join();
        }

        // void pause() {
        //     paused = true;
        //     recorder.pause();
        // }

        // void resume() {
        //     paused = false;
        //     recorder.resume();
        // }   

    private:
        VoiceRecorder& recorder;
        float threshold_pc;
        float rmax_decay_pc;
        size_t window;
        // atomic<bool> paused{true};
        atomic<bool> running{true};
        thread monitorThread;

        float calculate_rms(const vector<float>& buffer) {
            float sum = 0;
            for (float sample : buffer) sum += sample * sample;
            return sqrt(sum / buffer.size());
        }
    };

}