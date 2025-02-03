#pragma once

#include <iostream>
#include <string>
// #include <fstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <future>

#include "io.hpp"
#include "strings.hpp"
#include "files.hpp"
#include "JSON.hpp"
#include "Process.hpp"
#include "Rotary.hpp"
#include "ANSI_FMT.hpp"
#include "system.hpp"
#include "Logger.hpp"
#include "vectors.hpp"

#include "llm/Model.hpp"

using namespace std;

namespace tools {

    // class Recorder {
    // private:
    //     thread t;
    //     atomic<bool> paused = true; // Flag to control the thread
    //     atomic<bool> recording = false; // Flag to control the thread

    //     void record() {
    //         while (recording) {
    //             if (paused) continue;
                
    //             cout << "Recording..." << endl;
    //             this_thread::sleep_for(chrono::seconds(1));
    //         }
    //         cout << "Record stop." << endl;
    //     }

    // public:
    //     Recorder() {}
        
    //     virtual ~Recorder() {
    //         stop();
    //         if (t.joinable()) t.join();
    //     }

    //     void start() {
    //         t = thread(&Recorder::record, this);
    //         recording = true;
    //         paused = false;
    //     }

    //     void pause() {
    //         paused = true;
    //     }

    //     void resume() {
    //         paused = false;
    //     }
        
    //     void stop() {
    //         paused = true;
    //         recording = false;
    //     }
    // };

    class Speech {
    private:
        // ---------------------------------
        // common
        class STT_Result {
            mutex data_mutex;
            string data = "";

        public:
            void append(const string& new_data) {
                lock_guard<mutex> lock(data_mutex);
                data += (data.empty() ? "" : "\n") + new_data;
            }

            string fetch() {
                lock_guard<mutex> lock(data_mutex);
                string result = move(data);
                data.clear();
                return result;
            }

            bool empty() {
                lock_guard<mutex> lock(data_mutex);
                return data.empty();
            }

        } rec_stt_result;


        const string beep_cmd = "sox -v 0.1 beep.wav -t wav - | aplay -q -N";
        const string secret;
        const string lang;

        // ---------------------------------

        Process proc;

        // ---------------------------------
        // Rotaries

        static Rotary rotary_listen;
        static Rotary rotary_record;
        static Rotary rotary_speech;
        Rotary* rotary = nullptr;

        void rotary_clear() {
            if (rotary) rotary->clear();
            rotary = nullptr;
        }

        string ampstat() {
            return to_string(noise_treshold * 100) + "% / " + to_string(mic_amp * 100) + "% ";
        }

        long long reclastms = 0;
        void recstat(int timeout_ms = 150) {
            // cout << "." << flush;
            if (!rotary) return;
            if (rotary == &rotary_speech && !is_process_running("espeak")) {
                rotary = &rotary_listen;
            } else if (rotary == &rotary_listen) {
                if (mic_amp > noise_treshold) {
                    rotary = &rotary_record;
                } else if (is_process_running("espeak")) {
                    // rotary_clear();
                    rotary = &rotary_speech;
                }
            } else if (rotary == &rotary_record) {
                if (mic_amp < noise_treshold) {
                    rotary = &rotary_listen;
                }
            }
            long long nowms = get_time_ms();
            if (nowms > reclastms + timeout_ms) {
                reclastms = nowms;
                //string msg = rotary == &rotary_record ? ampstat() : "";
                if (rotary) rotary->tick(ampstat() + " ");
            }
        }

        // ---------------------------------
        // STT
        bool voice_in = true;

        thread rec_thread;
        atomic<bool> recording = true;
        atomic<bool> rec_idle = false;

        atomic<bool> amp_stop = false;
        thread amp_thread;

        bool rec_interrupted = false;

        map<string, string> langs = {
            { "en", "english" },
            { "hu", "hungarian" },
            // add more languages if you need
        };

        // const string base_path = "/tmp/";
        // const string speechf = base_path + "testrec.wav";
        // const string tempf = base_path + "sox-temp.wav";//"/tmp/temp-record-copy.wav";
        // const int kHz = 16000;
        double noise_treshold;
        double noise_treshold_min;
        double noise_treshold_max;

        void cleanfiles() {
            remove("rec_accu.wav", false);
            remove("rec_next.wav", false);
            remove("rec_next_trim.wav", false);
            remove("rec_stt.wav", false);
        }

        void cleanprocs() {
            Process pkiller;
            pkiller.writeln("pkill -9 aspeak");
            pkiller.writeln("pkill -9 arecord");
            pkiller.writeln("pkill -9 sox");
            pkiller.kill();
        }

        // ---------------------------------
        // TTS
        bool voice_out = true;

        bool say_interrupted = false;
        bool stalling = true;
        vector<string> hesitors = {};
        int speed;

        // ---------------------------------

    public:
        Speech( // TODO: pop up those millions of parameters that used in this class
            const string& secret, 
            const string& lang, // = "en",
            int speed, // = 175,
            bool voice_in,
            bool voice_out,
            double noise_treshold,
            double noise_treshold_min,
            double noise_treshold_max
        ): 
            secret(secret),
            lang(lang),
            speed(speed),
            voice_in(voice_in),
            voice_out(voice_out),
            noise_treshold(noise_treshold),
            noise_treshold_min(noise_treshold_min),
            noise_treshold_max(noise_treshold_max) 
        {
            cleanprocs();
            cleanfiles();
            recording = true;
            rec_idle = false;
            rec_thread = thread(&Speech::rec_bgtask, this);
            amp_stop = false;
            amp_thread = thread(&Speech::amp_bgtask, this);
        }

        virtual ~Speech() {
            recording = false;
            rec_idle = false;
            if (rec_thread.joinable()) rec_thread.join();
            amp_stop = true;
            if (amp_thread.joinable()) amp_thread.join();
            cleanprocs();
            cleanfiles();
        }

        // ---------------- Mic Amplitudo Updater --------------------

