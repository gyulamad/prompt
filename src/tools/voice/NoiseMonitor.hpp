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
            vector<float>& buffer,
            bool muted
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

        virtual ~NoiseMonitor() {
            stop(); // Ensure cleanup in destructor
        }

        virtual bool start(void* listener, NoiseCallback cb, long pollIntervalMs, bool throws = false) {
            lock_guard<mutex> lock(monitorMutex);
            if (monitorThread.joinable()) {
                if (throws) throw ERROR("Monitor is already running");
                return false;
            }
            running = true;
            joined = false;
            monitorThread = thread([=, this]{
                vector<float> buffer(window);
                while (running) {
                    Pa_Sleep(pollIntervalMs);
                    const size_t avail = recorder.available();
                    if (avail >= window) {
                        recorder.read_audio(buffer.data(), window);
                        float rms = calculate_rms(buffer);
                        if (rms >= rmax) rmax = rms;
                        float vol_pc = muted ? 0.0 : (rms / rmax);
                        if (isnan(vol_pc)) vol_pc = 0;
                        bool noisy = vol_pc > threshold_pc;
                        cb(listener, vol_pc, threshold_pc, rmax, rms, noisy, buffer, muted);
                        rmax += ((rmax < rmax * threshold_pc * 2) ? 1 : -1) * rmax * rmax_decay_pc;
                    }
                }
            });
            return true;
        }

        // inline void mute() {
        //     muted = true;
        // }

        // inline void unmute() {
        //     muted = false;
        // }

        inline void set_muted(bool muted) {
            this->muted = muted;
        }

        inline bool is_muted() const {
            return muted;
        }

        virtual void stop() {{
            lock_guard<mutex> lock(monitorMutex);
                running = false;
            }
            if (monitorThread.joinable() && !joined.exchange(true)) {
                monitorThread.join();
            }
        }

        // void pause() {
        //     paused = true;
        //     recorder.pause();
        // }

        // void resume() {
        //     paused = false;
        //     recorder.resume();
        // }   

    // TODO: private
        float calculate_rms(const vector<float>& buffer) {
            float sum = 0;
            for (float sample : buffer) sum += sample * sample;
            return sqrt(sum / buffer.size());
        }

    private:
        VoiceRecorder& recorder;

    protected:
        float threshold_pc;
        atomic<bool> running{false};

    private:
        float rmax_decay_pc;
        size_t window;
        // atomic<bool> paused{true};
        atomic<bool> joined{false};
        mutex monitorMutex;
        thread monitorThread;
        bool muted = false;
    };

}

#ifdef TEST

#include "../utils/Test.hpp"
#include "../utils/Suppressor.hpp"

#include "tests/MockVoiceRecorder.hpp"

using namespace tools::utils;
using namespace tools::voice;

// Test NoiseMonitor constructor
void test_NoiseMonitor_constructor_valid() {
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        NoiseMonitor monitor(recorder, 0.1f, 0.01f, 1024);
    }
    assert(true && "NoiseMonitor constructor should initialize without crashing");
}

// Test calculate_rms with zero buffer
void test_NoiseMonitor_calculate_rms_zero_buffer() {
    float expected = 0.0f;
    float actual = expected + 1;
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        NoiseMonitor monitor(recorder, 0.1f, 0.01f, 1024);
        
        vector<float> buffer(1024, 0.0f);
        actual = monitor.calculate_rms(buffer);
    }
    assert(actual == expected && "RMS of zero buffer should be 0");
}

// Test calculate_rms with known values
void test_NoiseMonitor_calculate_rms_known_values() {
    float expected = 1.0f;
    float actual = expected + 1;
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        NoiseMonitor monitor(recorder, 0.1f, 0.01f, 1024);

        vector<float> buffer = {1.0f, -1.0f, 1.0f, -1.0f};
        actual = monitor.calculate_rms(buffer); // sqrt((1^2 + (-1)^2 + 1^2 + (-1)^2) / 4) = 1
    }
    assert(actual == expected && "RMS calculation incorrect for known values");
}

// Test set_muted and is_muted
void test_NoiseMonitor_set_muted_state() {
    bool expected1 = true;
    bool actual1 = !expected1;
    bool expected2 = false;
    bool actual2 = !expected2;
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        NoiseMonitor monitor(recorder, 0.1f, 0.01f, 1024);

        monitor.set_muted(true);
        actual1 = monitor.is_muted();
        
        monitor.set_muted(false);
        actual2 = monitor.is_muted();
    }
    assert(actual1 == expected1 && "set_muted(true) should set muted state to true");
    assert(actual2 == expected2 && "set_muted(false) should set muted state to false");
}

// Test start and stop (basic functionality)
void test_NoiseMonitor_start_stop() {
    try {
        Suppressor supressor(stderr);
        VoiceRecorder recorder(16000.0, 512, 5);
        NoiseMonitor monitor(recorder, 0.1f, 0.01f, 1024);
    
        bool called = false;
        monitor.start(nullptr, [&](void*, float, float, float, float, bool, vector<float>&, bool) {
            called = true;
        }, 10);
        
        // Give some time for the thread to potentially process
        Pa_Sleep(50);
        monitor.stop();
    } catch (...) {
        assert(false && "start() and stop() should complete without crashing");
    }
}

