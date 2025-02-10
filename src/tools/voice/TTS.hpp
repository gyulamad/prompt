#pragma once

#include <atomic>

#include "../strings.hpp"
#include "../Process.hpp"

using namespace std;

namespace tools::voice {

    class TTS {
    private:
        // atomic<bool> paused{true};
        Process proc;
        string lang;
        int speed;
        int gap;
        string beep_cmd;
        string think_cmd;
    public:
        TTS(
            const string& lang,
            int speed, 
            int gap,
            const string& beep_cmd,
            const string& think_cmd
        ): 
            lang(lang), 
            speed(speed),
            gap(gap),
            beep_cmd(beep_cmd),
            think_cmd(think_cmd)
        {}
        
        virtual ~TTS() {
            speak_stop();
        }


        // void pause() {
        //     paused = true;
        // }

        // void resume() {
        //     paused = false;
        // }

        void speak_beep(const string& text, bool think = false) {
            // if (paused) return;
            string _text = str_replace({
                { "...", "."},
                { "***", ""},
                { "**", ""},
                { "*", ""},
                { "'", ""},
            }, text);
            if (think && !think_cmd.empty()) 
                proc.writeln(think_cmd + " & ");
            proc.writeln(
                "espeak -v" + lang + (gap ? " -g " + to_string(gap) : "") 
                + " -s " + to_string(speed) + " \"" + escape(_text) 
                + "\"" + (!beep_cmd.empty() ? " && " + beep_cmd : "")
                //+ (think ? + " " + (!think_cmd.empty() ? " && " + think_cmd : "") : "")
            );
        }

        void speak_pause() {
            // Send SIGSTOP to pause the espeak process
            cout << Process::execute("pkill -STOP espeak") << flush;
        }

        void speak_resume() {
            // Send SIGCONT to continue the espeak process
            cout << Process::execute("pkill -CONT espeak") << flush;
        }

        void speak_stop() {
            cout << Process::execute("pkill -9 espeak") << flush;
            cout << Process::execute("pkill -9 sox") << flush;
        }

        bool is_speaking() {
            return is_process_running("espeak");
        }

    };

}