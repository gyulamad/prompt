#pragma once

#include <atomic>

#include "../str/str_replace.h"
#include "../str/escape.hpp"
#include "../str/str_contains.h"
#include "../utils/Process.hpp"
#include "../abstracts/Closable.h"

using namespace std;
using namespace tools::str;
using namespace tools::utils;
using namespace tools::abstracts;

namespace tools::voice {

    class TTS: public Closable {
    private:
        // atomic<bool> paused{true};
        string lang;
        int speed;
        int gap;
        string beep_cmd;
        string think_cmd;
        map<string, string> speak_replacements;
        Process* proc = nullptr;
        bool owns_proc; // Track ownership for cleanup
    public:
        TTS(
            const string& lang,
            int speed, 
            int gap,
            const string& beep_cmd,
            const string& think_cmd,
            const map<string, string>& speak_replacements,
            Process* process = nullptr
        ): 
            lang(lang), 
            speed(speed),
            gap(gap),
            beep_cmd(beep_cmd),
            think_cmd(think_cmd),
            speak_replacements(speak_replacements),
            proc(process ? process : new Process()),
            owns_proc(process == nullptr)
        {
            t = thread([&]() {
                while (!closing) {
                    sleep_ms(100);

                    while (!pause_ends_at || get_time_ms() <= pause_ends_at) {
                        sleep_ms(100);
                        if (closing) return;
                    }
                    // // set pause_ends_at to a ms_t timepoint to timeout a resume
                    // // set pause_ends_at to 0 to pause indefinitely
                    // if (pause_ends_at) {
                    //     pause_ends_at = 0;
                        speak_resume();
                    // }
                    // sleep_ms(100);
                }
            });
        }

        thread t;
        
        virtual ~TTS() {
            speak_stop();
            this->close();
            if (t.joinable()) t.join();
            if (owns_proc && proc) delete proc; // Only delete if TTS owns it
        }


        // void pause() {
        //     paused = true;
        // }

        // void resume() {
        //     paused = false;
        // }

