#pragma once

#include <atomic>
#include <thread>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <cstddef>
#include <stdexcept> // For runtime_error
#include <fstream> // For file I/O
#include <cmath>
#include <functional>
#include <unistd.h> // For usleep()
#include <portaudio.h>
#include "../../../libs/ggerganov/whisper.cpp/include/whisper.h"

#include "VoiceRecorder.hpp"
#include "NoiseMonitor.hpp"
#include "SpeechListener.hpp"
#include "Transcriber.hpp"
#include "SpeechRecogniser.hpp"

using namespace std;
using namespace tools;

namespace tools::voice {

    class STT {
    public:
        STT(
            Transcriber& transcriber,
            const double stt_voice_recorder_sample_rate,
            const unsigned long stt_voice_recorder_frames_per_buffer,
            const size_t stt_voice_recorder_buffer_seconds,
            const float stt_noise_monitor_threshold_pc,
            const float stt_noise_monitor_rmax_decay_pc,
            const size_t stt_noise_monitor_window,
            // const string& stt_transcriber_model,
            // const string& stt_transcriber_lang,
            const long stt_poll_interval_ms
        ):
            transcriber(transcriber),
            stt_voice_recorder_sample_rate(stt_voice_recorder_sample_rate),
            stt_voice_recorder_frames_per_buffer(stt_voice_recorder_frames_per_buffer),
            stt_voice_recorder_buffer_seconds(stt_voice_recorder_buffer_seconds),
            stt_noise_monitor_threshold_pc(stt_noise_monitor_threshold_pc),
            stt_noise_monitor_rmax_decay_pc(stt_noise_monitor_rmax_decay_pc),
            stt_noise_monitor_window(stt_noise_monitor_window),
            stt_poll_interval_ms(stt_poll_interval_ms)
        {}

        virtual ~STT() {
            stop();
        }

        NoiseMonitor* getMonitorPtr() {
            return monitor;
        }

        SpeechRecogniser* getRecogniserPtr() {
            return recogniser;
        }

        const Transcriber& getTranscriberCRef() const {
            return transcriber;
        }

        void setRMSHandler(SpeechListener::RMSCallback rms_cb) {
            recogniser->set_rms_cb(rms_cb);
        }

        void setSpeechHandler(SpeechListener::SpeechCallback speech_cb) {
            recogniser->set_speech_cb(speech_cb);
        }

        void setTranscribeHandler(SpeechRecogniser::TranscribeCallback transcribe_cb) {
            recogniser->set_transcribe_cb(transcribe_cb);
        }

        void start() {
            lock_guard<mutex> lock(start_stop_mutex);
            if (started) return;
            open();
            recogniser->start();
            started = true;
        }

        void stop() {
            lock_guard<mutex> lock(start_stop_mutex);
            if (!started) return;
            recogniser->stop();
            close();
            started = false;
        }

        // void pause() {
        //     paused = true;
        //     recorder->pause();
        //     monitor->pause();
        //     listener->pause();
        //     // transcriber->pause();
        // }

        // void resume() {
        //     paused = false;
        //     recorder->resume();
        //     monitor->resume();
        //     listener->resume();
        //     // transcriber->resume();
        // }

        // Add this method to replace recogniser (for testing only)
        void replace_recogniser(SpeechRecogniser* new_recogniser) {
            if (started) throw ERROR("Cannot replace recogniser while STT is started");
            if (recogniser) delete recogniser; // Delete the old recogniser
            recogniser = new_recogniser; // Assign new recogniser, STT takes ownership
        }

    private:

        void open() {
            if (!recorder) {
                recorder = new VoiceRecorder(
                    stt_voice_recorder_sample_rate,
                    stt_voice_recorder_frames_per_buffer,
                    stt_voice_recorder_buffer_seconds
                );
            }
            
            if (!monitor) {
                monitor = new NoiseMonitor(
                    *recorder,
                    stt_noise_monitor_threshold_pc,
                    stt_noise_monitor_rmax_decay_pc,
                    stt_noise_monitor_window
                );
            }
            
            if (!listener) {
                listener = new SpeechListener(
                    *monitor
                );
            }
            
            if (!recogniser) {
                recogniser = new SpeechRecogniser(
                    *recorder,
                    *monitor,
                    *listener,
                    transcriber,
                    stt_poll_interval_ms
                );
            }
        }

        void close() {
            if (recorder) delete recorder; recorder = nullptr;
            if (monitor) delete monitor; monitor = nullptr;
            if (listener) delete listener; listener = nullptr;
            // if (transcriber) delete transcriber; transcriber = nullptr;
            if (recogniser) delete recogniser; recogniser = nullptr;
        }

