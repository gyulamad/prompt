#pragma once

#include "NoiseMonitor.hpp"

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

}