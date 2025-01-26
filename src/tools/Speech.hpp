#pragma once

#include <iostream>
#include <string>
// #include <fstream>


#include "io.hpp"
#include "strings.hpp"
#include "files.hpp"
#include "JSON.hpp"
#include "Process.hpp"
#include "Rotary.hpp"
#include "ANSI_FMT.hpp"
#include "system.hpp"

using namespace std;

namespace tools {

    class Speech {
    public:
        map<string, string> langs = {
            { "en", "english" },
            { "hu", "hungarian" },
            // add more languages if you need
        };

        static Rotary rotary_listen;
        static Rotary rotary_record;
        static Rotary rotary_speech;

        Rotary* rotary = nullptr;

    private:
        // -------------- configs -----------------
        const string base_path = "/tmp/";
        const string voice_check_sh_path = base_path + "voice_check.sh";
        const string speechf = base_path + "testrec.wav";
        const string tempf = base_path + "sox-temp.wav";//"/tmp/temp-record-copy.wav";
        const int kHz = 16000;
        // ----------------------------------------


        Process pkiller;
        Process proc;
        const string secret;
        string lang;

        bool rec_interrupted;
        bool say_interrupted;
        

        long long reclastms = 0;
        void recstat(int timeout_ms = 150) {
            long long nowms = get_time_ms();
            if (nowms > reclastms + timeout_ms) {
                reclastms = nowms;
                rotary->tick();
            }
        }

        // ------- voice check -------

        void create_voice_check_sh() { 
            rec_close();           
            if (!file_exists(voice_check_sh_path)) {
                file_put_contents(voice_check_sh_path, R"(
while true; do
    timeout 0.3 arecord -f cd -t wav -r 16000 -c 1 2>/dev/null > silent_check.wav
    sox silent_check.wav -n stat 2>&1  | grep "Maximum amplitude" | awk '{print $3}'
    sleep 0.1
done
                )");            
            }
            if (chmod(voice_check_sh_path.c_str(), 0x777) != 0) throw ERROR("Unable to create voice checker to listen...");
            sleep(1);
        }

        void remove_voice_check_sh() {
            rec_close();
            if (!remove(voice_check_sh_path)) cerr << "Unable to remove: " << voice_check_sh_path << endl;
        }

        // -----------------------------

    public:
        Speech( // TODO: pop up those millions of parameters that used in this class
            const string& secret, 
            const string& lang = "en"
        ): 
            secret(secret),
            lang(lang)
        {
            create_voice_check_sh();
            cleanprocs();
        }
        
        ~Speech() {
            cleanprocs();
            remove_voice_check_sh();
            // TODO: remove all files;
            proc.kill();
            pkiller.kill();
        }


        void cleanprocs() {
            shtup();
            pkiller.writeln("pkill -9 arecord");
            pkiller.writeln("pkill -9 sox");
            rec_close();
        }

        bool is_rec_interrupted() {
            return rec_interrupted;
        }

        bool is_say_interrupted() {
            return say_interrupted;
        }

        void rec_close() {
            pkiller.writeln("pkill -f " + voice_check_sh_path);
        }

        bool is_speaking() {
            // pkiller.writeln("pgrep espeak");
            // string pid = trim(pkiller.read(1000));
            // return !pid.empty();
            return is_process_running("espeak");
        }

        void shtup() {
            if (!is_speaking()) return;
            pkiller.writeln("pkill -9 espeak");            
            say_interrupted = true;
            if (rotary != &rotary_record) rotary = &rotary_listen;
            // cout << "SHHHHH!" << endl;
            // proc.writeln("pkill -9 espeak");
            // rec_close();
        }
        
        // transcribe, override if you need to
        virtual string stt() {
            int retry = 10;
            while (retry) {
                try {
                    // transcribe
        // cout << "transcribe..." << endl;

                    proc.writeln(
                        "curl https://api-inference.huggingface.co/models/jonatasgrosman/wav2vec2-large-xlsr-53-" + langs.at(lang) + " \
                            -X POST \
                            --data-binary '@" + speechf + "' \
                            -H 'Authorization: Bearer " + secret + "' -s");

                    string output;
                    while ((output = proc.read(100)).empty());// if (kbhit()) return misschars(); // waiting for transcriptions
        // cout << "resp: " << output << endl;

                    // extract text

                    JSON json(output);
                    // while (!json.isDefined("text")) {
                    //     DEBUG(json.dump());
                    //     //break;  // TODO throw ERROR("STT transcibe failed:" + json.dump());
                    // }
                    output = json.get<string>("text");

                    return output;
                } catch (exception &e) {
                    cerr << "STT transcibe failed. (" << retry << " retry left...)" << endl;
                    sleep(6);
                    retry--;
                    if (retry <= 0 || kbhit()) break;
                }
            }
            return "";
        }

