#pragma once

#include "../containers/vector_save.hpp"
#include "../containers/vector_load.hpp"
#include "../files/file_exists.hpp"
#include "STT.hpp"

using namespace tools::containers;
using namespace tools::files;

namespace tools::voice {

    class WhisperTranscriberAdapter: public Transcriber {
    private:
        whisper_full_params params;
        whisper_context* ctx = nullptr;
        string warmup_audio_path;
        vector<float> warmup_audio;
        size_t warmup_audio_max_length;
    public:
        WhisperTranscriberAdapter(
            const string& model_path,
            const string& warmup_audio_path,
            size_t warmup_audio_max_length,
            const char* lang = nullptr
        ): 
            Transcriber(),
            warmup_audio_path(warmup_audio_path),
            warmup_audio_max_length(warmup_audio_max_length)
        {
            // whisper_sampling_strategy strategy(WHISPER_SAMPLING_BEAM_SEARCH);
            whisper_sampling_strategy strategy(WHISPER_SAMPLING_BEAM_SEARCH);
            params = whisper_full_default_params(strategy);
            if (lang) params.language = lang;
            ctx = whisper_init_from_file_with_params(model_path.c_str(), whisper_context_default_params());
            if (!ctx) {
                throw ERROR("Failed to load Whisper model");
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

            if (file_exists(warmup_audio_path)) {
                vector_load<float>(warmup_audio, warmup_audio_path);
                _transcribe(warmup_audio);
            }
        }

        virtual ~WhisperTranscriberAdapter() {
            if (ctx) whisper_free(ctx);
            ctx = nullptr;
            vector_save<float>(warmup_audio, warmup_audio_path);
        }

        string transcribe(const vector<float>& audio_data) override {
            // whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
            // params.print_realtime = false;
            // params.print_progress = false;
            // params.print_timestamps = false;

            preTranscribe();

            string transcription = _transcribe(audio_data);

            for (float data: audio_data) 
                warmup_audio.push_back(data);
            if (warmup_audio.size() > warmup_audio_max_length)
                warmup_audio.resize(warmup_audio.size() - warmup_audio_max_length);

            postTranscribe();

            return transcription;
        }
    protected:
        string _transcribe(vector<float> audio_data) {
            if (whisper_full(ctx, params, audio_data.data(), audio_data.size()) != 0) {
                throw ERROR("Whisper transcription failed");
            }

            string transcription;
            int segments = whisper_full_n_segments(ctx);
            for (int i = 0; i < segments; ++i) {
                string segment = whisper_full_get_segment_text(ctx, i);
                transcription += segment;
            }

            return transcription;
        }
    };

}