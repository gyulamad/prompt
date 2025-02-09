#pragma once

#include <cstdio>
#include <iostream>
#include <string>
#include <sys/ioctl.h>

#include "tools/voice/STT.hpp"
#include "tools/voice/TTS.hpp"
#include "tools/strings.hpp"
#include "tools/Process.hpp"
#include "tools/Commander.hpp"

using namespace std;
using namespace tools::voice;

namespace prompt {

    class Speech {
    private:
        // bool paused = false;

        // ---- roller ----

        int rollnxt = 6000;
        const char* roller = "|/-\\";
        int recs = 0;

        // -----------------

        vector<string> text_puffer = {};
        string text_input = "";

        vector<string> hesitors = {};
        vector<string> repeaters = {};        
        WhisperSTT* stt = nullptr;
        TTS* tts = nullptr;

        long long speak_wait_ms = 3000; // TODO: to config
        long long speak_paused_at_ms;


        // ----- random speeches -----

        int rand_seed = 0;

        string rand_speak_select(const vector<string>& strings) {
            if (!rand_seed) srand(rand_seed = time(0));
            int index = rand() % repeaters.size();
            return strings[index];
        }
        
        void rand_speak_beep(const vector<string>& strings, bool think = false) {
            if (!tts->is_speaking()) speak_beep(rand_speak_select(strings), think);
        }

        // ---------------------------


