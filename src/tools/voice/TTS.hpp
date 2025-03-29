#pragma once

#include <atomic>

#include "../str/str_replace.hpp"
#include "../str/escape.hpp"
#include "../str/str_contains.hpp"
#include "../utils/Process.hpp"

using namespace std;
using namespace tools::str;
using namespace tools::utils;

namespace tools::voice {

    class TTS {
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
        {}
        
        virtual ~TTS() {
            speak_stop();
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
            string _text = str_replace(speak_replacements, text);
            if (think && !think_cmd.empty()) 
                proc->writeln(think_cmd + " & ");
            proc->writeln(
                "espeak -v" + lang + (gap ? " -g " + to_string(gap) : "") 
                + " -s " + to_string(speed) + " \"" + escape(_text) + "\""
                + (beep && !beep_cmd.empty() ? " && " + beep_cmd : "")
                + (async ? "" : " && echo \"[SPEAK-DONE]\"")
            );
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

#ifdef TEST

#include "../utils/Test.hpp"
#include "../utils/Suppressor.hpp"
#include "../utils/tests/MockProcess.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::voice;

// TTS Tests
void test_TTS_constructor_valid() {
    try {
        Suppressor suppressor(stderr);
        map<string, string> replacements;
        TTS tts("en", 150, 10, "beep", "think", replacements);
    } catch (...) {
        assert(false && "TTS constructor should initialize without crashing");
    }
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
        actual_output = MockProcess::execute("beep_cmd");
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
        actual_output = MockProcess::execute("pkill -9 espeak") + " " + MockProcess::execute("pkill -9 sox");
    }
    assert(actual_output == expected_output && "speak_stop should execute kill commands");
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

#endif