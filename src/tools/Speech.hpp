#pragma once

#include <iostream>
#include <string>
// #include <fstream>
#include <vector>
#include <cstdlib>
#include <ctime>

#include "io.hpp"
#include "strings.hpp"
#include "files.hpp"
#include "JSON.hpp"
#include "Process.hpp"
#include "Rotary.hpp"
#include "ANSI_FMT.hpp"
#include "system.hpp"

#include "llm/Model.hpp"

using namespace std;

namespace tools {

    class Speech {
    public:

        static Rotary rotary_listen;
        static Rotary rotary_record;
        static Rotary rotary_speech;

        Rotary* rotary = nullptr;

        map<string, string> langs = {
            { "en", "english" },
            { "hu", "hungarian" },
            // add more languages if you need
        };
        int speed;
        double noise_treshold;
        bool stalling = true;
        vector<string> hesitors = {
            // "Hmm, talán.",
            // "Nos, oké.",
            // "Oké, várjunk.",
            // "Lássuk, hát.",
            // "Talán, értem.",
            // "Hát, rendben.",
            // "Értem, persze.",
            // "Rendben, aha.",
            // "Persze, jó.",
            // "Aha, tényleg.",
            // "Jó, na...",
            // "Tényleg, hm...",
            // "Na, oké.",
            // "Hm, persze.",
            // "Várjunk, jó.",
            // "Értem, hmm.",
            // "Hmm, hadd gondoljam.",
            // "Nos, meglátjuk.",
            // "Oké, rendben van.",
            // "Lássuk csak, mi lesz.",
            // "Talán, ezt megbeszéljük.",
            // "Hát, ez érdekes kérdés.",
            // "Értem, de hadd gondolkozzak.",
            // "Rendben, erre még visszatérünk.",
            // "Persze, most nézem.",
            // "Aha, valóban így van.",
            // "Jó, értem már.",
            // "Tényleg? Akkor nézzük.",
            // "Na, lássuk a dolgokat.",
            // "Hm, ez bonyolultnak tűnik.",
            // "Várjunk csak, mi is a kérdés?",
            // "Értem, de ehhez idő kell.",
            // "Hmm, egy percet kérek.",
            // "Nos, akkor lássuk.",
            // "Oké, induljunk ki ebből.",
            // "Lássuk, mit tehetünk.",
            // "Talán, van rá megoldás.",
            // "Hát, ezt át kell gondolnom.",
            // "Értem, de kérek egy kis időt.",
            // "Nos, hadd gondoljam át ezt egy pillanatra, hogy biztos legyek a válaszomban.",
            // "Érdekes kérdés/kérés. Hadd nézzem meg alaposabban, hogy a legjobb választ tudjam adni.",
            // "Ez egy jó gondolat, engedjék meg, hogy egy kicsit elmerüljek a részletekben.",
            // "Rendben, értem a kérdést/kérést. Adj egy kis időt, amíg átgondolom a lehetőségeket.",
            // "Hadd mérlegeljem a különböző szempontokat, hogy biztosan a megfelelő választ adjam.",
            // "Éppen most próbálom átgondolni, hogy hogyan tudnám ezt a legjobban megközelíteni.",
            // "Ez egy összetett dolog, hadd szánjak rá egy kis időt, hogy alaposan megvizsgáljam.",
            // "Rendben, hadd fejtsem ki ezt a gondolatot egy picit bővebben, hogy jobban megértsem magam is.",
            // "Engedd meg, hogy egy picit elidőzzek ezen, hogy ne kapkodjam el a választ.",
            // "Nos, ez valóban elgondolkodtató. Adj egy pillanatot, amíg formálom a gondolataimat.",
            // "Értem, mit kérdezel. Lássuk csak, hogy is van ez.",
            // "Hmm, ez egy érdekes felvetés. Hadd gondolkodjak rajta egy percig.",
            // "Oké, vágom. Hadd vessem össze az információkat, és mindjárt válaszolok.",
            // "Értem, mire gondolsz. Adj egy pillanatot, hadd pontosítsam a válaszomat.",
            // "Hát persze, természetesen. Hadd gondoljam át a legjobb megoldást."
        };

    private:
        // -------------- TODO: configs -----------------
        const string base_path = "/tmp/";
        const string voice_check_sh_path = base_path + "voice_check.sh";
        const string speechf = base_path + "testrec.wav";
        const string tempf = base_path + "sox-temp.wav";//"/tmp/temp-record-copy.wav";
        const int kHz = 16000;
        const string beep_cmd = "sox -v 0.1 beep.wav -t wav - | aplay -q -N";
        // ----------------------------------------------


        Process pkiller;
        Process proc;
        const string secret;
        string lang;

        bool rec_interrupted = false;
        bool say_interrupted = false;
        

        long long reclastms = 0;
        void recstat(int timeout_ms = 150) {
            if (!rotary) return;
            long long nowms = get_time_ms();
            if (nowms > reclastms + timeout_ms) {
                reclastms = nowms;
                rotary->tick();
            }
        }

        // ------- voice check -------

