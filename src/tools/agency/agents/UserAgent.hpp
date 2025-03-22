#pragma once

#include "../../cmd/Commander.hpp"
#include "../../voice/STT.hpp"
#include "../../utils/InputPipeInterceptor.hpp"
#include "../Agent.hpp"

using namespace tools::cmd;
using namespace tools::voice;
using namespace tools::utils;
using namespace tools::agency;

namespace tools::agency::agents {
    
    template<typename T>
    class UserAgent: public Agent<T> {
    public:

        // TODO: make configurable:
        atomic<bool> voice_input_echo = true;
        atomic<bool> text_input_echo = true;

        const vector<string> speech_ignores_rgxs = { // TODO: in config
            // "^\\[.*\\]$",
            // "^\\*.*\\*$",
            // "^\\(.*\\)$",
            "^\\-.*[\\.\\!\\-]$" // (for hungarian??)
        };

        UserAgent(
            PackQueue<T>& queue, 
            Agency<T>& agency, 
            Commander* commander = nullptr, 
            STT* stt = nullptr,
            InputPipeInterceptor* interceptor = nullptr
        ): 
            Agent<T>(queue, "user"), 
            agency(agency), 
            commander(commander), 
            stt(stt),
            interceptor(interceptor)
        {
            stt_setup();
        }

        virtual ~UserAgent() {
            if (interceptor) {
                interceptor->unsubscribe(this);
                interceptor->close();
            }
            if (stt) stt->stop();
            // if (mute_thread.joinable()) mute_thread.join();
        }

        STT* getSttPtr() { return stt; }

        void clearln() {
            cout << "\33[2K\r" << flush;
        }

        void print(const string& output, bool clear = false) {
            if (clear) clearln();
            cout << output << flush;
        }

        void println(const string& output, bool clear = false, bool refresh = false) {
            print(output + "\n", clear);

            if (refresh && commander) {
                CommandLine& cline = commander->get_command_line_ref();
                cline.getEditorRef().RefreshLine();
            }
        }

        void setVoiceInput(bool state) {
            lock_guard<mutex> lock(stt_voice_input_mutex);
            if (stt_voice_input == state) return;
            stt_voice_input = state;
            if (!stt) return;
            if (stt_voice_input) stt->start();
            else if (!stt_voice_input) {
                stt->stop();
                if (commander) commander->get_command_line_ref().set_prompt("");
            }
        }

        void tick() override { //if (stt_voice_input) return;
            T input;
            if (commander) {
                if (agency.isClosing()) {
                    sleep_ms(100);
                    return;
                }
                CommandLine& cline = commander->get_command_line_ref();
                input = cline.readln();
                if (cline.is_exited()) {
                    this->exit();
                    return;
                }
            } else cin >> input;
            if (trim(input).empty()) return;
            else if (str_starts_with(input, "/")) commander->run_command(&agency, input); // TODO: add is_command(input) as a command matcher (regex or callback fn) instead just test for "/" 
            else if (text_input_echo) println(input, true, false);
        }

        Commander* getCommanderPtr() { return commander; }
        
    private:

        // thread mute_thread;

