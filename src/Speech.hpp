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
#include "tools/datetime.hpp"

#include "tools/voice/WhisperSTT.hpp"

using namespace std;
using namespace tools;
using namespace tools::voice;

namespace prompt {

    class Speech {
    private:
        // bool paused = false;
        atomic<bool> mic_enabled = true;
        atomic<bool> mic_hidden = false;

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
            if (strings.size() == 0) return "";
            int index = rand() % strings.size();
            return strings[index];
        }
        
        void rand_speak_beep(const vector<string>& strings, bool think = false) {
            if (!tts->is_speaking()) speak(rand_speak_select(strings), true, true, think);
        }

        // ------------- interruptions --------------

        bool loud_prev = false;
        bool tts_paused = false;
        long long tts_paused_at = 0;

        // ------------------------------------------

        Commander& commander;
        vector<string> speech_ignores_rgxs;
        long long speech_impatient_ms;

        // ---- MIC output ----

        int mic_out_size = 0;

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
            const map<string, string> speech_tts_speak_replacements,
            const double speech_stt_voice_recorder_sample_rate,
            const unsigned long speech_stt_voice_recorder_frames_per_buffer,
            const size_t speech_stt_voice_recorder_buffer_seconds,
            const float speech_stt_noise_monitor_threshold_pc,
            const float speech_stt_noise_monitor_rmax_decay_pc,
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
                speech_tts_think_cmd,
                speech_tts_speak_replacements
            );

            stt = new WhisperSTT(
                speech_stt_voice_recorder_sample_rate,
                speech_stt_voice_recorder_frames_per_buffer,
                speech_stt_voice_recorder_buffer_seconds,
                speech_stt_noise_monitor_threshold_pc,
                speech_stt_noise_monitor_rmax_decay_pc,
                speech_stt_noise_monitor_window,
                speech_stt_transcriber_model,
                lang,
                speech_stt_poll_interval_ms
            );

            tts->speak_stop(); // reset the voice outputs

            Speech& that = *this;

            stt->setRMSHandler([&](float vol_pc, float threshold_pc, float rmax, float rms, bool loud, bool muted) {
                if (!mic_enabled) return;

                // ----- handle speech interruptions --------------------------------------------

                if (!that.tts_paused && loud && !that.loud_prev && that.tts->is_speaking()) {
                    //cout << "[DEBUG] speach paused." << endl;
                    that.tts->speak_pause();
                    that.tts_paused = true;
                    that.tts_paused_at = get_time_ms();
                }

                that.loud_prev = loud;

                // ------------------------------------------------------------------------------
                
                if (mic_hidden) return;

                string out = ""; 

                // REC status
                out += (muted ? "MUTE" : string(loud ? "*REC" : "?MIC")) + " ";

                // RMS volume
                //out += "V:" + set_precision(threshold_pc * 100, 2) + "/" + set_precision(rms * 100, 2) + " \t[";
                out += "[";
                double step = 0.1;
                double i = threshold_pc;
                for (; i < 1; i += step) {
                    if (muted) out += "_";
                    else if (vol_pc > i) out += "•";
                    else out += "◦"; //"·•◦";
                }
                out += "] VOL: " + set_precision(threshold_pc * 100, 2) + "/" + set_precision(vol_pc * 100, 2) + "% ";
                out += "RMS: " + set_precision(rmax, 2) + "/" + set_precision(rms, 2) + " ";
                
                // progress roller
                that.rollnxt++;
                int at = that.rollnxt%4;
                char c = that.roller[at];
                string roll = "";
                roll += c;
                out += (stt->getTranscriberCRef().isInProgress() 
                    ? " [" + roll + "] Progress: " + tools::to_string(that.recs) + ".. " 
                    : "                   ") + " ";

                mic_draw(out);

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
                        //cout << "[DEBUG] speach resume." << endl;
                        tts->speak_resume();
                        resumed = true;
                    } else {
                        //cout << "[DEBUG] speach stop (interrupted)." << endl;
                        tts->speak_stop();
                        //interrupt_handler();
                    }
                    that.tts_paused = false;
                }

                if (!trim_text.empty()) {

                    struct winsize w;
                    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
                    mic_hide();
                    //cout << endl;
                    string outp = that.commander.get_command_line_ref().get_prompt() + trim_text;
                    // for (int i = outp.size(); i < w.ws_col - 2; i++) outp += " ";
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
            mic_hide();
            delete stt;
            delete tts;
        }
        

        bool is_mic_enabled() const {
            return mic_enabled;
        }

        void mic_disable() {
            mic_mute();
            mic_hide();
            mic_enabled = false;
        }

        void mic_enable() {
            mic_unmute();
            mic_enabled = true;
        }

        void mic_draw(string out) {
            mic_out_size = out.size();
            for (int i = 0; i < mic_out_size; i++) out += "\b";
            cout << out << flush;
        }

        void mic_show() {
            mic_hidden = false;
        }

        void mic_clear() {
            string out = "";
            for (int i = 0; i < mic_out_size; i++) out += "\b";
            for (int i = 0; i < mic_out_size; i++) out += " ";
            for (int i = 0; i < mic_out_size; i++) out += "\b";
            cout << out << flush;
            mic_out_size = 0;
        }

        void mic_hide() {
            mic_hidden = true;
            mic_clear();
        }

        bool is_mic_hidden() const {
            return mic_hidden;
        }

        void mic_mute() {
            NoiseMonitor* monitor = stt->getMonitorPtr();
            if (!monitor) return;
            monitor->set_muted(true);
        }

        void mic_unmute() {
            NoiseMonitor* monitor = stt->getMonitorPtr();
            if (!monitor) return;
            monitor->set_muted(false);
        }

        void mic_mute_toggle() {
            NoiseMonitor* monitor = stt->getMonitorPtr();
            if (!monitor) return;
            if (monitor->is_muted()) monitor->set_muted(false);
            else monitor->set_muted(true);
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

        bool speak(const string& text, bool async = false, bool beep = false, bool think = false) {
            tts->speak_stop();
            return tts->speak(text, async, beep, think);
        }

        void beep() {
            tts->beep();
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