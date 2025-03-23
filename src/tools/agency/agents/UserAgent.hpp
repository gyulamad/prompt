#pragma once

#include "../../str/trim.hpp"
#include "../../str/set_precision.hpp"
#include "../../regx/regx_match.hpp"
#include "../../cmd/Commander.hpp"
#include "../../voice/STT.hpp"
#include "../../voice/STTSwitch.hpp"
#include "../../voice/WhisperTranscriberSTTSwitch.hpp"
#include "../../utils/InputPipeInterceptor.hpp"
#include "../Agent.hpp"

using namespace tools::str;
using namespace tools::regx;
using namespace tools::cmd;
using namespace tools::voice;
using namespace tools::utils;
using namespace tools::agency;

namespace tools::agency::agents {

    class MicView {
    public:
        MicView() {}
        virtual ~MicView() {}
        void incRecs() { recs++; }
        void decRecs() { recs--; }
        string getView(bool muted, bool loud, float threshold_pc, float vol_pc, float rmax, float rms, bool in_progress) {

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
            out += (in_progress
                ? " " + roll + " " + to_string(recs)
                : "");

            return out;
        }
    private:

        // ---- roller ----

        int rollnxt = 6000;
        const char* roller = "|/-\\";
        int recs = 0;

        // -----------------
    };

    template<typename T>
    class UserAgent;

    template<typename T>
    class UserAgentWhisperTranscriberSTTSwitch: public WhisperTranscriberSTTSwitch<UserAgent<T>> {
    public:
        using WhisperTranscriberSTTSwitch<UserAgent<T>>::WhisperTranscriberSTTSwitch;
    };

    template<typename T>
    class UserAgentWhisperCommanderInterface {
    public:
        // TODO: make configurable:
        atomic<bool> voice_input_echo = true;

        // TODO: make configurable:
        const vector<string> speech_ignores_rgxs = { // TODO: in config
            // "^\\[.*\\]$",
            // "^\\*.*\\*$",
            // "^\\(.*\\)$",
            "^\\-.*[\\.\\!\\-]$" // (for hungarian??)
        };

        UserAgentWhisperCommanderInterface(
            UserAgentWhisperTranscriberSTTSwitch<T>& stt_switch,
            MicView& micView,
            Commander* commander = nullptr,
            InputPipeInterceptor* interceptor = nullptr
        ):
            stt_switch(stt_switch),
            micView(micView),
            commander(commander),
            interceptor(interceptor)
        {}

        virtual ~UserAgentWhisperCommanderInterface() {
            if (interceptor) {
                interceptor->unsubscribe(this);
                interceptor->close();
            }
            if (stt_switch.get_stt_ptr()) 
                stt_switch.get_stt_ptr()->stop();
        }

        Commander* getCommanderPtr() { return commander; }

        UserAgentWhisperTranscriberSTTSwitch<T>& get_stt_switch_ref() { return stt_switch; }


        void setVoiceInput(bool state) {
            lock_guard<mutex> lock(stt_voice_input_mutex);
            if (stt_voice_input == state) return;
            stt_voice_input = state;
            if (stt_voice_input) {
                stt_switch.on();
                if (!stt_initialized) stt_setup();
                stt_initialized = true;
            } else {
                stt_switch.off();
                stt_initialized = false;

                // TODO: I am not sure this one is needed anymore:
                if (commander) {
                    commander->get_command_line_ref().set_prompt("");
                    commander->get_command_line_ref().getEditorRef().WipeLine();
                }
            }
        }

        bool readln(T& input) {
            if (commander) {
                CommandLine& cline = commander->get_command_line_ref();
                input = cline.readln();
                if (cline.is_exited()) return true;
            } else cin >> input;
            return false;
        }


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

    private:

        void stt_setup() {
            STT* stt = stt_switch.get_stt_ptr();
            if (!stt) return;

            if (commander && interceptor) {
                interceptor->subsrcibe(this, [&](vector<char> sequence) {
                    if (sequence.empty()) return;
                    if (stt_voice_input && sequence.size() == 1 &&  sequence[0] == 13) { // Enter
                        this->commander->get_command_line_ref().getEditorRef().WipeLine();                         
                        return;
                    }
                    if (stt_voice_input && sequence.size() == 1 &&  sequence[0] == 27) { // TODO: ESC key - to config
                        STT* stt = stt_switch.get_stt_ptr();
                        if (!stt) return;
                        NoiseMonitor* monitor = stt->getMonitorPtr();
                        if (!monitor) return;
                        bool mute = !monitor->is_muted();
                        monitor->set_muted(mute);
                        println(mute 
                            ? "🎤 " ANSI_FMT_C_RED "✖" ANSI_FMT_RESET " STT muted"
                            : "🎤 " ANSI_FMT_C_GREEN "✔" ANSI_FMT_RESET " STT unmuted", true);
                        return;
                    }
                });
            }

            stt->setSpeechHandler([this](vector<float>& /*record*/) {
                // try {
                    
                
                    // recs++;
                    micView.incRecs();


                    // DEBUG("record:" + to_string(record.size()));
                // } catch (const exception& e) {
                //     cerr << "Error in speech callback: " << e.what() << endl;

                //     if (!this->stt) {
                //         cerr << "Speech to text adapter is missing, switching to text mode.." << endl;
                //         setVoiceInput(false);
                //     }
                // }
            });

            stt->setTranscribeHandler([this](const vector<float>& /*record*/, const string& text) {
                STT* stt = stt_switch.get_stt_ptr();
                try {
                    
                    // recs--;
                    micView.decRecs();

                    if (!stt) {
                        cerr << "Speech to text adapter is missing, falling back to text mode.." << endl;
                        return;
                    }

                    string input = trim(text);
                    for (const string& rgx: speech_ignores_rgxs)
                        if (regx_match(rgx, input)) return;
                    if (input.empty()) return;
                    if (voice_input_echo) println(input, true, true);

                } catch (const exception& e) {
                    cerr << "Error in transcription callback: " << e.what() << endl;
                }
            });

            stt->setRMSHandler([this](float vol_pc, float threshold_pc, float rmax, float rms, bool loud, bool muted) {
                STT* stt = stt_switch.get_stt_ptr();
                try {
                    if (!commander || !stt) return;
                    bool in_progress = stt->getTranscriberCRef().isInProgress();
                    string out = micView.getView(muted, loud, threshold_pc, vol_pc, rmax, rms, in_progress);
                    commander->get_command_line_ref().set_prompt(out + " ");
                } catch (const exception& e) {
                    cerr << "Error in RMS callback: " << e.what() << endl;
                }
            });
        }

        Commander* commander = nullptr;
        UserAgentWhisperTranscriberSTTSwitch<T>& stt_switch;
        MicView& micView;
        InputPipeInterceptor* interceptor = nullptr;

        mutex stt_voice_input_mutex;
        atomic<bool> stt_voice_input = false;
        atomic<bool> stt_initialized = false;
    };
    
    template<typename T>
    class UserAgent: public Agent<T> {
    public:

        // TODO: make configurable:
        atomic<bool> text_input_echo = true;

        UserAgent(
            PackQueue<T>& queue, 
            Agency<T>& agency, 
            UserAgentWhisperCommanderInterface<T>& interface
        ): 
            Agent<T>(queue, "user"), 
            agency(agency),
            interface(interface) 
        {}

        virtual ~UserAgent() {}

        UserAgentWhisperCommanderInterface<T>& getInterfaceRef() { return interface; }

        void tick() override {
            if (agency.isClosing()) {
                sleep_ms(100);
                return;
            }
            T input;
            if (interface.readln(input)) this->exit();
            if (trim(input).empty()) return;
            else if (str_starts_with(input, "/")) interface.getCommanderPtr()->run_command(&agency, input); // TODO: add is_command(input) as a command matcher (regex or callback fn) instead just test for "/" 
            else if (text_input_echo) interface.println(input, true, false);
        }

    private:
        Agency<T>& agency;
        UserAgentWhisperCommanderInterface<T>& interface;
    };
    
}