        void stt_setup() {
            if (!stt) return;

            if (commander && interceptor) {
                interceptor->subsrcibe(this, [&](vector<char> sequence) {
                    if (sequence.empty()) return;
                    if (stt_voice_input && sequence[0] == 27) { // TODO: ESC key - to config
                        if (!stt) return;
                        NoiseMonitor* monitor = stt->getMonitorPtr();
                        if (!monitor) return;
                        bool mute = !monitor->is_muted();
                        monitor->set_muted(mute);
                        println(mute 
                            ? "🎤 " ANSI_FMT_C_RED "✖" ANSI_FMT_RESET " STT mute"
                            : "🎤 " ANSI_FMT_C_GREEN "✔" ANSI_FMT_RESET " STT unmute", true);
                        // cout << "STT mute: " << (currentMute ? "OFF" : "ON") << endl;
                    }
                });
                // TODO:
                // commander->get_command_line_ref().set_keypress_callback([](char *cbuf, int *c) {
                //    if (*c == 27) cout << "!!ESC!!" << endl;
                //    else cout << "[" << *c << "]" << endl;
                //     //cout << "HIT[" << *c << "][" + to_string(cbuf[0]) + "," + to_string(cbuf[1]) + "," + to_string(cbuf[2]) + "," + to_string(cbuf[3]) + "][" + to_string(cbuf) + "]" << endl;
                // });
                // mute_thread = thread([this](){ // TODO we can not make it work, cpp-linenose blocks the kbhit functions, we have to find a way to callback from linenoise
                //     while (kbhit()) cout << getchar() << endl;
                    
                //     // Check for keypress to toggle mute
                //     if (kbhit_chk(27)) { // TODO: ESC key - to config
                //         bool currentMute = stt->getMonitorPtr()->is_muted();
                //         stt->getMonitorPtr()->set_muted(!currentMute);
                //         cout << "STT mute: " << (currentMute ? "OFF" : "ON") << endl;
                //     }
                // });
            }

            stt->setSpeechHandler([this](vector<float>& /*record*/) {
                try {
                    recs++;
                    // DEBUG("record:" + to_string(record.size()));
                } catch (const exception& e) {
                    cerr << "Error in speech callback: " << e.what() << endl;

                    if (!this->stt) {
                        cerr << "Speach to text adapter is missing, switching to text mode.." << endl;
                        setVoiceInput(false);
                    }
                }
            });

            stt->setTranscribeHandler([this](const vector<float>& /*record*/, const string& text) {
                try {
                    recs--;
                    NULLCHK(this->stt);

                    string input = trim(text);
                    for (const string& rgx: speech_ignores_rgxs)
                        if (regx_match(rgx, input)) return;
                    if (input.empty()) return;
                    if (voice_input_echo) println(input, true, true);

                } catch (const exception& e) {
                    cerr << "Error in transcription callback: " << e.what() << endl;

                    if (!this->stt) {
                        cerr << "Speach to text adapter is missing, switching to text mode.." << endl;
                        setVoiceInput(false);
                    }
                }
            });

            stt->setRMSHandler([this](float vol_pc, float threshold_pc, float rmax, float rms, bool loud, bool muted) {
                try {

                    // DEBUG(
                    //     "vol_pc:" + to_string(vol_pc) + 
                    //     ", threshold_pc:" + to_string(threshold_pc) + 
                    //     ", rmax:" + to_string(rmax) + 
                    //     ", rms:" + to_string(rms) + 
                    //     ", loud:" + to_string(loud) + 
                    //     ", muted:" + to_string(muted)
                    // );
                    if (!commander || !stt) return;

                    string out = "";

                    
                    // REC status
                    out += (
                        muted 
                            ? ANSI_FMT(ANSI_FMT_C_BLACK, "MUTE") 
                            : loud 
                                ? ANSI_FMT(ANSI_FMT_C_RED, "●") + "REC" 
                                : "?MIC"
                    );


                    // RMS volume
                    //out += "V:" + set_precision(threshold_pc * 100, 2) + "/" + set_precision(rms * 100, 2) + " \t[";
                    out += "[" + string(muted ? ANSI_FMT_C_BLACK : ANSI_FMT_C_GREEN);
                    double step = 0.2;
                    double i = threshold_pc;
                    for (; i < 1; i += step) {
                        if (muted) out += "◦";
                        else if (vol_pc > i) out += "•";
                        else out += "◦"; //"·•◦";
                    }
                    out += string(ANSI_FMT_RESET) + "] VOL:" + set_precision(threshold_pc * 100, 2) + "/" + set_precision(vol_pc * 100, 2) + "%";
                    out += " RMS:" + set_precision(rmax, 2) + "/" + set_precision(rms, 2);


                    // progress roller
                    rollnxt++;
                    int at = rollnxt%4;
                    char c = roller[at];
                    string roll = "";
                    roll += c;
                    out += (stt->getTranscriberCRef().isInProgress() 
                        ? " " + roll + " " + to_string(recs)
                        : "");

                    commander->get_command_line_ref().set_prompt(out + " ");

                } catch (const exception& e) {
                    cerr << "Error in RMS callback: " << e.what() << endl;

                    if (!this->stt) {
                        cerr << "Speach to text adapter is missing, switching to text mode.." << endl;
                        setVoiceInput(false);
                    }
                }
            });
        }

        // ---- roller ----

        int rollnxt = 6000;
        const char* roller = "|/-\\";
        int recs = 0;

        // -----------------

        atomic<bool> stt_voice_input = false;

        Agency<T>& agency;
        Commander* commander = nullptr;
        STT* stt = nullptr;
        InputPipeInterceptor* interceptor = nullptr;

        mutex stt_voice_input_mutex;
    };
    
}