        string rec() {
            pkiller.writeln("pkill -9 arecord");
            pkiller.writeln("pkill -9 sox");

            rec_interrupted = false;
            string output;
            fs::remove(speechf);
            fs::remove(tempf);

            Process checker;
            checker.writeln(voice_check_sh_path); //("voice_check.sh"); // TODO: create if not exists            

            while (true) { 

                // recording the speech
                proc.writeln(
                    //"timeout " + to_string(timeout) + " "
                    "arecord -f cd -t wav -r " + to_string(kHz) + " -c 1 2>/dev/null | " // TODO error to a log that is "/dev/null" by default + add every parameter cusomizable
                    "sox -t wav - -t wav " + speechf + " silence 1 0.1 2% 1 0.9 2%  && "
                    "sox " + speechf + " " + tempf + " trim 0.2 && "
                    // "mv -f " + tempf + " " + speechf + " && "
                    "echo \"[record_done]\"");

                while (true) { // waiting for "done"  

                    recstat();
                    output = trim(proc.read());
                    if (!output.empty()) {
                        if (output == "[record_done]") break;
                        if (str_contains(output, "[record_done]")) {
                            //TODO: cerr << "\nrecord: " << output << endl;                           
                            break;
                        }
                    }

                    string check = trim(checker.read());
                    if (!check.empty()) {
                        double ampl = parse<double>(check);
                        //cout << "?: " << check << endl;
                        if (ampl > 0.2 && ampl < 0.99) {
                            shtup();
                            rotary = &rotary_record;
                        } else if (!is_speaking()) if (rotary != &rotary_record) rotary = &rotary_listen;

                        // if (!str_starts_with(check, "0.0") && 
                        //     !str_starts_with(check, "0.1") && 
                        //     !str_starts_with(check, "0.99") &&
                        //     !str_starts_with(check, "1.0") &&
                        //     !str_starts_with(check, "-")) {// TODO: convert to double
                        //     //cout << "shhhh!" << endl;
                        //     shtup(); 
                        // } 
                    }

                    if (kbhit()) {
                        while (kbhit()) getchar();
                        rotary->clear();
                        shtup();
                        rec_close();
                        checker.kill();
                        return "";
                    }

                } 

                rotary->clear();

                if (!is_silence(tempf)) {
                    proc.writeln("sox -v 0.1 beep.wav -t wav - | aplay -q -N");
                    shtup();
                    rec_close();
                    checker.kill();
                    string userin = stt();
                    return userin;
                }
            }

            throw ERROR("Should not reach here!");
        }
        
        bool is_silence(const string& recordf) {
            proc.writeln("sox " + recordf + " -n stat 2>&1 | grep \"Maximum amplitude\" | awk '{print $3}'");
            string output = proc.read();
            // string output = "";
            // while ((output = trim(proc.read())).empty());
            return str_starts_with(output, "0.0"); // TODO: convert to double
        }

        void say(const string& text, int speed = 175, bool async = false) {
            shtup();
            rotary = &rotary_speech;
            say_interrupted = false;
            proc.writeln("espeak -v" + lang + " -s " + to_string(speed) + " \"" + escape(str_replace({
                { "...", "."},
                { "***", ""},
                { "**", ""},
                { "*", ""},
            }, text)) + "\"" + (async ? "" : " && echo"));
            if (!async) while ((proc.read()).empty()) { // waiting for "done"
                if (kbhit()) { // keyhit breaks
                    shtup();
                    return; // misschars();
                }
            }
        }

    };

    Rotary Speech::rotary_listen({
        // RotaryFrames({ ANSI_FMT(ANSI_FMT_C_RED ANSI_FMT_T_BOLD, "🔴"/*"●"*/), "  "}, 2),     // Emojis
        RotaryFrames({ 

    #define _X_ ANSI_FMT_RESET ANSI_FMT_C_GREEN "═"
    #define _O_ ANSI_FMT_RESET ANSI_FMT_C_GREEN "-"
    #define _o_ ANSI_FMT_T_DIM ANSI_FMT_C_GREEN "-"
    #define ___ ANSI_FMT_T_DIM ANSI_FMT_C_BLACK "-"

            ANSI_FMT("[", _o_ _O_ _X_ ___ ___ ) + "]",
            ANSI_FMT("[", _o_ _o_ _O_ _X_ " " ) + "]",
            ANSI_FMT("[", _o_ _o_ _o_ _O_ _X_ ) + "]",
            ANSI_FMT("[", _o_ _o_ _o_ _o_ _X_ ) + "]",
            ANSI_FMT("[", ___ _o_ _o_ _o_ _X_ ) + "]",
            ANSI_FMT("[", ___ ___ _o_ _X_ _O_ ) + "]",
            ANSI_FMT("[", ___ ___ _X_ _O_ _o_ ) + "]",
            ANSI_FMT("[", " " _X_ _O_ _o_ _o_ ) + "]",
            ANSI_FMT("[", _X_ _O_ _o_ _o_ _o_ ) + "]",
            ANSI_FMT("[", _X_ _o_ _o_ _o_ _o_ ) + "]",
            ANSI_FMT("[", _X_ _o_ _o_ _o_ ___ ) + "]",
            ANSI_FMT("[", _O_ _X_ _o_ ___ ___ ) + "]",

    #undef _X_
    #undef _O_
    #undef _o_
    #undef ___

        }, 1),     // Dots
        // RotaryFrames({ 
        //     ANSI_FMT(ANSI_FMT_C_BLACK, "-") + " ]", 
        //     ANSI_FMT(ANSI_FMT_C_BLACK, "\\") + " ]", 
        //     ANSI_FMT(ANSI_FMT_C_BLACK, "|") + " ]", 
        //     ANSI_FMT(ANSI_FMT_C_BLACK, "/") + " ]",
        // }, 1),     // Sticks
    });


    Rotary Speech::rotary_record({

    #define _X_ ANSI_FMT_RESET ANSI_FMT_C_RED "●"

        RotaryFrames({ 
            ANSI_FMT("[", ANSI_FMT_RESET ANSI_FMT_C_RED "● " ANSI_FMT_RESET ANSI_FMT_C_BLACK "rec" ) + "]",
            ANSI_FMT("[", ANSI_FMT_RESET ANSI_FMT_C_RED "● " ANSI_FMT_T_DIM ANSI_FMT_C_BLACK "rec" ) + "]",
            ANSI_FMT("[", ANSI_FMT_RESET ANSI_FMT_C_RED "● " ANSI_FMT_RESET ANSI_FMT_C_BLACK "   " ) + "]",
        }, 1),     // Sticks
    #undef _X_
    #undef _O_
    #undef _o_
    #undef ___

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

    });


}