        void create_voice_check_sh() { 
            rec_close();           
            if (file_exists(voice_check_sh_path)) remove_voice_check_sh();
            if (!file_put_contents(voice_check_sh_path, R"(
                while true; do
                    timeout 0.3 arecord -f cd -t wav -r 16000 -c 1 2>/dev/null > /tmp/silent_check.wav
                    sox /tmp/silent_check.wav -n stat 2>&1  | grep "Maximum amplitude" | awk '{print $3}'
                    sleep 0.1
                done
            )")) 
                throw ERROR("Unable to create: " + voice_check_sh_path);
            if (chmod(voice_check_sh_path.c_str(), 0x777) != 0) 
                throw ERROR("Unable to create voice checker to listen...");
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
            const string& lang, // = "en",
            int speed, // = 175,
            double noise_treshold // = 0.2
        ): 
            secret(secret),
            lang(lang),
            speed(speed),
            noise_treshold(noise_treshold)
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
            if (is_silence(tempf)) return "";
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
                            --data-binary '@" + tempf + "' \
                            -H 'Authorization: Bearer " + secret + "' -s";
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
            pkiller.writeln("pkill -9 arecord");
            pkiller.writeln("pkill -9 sox");

            rec_interrupted = false;
            string output;
            fs::remove(speechf);
            fs::remove(tempf);

            Process checker;
            checker.writeln(voice_check_sh_path); //("voice_check.sh"); // TODO: create if not exists            

            while (true) { 
                string recerr = "";
                bool recdone = false;

                // recording the speech
                // cout << "RECSTART!" << endl;
                if (is_process_running("arecord")) {
                    pkiller.writeln("pkill -9 arecord");
                    continue;
                }
                proc.writeln(
                    //"timeout " + to_string(timeout) + " "
                    //"arecord -f cd -t wav -r " + to_string(kHz) + " -c 1 2>&1 | " ///dev/null | " // TODO error to a log that is "/dev/null" by default + add every parameter cusomizable
                    "arecord -f cd -t wav -r " + to_string(kHz) + " -c 1 2>/dev/null | " // TODO error to a log that is "/dev/null" by default + add every parameter cusomizable
                    "sox -t wav - -t wav " + speechf + " silence 1 0.1 2% 1 0.9 2%  && "
                    "sox " + speechf + " " + tempf + " trim 0.01 && "
                    // "mv -f " + tempf + " " + speechf + " && "
                    "echo \"[record_done]\""
                );

                while (true) { // waiting for "done"

                    recstat();
                    output = trim(proc.read());
                    if (!output.empty()) {
                        // if (str_contains(output, "[record_done]")) 
                            recdone = true;
                        if (output == "[record_done]") break;
                        if (str_contains(output, "[record_done]")) {
                            //TODO: cerr << "\nrecord: " << output << endl;  
                            recerr = output;                       
                            break;
                        }
                        //DEBUG(output);
                        continue;
                    }

                    string check = trim(checker.read());
                    if (!check.empty()) {
                        //DEBUG(check);
                        vector<string> checks = explode("\n", check);
                        double ampl = .0;
                        for (const string& check: checks) {
                            double a = is_numeric(check) ? parse<double>(check) : .0;
                            if (ampl < a) ampl = a;
                        }
                        //cout << "?: " << check << endl;
                        if (ampl > noise_treshold && ampl < 0.99) {
                            shtup();
                            // if (rotary == &rotary_record) { // TODO: hack: do not use status output to track record state!!!
                            //     break;
                            // }
                            rotary = &rotary_record;
                        } else {
                            if (!is_speaking()) {
                                if (rotary != &rotary_record) 
                                    rotary = &rotary_listen;
                            }
                        }

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
                        if (rotary) rotary->clear();
                        shtup();
                        rec_close();
                        cleanprocs();                    
                        checker.kill();
                        rec_interrupted = true;
                        return "";
                    }

                } 

                if(rotary) rotary->clear();

                if (recdone && recerr.empty() && !is_silence(tempf)) {
                    proc.writeln(beep_cmd + " &");
                    shtup();
                    rec_close();
                    checker.kill();

                    stall();

                    string userin = stt();
                    return userin;
                }
            }

            throw ERROR("Should not reach here!");
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

        bool is_silence(const string& recordf) {
            string output = trim(
                Process::execute("sox " + recordf + " -n stat 2>&1 | grep \"Maximum amplitude\" | awk '{print $3}'")
            );
            // proc.writeln("sox " + recordf + " -n stat 2>&1 | grep \"Maximum amplitude\" | awk '{print $3}'");
            // string output = "";
            // output = trim(proc.read());
            // int i = 10;
            // while (output.empty() && i--)
            //     output = trim(proc.read());
                
            if (!is_numeric(output)) return true;
            double ampl = parse<double>(output);
            return ampl < noise_treshold;
            // string output = "";
            // while ((output = trim(proc.read())).empty());
            // return str_starts_with(output, "0.0"); // TODO: convert to double
        }

        void say(const string& text, bool async = false, int gap = 0, int speed_override = 0, bool beep = false) {
            shtup();
            rotary = &rotary_speech;
            say_interrupted = false;
            proc.writeln("espeak -v" + lang + (gap ? " -g " + to_string(gap) : "") + " -s " + to_string(speed_override ? speed_override : speed) + " \"" + escape(str_replace({
                { "...", "."},
                { "***", ""},
                { "**", ""},
                { "*", ""},
                { "'", ""},
            }, text)) + "\"" + (beep ? " && " + beep_cmd : "") + (async ? "" : " && echo"));
            if (!async) while ((proc.read()).empty()) { // waiting for "done"
                if (kbhit()) { // keyhit breaks
                    shtup();
                    return; // misschars();
                }
            }
        }

        void say_beep(const string& text, bool async = false, int gap = 10, int speed_override = 0) {
            say(text, async, gap, speed_override, true);
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


}