// Test callback behavior with simulated audio data
void test_NoiseMonitor_start_callback_behavior() {
    float last_vol_pc = -1.0f;
    bool callback_called = false;
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        NoiseMonitor monitor(recorder, 0.1f, 0.01f, 1024);
    
        vector<float> audio_data(1024, 0.5f); // Constant 0.5 amplitude
        recorder.set_available(1024);
        recorder.set_audio_data(audio_data);
        
        monitor.start(nullptr, [&](void*, float vol_pc, float /*threshold_pc*/, float /*rmax*/, float rms, bool /*is_noisy*/, vector<float>& buffer, bool /*muted*/) {
            last_vol_pc = vol_pc;
            callback_called = true;
            assert(buffer.size() == 1024 && "Buffer size should match window");
            assert(rms > 0.0f && "RMS should be positive with non-zero input");
        }, 10);
        
        Pa_Sleep(50); // Allow thread to process
        monitor.stop();
    }
    
    assert(callback_called && "Callback should be called with available audio");
    assert(last_vol_pc > 0.0f && "Volume percentage should be positive with audio input");
}

// Test thread safety with multiple threads calling start/stop
void test_NoiseMonitor_thread_safety_start_stop() {
    int expected = 0;
    int actual = expected + 1;
    {
        Suppressor supressor(stderr); // Suppress stderr output
        MockVoiceRecorder recorder(16000.0, 512, 5);
        NoiseMonitor monitor(recorder, 0.1f, 0.01f, 1024);
    
        const int num_threads = 4;
        const int iterations = 5;
        vector<thread> threads;
        atomic<int> success_count(0);
        
        auto worker = [&monitor, &success_count]() {
            for (int i = 0; i < iterations; ++i) {
                bool started = monitor.start(nullptr, 
                    [](void*, float, float, float, float, bool, vector<float>&, bool) {}, 
                    1, false);
                if (started) {
                    Pa_Sleep(1);
                    monitor.stop();
                    success_count++;
                }
            }
        };
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back(worker);
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        actual = success_count.load();
        expected = iterations;
    }
    assert(actual == expected && "All start/stop operations should complete successfully in multi-threaded environment");
}

// Additional test to verify throw behavior
void test_NoiseMonitor_start_throws_when_running() {
    bool thrown = false;
    try {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        NoiseMonitor monitor(recorder, 0.1f, 0.01f, 1024);

        // Start the monitor
        monitor.start(nullptr, [](void*, float, float, float, float, bool, vector<float>&, bool) {}, 10);
        
        // Try to start again with throws = true
        monitor.start(nullptr, [](void*, float, float, float, float, bool, vector<float>&, bool) {}, 10, true);
        monitor.stop();
    } catch (const runtime_error& e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Monitor is already running") && "Exception message should indicate monitor is running");
    }
    assert(thrown && "start() should throw when already running with throws = true");
}

// Additional test to verify fail-silent behavior
void test_NoiseMonitor_start_fails_silently_when_running() {
    bool started = true;
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        NoiseMonitor monitor(recorder, 0.1f, 0.01f, 1024);
    
        // Start the monitor
        monitor.start(nullptr, [](void*, float, float, float, float, bool, vector<float>&, bool) {}, 10);
        
        // Try to start again with throws = false, join = false
        started = monitor.start(nullptr, [](void*, float, float, float, float, bool, vector<float>&, bool) {}, 10, false);
        
        monitor.stop();
    }

    assert(!started && "start() should return false when already running with throws = false");
}

// Test edge case: empty buffer
void test_NoiseMonitor_start_empty_buffer() {
    bool callback_called = false;
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        NoiseMonitor monitor(recorder, 0.1f, 0.01f, 1024);
    
        recorder.set_available(0); // No data available
        
        monitor.start(nullptr, [&](void*, float, float, float, float, bool, vector<float>&, bool) {
            callback_called = true;
        }, 10);
        
        Pa_Sleep(50);
        monitor.stop();
    }
    
    assert(!callback_called && "Callback should not be called with empty buffer");
}

// Register tests
TEST(test_NoiseMonitor_constructor_valid);
TEST(test_NoiseMonitor_calculate_rms_zero_buffer);
TEST(test_NoiseMonitor_calculate_rms_known_values);
TEST(test_NoiseMonitor_set_muted_state);
TEST(test_NoiseMonitor_start_stop);
TEST(test_NoiseMonitor_start_callback_behavior);
TEST(test_NoiseMonitor_thread_safety_start_stop);
TEST(test_NoiseMonitor_start_throws_when_running);
TEST(test_NoiseMonitor_start_fails_silently_when_running);
TEST(test_NoiseMonitor_start_empty_buffer);

#endif