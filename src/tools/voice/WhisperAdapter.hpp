#pragma once

#include "STT.hpp"

namespace tools::voice {

    class WhisperAdapter: public Transcriber {
    private:
        whisper_full_params params;
        whisper_context* ctx;

    public:
        WhisperAdapter(const string& model_path, const char* lang = nullptr): Transcriber() 
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

        ~WhisperAdapter() {
            if (ctx) whisper_free(ctx);
        }
    };

    class WhisperSTT: public STT<WhisperAdapter> {
    public:
        using STT::STT;
    };

}