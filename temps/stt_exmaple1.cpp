#include <iostream>
#include <unistd.h>


// g++ -std=c++23 temps/stt_exmaple1.cpp -o builds/stt_exmaple1 -Ofast -Ilibs/ggerganov/whisper.cpp/include -Ilibs/ggerganov/whisper.cpp/ggml/include libs/ggerganov/whisper.cpp/build/src/libwhisper.so.1.7.4 -lrt -lm -lasound -ljack -lportaudio -pthread


#include "../src/tools/stt/STT.hpp"

using namespace std;
using namespace tools::stt;

int stt_exmaple1() {
    try {
        const double stt_voice_recorder_sample_rate = 16000; //16000;
        const unsigned long stt_voice_recorder_frames_per_buffer = 512;
        const size_t stt_voice_recorder_buffer_seconds = 5;
        const float stt_noise_monitor_threshold = 0.03f;
        const size_t stt_noise_monitor_window = 16384; // 16384;
        const string stt_transcriber_model = "libs/ggerganov/whisper.cpp/models/ggml-small-q8_0.bin";
        const string stt_transcriber_lang = "hu";
        const long stt_poll_interval_ms = 100;

        WhisperSTT stt(
            stt_voice_recorder_sample_rate,
            stt_voice_recorder_frames_per_buffer,
            stt_voice_recorder_buffer_seconds,
            stt_noise_monitor_threshold,
            stt_noise_monitor_window,
            stt_transcriber_model,
            stt_transcriber_lang,
            stt_poll_interval_ms
        );

        int i = 6000;
        const char* roller = "|/-\\";
        int recs = 0;

        stt.setRMSHandler([&](float rms, float threshold, bool loud) {
            string out; 
            int at = i%4;
            char c = roller[at];
            string roll;
            roll += c;
            out += "\r";
            out += (stt.getTranscriberCRef().isInProgress() ? roll + " " + to_string(recs) : "   ");
            // out += "\t";
            out += (loud ? " *REC" : "     ");
            out += " ";
            out += to_string(threshold); 
            out += "/";
            out += to_string(rms);
            out += "       \t t-";
            out += to_string(i/10);
            out += "       \r";
            cout << out << flush;
            // cout << "\b" << (loud ? "*REC" : "    ") << " " << threshold << "/" << rms 
            //     << "       \t t-" << i << "\b";
        });

        stt.setSpeechHandler([&](vector<float>& record) {
            recs++;
            cout << "\rrecorded samples: " << record.size() << "          \t" << endl;
        });

        stt.setTranscribeHandler([&](const vector<float>& record, const string& text) {
            recs--;
            cout << "\r<" << text << "                   \t" <<  endl; 
        });

        stt.start();

        // Keep main thread alive
        while (i--) {
            //cout << i << endl;
            Pa_Sleep(100);
        }

        stt.stop();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

int main() {
    return stt_exmaple1();
}