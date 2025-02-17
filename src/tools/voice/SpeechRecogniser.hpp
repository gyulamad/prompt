#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <unistd.h> // For usleep()

#include "Transcriber.hpp"
#include "SpeechListener.hpp"

using namespace std;
using namespace tools;

namespace tools::voice {

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


        SpeechListener::RMSCallback rms_cb = [](float vol_pc, float threshold_pc, float rmax, float rms, bool loud, bool muted) {};
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
            //while (!transcriberThread.joinable());
            transcriberThread.join();
            listener.stop();
        }
    };

}