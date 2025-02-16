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
        map<string, string> speak_replacements;
    public:
        TTS(
            const string& lang,
            int speed, 
            int gap,
            const string& beep_cmd,
            const string& think_cmd,
            const map<string, string>& speak_replacements
        ): 
            lang(lang), 
            speed(speed),
            gap(gap),
            beep_cmd(beep_cmd),
            think_cmd(think_cmd),
            speak_replacements(speak_replacements)
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

        bool speak(const string& text, bool async = false, bool beep = false, bool think = false) {
            string _text = str_replace(speak_replacements, text);
            if (think && !think_cmd.empty()) 
                proc.writeln(think_cmd + " & ");
            proc.writeln(
                "espeak -v" + lang + (gap ? " -g " + to_string(gap) : "") 
                + " -s " + to_string(speed) + " \"" + escape(_text) + "\""
                + (beep && !beep_cmd.empty() ? " && " + beep_cmd : "")
                + (async ? "" : " && echo \"[SPEAK-DONE]\"")
            );
            if (async) return true;
            bool finished = false;
            while (true) {
                if (!proc.ready()) continue;
                if (str_contains(proc.read(), "[SPEAK-DONE]")) {
                    finished = true;
                    break;
                }
                if (!is_speaking()) break;
            }
            return finished;
        }

        void beep() {
            if (!beep_cmd.empty()) Process::execute(beep_cmd); // proc.writeln(beep_cmd);
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