        double mic_amp = 0;
        void amp_bgtask() {
            Process ampproc;
            while(!amp_stop) {
                usleep(300);
                ampproc.writeln(
                    "arecord -d 1 -f cd -t wav 2>/dev/null "
                    "| sox -t wav - -n stat 2>&1 "
                    "| grep \"Maximum amplitude\" "
                    "| awk '{print $3}'"
                );
                sleep(1);
                long long timeout_ms = 1200;
                long long timeout_at = get_time_ms() + timeout_ms;
                string output = "";
                while (true) {
                    // if (ampproc.ready())
                        output = trim(ampproc.read());
                    if (timeout_at < get_time_ms()) break;
                    if (output.empty()) continue;
                    break;
                }
                //cout << "(((" << output << ")))" << endl;
                vector<string> splits = explode("\n", output);
                vector<string> lines = array_filter(splits);
                if (lines.empty()) continue;
                string last = lines.back();
                if (!is_numeric(last)) continue;
                double mic_amp_prev = mic_amp;
                mic_amp = parse<double>(last);
                //cout << "AMP-MIC:" << to_string(mic_amp) << "/" << noise_treshold << endl;
                // mic_amp = parse<double>(amps);
                if (!rec_idle) { // rec() called (listen/rec)
                    if (mic_amp > noise_treshold) {                    
                        // if (mic_amp_prev <= noise_treshold) {
                        //     //cout << "******?>>>USER START TALKING???" << endl;
                        //     rotary = &rotary_record;
                        // }
                        shtup();
                        // say_interrupted = true;
                    }
                }
            }
            ampproc.kill();
        }

        // ---------------- STT --------------------

        void set_voice_in(bool voice_in) {
            this->voice_in = voice_in;
        }

        bool is_voice_in() const {
            return voice_in;
        }

        void set_noise_treshold(double noise_treshold) {
            this->noise_treshold = noise_treshold;
        }

        double get_noise_treshold() const {
            return noise_treshold;
        }

        void set_noise_treshold_min(double noise_treshold_min) {
            this->noise_treshold_min = noise_treshold_min;
        }

        double get_noise_treshold_min() const {
            return noise_treshold_min;
        }

        void set_noise_treshold_max(double noise_treshold_max) {
            this->noise_treshold_max = noise_treshold_max;
        }

        double get_noise_treshold_max() const {
            return noise_treshold_max;
        }

        bool is_rec_interrupted() const {
            return rec_interrupted;
        }

        void rec_bgtask() {
            if (file_exists("rec.log")) remove("rec.log");
            Logger reclog("reclog", "rec.log");
            Process recproc;
            
            // records the next chunk of speech
                            
            long long rec_timeout_ms = 1000;
            // double speech_min_amp = 0.05;

            //cout << "recording rec_accu.wav" << endl; 
            string output;
            long long rec_timeouts_at;

            while (recording) {
                while (rec_idle || rec_interrupted) usleep(100);

                //cout << "REC_next starts" << endl;
                output = "";
                string noise_treshold_pc_start = to_string((int)(noise_treshold*150));
                string noise_treshold_pc_stop = to_string((int)(noise_treshold*100));
                string cmd = 
                    "sox -d -r 16000 -c 1 -t wav rec_next.wav "
                    "silence "
                        "1 0.1 " + noise_treshold_pc_start + "% "
                        "1 0.9 " + noise_treshold_pc_stop + "%";
                //cout << cmd << endl;
                recproc.writeln(cmd);
                rec_timeouts_at = get_time_ms() + rec_timeout_ms;

                while (recording) {
                    recstat();
                    if (get_time_ms() > rec_timeouts_at) {
                        //cout << "TIMEOUT!!!" << endl;
                        reclog.info(Process::execute("pkill -9 sox"));
                        break;
                    }
                    if (!recproc.ready()) continue;
                    string output = recproc.read();
                    if (output.empty()) continue;
                    rec_timeouts_at = get_time_ms() + rec_timeout_ms;
                    
                    //////////////////////////////////////////////
                    // cout << output << flush;
                    //////////////////////////////////////////////


                    // if (str_contains(output, "sox WARN")) {
                    //     cout << "sox WARN!!!" << endl;
                    //     //reclog.info(Process::execute("pkill -9 sox"));
                    //     continue;
                    // }
                    if (str_ends_with(trim(output), "Done.")) {

                        // Cut any artifacts to check if it's silence and repeat if so..
                        if (file_exists("rec_next_trim.wav")) remove("rec_next_trim.wav");
                        string ampls = trim(Process::execute(
                            "sox rec_next.wav rec_next_trim.wav trim 0.2 && "
                            "sox rec_next_trim.wav -n stat 2>&1 "
                            "| grep \"Maximum amplitude\" "
                            "| awk '{print $3}'"
                        ));
                        if (!is_numeric(ampls)) {
                            //cout << "*************WTF????? NOT NUMERIC: " << ampls << endl;
                            break;
                        }
                        double ampl = parse<double>(ampls);
                        //cout << "-----------MAX AMP: [" << to_string(ampl) << "]" << endl;
                        if (ampl < noise_treshold) {
                            // cout << "*************WTF????? ..too quite... (maybe artifacts trig?gers ?? )" << endl;
                            break;
                        }

                        // accumulate the next pieces of speech to the full speach
                        if (!file_exists("rec_accu.wav")) {
                            //cout << "creating rec_accu.wav" << endl;
                            rename("rec_next.wav", "rec_accu.wav");
                        } else {
                            //cout << "appending to rec_accu.wav" << endl;
                            reclog.info(Process::execute("sox rec_accu.wav rec_next.wav rec_tmp.wav"));
                            rename("rec_tmp.wav", "rec_accu.wav");
                        }

                        if (file_exists("rec_stt.wav")) break;

                        // send to STT if it's free..
                        // cout << "********NEW RECORD DONE (rec_stt.wav) => we can go for STT!!!!!!!!" << endl; 
                        proc.writeln(beep_cmd + " &");
                        rotary_clear();
                        thread([&]{    
                            long long timeout_ms = 10000;
                            long long timeout_at = get_time_ms() + timeout_ms;
                            while(file_exists("rec_stt.wav")) {
                                if (timeout_at < get_time_ms()) break;
                                usleep(300);
                            }
                            rename("rec_accu.wav", "rec_stt.wav", true);
                            string inp = trim(stt("rec_stt.wav"));
                            if (!inp.empty()) rec_stt_result.append(inp);
                            remove("rec_stt.wav", true);
                        }).detach();
                        //cout << "---------STT request sent to async" << endl;                      
                        
                        // }
                        
                        break;
                    }  
                } // [/while]
                //cout << "REC end" << endl;
            } // [/while]

            recproc.kill();
        }

