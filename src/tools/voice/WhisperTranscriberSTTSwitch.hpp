#pragma once

#include "STTSwitch.hpp"
#include "WhisperTranscriberAdapter.hpp"

namespace tools::voice {

    class WhisperTranscriberSTTSwitch: public STTSwitch {
    public:

        WhisperTranscriberSTTSwitch(
            // for wishper transcriber
            const string& whisper_model_path,
            const string& whisper_lang,

            // for STT
            const double stt_voice_recorder_sample_rate,
            const unsigned long stt_voice_recorder_frames_per_buffer,
            const size_t stt_voice_recorder_buffer_seconds,
            const float stt_noise_monitor_threshold_pc,
            const float stt_noise_monitor_rmax_decay_pc,
            const size_t stt_noise_monitor_window,
            const long stt_poll_interval_ms,

            // for Switch
            SwitchState state = OFF
        ): 

            // for wishper transcriber
            whisper_model_path(whisper_model_path),
            whisper_lang(whisper_lang),

            // for STT
            stt_voice_recorder_sample_rate(stt_voice_recorder_sample_rate),
            stt_voice_recorder_frames_per_buffer(stt_voice_recorder_frames_per_buffer),
            stt_voice_recorder_buffer_seconds(stt_voice_recorder_buffer_seconds),
            stt_noise_monitor_threshold_pc(stt_noise_monitor_threshold_pc),
            stt_noise_monitor_rmax_decay_pc(stt_noise_monitor_rmax_decay_pc),
            stt_noise_monitor_window(stt_noise_monitor_window),
            stt_poll_interval_ms(stt_poll_interval_ms),

            // for Switch
            STTSwitch(state)
        {}

        void on() override {
            if (STTSwitch::is_on()) return;
            if (!transcriber) transcriber = new WhisperTranscriberAdapter(whisper_model_path, whisper_lang.c_str());
            if (!this->stt) this->stt = new STT(
                *transcriber,
                stt_voice_recorder_sample_rate,
                stt_voice_recorder_frames_per_buffer,
                stt_voice_recorder_buffer_seconds,
                stt_noise_monitor_threshold_pc,
                stt_noise_monitor_rmax_decay_pc,
                stt_noise_monitor_window,
                stt_poll_interval_ms
            );
            STTSwitch::on();
            this->stt->start();
        }
    
        void off() override {
            STTSwitch::off();
            if (this->stt) {
                this->stt->stop();
                delete this->stt; this->stt = nullptr;
            }
            if (transcriber) delete transcriber; transcriber = nullptr;
        }
    private:
        WhisperTranscriberAdapter* transcriber = nullptr;

        // for wishper transcriber
        const string& whisper_model_path;
        const string& whisper_lang;

        // for STT
        const double stt_voice_recorder_sample_rate;
        const unsigned long stt_voice_recorder_frames_per_buffer;
        const size_t stt_voice_recorder_buffer_seconds;
        const float stt_noise_monitor_threshold_pc;
        const float stt_noise_monitor_rmax_decay_pc;
        const size_t stt_noise_monitor_window;
        const long stt_poll_interval_ms;
    };

}
