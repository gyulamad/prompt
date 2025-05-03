#pragma once

#include "../../abstracts/UserInterface.hpp"
#include "../../regx/regx_match.hpp"
#include "../../cmd/Commander.hpp"
#include "../../voice/MicView.hpp"
#include "../../voice/TTS.hpp"
#include "../../voice/STTSwitch.hpp"
#include "../../utils/InputPipeInterceptor.hpp"

using namespace tools::abstracts;
using namespace tools::regx;
using namespace tools::cmd;
using namespace tools::voice;
using namespace tools::utils;

namespace tools::agency::agents {

    template<typename T>
    class UserAgent;

    template<typename T>
    class UserAgentInterface: public UserInterface<T> { // TODO: public UserInterface (abstract)
    public:
        // TODO: make configurable:
        atomic<bool> voice_input_echo = false;

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

        TTS& getTTSRef() { return tts; }

        // =================================================================
        // =================================================================
        // =================================================================


    //     virtual void set_prompt_visible(bool prompt_visible) { 
    //         if (this->prompt_visible == prompt_visible) return;
    //         this->prompt_visible = prompt_visible;
    //     }
    //     virtual void hide_prompt() { set_prompt_visible(false); }
    //     virtual void show_prompt() { set_prompt_visible(true); }
    //     virtual bool is_prompt_visible() const { return prompt_visible; }
    //     virtual bool is_prompt_hidden() const { return !prompt_visible; }
        
    // // protected: // TODO
    //     atomic<bool> prompt_visible = true;

        // =================================================================
        // ============================== TTS ==============================
        // =================================================================

    //     void setVoiceOutput(bool state) { tts_voice_output = state; }

    //     bool isVoiceOutput() const { return tts_voice_output; }

    //     //[[nodiscard]]
    //     bool speak(const string& text) {
    //         return tts.speak(text);
    //     }

    // private:
    //     bool tts_voice_output = false;

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
            lock_guard<mutex> lock(this->input_mutex);
            CommandLine& cline = commander.getCommandLineRef();
            input = cline.readln();
            return cline.isExited();
        }


        void clearln() override {
            CommandLine& cline = commander.getCommandLineRef();
            cline.clearln();
        }

        // =================================================================
        // =================================================================
        // ================================================================= 

        void refresh() {
            CommandLine& cline = commander.getCommandLineRef();
            cline.getEditorRef().refreshLine();
        }

        bool toggleMute() {
            STT* stt = sttSwitch.getSttPtr();
            if (!stt) return true;
            NoiseMonitor* monitor = stt->getMonitorPtr();
            if (!monitor) return true;
            bool mute = !monitor->isMuted();
            monitor->setMuted(mute);
            clearln();
            this->println(mute 
                ? "🎤 " ANSI_FMT_C_RED "✖" ANSI_FMT_RESET " STT muted"
                : "🎤 " ANSI_FMT_C_GREEN "✔" ANSI_FMT_RESET " STT unmuted"
            );
            return monitor->isMuted();
        }

        bool isMuted() {
            STT* stt = sttSwitch.getSttPtr();
            if (!stt) return true;
            NoiseMonitor* monitor = stt->getMonitorPtr();
            if (!monitor) return true;
            return monitor->isMuted();
        }

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
                    toggleMute();
                    return;
                }
            });

            stt->setSpeechHandler([this](vector<float>& /*record*/) {
                // try {
                    
                
                    // recs++;
                    micView.incRecs();
                    // DEBUG("inc recs to:" + to_string(micView.getRecs()));
                    // if (micView.getRecs() >= 1) tts.speak_pause(/*3000*/);
                    // if (micView.getRecs() >= 2) tts.speak_stop();
                    


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
                // if (tts.is_speaking() || this->is_prompt_hidden()) return;
                STT* stt = sttSwitch.getSttPtr();
                try {
                    if (!stt) return;
                    bool in_progress = stt->getTranscriberCRef().isInProgress();
                    string out = micView.getView(muted, loud, threshold_pc, vol_pc, rmax, rms, in_progress);
                    commander.getCommandLineRef().setPrompt(out + " ");

                    // interruption check starts...
                    // TODO: we need a vol_pc_to_resume to config that is may higher than vol_pc
                    if (micView.getRecs() >= 1 || (vol_pc > threshold_pc && tts.is_speaking())) {
                        // tts.speak_pause(/*3000*/);
                        tts.speak_pause(3000); // TODO: to config
                    }

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