        string fetch_rec_stt_result() {
            return rec_stt_result.fetch();
        }
        
        // transcribe, override if you need to
        virtual string stt(const string& speechfile) {
            while (kbhit()) getchar(); // TODO ??
            int retry = 3;
            string cmd = "";
            while (retry) {
                try {
                    // transcribe
        // cout << "transcribe..." << endl;

                    Process proc;
                    cmd = "curl https://api-inference.huggingface.co/models/jonatasgrosman/wav2vec2-large-xlsr-53-" + langs.at(lang) + " \
                            -X POST \
                            --data-binary '@" + speechfile + "' \
                            -H 'Authorization: Bearer " + secret + "' -s";
                    // cout << cmd << endl;
                    proc.writeln(cmd);

                    string output;
                    while((output = proc.read()).empty());// if (kbhit()) return misschars(); // waiting for transcriptions
                    proc.kill();
        // cout << "resp: " << output << endl;

                    // extract text

                    JSON json(output);
                    if (json.isDefined("error")) throw ERROR("STT error: " + json.dump());
                    // while (!json.isDefined("text")) {
                    //     DEBUG(json.dump());
                    //     //break;  // TODO throw ERROR("STT transcibe failed:" + json.dump());
                    // }
                    output = json.get<string>("text");

                    while (kbhit()) getchar(); // TODO ??
                    return output;
                } catch (exception &e) {
                    cerr 
                        << "STT transcibe failed: " << e.what() << endl
                        << --retry << " retry left, press a key to break..." << endl;
                    DEBUG("command: " + cmd);
                    sleep(5);
                    if (retry <= 0 || kbhit()) break;
                }
            }
            while (kbhit()) getchar(); // TODO ??
            return "";
        }

        string rec() {
            rec_interrupted = false;
            string inp = "";
            while (true) { 
                if (!rotary) rotary = &rotary_listen;
                if (kbhit()) {
                    rec_interrupted = true;
                    rotary_clear();
                    while (kbhit()) getchar();
                    //cleanfiles();
                    return "";
                }               
                string inp = rec_stt_result.fetch();
                if (!inp.empty()) {
                    rotary_clear();
                    //cleanfiles();
                    return inp;
                }
                usleep(300);
            }

            // if (!rotary) rotary = &rotary_listen;
            // rec_interrupted = false;
            // rec_idle = false;
            // while (file_exists("rec_stt.wav")) {
            //     if (kbhit()) {
            //         rec_interrupted = true;
            //         rotary_clear();
            //         while (kbhit()) getchar();
            //         cleanfiles();
            //         return "";
            //     }
            //     usleep(100);
            // }
            // while (!file_exists("rec_accu.wav")) {
            //     if (kbhit()) {
            //         rec_interrupted = true;
            //         rotary_clear();
            //         while (kbhit()) getchar();
            //         cleanfiles();
            //         return "";
            //     }
            //     usleep(100);
            // }
            // proc.writeln(beep_cmd + " &");
            // // send to STT if it's free..
            // rename("rec_accu.wav", "rec_stt.wav", true);
            // rotary_clear();
            // // cout << "********NEW RECORD DONE (rec_stt.wav) => we can go for STT!!!!!!!!" << endl;                                    
            // string text = stt("rec_stt.wav");
            // remove("rec_stt.wav", true);
            // rec_idle = true;
            // return text; // TODO
        }


        // ---------------- TTS --------------------


        void set_voice_out(bool voice_in) {
            this->voice_out = voice_out;
        }

        bool is_voice_out() const {
            return voice_out;   
        }


        bool is_say_interrupted() const {
            return say_interrupted;
        }

        void set_hesitors(const vector<string>& hesitors) {
            this->hesitors = hesitors;
        }

        vector<string>& get_hesitors_ref() {
            return hesitors;
        }

        void shtup() {
            if (!is_process_running("espeak")) return;
            cout << Process::execute("pkill -9 espeak") << flush;
            say_interrupted = true;
        }

        void say(const string& text, bool async = false, int gap = 0, int speed_override = 0, bool beep = false) {
            shtup();
            say_interrupted = false;
            proc.writeln("espeak -v" + lang + (gap ? " -g " + to_string(gap) : "") + " -s " + to_string(speed_override ? speed_override : speed) + " \"" + escape(str_replace({
                { "...", "."},
                { "***", ""},
                { "**", ""},
                { "*", ""},
                { "'", ""},
            }, text)) + "\"" + (beep ? " && " + beep_cmd : "") + (async ? "" : " && echo"));


            // TODO: sync talk is deprecated!!!!!
            if (!async) while ((proc.read()).empty()) { // waiting for "done"
                cout << "***********************????BLOCKING SPEECH?????? --------*********************";
                if (kbhit()) { // keyhit breaks
                    shtup();
                    return; // misschars();
                }
            }
        }

        void say_beep(const string& text, bool async = false, int gap = 10, int speed_override = 0) {
            //rotary = &rotary_speech;
            say(text, async, gap, speed_override, true);
        }

