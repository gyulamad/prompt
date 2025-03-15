#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <unistd.h> // For usleep()

#include "../utils/system.hpp"

#include "Transcriber.hpp"
#include "SpeechListener.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::voice {

    class SpeechRecogniser {
    protected:
        atomic<bool> running{true};

    private:
        // atomic<bool> paused{true};

        atomic<bool> records_lock = false;
        vector<vector<float>> records;
        int record_counter = 0;
        thread transcriberThread;

        VoiceRecorder& recorder;
        NoiseMonitor& monitor;
    protected:
        SpeechListener& listener;
    private:
        Transcriber& transcriber;
        long pollIntervalMs;
    public:

        using TranscribeCallback = function<void(const vector<float>& record, const string& text)>;

        SpeechRecogniser(
            VoiceRecorder& recorder, 
            NoiseMonitor& monitor, 
            SpeechListener& listener, 
            Transcriber& transcriber,
            long pollIntervalMs
        ):
            recorder(recorder), 
            monitor(monitor), 
            listener(listener), 
            transcriber(transcriber),
            pollIntervalMs(pollIntervalMs)
        {}

        virtual ~SpeechRecogniser() {}

        virtual void start() {
            listener.start(
                [&](float vol_pc, float threshold_pc, float rmax, float rms, bool loud, bool muted) {
                    rms_cb(vol_pc, threshold_pc, rmax, rms, loud, muted);
                },

                [&](vector<float>& record) {
                    // string fname = "output-" + to_string(record_counter++) + ".pcm";
                    // cout << "save: " << fname << endl;
                    // VoiceRecorder::save_as_pcm(fname, record);
                    speech_cb(record);                
                    records_lock = true;
                    records.push_back(record);
                    records_lock = false;
                    record.clear();
                },
                pollIntervalMs
            );

            // cout << "[DEBUG] SpeachRecogniser transcriber thread start..." << endl;
            transcriberThread = thread([&]{
                while(running) {
                    sleep_ms(300);
                    // if (paused) continue;
                    if (!records.empty()) {
                        // shift out the first record:
                        if (records_lock) continue;
                        vector<float> record = move(records.front());
                        records.erase(records.begin());
                        string transcript = transcriber.transcribe(record);

                        // cout << ">>>>>>>>>>>>" << transcript << endl;
                        transcribe_cb(record, transcript);
                    }
                }
                //cout << "SpeechRecognition stopped" << endl;
            });
        }

        virtual void stop() {
            running = false;
            //while (!transcriberThread.joinable());
            transcriberThread.join();
            listener.stop();
        }

        void set_rms_cb(SpeechListener::RMSCallback rms_cb) {
            this->rms_cb = rms_cb;
        }

        void set_speech_cb(SpeechListener::SpeechCallback speech_cb) {
            this->speech_cb = speech_cb;
        }

        void set_transcribe_cb(SpeechRecogniser::TranscribeCallback transcribe_cb) {
            this->transcribe_cb = transcribe_cb;
        }

    protected:
        SpeechListener::RMSCallback rms_cb = [](float /*vol_pc*/, float /*threshold_pc*/, float /*rmax*/, float /*rms*/, bool /*loud*/, bool /*muted*/) {};
        SpeechListener::SpeechCallback speech_cb = [](vector<float>& /*record*/) {};
        SpeechRecogniser::TranscribeCallback transcribe_cb = [](const vector<float>& /*record*/, const string& /*text*/) {};
        
    };

}

#ifdef TEST

#include "../utils/Test.hpp"

#include "tests/MockVoiceRecorder.hpp"
#include "tests/MockNoiseMonitor.hpp"
#include "tests/MockSpeechListener.hpp"
#include "tests/MockTranscriber.hpp"

using namespace tools::voice;

// SpeechRecogniser Tests
void test_SpeechRecogniser_constructor_valid() {
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        MockNoiseMonitor monitor(recorder);
        MockSpeechListener listener(monitor);
        MockTranscriber transcriber;
        SpeechRecogniser recogniser(recorder, monitor, listener, transcriber, 10);
    }
    assert(true && "SpeechRecogniser constructor should initialize without crashing");
}

void test_SpeechRecogniser_start_basic() {
    bool rms_called = false;
    bool speech_called = false;
    bool transcribe_called = false;
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        MockNoiseMonitor monitor(recorder);
        MockSpeechListener listener(monitor);
        MockTranscriber transcriber;
        SpeechRecogniser recogniser(recorder, monitor, listener, transcriber, 10);
        
        recogniser.set_rms_cb([&](float /*vol_pc*/, float, float, float, bool, bool) { rms_called = true; });
        recogniser.set_speech_cb([&](vector<float>&) { speech_called = true; });
        recogniser.set_transcribe_cb([&](const vector<float>&, const string&) { transcribe_called = true; });
        
        recogniser.start();
        listener.simulate_noise(0.2f, true, vector<float>(1024, 0.5f), false);
        listener.simulate_noise(0.05f, false, vector<float>(1024, 0.0f), false);
        Pa_Sleep(50); // Brief wait for transcriber thread
        recogniser.stop();
    }
    assert(rms_called && "RMS callback should be called");
    assert(speech_called && "Speech callback should be called");
    assert(transcribe_called && "Transcribe callback should be called");
}

