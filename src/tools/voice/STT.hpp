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
            const float stt_noise_monitor_threshold_pc,
            const float stt_noise_monitor_rmax_decay_pc,
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
                stt_noise_monitor_threshold_pc,
                stt_noise_monitor_rmax_decay_pc,
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

        NoiseMonitor* getMonitorPtr() {
            return monitor;
        }

        const Transcriber& getTranscriberCRef() {
            return *transcriber;
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

}