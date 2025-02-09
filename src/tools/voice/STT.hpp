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

using namespace std;
using namespace tools;

namespace tools::voice {

    class SpeechListener {
    public:
        using RMSCallback = function<void(float vol_pc, float threshold_pc, float rmax, float rms, bool loud)>;
        using SpeechCallback = function<void(vector<float>& record)>;

        SpeechListener(NoiseMonitor& monitor): monitor(monitor) {}
        
        virtual ~SpeechListener() {}

        void start(RMSCallback rms_cb, SpeechCallback speech_cb, long pollIntervalMs) {
            this->rms_cb = rms_cb;
            this->speech_cb = speech_cb;
            monitor.start(this, noise_cb, pollIntervalMs); // Check every 100ms
        }

        void stop() {
            monitor.stop();
        }

        // void pause() {
        //     monitor.pause();
        // }

        // void resume() {
        //     monitor.resume();
        // }   

    private:
        NoiseMonitor& monitor;

        bool is_noisy_prev = false;
        int n = 0;
        vector<float> record;
        RMSCallback rms_cb;
        SpeechCallback speech_cb;

        static void noise_cb(
            void* listener, 
            float vol_pc, 
            float threshold_pc, 
            float rmax, 
            float rms, 
            bool is_noisy, 
            vector<float>& buffer
        ) {
            SpeechListener* that = (SpeechListener*)listener;
            if (is_noisy) for (float sample: buffer) that->record.push_back(sample);
            else if (!is_noisy && that->is_noisy_prev) that->speech_cb(that->record);
            that->is_noisy_prev = is_noisy;
            that->rms_cb(vol_pc, threshold_pc, rmax, rms, is_noisy);
        };

    };


    class Transcriber {
    private:
        bool inProgress = false;
        
    protected:

        void preTranscribe() {
            inProgress = true;
        }

        void postTranscribe() {
            inProgress = false;
        }

    public:
        Transcriber(const string& conf, const char* lang = nullptr) {}

        virtual ~Transcriber() {}

        bool isInProgress() const {
            return inProgress;
        }    

        virtual string transcribe(const vector<float>& audio_data) { throw runtime_error("Unimplemented"); }
    };


    class WhisperTranscriber: public Transcriber {
    private:
        whisper_full_params params;
        whisper_context* ctx;

    public:
        WhisperTranscriber(const string& model_path, const char* lang = nullptr): 
            Transcriber(model_path, lang) 
        {
            // whisper_sampling_strategy strategy(WHISPER_SAMPLING_BEAM_SEARCH);
            whisper_sampling_strategy strategy(WHISPER_SAMPLING_BEAM_SEARCH);
            params = whisper_full_default_params(strategy);
            if (lang) params.language = lang;
            ctx = whisper_init_from_file_with_params(model_path.c_str(), whisper_context_default_params());
            if (!ctx) {
                throw runtime_error("Failed to load Whisper model");
            }

            // if (!whisper_is_multilingual(ctx)) {
            //     if (params.language != "en" || params.translate) {
            //         params.language = "en";
            //         params.translate = false;
            //         fprintf(stderr, "%s: WARNING: model is not multilingual, ignoring language and translation options\n", __func__);
            //     }
            // } else {
            //     params.detect_language = false;
            //     params.language = lang.c_str();
            // }
        }

        string transcribe(const vector<float>& audio_data) override {
            // whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
            // params.print_realtime = false;
            // params.print_progress = false;
            // params.print_timestamps = false;

            preTranscribe();

            if (whisper_full(ctx, params, audio_data.data(), audio_data.size()) != 0) {
                throw runtime_error("Whisper transcription failed");
            }

            string transcription;
            int segments = whisper_full_n_segments(ctx);
            for (int i = 0; i < segments; ++i) {
                string segment = whisper_full_get_segment_text(ctx, i);
                transcription += segment;
            }

            postTranscribe();

            return transcription;
        }

        ~WhisperTranscriber() {
            if (ctx) whisper_free(ctx);
        }
    };



    class SpeechRecogniser {
    private:
        // atomic<bool> paused{true};

