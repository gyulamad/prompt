#pragma once

#include "../utils/ERROR.hpp"
#include "NoiseMonitor.hpp"

using namespace tools::utils;

namespace tools::voice {

    class SpeechListener {
    public:
        using RMSCallback = function<void(float vol_pc, float threshold_pc, float rmax, float rms, bool loud, bool muted)>;
        using SpeechCallback = function<void(vector<float>& record)>;

        SpeechListener(NoiseMonitor& monitor): monitor(monitor) {}
        
        virtual ~SpeechListener() {}

        virtual void start(RMSCallback rms_cb, SpeechCallback speech_cb, long pollIntervalMs) {
            this->rms_cb = rms_cb;
            this->speech_cb = speech_cb;
            monitor.start(this, noise_cb, pollIntervalMs); // Check every 100ms
        }

        virtual void stop() {
            monitor.stop();
        }

        // void pause() {
        //     monitor.pause();
        // }

        // void resume() {
        //     monitor.resume();
        // }   
    // TODO private
        static void noise_cb(
            void* listener, 
            float vol_pc, 
            float threshold_pc, 
            float rmax, 
            float rms, 
            bool is_noisy, 
            vector<float>& buffer,
            bool muted
        ) {
            NULLCHK(listener, "Listener cannot be null");
            SpeechListener* that = (SpeechListener*)listener;
            if (is_noisy) for (float sample: buffer) that->record.push_back(sample);
            else if (!is_noisy && that->is_noisy_prev) if (that->speech_cb) that->speech_cb(that->record);
            that->is_noisy_prev = is_noisy;
            if (that->rms_cb) that->rms_cb(vol_pc, threshold_pc, rmax, rms, is_noisy, muted);
        };

        
    protected:
        RMSCallback rms_cb = nullptr;
        SpeechCallback speech_cb = nullptr;

    private:
        NoiseMonitor& monitor;

        bool is_noisy_prev = false;
        int n = 0;
        vector<float> record;

    };

}

#ifdef TEST

#include "../utils/Test.hpp"

#include "tests/MockNoiseMonitor.hpp"

using namespace tools::voice;

// Test SpeechListener constructor
void test_SpeechListener_constructor_valid() {
    {
        Suppressor supressor(stderr);
        VoiceRecorder recorder(16000.0, 512, 5);
        NoiseMonitor monitor(recorder, 0.1f, 0.01f, 1024);
        SpeechListener listener(monitor);
    }
    assert(true && "SpeechListener constructor should initialize without crashing");
}

// Test start with basic callback functionality
void test_SpeechListener_start_basic() {
    {
        Suppressor supressor(stderr);
        VoiceRecorder recorder(16000.0, 512, 5);
        NoiseMonitor monitor(recorder, 0.1f, 0.01f, 1024);
        SpeechListener listener(monitor);
    
        bool rms_called = false;
        bool speech_called = false;
        
        listener.start(
            [&](float, float, float, float, bool, bool) {
                rms_called = true;
            },
            [&](vector<float>&) {
                speech_called = true;
            },
            10
        );
        
        Pa_Sleep(50); // Give thread time to run
        listener.stop();
    }
    assert(true && "start() should complete without crashing");
}

// Test noise callback - speech detection
void test_SpeechListener_noise_cb_speech_detection() {
    size_t expected_size = 0; // Should be empty since we didn't set speech_cb
    size_t actual_size = expected_size + 1;
    {
        Suppressor supressor(stderr);
        VoiceRecorder recorder(16000.0, 512, 5);
        NoiseMonitor monitor(recorder, 0.1f, 0.01f, 1024);
        SpeechListener listener(monitor);
    
        vector<float> record;
        
        // Simulate noise callback directly
        vector<float> buffer(1024, 1.0f); // Non-zero values to simulate noise
        
        // First call: noisy (should add to record)
        SpeechListener::noise_cb(
            &listener,
            0.2f,      // vol_pc
            0.1f,      // threshold_pc
            1.0f,      // rmax
            0.5f,      // rms
            true,      // is_noisy
            buffer,    // buffer
            false      // muted
        );
        
        // Second call: not noisy (should trigger speech callback)
        SpeechListener::noise_cb(
            &listener,
            0.05f,     // vol_pc
            0.1f,      // threshold_pc
            1.0f,      // rmax
            0.5f,      // rms
            false,     // is_noisy
            buffer,    // buffer
            false      // muted
        );
        
        actual_size = record.size();
    }
    assert(actual_size == expected_size && "Record should not grow without speech callback set");
}

// Test noise callback - continuous noise
void test_SpeechListener_noise_cb_continuous_noise() {
    size_t expected_size = 2048; // Two buffers of noise
    size_t actual_size = expected_size + 1;
    {
        Suppressor supressor(stderr);
        VoiceRecorder recorder(16000.0, 512, 5);
        NoiseMonitor monitor(recorder, 0.1f, 0.01f, 1024);
        SpeechListener listener(monitor);
    
        vector<float> captured_record;
        listener.start(
            [](float, float, float, float, bool, bool) {},
            [&](vector<float>& record) {
                captured_record = record;
            },
            10
        );
        
        vector<float> buffer(1024, 1.0f);
        
        // Simulate continuous noise
        SpeechListener::noise_cb(
            &listener,
            0.2f, true, 1.0f, 0.5f, true, buffer, false
        );
        SpeechListener::noise_cb(
            &listener,
            0.2f, true, 1.0f, 0.5f, true, buffer, false
        );
        
        // Then silence
        SpeechListener::noise_cb(
            &listener,
            0.05f, true, 1.0f, 0.5f, false, buffer, false
        );
        
        Pa_Sleep(50);
        listener.stop();
        
        actual_size = captured_record.size();
    }
    assert(actual_size == expected_size && "Should capture all noise samples until silence");
}