        static mutex start_stop_mutex;
        atomic<bool> started = false;

        Transcriber& transcriber;
        const double stt_voice_recorder_sample_rate;
        const unsigned long stt_voice_recorder_frames_per_buffer;
        const size_t stt_voice_recorder_buffer_seconds;
        const float stt_noise_monitor_threshold_pc;
        const float stt_noise_monitor_rmax_decay_pc;
        const size_t stt_noise_monitor_window;
        const long stt_poll_interval_ms;

        VoiceRecorder* recorder = nullptr;
        NoiseMonitor* monitor = nullptr;
        SpeechListener* listener = nullptr; 
        SpeechRecogniser* recogniser = nullptr;
    };

    mutex STT::start_stop_mutex;

}

#ifdef TEST

#include "../utils/Test.hpp"

#include "tests/MockVoiceRecorder.hpp"
#include "tests/MockNoiseMonitor.hpp"
#include "tests/MockSpeechListener.hpp"
#include "tests/MockTranscriber.hpp"
#include "tests/MockSpeechRecogniser.hpp"

using namespace std;
using namespace tools;
using namespace tools::voice;

// STT Tests with MockTranscriber
void test_STT_constructor_valid() {
    Suppressor supressor(stderr);
    MockTranscriber transcriber;
    STT stt(
        transcriber,
        16000.0,  // sample_rate
        512,      // frames_per_buffer
        5,        // buffer_seconds
        0.1f,     // noise_monitor_threshold_pc
        0.01f,    // noise_monitor_rmax_decay_pc
        1024,     // noise_monitor_window
        // "mock_model", // transcriber_model
        // "en",     // transcriber_lang
        10        // poll_interval_ms
    );
    assert(true && "STT constructor should initialize without crashing");
    assert(&stt.getTranscriberCRef() == &transcriber && "STT uses transcriber");
}

void test_STT_start_basic() {
    bool rms_called = false;
    bool transcribe_called = false;
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        MockNoiseMonitor monitor(recorder);
        MockSpeechListener listener(monitor);
        MockTranscriber transcriber;
        MockSpeechRecogniser* recogniser = new MockSpeechRecogniser(recorder, monitor, listener, transcriber);

        STT stt(
            transcriber, 16000.0, 512, 5, 0.1f, 0.01f, 1024, /*"mock_model", "en",*/ 10
        );
        stt.replace_recogniser(recogniser);
        
        stt.setRMSHandler([&](float /*vol_pc*/, float, float, float, bool, bool) { rms_called = true; });
        stt.setTranscribeHandler([&](const vector<float>&, const string&) { transcribe_called = true; });
        
        stt.start();
        vector<float> buffer = vector<float>(1024, 0.5f);
        listener.simulate_noise(0.2f, true, buffer, false);  // Noisy event
        listener.simulate_noise(0.05f, false, buffer, false); // Silence, triggers speech

        // Brief delay to allow processing
        Pa_Sleep(50);
        stt.stop();
    }
    assert(rms_called && "RMS handler should be called");
    assert(transcribe_called && "Transcribe handler should be called");
}

void test_STT_empty_record() {
    bool transcribe_called = false;
    string last_transcript = "not empty";
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        MockNoiseMonitor monitor(recorder);
        MockSpeechListener listener(monitor);
        MockTranscriber transcriber;
        MockSpeechRecogniser* recogniser = new MockSpeechRecogniser(recorder, monitor, listener, transcriber);

        STT stt(
            transcriber, 16000.0, 512, 5, 0.1f, 0.01f, 1024, /*"mock_model", "en",*/ 10
        );
        stt.replace_recogniser(recogniser);

        stt.setTranscribeHandler([&](const vector<float>& record, const string& text) {
            transcribe_called = true;
            last_transcript = text;
            assert(record.empty() && "Record should be empty");
        });

        stt.start();

        // Simulate speech with an empty record
        vector<float> empty_record; // Empty vector
        recogniser->simulate_speech(empty_record);

        // Brief delay to allow processing (optional with mock, but kept for robustness)
        Pa_Sleep(50);

        stt.stop();
    } // Suppressor goes out of scope here, restoring stderr for assertions
    assert(transcribe_called && "Transcribe handler should handle empty record");
    assert(last_transcript == "mock_transcription" && "Empty record should yield empty transcription");
}