        atomic<bool> running{true};
        atomic<bool> records_lock = false;
        vector<vector<float>> records;
        int record_counter = 0;
        thread transcriberThread;

        VoiceRecorder& recorder;
        NoiseMonitor& monitor;
        SpeechListener& listener;
        Transcriber& transcriber;
        long pollIntervalMs;
    public:

        using TranscribeCallback = function<void(const vector<float>& record, const string& text)>;


        SpeechListener::RMSCallback rms_cb = [](float vol_pc, float threshold_pc, float rmax, float rms, bool loud) {};
        SpeechListener::SpeechCallback speech_cb = [](vector<float>& record) {};
        SpeechRecogniser::TranscribeCallback transcribe_cb = [](const vector<float>& record, const string& text) {};


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

        void start() {
            listener.start(
                [&](float vol_pc, float threshold_pc, float rmax, float rms, bool loud) {
                    // cout << "RMS: " << threshold_pc << "/" << rms << " - " << (loud ? "Loud" : "Quiet") << endl;
                    rms_cb(vol_pc, threshold_pc, rmax, rms, loud);
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

            cout << "DEBUG: SpeachRecogniser transcriber thread start..." << endl;
            transcriberThread = thread([&]{
                while(running) {
                    usleep(30000);
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

        void stop() {
            running = false;
            while (!transcriberThread.joinable());
            transcriberThread.join();
            listener.stop();
        }
    };

    template<typename TranscriberT>
    class STT {
    public:
        // SpeechListener::RMSCallback rms_cb = [](float rms, float threshold_pc, bool loud) {};
        // SpeechListener::SpeechCallback speech_cb = [](vector<float>& record) {};
        // SpeechRecogniser::TranscribeCallback transcribe_cb = [](const vector<float>& record, const string& text) {};

    private:
        // atomic<bool> paused{true};
        bool started = false;
        VoiceRecorder* recorder;
        NoiseMonitor* monitor;
        SpeechListener* listener; 
        Transcriber* transcriber;
        SpeechRecogniser* recogniser;
    public:
        STT(
            const double stt_voice_recorder_sample_rate,
            const unsigned long stt_voice_recorder_frames_per_buffer,
            const size_t stt_voice_recorder_buffer_seconds,
            const float stt_noise_monitor_threshold,
            const size_t stt_noise_monitor_window,
            const string& stt_transcriber_model,
            const string& stt_transcriber_lang,
            const long stt_poll_interval_ms
        ) {
            recorder = new VoiceRecorder(
                stt_voice_recorder_sample_rate,
                stt_voice_recorder_frames_per_buffer,
                stt_voice_recorder_buffer_seconds
            );
            
            monitor = new NoiseMonitor(
                *recorder,
                stt_noise_monitor_threshold,
                stt_noise_monitor_window
            );
            
            listener = new SpeechListener(
                *monitor
            );
            
            transcriber = new TranscriberT(
                stt_transcriber_model,
                stt_transcriber_lang.c_str()
            );
            
            recogniser = new SpeechRecogniser(
                *recorder,
                *monitor,
                *listener,
                *transcriber,
                stt_poll_interval_ms
            );
            
        }

        const Transcriber& getTranscriberCRef() {
            return *transcriber;
        }

        void setRMSHandler(SpeechListener::RMSCallback rms_cb) {
            recogniser->rms_cb = rms_cb;
        }

        void setSpeechHandler(SpeechListener::SpeechCallback speech_cb) {
            recogniser->speech_cb = speech_cb;
        }

        void setTranscribeHandler(SpeechRecogniser::TranscribeCallback transcribe_cb) {
            recogniser->transcribe_cb = transcribe_cb;
        }

        void start() {
            recogniser->start();
            started = true;
        }

        void stop() {
            if (started) recogniser->stop();
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

        virtual ~STT() {
            if (recorder) delete recorder;
            if (monitor) delete monitor;
            if (listener) delete listener;
            if (transcriber) delete transcriber;
            if (recogniser) delete recogniser;
        }
    };

    class WhisperSTT: public STT<WhisperTranscriber> {
    public:
        using STT::STT;
    };

}