void test_SpeechRecogniser_empty_records() {
    bool transcribe_called = false;
    string last_transcript = "not empty";
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        MockNoiseMonitor monitor(recorder);
        MockSpeechListener listener(monitor);
        MockTranscriber transcriber;
        SpeechRecogniser recogniser(recorder, monitor, listener, transcriber, 10);
        
        recogniser.set_transcribe_cb([&](const vector<float>& record, const string& text) {
            transcribe_called = true;
            last_transcript = text;
            assert(record.empty() && "Record should be empty");
        });
        
        recogniser.start();
        listener.simulate_noise(0.2f, true, vector<float>(), false); // Empty buffer
        listener.simulate_noise(0.05f, false, vector<float>(), false);
        Pa_Sleep(50);
        recogniser.stop();
    }
    assert(transcribe_called && "Transcribe callback should handle empty records");
    assert(last_transcript == "" && "Empty record should yield empty transcription");
}

void test_SpeechRecogniser_callback_comprehensive() {
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
        SpeechRecogniser recogniser(recorder, monitor, listener, transcriber, 10);
        
        recogniser.set_rms_cb([&](float vol_pc, float, float, float, bool, bool) {
            data.rms_count++;
            data.last_vol_pc = vol_pc;
        });
        recogniser.set_speech_cb([&](vector<float>& record) {
            data.speech_count++;
            data.last_record = record;
        });
        recogniser.set_transcribe_cb([&](const vector<float>&, const string& text) {
            data.transcribe_count++;
            data.last_transcript = text;
        });
        
        recogniser.start();
        vector<float> buffer1(512, 1.0f);
        vector<float> buffer2(512, 0.5f);
        listener.simulate_noise(0.2f, true, buffer1, false);
        listener.simulate_noise(0.15f, true, buffer2, false);
        listener.simulate_noise(0.05f, false, buffer1, false);
        Pa_Sleep(50);
        recogniser.stop();
    }
    assert(data.rms_count == 3 && "RMS callback should fire for each noise event");
    assert(data.speech_count == 1 && "Speech callback should fire once when noise ends");
    assert(data.transcribe_count == 1 && "Transcribe callback should fire once");
    assert(data.last_vol_pc == 0.05f && "Last volume should match final event");
    assert(data.last_record.size() == 1024 && "Record should combine buffers");
    assert(data.last_transcript == "transcribed_text" && "Transcription should match mock output");
}

void test_SpeechRecogniser_stop_without_transcription() {
    bool transcribe_called = false;
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        MockNoiseMonitor monitor(recorder);
        MockSpeechListener listener(monitor);
        MockTranscriber transcriber;
        SpeechRecogniser recogniser(recorder, monitor, listener, transcriber, 10);
        
        recogniser.set_transcribe_cb([&](const vector<float>&, const string&) { transcribe_called = true; });
        
        recogniser.start();
        listener.simulate_noise(0.2f, true, vector<float>(1024, 0.5f), false); // Noise, but no silence yet
        recogniser.stop();
    }
    assert(!transcribe_called && "Transcribe callback should not fire without silence");
}

void test_SpeechRecogniser_muted_state() {
    bool rms_called = false;
    {
        Suppressor supressor(stderr);
        MockVoiceRecorder recorder(16000.0, 512, 5);
        MockNoiseMonitor monitor(recorder);
        MockSpeechListener listener(monitor);
        MockTranscriber transcriber;
        SpeechRecogniser recogniser(recorder, monitor, listener, transcriber, 10);
        
        recogniser.set_rms_cb([&](float, float, float, float, bool, bool muted) {
            rms_called = true;
            assert(muted && "Muted state should be reported");
        });
        
        recogniser.start();
        listener.simulate_noise(0.2f, true, vector<float>(1024, 0.5f), true); // Muted
        Pa_Sleep(50);
        recogniser.stop();
    }
    assert(rms_called && "RMS callback should fire in muted state");
}

// Register tests
TEST(test_SpeechRecogniser_constructor_valid);
TEST(test_SpeechRecogniser_start_basic);
TEST(test_SpeechRecogniser_empty_records);
TEST(test_SpeechRecogniser_callback_comprehensive);
TEST(test_SpeechRecogniser_stop_without_transcription);
TEST(test_SpeechRecogniser_muted_state);

#endif