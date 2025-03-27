#pragma once

#include "../../abstracts/Switch.hpp"
#include "../../abstracts/UserInterface.hpp"
#include "../../regx/regx_match.hpp"
#include "../../cmd/Commander.hpp"
#include "../../voice/MicView.hpp"
#include "../../voice/TTS.hpp"
#include "../../voice/STTSwitch.hpp"

using namespace tools::abstracts;
using namespace tools::regx;
using namespace tools::cmd;
using namespace tools::voice;

namespace tools::agency::agents {

    template<typename T>
    class UserAgent;

    template<typename T>
    class UserAgentInterface: public UserInterface<T> { // TODO: public UserInterface (abstract)
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

        UserAgentInterface(
            TTS& tts,
            STTSwitch& sttSwitch,
            MicView& micView,
            Commander& commander,
            InputPipeInterceptor& interceptor
        ):
            tts(tts),
            sttSwitch(sttSwitch),
            micView(micView),
            commander(commander),
            interceptor(interceptor)
        {}

        virtual ~UserAgentInterface() {
            interceptor.unsubscribe(this);
            interceptor.close();
                
            if (sttSwitch.getSttPtr()) 
                sttSwitch.getSttPtr()->stop();
        }



        // =================================================================
        // ============================== TTS ==============================
        // =================================================================
        
        void setVoiceOutput(bool state) { tts_voice_output = state; }

        bool isVoiceOutput() const { return tts_voice_output; }

        void speak(const string& text) {
            if (tts_voice_output) tts.speak(text);
        }

    private:
        bool tts_voice_output = false;

        // =================================================================
        // =================================================================
        // =================================================================        
    public:



        void setUser(UserAgent<T>* user) { this->user = user; }

        Commander& getCommanderRef() { return commander; }

        STTSwitch& getSttSwitchRef() { return sttSwitch; }

        void setVoiceInput(bool state) {
            lock_guard<mutex> lock(stt_voice_input_mutex);
            if (stt_voice_input == state) return;
            stt_voice_input = state;
            if (stt_voice_input) {
                sttSwitch.on();
                if (!stt_initialized) sttSetup();
                stt_initialized = true;
            } else {
                sttSwitch.off();
                stt_initialized = false;

                // remove the last mic-view from the input prompt
                commander.getCommandLineRef().setPrompt("");
                commander.getCommandLineRef().getEditorRef().wipeLine();
            }
        }


        // =================================================================
        // ==================== UserInterface overrides ====================
        // ================================================================= 

        bool readln(T& input) override {
            CommandLine& cline = commander.getCommandLineRef();
            input = cline.readln();
            return cline.isExited();
        }


        void clearln() override {
            CommandLine& cline = commander.getCommandLineRef();
            cline.clearln();
        }

        // void print(const string& output, bool clear = false) {
        //     if (clear) clearln();
        //     cout << output << flush;
        // }

        // void println(const string& output, bool clear = false, bool refresh = false) {
        //     print(output + "\n", clear);

        //     if (refresh) {
        //         CommandLine& cline = commander.getCommandLineRef();
        //         cline.getEditorRef().refreshLine();
        //     }
        // }

        void refresh() {
            CommandLine& cline = commander.getCommandLineRef();
            cline.getEditorRef().refreshLine();
        }

        // =================================================================
        // =================================================================
        // ================================================================= 


    private:

        void sttSetup() {
            STT* stt = sttSwitch.getSttPtr();
            if (!stt) return;

            interceptor.subsrcibe(this, [&](vector<char> sequence) {
                if (sequence.empty()) return;
                if (stt_voice_input && sequence.size() == 1 &&  sequence[0] == 13) { // Enter
                    commander.getCommandLineRef().getEditorRef().wipeLine();                         
                    return;
                }
                if (stt_voice_input && sequence.size() == 1 &&  sequence[0] == 27) { // TODO: ESC key - to config
                    STT* stt = sttSwitch.getSttPtr();
                    if (!stt) return;
                    NoiseMonitor* monitor = stt->getMonitorPtr();
                    if (!monitor) return;
                    bool mute = !monitor->isMuted();
                    monitor->setMuted(mute);
                    clearln();
                    this->println(mute 
                        ? "🎤 " ANSI_FMT_C_RED "✖" ANSI_FMT_RESET " STT muted"
                        : "🎤 " ANSI_FMT_C_GREEN "✔" ANSI_FMT_RESET " STT unmuted"
                    );
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
                STT* stt = sttSwitch.getSttPtr();
                try {
                    
                    micView.decRecs();

                    if (!stt) {
                        cerr << "Speech to text adapter is missing, falling back to text mode.." << endl;
                        return;
                    }

                    string input = trim(text);
                    for (const string& rgx: speech_ignores_rgxs)
                        if (regx_match(rgx, input)) return;
                    if (input.empty()) return;
                    if (voice_input_echo) {
                        clearln();
                        this->println(input);
                        refresh();
                    }
                    NULLCHK(user);
                    user->onInput(input);
                } catch (const exception& e) {
                    cerr << "Error in transcription callback: " << e.what() << endl;
                }
            });

            stt->setRMSHandler([this](float vol_pc, float threshold_pc, float rmax, float rms, bool loud, bool muted) {
                STT* stt = sttSwitch.getSttPtr();
                try {
                    if (!stt) return;
                    bool in_progress = stt->getTranscriberCRef().isInProgress();
                    string out = micView.getView(muted, loud, threshold_pc, vol_pc, rmax, rms, in_progress);
                    commander.getCommandLineRef().setPrompt(out + " ");
                } catch (const exception& e) {
                    cerr << "Error in RMS callback: " << e.what() << endl;
                }
            });
        }

        UserAgent<T>* user = nullptr;
        TTS& tts;
        STTSwitch& sttSwitch;
        MicView& micView;
        Commander& commander;
        InputPipeInterceptor& interceptor;

        mutex stt_voice_input_mutex;
        atomic<bool> stt_voice_input = false;
        atomic<bool> stt_initialized = false;
    };

}