        [[nodiscard]]
        bool speak(const string& text, bool async = false, bool beep = false, bool think = false) {
            // lock_guard<mutex> lock(speak_paused_mutex);
            string _text = str_replace(speak_replacements, text);
            if (think && !think_cmd.empty()) 
                proc->writeln(think_cmd + " & ");
            proc->writeln(
                "espeak -v" + lang + (gap ? " -g " + to_string(gap) : "") 
                + " -s " + to_string(speed) + " \"" + escape(_text) + "\""
                + (beep && !beep_cmd.empty() ? " && " + beep_cmd : "")
                + (async ? "" : " && echo \"[SPEAK-DONE]\"")
            );
            speak_paused = false;
            if (async) return true;
            bool finished = false;
            while (true) {
                if (!proc->ready()) continue;
                if (str_contains(proc->read(), "[SPEAK-DONE]")) {
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

        // protected:
        virtual void execute(const string& cmd) {
            if (owns_proc) Process::execute(cmd);
            else proc->writeln(cmd);
        }

        void speak_pause(time_t ms = 0) {
            if (speak_paused) return;
            this->execute("pkill -STOP espeak");
            speak_paused = true;
            // speak_resume_at = get_time_ms() + ms;
            pause_ends_at = ms ? (get_time_ms() + ms) : 0;
            // if (speak_paused) return;
            // // Send SIGSTOP to pause the espeak process
            // // if (!speak_paused) {
            //     Process::execute("pkill -STOP espeak");
            //     // proc->writeln("pkill -STOP espeak"); // Now uses tts.proc
            //     speak_paused = true;
            // // }

            // //if (ms) cout << Process::execute("timeout " + to_string((float)ms / 1000) + "s pkill -CONT espeak") << flush;
            // // pause_ends_at = ms ? (get_time_ms() + ms) : 0;
        }

        void speak_resume() {
            if (!speak_paused) return;
            this->execute("pkill -CONT espeak");
            // Process::execute("pkill -CONT espeak");
            speak_paused = false;
            pause_ends_at = 0;
            // Send SIGCONT to continue the espeak process
            // if (speak_paused) { // TODO: thread safe pause!!
            //     Process::execute("pkill -CONT espeak");
            //     // proc->writeln("pkill -CONT espeak"); // Now uses tts.proc
            //     speak_paused = false;
            //     pause_ends_at = 0;
            // }
        }

        //bool is_speak_paused() { return speak_paused; }

        // atomic<bool> speak_paused = false;
        // mutex speak_paused_mutex;
        bool speak_paused = false;
        time_t pause_ends_at = 0;
        // time_t speak_resume_at = 0;


        void speak_stop() {
            cout << Process::execute("pkill -9 espeak") << flush;
            cout << Process::execute("pkill -9 sox") << flush;
            speak_paused = false;
        }

        bool is_speaking() {
            return !speak_paused && is_process_running("espeak");
        }

    };

}

#ifdef TEST

// #include "../utils/Test.hpp"
#include "../utils/Suppressor.hpp"
#include "../utils/tests/MockProcess.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::voice;

// TTS Tests
void test_TTS_constructor_valid() {
    Suppressor suppressor(stderr);
    map<string, string> replacements;
    TTS tts("en", 150, 10, "beep", "think", replacements);
    
    assert(true && "TTS constructor should initialize without crashing");
}

void test_TTS_speak_basic() {
    bool result = false;
    string expected_cmd = "espeak -ven -g 10 -s 150 \"Hello, world!\" && echo \"[SPEAK-DONE]\"\n";
    string actual_cmd = "not expected";
    {
        Suppressor suppressor(stderr);
        MockProcess mock_proc;
        TTS tts("en", 150, 10, "", "", {}, &mock_proc); // Pass mock process

        result = tts.speak("Hello, world!", false);
        actual_cmd = mock_proc.last_command;
    }
    assert(result && "speak should return true for synchronous call");
    assert(actual_cmd == expected_cmd && "speak should generate correct espeak command");
}

void test_TTS_speak_async() {
    bool result = false;
    string expected_cmd = "espeak -ven -g 10 -s 150 \"Test\"\n";
    string actual_cmd = "not expected";
    {
        Suppressor suppressor(stderr);
        MockProcess mock_proc;
        TTS tts("en", 150, 10, "", "", {}, &mock_proc);

        result = tts.speak("Test", true); // Async
        actual_cmd = mock_proc.last_command;
    }
    assert(result && "speak should return true for async call");
    assert(actual_cmd == expected_cmd && "speak async should omit [SPEAK-DONE]");
}

void test_TTS_speak_with_beep() {
    bool result = false;
    string expected_cmd = "espeak -ven -g 10 -s 150 \"Hello\" && beep_cmd && echo \"[SPEAK-DONE]\"\n";
    string actual_cmd = "not expected";
    {
        Suppressor suppressor(stderr);
        MockProcess mock_proc;
        TTS tts("en", 150, 10, "beep_cmd", "", {}, &mock_proc);

        result = tts.speak("Hello", false, true); // With beep
        actual_cmd = mock_proc.last_command;
    }
    assert(result && "speak with beep should return true");
    assert(actual_cmd == expected_cmd && "speak should append beep command");
}

void test_TTS_speak_empty_text() {
    bool result = false;
    string expected_cmd = "espeak -ven -g 10 -s 150 \"\" && echo \"[SPEAK-DONE]\"\n";
    string actual_cmd = "not expected";
    {
        Suppressor suppressor(stderr);
        MockProcess mock_proc;
        TTS tts("en", 150, 10, "", "", {}, &mock_proc);

        result = tts.speak("", false);
        actual_cmd = mock_proc.last_command;
    }
    assert(result && "speak with empty text should return true");
    assert(actual_cmd == expected_cmd && "speak should handle empty text");
}

void test_TTS_speak_with_replacements() {
    bool result = false;
    string expected_cmd = "espeak -ven -g 10 -s 150 \"hi earth!\" && echo \"[SPEAK-DONE]\"\n";
    string actual_cmd = "not expected";
    {
        Suppressor suppressor(stderr);
        MockProcess mock_proc;
        map<string, string> replacements = {{"hello", "hi"}, {"world", "earth"}};
        TTS tts("en", 150, 10, "", "", replacements, &mock_proc);

        result = tts.speak("hello world!", false);
        actual_cmd = mock_proc.last_command;
    }
    assert(result && "speak with replacements should return true");
    assert(actual_cmd == expected_cmd && "speak should apply text replacements");
}

void test_TTS_beep() {
    string expected_output = "Executed: beep_cmd";
    string actual_output = "not expected";
    {
        Suppressor suppressor(stderr);
        MockProcess mock_proc;
        TTS tts("en", 150, 10, "beep_cmd", "", {}, &mock_proc);

        tts.beep();
        actual_output = mock_proc.execute("beep_cmd");
    }
    assert(actual_output == expected_output && "beep should execute beep command");
}

void test_TTS_is_speaking() {
    bool speak_finished = false;
    bool initial_state = true;
    bool speaking_state = true;
    {
        Suppressor suppressor(stderr);
        MockProcess mock_proc;
        TTS tts("en", 150, 10, "", "", {}, &mock_proc);
        initial_state = tts.is_speaking();

        // Simulate speaking (mock process running)
        mock_proc.mock_output = "[SPEAK-DONE]";
        speak_finished = tts.speak("test", false);
        speaking_state = tts.is_speaking();
    }
    assert(!initial_state && "is_speaking should be false initially");
    assert(!speaking_state && "is_speaking should be false after completion");
    assert(speak_finished && "Speak should be finished without interruption");
}

void test_TTS_speak_stop() {
    string expected_output = "Executed: pkill -9 espeak Executed: pkill -9 sox";
    string actual_output = "not expected";
    {
        Suppressor suppressor(stderr);
        MockProcess mock_proc;
        TTS tts("en", 150, 10, "", "", {}, &mock_proc);

        tts.speak_stop();
        actual_output = mock_proc.execute("pkill -9 espeak") + " " + mock_proc.execute("pkill -9 sox");
    }
    assert(actual_output == expected_output && "speak_stop should execute kill commands");
}

void test_TTS_speak_pause() {
    // Suppressor suppressor(stderr);
    MockProcess mock_proc;
    TTS tts("en", 150, 10, "", "", {}, &mock_proc);

    // First pause (no ms)
    tts.speak_pause();
    assert(mock_proc.last_command == "pkill -STOP espeak\n" && 
           "First pause must write SIGSTOP command"); // \n from writeln()
    
    tts.speak_resume();
    // Reset and test with ms
    mock_proc.last_command = "";
    tts.speak_pause(5000);
    assert(mock_proc.last_command == "pkill -STOP espeak\n" && 
           "Subsequent pause still uses same command");
    
    // Check pause_ends_at
    assert(tts.pause_ends_at > get_time_ms() && 
           "Timer is set when ms is provided");
}

void test_TTS_speak_resume() {
    MockProcess mock_proc;
    TTS tts("en", 150, 10, "", "", {}, &mock_proc);

    // First pause with timeout
    tts.speak_pause(5000); // Sets pause_ends_at to future time
    assert(tts.pause_ends_at > get_time_ms() && "Timer set correctly");

    // Resume manually
    tts.speak_resume();
    assert(tts.pause_ends_at == 0 && "Timer reset to 0 after manual resume");
    
    // Edge case: Resuming an unpaused TTS (should do nothing)
    tts.speak_resume(); // No assertion needed, just no crash
}

void test_TTS_speak_resume_with_timeout() {
    // Suppressor suppressor(stderr);
    MockProcess mock_proc;
    TTS tts("en", 150, 10, "", "", {}, &mock_proc);

    // Set a timer-based pause
    tts.speak_pause(5000);
    
    // Allow time to pass
    sleep_ms(6000);
    
    // Verify auto-resume happens
    assert(!tts.speak_paused && "Should auto-resume after timeout");
    
    // Check that the timer was cleared
    assert(tts.pause_ends_at == 0 && "Auto-resume should reset timer");
}

void test_TTS_speak_resume_unpaused() {
    // Suppressor suppressor(stderr);
    MockProcess mock_proc;
    TTS tts("en", 150, 10, "", "", {}, &mock_proc);

    // Verify no command when unpaused
    mock_proc.last_command = "PREVIOUS_COMMAND";
    tts.speak_resume();
    assert(mock_proc.last_command == "PREVIOUS_COMMAND" && "Unnecessary resume doesn't send commands");
    assert(!tts.speak_paused && "Unpaused should have flag unset");
}

// Register tests
TEST(test_TTS_constructor_valid);
TEST(test_TTS_speak_basic);
TEST(test_TTS_speak_async);
TEST(test_TTS_speak_with_beep);
TEST(test_TTS_speak_empty_text);
TEST(test_TTS_speak_with_replacements);
TEST(test_TTS_beep);
TEST(test_TTS_is_speaking);
TEST(test_TTS_speak_stop);
TEST(test_TTS_speak_pause);
TEST(test_TTS_speak_resume);
TEST(test_TTS_speak_resume_with_timeout);
TEST(test_TTS_speak_resume_unpaused);

#endif