        void stall() {
            if (!stalling || hesitors.empty() || is_process_running("espeak")) return;
            // TODO:
            // *   **Filler words:** Ez a legáltalánosabb kifejezés, azt jelenti, "kitöltő szavak".
            // *   **Discourse markers:** Ez egy tudományosabb megnevezés, a "diszkurzív partikula" angol megfelelője.
            // *   **Hesitation markers:** Ez a habozást jelző szavak gyűjtőneve.
            // *   **Stall words:** Ez a kifejezés arra utal, hogy ezekkel a szavakkal időt nyerünk a gondolkodásra.            

            // Szükséges a random számok generálásához
            static bool seeded = false;
            if (!seeded) {
                srand(time(0));
                seeded = true;
            }
            // Véletlenszerű index generálása
            int index1 = rand() % hesitors.size();
            int index2 = rand() % hesitors.size();
            int index3 = rand() % hesitors.size();
            say(hesitors[index1] + "  " + 
                hesitors[index2] + "  " + 
                hesitors[index3] + "  ", 
                true, 20, speed * 0.7);
        }
    };

    Rotary Speech::rotary_listen({
        // RotaryFrames({ ANSI_FMT(ANSI_FMT_C_RED ANSI_FMT_T_BOLD, "🔴"/*"●"*/), "  "}, 2),     // Emojis
        RotaryFrames({ 

    #define _X_ ANSI_FMT_RESET ANSI_FMT_C_GREEN "═"
    #define _O_ ANSI_FMT_RESET ANSI_FMT_C_GREEN "-"
    #define _o_ ANSI_FMT_T_DIM ANSI_FMT_C_GREEN "-"
    #define ___ ANSI_FMT_T_DIM ANSI_FMT_C_BLACK "-"

            ANSI_FMT("[", _o_ _O_ _X_ ___ " " ) + "]",
            ANSI_FMT("[", _o_ _o_ _O_ _X_ " " ) + "]",
            ANSI_FMT("[", _o_ _o_ _o_ _O_ _X_ ) + "]",
            ANSI_FMT("[", ___ _o_ _o_ _o_ _X_ ) + "]",
            ANSI_FMT("[", ___ ___ _o_ _o_ _X_ ) + "]",
            ANSI_FMT("[", ___ ___ ___ _X_ _O_ ) + "]",
            ANSI_FMT("[", " " ___ _X_ _O_ _o_ ) + "]",
            ANSI_FMT("[", " " _X_ _O_ _o_ _o_ ) + "]",
            ANSI_FMT("[", _X_ _O_ _o_ _o_ _o_ ) + "]",
            ANSI_FMT("[", _X_ _o_ _o_ _o_ ___ ) + "]",
            ANSI_FMT("[", _X_ _o_ _o_ ___ ___ ) + "]",
            ANSI_FMT("[", _O_ _X_ ___ ___ ___ ) + "]",

    #undef _X_
    #undef _O_
    #undef _o_
    #undef ___

        }, 1),     // Dots
        RotaryFrames({ 
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"), 
            ANSI_FMT(ANSI_FMT_C_BLACK, "\\>"), 
            ANSI_FMT(ANSI_FMT_C_BLACK, "|>"), 
            ANSI_FMT(ANSI_FMT_C_BLACK, "/>"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"), 
            ANSI_FMT(ANSI_FMT_C_BLACK, "\\>"), 
            ANSI_FMT(ANSI_FMT_C_BLACK, "|>"), 
            ANSI_FMT(ANSI_FMT_C_BLACK, "/>"),
            ANSI_FMT(ANSI_FMT_C_BLACK, "->"), 
            ANSI_FMT(ANSI_FMT_C_BLACK, "\\>"), 
            ANSI_FMT(ANSI_FMT_C_BLACK, "|>"), 
            ANSI_FMT(ANSI_FMT_C_BLACK, "/>"),
        }, 1),     // Sticks
    });


    Rotary Speech::rotary_record({

    #define _X_ ANSI_FMT_RESET ANSI_FMT_C_RED "●"
        RotaryFrames({ 
            ANSI_FMT("[", ANSI_FMT_RESET ANSI_FMT_C_RED "● " ANSI_FMT_RESET ANSI_FMT_C_BLACK "rec" ) + "]",
            ANSI_FMT("[", ANSI_FMT_RESET ANSI_FMT_C_RED "● " ANSI_FMT_T_DIM ANSI_FMT_C_BLACK "rec" ) + "]",
            ANSI_FMT("[", ANSI_FMT_RESET ANSI_FMT_C_RED "● " ANSI_FMT_RESET ANSI_FMT_C_BLACK "   " ) + "]",
        }, 1),     // Sticks
    #undef _X_
    
        RotaryFrames({ "🎤" }, 1),

    });


    Rotary Speech::rotary_speech({

    #define _X_ ANSI_FMT_RESET ANSI_FMT_C_BLUE "●"
    #define _O_ ANSI_FMT_T_DIM ANSI_FMT_C_BLUE "◎"
    #define _o_ ANSI_FMT_T_DIM ANSI_FMT_C_BLUE "○"
    #define ___ ANSI_FMT_T_DIM ANSI_FMT_C_BLUE "-"
        RotaryFrames({ 
            ANSI_FMT("[", ___ ___ _X_ ___ ___ ) + "]",
            ANSI_FMT("[", ___ ___ _O_ _o_ ___ ) + "]",
            ANSI_FMT("[", _X_ ___ _o_ ___ ___ ) + "]",
            ANSI_FMT("[", _O_ ___ _O_ ___ _O_ ) + "]",
            ANSI_FMT("[", _o_ ___ _X_ ___ _X_ ) + "]",
            ANSI_FMT("[", ___ ___ _o_ _X_ _O_ ) + "]",
            ANSI_FMT("[", ___ ___ _X_ _o_ _o_ ) + "]",
            ANSI_FMT("[", ___ _o_ _O_ _O_ ___ ) + "]",
            ANSI_FMT("[", ___ ___ _o_ _o_ ___ ) + "]",
            ANSI_FMT("[", ___ ___ _o_ ___ ___ ) + "]",
            ANSI_FMT("[", _o_ ___ ___ ___ _O_ ) + "]",
            ANSI_FMT("[", ___ ___ _X_ ___ _o_ ) + "]",
            ANSI_FMT("[", _O_ ___ _X_ ___ ___ ) + "]",
            ANSI_FMT("[", _o_ _O_ _O_ ___ _o_ ) + "]",
            ANSI_FMT("[", ___ _X_ _o_ ___ ___ ) + "]",
            ANSI_FMT("[", ___ _O_ _o_ ___ _o_ ) + "]",
            ANSI_FMT("[", _o_ _o_ ___ ___ ___ ) + "]",
        }, 1),     // Sticks
    #undef _X_
    #undef _O_
    #undef _o_
    #undef ___

        RotaryFrames({ "🤖" }, 1),

    });









    // ===================================================================
    // ===================================================================
    // ===================================================================
    // ===================================================================
    // ===================================================================
    // ===================================================================
    // ===================================================================
    // ===================================================================
    // ===================================================================
    // ===================================================================
    // ===================================================================
    // ===================================================================
    // ===================================================================

    // class _Speech {
    // public:

    //     static Rotary rotary_listen;
    //     static Rotary rotary_record;
    //     static Rotary rotary_speech;

    //     Rotary* rotary = nullptr;

    //     map<string, string> langs = {
    //         { "en", "english" },
    //         { "hu", "hungarian" },
    //         // add more languages if you need
    //     };
    //     int speed;
    //     double noise_treshold;
    //     bool stalling = true;
    //     vector<string> hesitors = {
    //         // "Hmm, talán.",
    //         // "Nos, oké.",
    //         // "Oké, várjunk.",
    //         // "Lássuk, hát.",
    //         // "Talán, értem.",
    //         // "Hát, rendben.",
    //         // "Értem, persze.",
    //         // "Rendben, aha.",
    //         // "Persze, jó.",
    //         // "Aha, tényleg.",
    //         // "Jó, na...",
    //         // "Tényleg, hm...",
    //         // "Na, oké.",
    //         // "Hm, persze.",
    //         // "Várjunk, jó.",
    //         // "Értem, hmm.",
    //         // "Hmm, hadd gondoljam.",
    //         // "Nos, meglátjuk.",
    //         // "Oké, rendben van.",
    //         // "Lássuk csak, mi lesz.",
    //         // "Talán, ezt megbeszéljük.",
    //         // "Hát, ez érdekes kérdés.",
    //         // "Értem, de hadd gondolkozzak.",
    //         // "Rendben, erre még visszatérünk.",
    //         // "Persze, most nézem.",
    //         // "Aha, valóban így van.",
    //         // "Jó, értem már.",
    //         // "Tényleg? Akkor nézzük.",
    //         // "Na, lássuk a dolgokat.",
    //         // "Hm, ez bonyolultnak tűnik.",
    //         // "Várjunk csak, mi is a kérdés?",
    //         // "Értem, de ehhez idő kell.",
    //         // "Hmm, egy percet kérek.",
    //         // "Nos, akkor lássuk.",
    //         // "Oké, induljunk ki ebből.",
    //         // "Lássuk, mit tehetünk.",
    //         // "Talán, van rá megoldás.",
    //         // "Hát, ezt át kell gondolnom.",
    //         // "Értem, de kérek egy kis időt.",
    //         // "Nos, hadd gondoljam át ezt egy pillanatra, hogy biztos legyek a válaszomban.",
    //         // "Érdekes kérdés/kérés. Hadd nézzem meg alaposabban, hogy a legjobb választ tudjam adni.",
    //         // "Ez egy jó gondolat, engedjék meg, hogy egy kicsit elmerüljek a részletekben.",
    //         // "Rendben, értem a kérdést/kérést. Adj egy kis időt, amíg átgondolom a lehetőségeket.",
    //         // "Hadd mérlegeljem a különböző szempontokat, hogy biztosan a megfelelő választ adjam.",
    //         // "Éppen most próbálom átgondolni, hogy hogyan tudnám ezt a legjobban megközelíteni.",
    //         // "Ez egy összetett dolog, hadd szánjak rá egy kis időt, hogy alaposan megvizsgáljam.",
    //         // "Rendben, hadd fejtsem ki ezt a gondolatot egy picit bővebben, hogy jobban megértsem magam is.",
    //         // "Engedd meg, hogy egy picit elidőzzek ezen, hogy ne kapkodjam el a választ.",
    //         // "Nos, ez valóban elgondolkodtató. Adj egy pillanatot, amíg formálom a gondolataimat.",
    //         // "Értem, mit kérdezel. Lássuk csak, hogy is van ez.",
    //         // "Hmm, ez egy érdekes felvetés. Hadd gondolkodjak rajta egy percig.",
    //         // "Oké, vágom. Hadd vessem össze az információkat, és mindjárt válaszolok.",
    //         // "Értem, mire gondolsz. Adj egy pillanatot, hadd pontosítsam a válaszomat.",
    //         // "Hát persze, természetesen. Hadd gondoljam át a legjobb megoldást."
    //     };

    // private:
    //     // -------------- TODO: configs -----------------
    //     const string base_path = "/tmp/";
    //     const string voice_check_sh_path = base_path + "voice_check.sh";
    //     const string speechf = base_path + "testrec.wav";
    //     const string tempf = base_path + "sox-temp.wav";//"/tmp/temp-record-copy.wav";
    //     const int kHz = 16000;
    //     const string beep_cmd = "sox -v 0.1 beep.wav -t wav - | aplay -q -N";
    //     // ----------------------------------------------


    //     Process pkiller;
    //     Process proc;
    //     const string secret;
    //     string lang;

    //     bool rec_interrupted = false;
    //     bool say_interrupted = false;
        

    //     long long reclastms = 0;
    //     void recstat(int timeout_ms = 150) {
    //         if (!rotary) return;
    //         long long nowms = get_time_ms();
    //         if (nowms > reclastms + timeout_ms) {
    //             reclastms = nowms;
    //             rotary->tick();
    //         }
    //     }

    //     // ------- voice check -------

    //     void create_voice_check_sh() { 
    //         rec_close();           
    //         if (file_exists(voice_check_sh_path)) remove_voice_check_sh();
    //         if (!file_put_contents(voice_check_sh_path, R"(
    //             while true; do
    //                 timeout 0.3 arecord -f cd -t wav -r 16000 -c 1 2>/dev/null > /tmp/silent_check.wav
    //                 sox /tmp/silent_check.wav -n stat 2>&1  | grep "Maximum amplitude" | awk '{print $3}'
    //                 sleep 0.1
    //             done
    //         )")) 
    //             throw ERROR("Unable to create: " + voice_check_sh_path);
    //         if (chmod(voice_check_sh_path.c_str(), 0x777) != 0) 
    //             throw ERROR("Unable to create voice checker to listen...");
    //         sleep(1);
    //     }

    //     void remove_voice_check_sh() {
    //         rec_close();
    //         if (!remove(voice_check_sh_path)) cerr << "Unable to remove: " << voice_check_sh_path << endl;
    //     }

    //     // -----------------------------

    // public:
    //     _Speech( // TODO: pop up those millions of parameters that used in this class
    //         const string& secret, 
    //         const string& lang, // = "en",
    //         int speed, // = 175,
    //         double noise_treshold // = 0.2
    //     ): 
    //         secret(secret),
    //         lang(lang),
    //         speed(speed),
    //         noise_treshold(noise_treshold)
    //     {
    //         create_voice_check_sh();
    //         cleanprocs();
    //     }
        
    //     ~_Speech() {
    //         cleanprocs();
    //         remove_voice_check_sh();
    //         // TODO: remove all files;
    //         proc.kill();
    //         pkiller.kill();
    //     }


    //     void cleanprocs() {
    //         shtup();
    //         pkiller.writeln("pkill -9 arecord");
    //         pkiller.writeln("pkill -9 sox");
    //         rec_close();
    //     }

    //     bool is_rec_interrupted() {
    //         return rec_interrupted;
    //     }

    //     bool is_say_interrupted() {
    //         return say_interrupted;
    //     }

    //     void rec_close() {
    //         pkiller.writeln("pkill -f " + voice_check_sh_path);
    //     }

    //     bool is_speaking() {
    //         // pkiller.writeln("pgrep espeak");
    //         // string pid = trim(pkiller.read(1000));
    //         // return !pid.empty();
    //         return is_process_running("espeak");
    //     }

    //     void shtup() {
    //         if (!is_speaking()) return;
    //         pkiller.writeln("pkill -9 espeak");            
    //         say_interrupted = true;
    //         if (rotary != &rotary_record) rotary = &rotary_listen;
    //         // cout << "SHHHHH!" << endl;
    //         // proc.writeln("pkill -9 espeak");
    //         // rec_close();
    //     }
        
    //     // transcribe, override if you need to
    //     virtual string stt() {
    //         if (is_silence(tempf)) return "";
    //         while (kbhit()) getchar(); // TODO ??
    //         int retry = 3;
    //         string cmd = "";
    //         while (retry) {
    //             try {
    //                 // transcribe
    //     // cout << "transcribe..." << endl;

    //                 Process proc;
    //                 cmd = "curl https://api-inference.huggingface.co/models/jonatasgrosman/wav2vec2-large-xlsr-53-" + langs.at(lang) + " \
    //                         -X POST \
    //                         --data-binary '@" + tempf + "' \
    //                         -H 'Authorization: Bearer " + secret + "' -s";
    //                 proc.writeln(cmd);

    //                 string output;
    //                 while((output = proc.read()).empty());// if (kbhit()) return misschars(); // waiting for transcriptions
    //                 proc.kill();
    //     // cout << "resp: " << output << endl;

    //                 // extract text

    //                 JSON json(output);
    //                 if (json.isDefined("error")) throw ERROR("STT error: " + json.dump());
    //                 // while (!json.isDefined("text")) {
    //                 //     DEBUG(json.dump());
    //                 //     //break;  // TODO throw ERROR("STT transcibe failed:" + json.dump());
    //                 // }
    //                 output = json.get<string>("text");

    //                 while (kbhit()) getchar(); // TODO ??
    //                 return output;
    //             } catch (exception &e) {
    //                 cerr 
    //                     << "STT transcibe failed: " << e.what() << endl
    //                     << --retry << " retry left, press a key to break..." << endl;
    //                 DEBUG("command: " + cmd);
    //                 sleep(5);
    //                 if (retry <= 0 || kbhit()) break;
    //             }
    //         }
    //         while (kbhit()) getchar(); // TODO ??
    //         return "";
    //     }

    //     string rec() {
    //         pkiller.writeln("pkill -9 arecord");
    //         pkiller.writeln("pkill -9 sox");

    //         rec_interrupted = false;
    //         string output;
    //         fs::remove(speechf);
    //         fs::remove(tempf);

    //         Process checker;
    //         checker.writeln(voice_check_sh_path); //("voice_check.sh"); // TODO: create if not exists            

    //         while (true) { 
    //             string recerr = "";
    //             bool recdone = false;

    //             // recording the speech
    //             // cout << "RECSTART!" << endl;
    //             if (is_process_running("arecord")) {
    //                 pkiller.writeln("pkill -9 arecord");
    //                 continue;
    //             }
    //             proc.writeln(
    //                 //"timeout " + to_string(timeout) + " "
    //                 //"arecord -f cd -t wav -r " + to_string(kHz) + " -c 1 2>&1 | " ///dev/null | " // TODO error to a log that is "/dev/null" by default + add every parameter cusomizable
    //                 "arecord -f cd -t wav -r " + to_string(kHz) + " -c 1 2>/dev/null | " // TODO error to a log that is "/dev/null" by default + add every parameter cusomizable
    //                 "sox -t wav - -t wav " + speechf + " silence 1 0.1 2% 1 0.9 2%  && "
    //                 "sox " + speechf + " " + tempf + " trim 0.01 && "
    //                 // "mv -f " + tempf + " " + speechf + " && "
    //                 "echo \"[record_done]\""
    //             );

    //             while (true) { // waiting for "done"

    //                 recstat();
    //                 output = trim(proc.read());
    //                 if (!output.empty()) {
    //                     DEBUG("[ouypuy]" + output);
    //                     // if (str_contains(output, "[record_done]")) 
    //                         recdone = true;
    //                     if (output == "[record_done]") break;
    //                     if (str_contains(output, "[record_done]")) {
    //                         //TODO: cerr << "\nrecord: " << output << endl;  
    //                         recerr = output;                       
    //                         break;
    //                     }
    //                     //DEBUG(output);
    //                     continue;
    //                 }

    //                 string check = trim(checker.read());
    //                 if (!check.empty()) {
    //                     //DEBUG(check);
    //                     vector<string> checks = explode("\n", check);
    //                     double ampl = .0;
    //                     for (const string& check: checks) {
    //                         double a = is_numeric(check) ? parse<double>(check) : .0;
    //                         if (ampl < a) ampl = a;
    //                     }
    //                     //cout << "?: " << check << endl;
    //                     if (ampl > noise_treshold && ampl < 0.99) {
    //                         shtup();
    //                         // if (rotary == &rotary_record) { // TODO: hack: do not use status output to track record state!!!
    //                         //     break;
    //                         // }
    //                         rotary = &rotary_record;
    //                     } else {
    //                         if (!is_speaking()) {
    //                             if (rotary != &rotary_record) 
    //                                 rotary = &rotary_listen;
    //                         }
    //                     }

    //                     // if (!str_starts_with(check, "0.0") && 
    //                     //     !str_starts_with(check, "0.1") && 
    //                     //     !str_starts_with(check, "0.99") &&
    //                     //     !str_starts_with(check, "1.0") &&
    //                     //     !str_starts_with(check, "-")) {// TODO: convert to double
    //                     //     //cout << "shhhh!" << endl;
    //                     //     shtup(); 
    //                     // } 
    //                 }

    //                 if (kbhit()) {
    //                     while (kbhit()) getchar();
    //                     if (rotary) rotary->clear();
    //                     shtup();
    //                     rec_close();
    //                     cleanprocs();                    
    //                     checker.kill();
    //                     rec_interrupted = true;
    //                     return "";
    //                 }

    //             } 

    //             if(rotary) rotary->clear();

    //             if (recdone && recerr.empty() && !is_silence(tempf)) {
    //                 proc.writeln(beep_cmd + " &");
    //                 shtup();
    //                 rec_close();
    //                 checker.kill();

    //                 stall();

    //                 string userin = stt();
    //                 return userin;
    //             }
    //         }

    //         throw ERROR("Should not reach here!");
    //     }

    //     void stall() {
    //         if (!stalling || hesitors.empty() || is_process_running("espeak")) return;
    //         // TODO:
    //         // *   **Filler words:** Ez a legáltalánosabb kifejezés, azt jelenti, "kitöltő szavak".
    //         // *   **Discourse markers:** Ez egy tudományosabb megnevezés, a "diszkurzív partikula" angol megfelelője.
    //         // *   **Hesitation markers:** Ez a habozást jelző szavak gyűjtőneve.
    //         // *   **Stall words:** Ez a kifejezés arra utal, hogy ezekkel a szavakkal időt nyerünk a gondolkodásra.            

    //         // Szükséges a random számok generálásához
    //         static bool seeded = false;
    //         if (!seeded) {
    //             srand(time(0));
    //             seeded = true;
    //         }
    //         // Véletlenszerű index generálása
    //         int index1 = rand() % hesitors.size();
    //         int index2 = rand() % hesitors.size();
    //         int index3 = rand() % hesitors.size();
    //         say(hesitors[index1] + "  " + 
    //             hesitors[index2] + "  " + 
    //             hesitors[index3] + "  ", 
    //             true, 20, speed * 0.7);
    //     }

    //     bool is_silence(const string& recordf) {
    //         string output = trim(
    //             Process::execute("sox " + recordf + " -n stat 2>&1 | grep \"Maximum amplitude\" | awk '{print $3}'")
    //         );
    //         // proc.writeln("sox " + recordf + " -n stat 2>&1 | grep \"Maximum amplitude\" | awk '{print $3}'");
    //         // string output = "";
    //         // output = trim(proc.read());
    //         // int i = 10;
    //         // while (output.empty() && i--)
    //         //     output = trim(proc.read());
                
    //         if (!is_numeric(output)) return true;
    //         double ampl = parse<double>(output);
    //         return ampl < noise_treshold;
    //         // string output = "";
    //         // while ((output = trim(proc.read())).empty());
    //         // return str_starts_with(output, "0.0"); // TODO: convert to double
    //     }

    //     void say(const string& text, bool async = false, int gap = 0, int speed_override = 0, bool beep = false) {
    //         shtup();
    //         rotary = &rotary_speech;
    //         say_interrupted = false;
    //         proc.writeln("espeak -v" + lang + (gap ? " -g " + to_string(gap) : "") + " -s " + to_string(speed_override ? speed_override : speed) + " \"" + escape(str_replace({
    //             { "...", "."},
    //             { "***", ""},
    //             { "**", ""},
    //             { "*", ""},
    //             { "'", ""},
    //         }, text)) + "\"" + (beep ? " && " + beep_cmd : "") + (async ? "" : " && echo"));
    //         if (!async) while ((proc.read()).empty()) { // waiting for "done"
    //             if (kbhit()) { // keyhit breaks
    //                 shtup();
    //                 return; // misschars();
    //             }
    //         }
    //     }

    //     void say_beep(const string& text, bool async = false, int gap = 10, int speed_override = 0) {
    //         say(text, async, gap, speed_override, true);
    //     }

    // };

    // Rotary _Speech::rotary_listen({
    //     // RotaryFrames({ ANSI_FMT(ANSI_FMT_C_RED ANSI_FMT_T_BOLD, "🔴"/*"●"*/), "  "}, 2),     // Emojis
    //     RotaryFrames({ 

    // #define _X_ ANSI_FMT_RESET ANSI_FMT_C_GREEN "═"
    // #define _O_ ANSI_FMT_RESET ANSI_FMT_C_GREEN "-"
    // #define _o_ ANSI_FMT_T_DIM ANSI_FMT_C_GREEN "-"
    // #define ___ ANSI_FMT_T_DIM ANSI_FMT_C_BLACK "-"

    //         ANSI_FMT("[", _o_ _O_ _X_ ___ " " ) + "]",
    //         ANSI_FMT("[", _o_ _o_ _O_ _X_ " " ) + "]",
    //         ANSI_FMT("[", _o_ _o_ _o_ _O_ _X_ ) + "]",
    //         ANSI_FMT("[", ___ _o_ _o_ _o_ _X_ ) + "]",
    //         ANSI_FMT("[", ___ ___ _o_ _o_ _X_ ) + "]",
    //         ANSI_FMT("[", ___ ___ ___ _X_ _O_ ) + "]",
    //         ANSI_FMT("[", " " ___ _X_ _O_ _o_ ) + "]",
    //         ANSI_FMT("[", " " _X_ _O_ _o_ _o_ ) + "]",
    //         ANSI_FMT("[", _X_ _O_ _o_ _o_ _o_ ) + "]",
    //         ANSI_FMT("[", _X_ _o_ _o_ _o_ ___ ) + "]",
    //         ANSI_FMT("[", _X_ _o_ _o_ ___ ___ ) + "]",
    //         ANSI_FMT("[", _O_ _X_ ___ ___ ___ ) + "]",

    // #undef _X_
    // #undef _O_
    // #undef _o_
    // #undef ___

    //     }, 1),     // Dots
    //     RotaryFrames({ 
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"), 
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "\\>"), 
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "|>"), 
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "/>"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"), 
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "\\>"), 
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "|>"), 
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "/>"),
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "->"), 
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "\\>"), 
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "|>"), 
    //         ANSI_FMT(ANSI_FMT_C_BLACK, "/>"),
    //     }, 1),     // Sticks
    // });


    // Rotary _Speech::rotary_record({

    // #define _X_ ANSI_FMT_RESET ANSI_FMT_C_RED "●"
    //     RotaryFrames({ 
    //         ANSI_FMT("[", ANSI_FMT_RESET ANSI_FMT_C_RED "● " ANSI_FMT_RESET ANSI_FMT_C_BLACK "rec" ) + "]",
    //         ANSI_FMT("[", ANSI_FMT_RESET ANSI_FMT_C_RED "● " ANSI_FMT_T_DIM ANSI_FMT_C_BLACK "rec" ) + "]",
    //         ANSI_FMT("[", ANSI_FMT_RESET ANSI_FMT_C_RED "● " ANSI_FMT_RESET ANSI_FMT_C_BLACK "   " ) + "]",
    //     }, 1),     // Sticks
    // #undef _X_
    
    //     RotaryFrames({ "🎤" }, 1),

    // });


    // Rotary _Speech::rotary_speech({

    // #define _X_ ANSI_FMT_RESET ANSI_FMT_C_BLUE "●"
    // #define _O_ ANSI_FMT_T_DIM ANSI_FMT_C_BLUE "◎"
    // #define _o_ ANSI_FMT_T_DIM ANSI_FMT_C_BLUE "○"
    // #define ___ ANSI_FMT_T_DIM ANSI_FMT_C_BLUE "-"
    //     RotaryFrames({ 
    //         ANSI_FMT("[", ___ ___ _X_ ___ ___ ) + "]",
    //         ANSI_FMT("[", ___ ___ _O_ _o_ ___ ) + "]",
    //         ANSI_FMT("[", _X_ ___ _o_ ___ ___ ) + "]",
    //         ANSI_FMT("[", _O_ ___ _O_ ___ _O_ ) + "]",
    //         ANSI_FMT("[", _o_ ___ _X_ ___ _X_ ) + "]",
    //         ANSI_FMT("[", ___ ___ _o_ _X_ _O_ ) + "]",
    //         ANSI_FMT("[", ___ ___ _X_ _o_ _o_ ) + "]",
    //         ANSI_FMT("[", ___ _o_ _O_ _O_ ___ ) + "]",
    //         ANSI_FMT("[", ___ ___ _o_ _o_ ___ ) + "]",
    //         ANSI_FMT("[", ___ ___ _o_ ___ ___ ) + "]",
    //         ANSI_FMT("[", _o_ ___ ___ ___ _O_ ) + "]",
    //         ANSI_FMT("[", ___ ___ _X_ ___ _o_ ) + "]",
    //         ANSI_FMT("[", _O_ ___ _X_ ___ ___ ) + "]",
    //         ANSI_FMT("[", _o_ _O_ _O_ ___ _o_ ) + "]",
    //         ANSI_FMT("[", ___ _X_ _o_ ___ ___ ) + "]",
    //         ANSI_FMT("[", ___ _O_ _o_ ___ _o_ ) + "]",
    //         ANSI_FMT("[", _o_ _o_ ___ ___ ___ ) + "]",
    //     }, 1),     // Sticks
    // #undef _X_
    // #undef _O_
    // #undef _o_
    // #undef ___

    //     RotaryFrames({ "🤖" }, 1),

    // });


}