    public:
        Speech(
            Commander& commander,
            const string& lang,
            int speech_tts_speed,
            int speech_tts_gap,
            const string& speech_tts_beep_cmd,
            const string& speech_tts_think_cmd,
            const double speech_stt_voice_recorder_sample_rate,
            const unsigned long speech_stt_voice_recorder_frames_per_buffer,
            const size_t speech_stt_voice_recorder_buffer_seconds,
            const float speech_stt_noise_monitor_threshold,
            const size_t speech_stt_noise_monitor_window,
            const string& speech_stt_transcriber_model,
            const long speech_stt_poll_interval_ms
        )//:
            // stt(
            //     speech_stt_voice_recorder_sample_rate,
            //     speech_stt_voice_recorder_frames_per_buffer,
            //     speech_stt_voice_recorder_buffer_seconds,
            //     speech_stt_noise_monitor_threshold,
            //     speech_stt_noise_monitor_window,
            //     speech_stt_transcriber_model,
            //     speech_stt_transcriber_lang,
            //     speech_stt_poll_interval_ms
            // ) 
        {

            tts = new TTS(
                lang,
                speech_tts_speed,
                speech_tts_gap,
                speech_tts_beep_cmd,
                speech_tts_think_cmd
            );

            stt = new WhisperSTT(
                speech_stt_voice_recorder_sample_rate,
                speech_stt_voice_recorder_frames_per_buffer,
                speech_stt_voice_recorder_buffer_seconds,
                speech_stt_noise_monitor_threshold,
                speech_stt_noise_monitor_window,
                speech_stt_transcriber_model,
                lang,
                speech_stt_poll_interval_ms
            );

            stt->setRMSHandler([&](float vol_pc, float threshold_pc, float rmax, float rms, bool loud) {
                string out = ""; 

                // REC status
                out += string(loud ? "*REC" : "?MIC") + " ";

                // RMS volume
                //out += "V:" + set_precision(threshold_pc * 100, 2) + "/" + set_precision(rms * 100, 2) + " \t[";
                out += "[";
                double step = 0.1;
                double i = threshold_pc;
                for (; i < 1; i += step) {
                    if (vol_pc > i) out += "=";
                    else out += " ";
                }
                out += "] " + set_precision(threshold_pc * 100, 2) + "/" + set_precision(vol_pc * 100, 2) + "% ";
                
                // progress roller
                rollnxt++;
                int at = rollnxt%4;
                char c = roller[at];
                string roll;
                roll += c;
                out += (stt->getTranscriberCRef().isInProgress() ? roll + " " + to_string(recs) : "   ") + " ";

                // show
                // int y, x;
                // get_cursor_position(y, x);
                // struct winsize w;
                // ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
                // int outpos = w.ws_col - out.length() - 1;
                
                
                int s = out.size();
                for (int i = 0; i < s; i++) out += "\b";
                cout << out << flush;
                // struct winsize w;
                // ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
                // int outpos = w.ws_col - out.length() - 1;
                // string final_out = "";
                // for (int i=w.ws_col; i < outpos; i++) final_out += "\033[C";
                // final_out += out;
                // cout << final_out << flush;


                // ----- handle speech interruptions -----

                if (loud && !speak_paused_at_ms) {
                    // cout << "[[PAUSE]]" << endl;
                    tts->speak_pause();
                    speak_paused_at_ms = get_time_ms();
                }

                if (!loud) {
                    if (speak_paused_at_ms) {
                        if (speak_paused_at_ms + speak_wait_ms > get_time_ms()) {
                            // cout << "[[CONTINUE]]" << endl;
                            tts->speak_resume();
                            speak_paused_at_ms = 0;
                        }
                    }
                } else {
                    if (speak_paused_at_ms) {
                        if (speak_paused_at_ms + speak_wait_ms < get_time_ms()) {
                            // cout << "[[STOP]]" << endl;
                            tts->speak_stop();
                            speak_paused_at_ms = 0;
                        }
                    }
                }
            });

            stt->setSpeechHandler([&](vector<float>& record) {
                recs++;
                // cout << "\rrecorded samples: " << record.size() << "          \t" << endl;
                rand_speak_hesitate();
            });

            stt->setTranscribeHandler([&](const vector<float>& record, const string& text) {
                recs--;
                string trim_text = trim(text);
                // cout << "txt:[" << trim_text << "]" << endl;
                 if (   !text.empty() &&
                        !regx_match("^\\[.*\\]$", trim_text) && // TODO: to config
                        !regx_match("^\\*.*\\*$", trim_text) &&
                        !regx_match("^\\(.*\\)$", trim_text) &&
                        !regx_match("^\\-.*[\\.\\!]$", trim_text) // (for hungarian?)
                    ) 
                {
                    struct winsize w;
                    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
                    string outp = "\r" + commander.get_command_line_ref().get_prompt() + trim_text; //commander.get_command_line_ref().get_prompt() + text;
                    for (int i = outp.size(); i < w.ws_col - 10; i++) outp += " ";
                    cout << outp << endl;
                    text_puffer.push_back(trim_text);
                    if (!recs) {
                        string inp = (text_input.empty() ? "" : "\n") + implode("\n", text_puffer);
                        text_input += inp;
                        text_puffer.clear();
                    }
                    return;
                }

                if (recs == 0 && !tts->is_speaking()) 
                    speak_beep(" ");
            });

            stt->start();
        }

        virtual ~Speech() {
            stt->stop();
            delete stt;
            delete tts;
        }

        // void pause() {
        //     paused = true;
        //     stt->pause();
        //     tts->pause();
        // }

        // void resume() {
        //     stt->resume();
        //     tts->resume();
        //     paused = false;
        // }

        // bool is_paused() {
        //     return paused;
        // }

        string get_text_input() {
            string inp = text_input;
            text_input = "";
            return inp;
        }

        void speak_beep(const string& text, bool think = false) {
            while (!speak_paused_at_ms && tts->is_speaking()) sleep(1);
            // cout << "[[START]]" << endl;
            tts->speak_stop();
            speak_paused_at_ms = 0;
            tts->speak_beep(text, think);
        }


        // ----- random speeches -----

        void rand_speak_hesitate() {
            rand_speak_beep(hesitors, true);
        }

        void rand_speak_repeates() {
            rand_speak_beep(repeaters);
        }


        void set_hesitors(const vector<string>& hesitors) {
            this->hesitors = hesitors;
        }

        vector<string>& get_hesitors_ref() {
            return hesitors;
        }

        void set_repeaters(const vector<string>& repeaters) {
            this->repeaters = repeaters;
        }

        vector<string>& get_repeaters_ref() {
            return repeaters;
        }

        // ---------------------------


    };

}