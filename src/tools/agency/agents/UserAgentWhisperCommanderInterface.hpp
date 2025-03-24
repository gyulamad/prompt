#pragma once

#include "../../regx/regx_match.hpp"
#include "../../cmd/Commander.hpp"
#include "../../voice/MicView.hpp"

#include "UserAgentWhisperTranscriberSTTSwitch.hpp"

using namespace tools::regx;
using namespace tools::cmd;
using namespace tools::voice;

namespace tools::agency::agents {

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
            Commander& commander,
            InputPipeInterceptor& interceptor
        ):
            stt_switch(stt_switch),
            micView(micView),
            commander(commander),
            interceptor(interceptor)
        {}

        virtual ~UserAgentWhisperCommanderInterface() {
            interceptor.unsubscribe(this);
            interceptor.close();
                
            if (stt_switch.get_stt_ptr()) 
                stt_switch.get_stt_ptr()->stop();
        }

        Commander& getCommanderRef() { return commander; }

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
                // if (commander) {
                //     commander->getCommandLineRef().set_prompt("");
                //     commander->getCommandLineRef().getEditorRef().WipeLine();
                // }
            }
        }

        bool readln(T& input) {
            CommandLine& cline = commander.getCommandLineRef();
            input = cline.readln();
            return cline.is_exited();
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

            if (refresh) {
                CommandLine& cline = commander.getCommandLineRef();
                cline.getEditorRef().RefreshLine();
            }
        }

    private:

        void stt_setup() {
            STT* stt = stt_switch.get_stt_ptr();
            if (!stt) return;

            interceptor.subsrcibe(this, [&](vector<char> sequence) {
                if (sequence.empty()) return;
                if (stt_voice_input && sequence.size() == 1 &&  sequence[0] == 13) { // Enter
                    commander.getCommandLineRef().getEditorRef().WipeLine();                         
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
                    if (!stt) return;
                    bool in_progress = stt->getTranscriberCRef().isInProgress();
                    string out = micView.getView(muted, loud, threshold_pc, vol_pc, rmax, rms, in_progress);
                    commander.getCommandLineRef().set_prompt(out + " ");
                } catch (const exception& e) {
                    cerr << "Error in RMS callback: " << e.what() << endl;
                }
            });
        }

        UserAgentWhisperTranscriberSTTSwitch<T>& stt_switch;
        MicView& micView;
        Commander& commander;
        InputPipeInterceptor& interceptor;

        mutex stt_voice_input_mutex;
        atomic<bool> stt_voice_input = false;
        atomic<bool> stt_initialized = false;
    };

}