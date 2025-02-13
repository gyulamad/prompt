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

#include "tools/voice/WhisperSTT.hpp"

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

        // long long speak_wait_ms;
        // long long speak_paused_at_ms;


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

        // ------------- interruptions --------------

        bool loud_prev = false;
        bool tts_paused = false;
        long long tts_paused_at = 0;
        // long long speech_impatient_ms; // = 10000; // TODO: config
        //function<void()> = []() {};

        // ------------------------------------------

        Commander& commander;
        vector<string> speech_ignores_rgxs;
        long long speech_impatient_ms;
    public:
        Speech(
            Commander& commander,
            const string& lang,
            const vector<string>& speech_ignores_rgxs,
            const long long speech_impatient_ms,
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
        ):
            commander(commander),
            speech_ignores_rgxs(speech_ignores_rgxs),
            speech_impatient_ms(speech_impatient_ms)

        // :
        //     speak_wait_ms(speak_wait_ms)
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

            tts->speak_stop(); // reset the voice outputs

            Speech& that = *this;

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
                that.rollnxt++;
                int at = that.rollnxt%4;
                char c = that.roller[at];
                string roll;
                roll += c;
                out += (stt->getTranscriberCRef().isInProgress() ? roll + " " + to_string(that.recs) : "   ") + " ";

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

                if (!that.tts_paused && loud && !that.loud_prev && that.tts->is_speaking()) {
                    that.tts->speak_pause();
                    that.tts_paused = true;
                    that.tts_paused_at = get_time_ms();
                }

                // if (!loud && tts_paused) {
                //     if (tts_paused_at + speech_impatient_ms > get_time_ms()) 
                //         tts->speak_resume();
                //     else 
                //         tts->speak_stop();
                //     tts_paused = false;
                // }

                that.loud_prev = loud;

                // cout << speak_paused_at_ms << endl;
                // if (loud && !speak_paused_at_ms) {
                //     cout << "[[PAUSE]]" << endl;
                //     tts->speak_pause();
                //     speak_paused_at_ms = get_time_ms();
                // }

                // if (!loud) {
                //     if (speak_paused_at_ms) {
                //         if (speak_paused_at_ms + speak_wait_ms > get_time_ms()) {
                //             cout << "[[CONTINUE]]" << endl;
                //             tts->speak_resume();
                //             speak_paused_at_ms = 0;
                //         }
                //     }
                // } else {
                //     if (speak_paused_at_ms) {
                //         if (speak_paused_at_ms + speak_wait_ms < get_time_ms()) {
                //             cout << "[[STOP]]" << endl;
                //             tts->speak_stop();
                //             speak_paused_at_ms = 0;
                //         }
                //     }
                // }
            });

            stt->setSpeechHandler([&](vector<float>& record) {
                that.recs++;
                // cout << "\rrecorded samples: " << record.size() << "          \t" << endl;
                rand_speak_hesitate();
            });

            stt->setTranscribeHandler([&](const vector<float>& record, const string& text) {
                that.recs--;
                string trim_text = trim(text);
                for (const string& rgx: that.speech_ignores_rgxs)
                    if (regx_match(rgx, trim_text)) {
                        trim_text = "";
                        break;
                    }

                bool resumed = false;
                if (that.tts_paused) {
                    if (that.tts_paused_at + that.speech_impatient_ms > get_time_ms()) {
                        tts->speak_resume();
                        resumed = true;
                    } else {
                        tts->speak_stop();
                        //interrupt_handler();
                    }
                    that.tts_paused = false;
                }

                if (!trim_text.empty()) {

                    struct winsize w;
                    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
                    string outp = "\r" + that.commander.get_command_line_ref().get_prompt() + trim_text;
                    for (int i = outp.size(); i < w.ws_col - 2; i++) outp += " ";
                    cout << outp << endl;
                    that.text_puffer.push_back(trim_text);
                    if (!recs && !resumed) {
                        string inp = (text_input.empty() ? "" : "\n") + implode("\n", that.text_puffer);
                        that.text_input += inp;
                        that.text_puffer.clear();
                    }

                    // if (recs == 0 && !tts->is_speaking()) 
                    //     speak_beep(" ");
                }

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
            // while (/*!speak_paused_at_ms &&*/ tts->is_speaking()) sleep(1);
            // cout << "[[START]]" << endl;
            tts->speak_stop();
            // speak_paused_at_ms = 0;
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