// Test stop functionality
void test_SpeechListener_stop() {
    try {
        Suppressor supressor(stderr);
        VoiceRecorder recorder(16000.0, 512, 5);
        NoiseMonitor monitor(recorder, 0.1f, 0.01f, 1024);
        SpeechListener listener(monitor);
    
        listener.start(
            [](float, float, float, float, bool, bool) {},
            [](vector<float>&) {},
            10
        );
        listener.stop();
    } catch (...) {
        assert(false && "stop() should complete without crashing");
    }
}

// Test with muted state
void test_SpeechListener_muted_state() {        
    bool rms_called = false;
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        MockNoiseMonitor monitor(recorder);
        SpeechListener listener(monitor);
        
        listener.start(
            [&](float, float, float, float, bool, bool muted) {
                rms_called = true;
                assert(muted && "Muted state should be reported as true");
            },
            [](vector<float>&) {},
            10
        );
        
        monitor.simulate_noise(&listener, 0.2f, true, vector<float>(1024, 0.5f), true); // Muted
        listener.stop();
    }
    
    assert(rms_called && "RMS callback should be called in muted state");
}

// Test edge case: empty buffer
void test_SpeechListener_empty_buffer() {
    bool speech_called = false;
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        MockNoiseMonitor monitor(recorder);
        SpeechListener listener(monitor);
        
        listener.start(
            [](float, float, float, float, bool, bool) {},
            [&](vector<float>& record) {
                speech_called = true;
                assert(record.empty() && "Record should be empty with empty buffer");
            },
            10
        );
        
        vector<float> empty_buffer;
        monitor.simulate_noise(&listener, 0.2f, true, empty_buffer);
        monitor.simulate_noise(&listener, 0.05f, false, empty_buffer);
        
        listener.stop();
    }
    
    assert(speech_called && "Speech callback should handle empty buffer");
}

// Comprehensive callback verification
void test_SpeechListener_callback_comprehensive() {
    struct CallbackData {
        int rms_count = 0;
        int speech_count = 0;
        float last_vol_pc = -1.0f;
        float last_rms = -1.0f;
        bool last_noisy = false;
        vector<float> last_record;
    } data;
        
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        MockNoiseMonitor monitor(recorder);
        SpeechListener listener(monitor);
        
        listener.start(
            [&](float vol_pc, float threshold_pc, float rmax, float rms, bool noisy, bool muted) {
                data.rms_count++;
                data.last_vol_pc = vol_pc;
                data.last_rms = rms;
                data.last_noisy = noisy;
                assert(threshold_pc == 0.1f && "Threshold should match NoiseMonitor");
                assert(rmax == 1.0f && "Rmax should match simulated value");
                assert(!muted && "Muted should be false by default");
            },
            [&](vector<float>& record) {
                data.speech_count++;
                data.last_record = record;
            },
            10
        );
        
        vector<float> buffer(1024, 0.5f);
        monitor.simulate_noise(&listener, 0.3f, true, buffer);  // Noisy
        monitor.simulate_noise(&listener, 0.2f, true, buffer);  // Still noisy
        monitor.simulate_noise(&listener, 0.05f, false, buffer); // Silence
        
        listener.stop();
    }
    
    assert(data.rms_count == 3 && "RMS callback should be called for each simulation");
    assert(data.speech_count == 1 && "Speech callback should be called once when noise ends");
    assert(data.last_vol_pc == 0.05f && "Last volume should match final simulation");
    assert(data.last_rms == 0.05f && "Last RMS should match final simulation");
    assert(!data.last_noisy && "Last noisy state should be false");
    assert(data.last_record.size() == 2048 && "Record should accumulate two noisy buffers");
}

// Test with controlled NoiseMonitor input
void test_SpeechListener_controlled_input() {
    int speech_count = 0;
    vector<float> accumulated_record;
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        MockNoiseMonitor monitor(recorder);
        SpeechListener listener(monitor);
        
        listener.start(
            [](float, float, float, float, bool, bool) {},
            [&](vector<float>& record) {
                speech_count++;
                accumulated_record.insert(accumulated_record.end(), record.begin(), record.end());
            },
            10
        );
        
        vector<float> buffer1(512, 1.0f);
        vector<float> buffer2(512, 0.5f);
        monitor.simulate_noise(&listener, 0.2f, true, buffer1);  // First noisy buffer
        monitor.simulate_noise(&listener, 0.15f, true, buffer2); // Second noisy buffer
        monitor.simulate_noise(&listener, 0.05f, false, buffer1); // Silence
        
        listener.stop();
    }
    
    assert(speech_count == 1 && "Speech callback should be called once");
    assert(accumulated_record.size() == 1024 && "Record should combine both buffers");
    assert(accumulated_record[0] == 1.0f && "First buffer samples should be preserved");
    assert(accumulated_record[512] == 0.5f && "Second buffer samples should be preserved");
}

// Register tests
TEST(test_SpeechListener_constructor_valid);
TEST(test_SpeechListener_start_basic);
TEST(test_SpeechListener_noise_cb_speech_detection);
TEST(test_SpeechListener_noise_cb_continuous_noise);
TEST(test_SpeechListener_stop);
TEST(test_SpeechListener_muted_state);
TEST(test_SpeechListener_empty_buffer);
TEST(test_SpeechListener_callback_comprehensive);
TEST(test_SpeechListener_controlled_input);

#endif