void test_STT_callback_comprehensive() {
    struct CallbackData {
        int rms_count = 0;
        int speech_count = 0;
        int transcribe_count = 0;
        float last_vol_pc = -1.0f;
        vector<float> last_record;
        string last_transcript;
    } data;

    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        MockNoiseMonitor monitor(recorder);
        MockSpeechListener listener(monitor);
        MockTranscriber transcriber;
        MockSpeechRecogniser* recogniser = new MockSpeechRecogniser(recorder, monitor, listener, transcriber);

        STT stt(
            transcriber, 16000.0, 512, 5, 0.1f, 0.01f, 1024, /*"mock_model", "en",*/ 10
        );
        stt.replace_recogniser(recogniser);

        stt.setRMSHandler([&](float vol_pc, float, float, float, bool, bool) {
            data.rms_count++;
            data.last_vol_pc = vol_pc;
        });
        stt.setSpeechHandler([&](vector<float>& record) {
            data.speech_count++;
            data.last_record = record;
        });
        stt.setTranscribeHandler([&](const vector<float>&, const string& text) {
            data.transcribe_count++;
            data.last_transcript = text;
        });

        stt.start();

        // Simulate noise events to trigger full pipeline
        vector<float> record1(512, 1.0f);
        vector<float> record2(512, 0.5f);
        listener.simulate_noise(0.2f, true, record1, false);  // First noisy event
        listener.simulate_noise(0.15f, true, record2, false); // Second noisy event
        listener.simulate_noise(0.05f, false, record1, false); // Silence, triggers speech

        // Brief delay to allow processing
        Pa_Sleep(50);

        stt.stop();
    }

    assert(data.rms_count >= 1 && "RMS handler should fire at least once");
    assert(data.speech_count == 1 && "Speech handler should fire once for combined record");
    assert(data.transcribe_count == 1 && "Transcribe handler should fire once for combined record");
    assert(data.last_record.size() == 1024 && "Last record should combine both simulated inputs");
    assert(data.last_transcript == "mock_transcription" && "Transcription should match mock output");
}

void test_STT_muted_state() {
    bool rms_called = false;
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        MockNoiseMonitor monitor(recorder);
        MockSpeechListener listener(monitor);
        MockTranscriber transcriber;
        MockSpeechRecogniser* recogniser = new MockSpeechRecogniser(recorder, monitor, listener, transcriber);
        
        STT stt(
            transcriber, 16000.0, 512, 5, 0.1f, 0.01f, 1024, /*"mock_model", "en",*/ 10
        );
        stt.replace_recogniser(recogniser);
        
        stt.setRMSHandler([&](float, float, float, float, bool, bool muted) {
            rms_called = true;
            assert(muted && "Muted state should be reported");
        });
        
        stt.start();
        listener.simulate_noise(0.2f, true, vector<float>(1024, 0.5f), true); // Muted
        stt.stop();
    }
    assert(rms_called && "RMS handler should fire in muted state");
}

void test_STT_stop_without_speech() {
    bool transcribe_called = false;
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        MockNoiseMonitor monitor(recorder);
        MockSpeechListener listener(monitor);
        MockTranscriber transcriber;
        MockSpeechRecogniser* recogniser = new MockSpeechRecogniser(recorder, monitor, listener, transcriber);
        
        STT stt(
            transcriber, 16000.0, 512, 5, 0.1f, 0.01f, 1024, /*"mock_model", "en",*/ 10
        );
        stt.replace_recogniser(recogniser);
        
        stt.setTranscribeHandler([&](const vector<float>&, const string&) { transcribe_called = true; });
        
        stt.start();
        // No speech simulated
        stt.stop();
    }
    assert(!transcribe_called && "Transcribe handler should not fire without speech");
}

void test_STT_recogniser_initialization() {
    Suppressor supressor(stderr);
    MockTranscriber transcriber;
    STT stt(
        transcriber,
        16000.0,  // sample_rate
        512,      // frames_per_buffer
        5,        // buffer_seconds
        0.1f,     // noise_monitor_threshold_pc
        0.01f,    // noise_monitor_rmax_decay_pc
        1024,     // noise_monitor_window
        10        // poll_interval_ms
    );
    stt.start();
    assert(stt.getMonitorPtr() != nullptr && "Noise monitor should be initialized");
    assert(stt.getRecogniserPtr() != nullptr && "SpeechRecogniser should be initialized");
}

// Register tests
TEST(test_STT_constructor_valid);
TEST(test_STT_start_basic);
TEST(test_STT_empty_record);
TEST(test_STT_callback_comprehensive);
TEST(test_STT_muted_state);
TEST(test_STT_stop_without_speech);
TEST(test_STT_recogniser_initialization);

#endif