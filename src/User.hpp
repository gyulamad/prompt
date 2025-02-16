#pragma once

#include "../libs/yhirose/cpp-linenoise/linenoise.hpp"

#include "tools/llm/Model.hpp"

#include "tools/ERROR.hpp"
#include "tools/io.hpp"
#include "tools/files.hpp"
#include "tools/strings.hpp"
#include "tools/vectors.hpp"
#include "tools/Process.hpp"
#include "tools/JSON.hpp"
#include "tools/Commander.hpp"
#include "tools/Process.hpp"
#include "tools/Logger.hpp"
#include "tools/system.hpp"
#include "tools/Arguments.hpp"

#include "Speech.hpp"

using namespace tools;
using namespace tools::llm;

namespace prompt {

    typedef enum { MODE_CHAT, MODE_THINK, MODE_SOLVE } mode_t;

    mode_t get_mode(const string& name) {
        string name_lower = strtolower(name);
        if (name == "mode_chat" || name == "chat") return MODE_CHAT;
        if (name == "mode_think" || name == "think") return MODE_THINK;
        if (name == "mode_solve" || name == "solve") return MODE_SOLVE;
        throw ERROR("Invalid mode: " + name);
    }

    class User {
    private:
        Commander commander;

        atomic<bool> streaming = false;
        atomic<bool> requesting = false;
        atomic<bool> exiting = false;

        Model& model;
        string model_name;
        string user_lang;
        bool auto_save;
        mode_t mode;
        bool stream;
        string basedir;

        // ----------- speech -----------
        bool speech_stall;
        // long long speech_speak_wait_ms;
        vector<string> speech_ignores_rgxs;
        const long long speech_impatient_ms;
        int speech_tts_speed;
        int speech_tts_gap;
        string speech_tts_beep_cmd;
        string speech_tts_think_cmd;
        map<string, string> speech_tts_speak_replacements;
        bool speech_stt_voice_in;
        double speech_stt_voice_recorder_sample_rate;
        unsigned long speech_stt_voice_recorder_frames_per_buffer;
        size_t speech_stt_voice_recorder_buffer_seconds;
        float speech_stt_noise_monitor_threshold_pc;
        float speech_stt_noise_monitor_rmax_decay_pc;
        size_t speech_stt_noise_monitor_window;
        string speech_stt_transcriber_model;
        long speech_stt_poll_interval_ms;
        // ------------------------------
        Speech* speech = nullptr;
        // ------------------------------
        
        // string speech_interrupt_info_token;
        // string speech_amplitude_threshold_pc_setter_token;

    public:

        User(
            Model& model,
            const string& model_name,
            const string& user_lang,
            bool auto_save,
            mode_t mode,
            bool stream,
            const string& prompt, // = "> ", 
            const string& basedir, // = "./prompt/",,
            bool speech_stall,
            const vector<string>& speech_ignores_rgxs,
            const long long speech_impatient_ms,
            int speech_tts_speed,
            int speech_tts_gap,
            const string& speech_tts_beep_cmd,
            const string& speech_tts_think_cmd,
            const map<string, string>& speech_tts_speak_replacements,
            bool speech_stt_voice_in,
            double speech_stt_voice_recorder_sample_rate,
            unsigned long speech_stt_voice_recorder_frames_per_buffer,
            size_t speech_stt_voice_recorder_buffer_seconds,
            float speech_stt_noise_monitor_threshold_pc,
            float speech_stt_noise_monitor_rmax_decay_pc,
            size_t speech_stt_noise_monitor_window,
            const string& speech_stt_transcriber_model,
            long speech_stt_poll_interval_ms
            //Speech* speech//, // = nullptr,
            // const string& speech_interrupt_info_token, // = "TTS interrupted",
            // const string& speech_amplitude_threshold_pc_setter_token // = "SETRECAMP"
        ): 
            model(model),
            model_name(model_name),
            user_lang(user_lang),
            auto_save(auto_save), 
            mode(mode),
            stream(stream),
            commander(CommandLine(prompt)), 
            basedir(basedir),
            speech_stall(speech_stall),
            // speech_speak_wait_ms(speech_speak_wait_ms),
            speech_ignores_rgxs(speech_ignores_rgxs),
            speech_impatient_ms(speech_impatient_ms),
            speech_tts_speed(speech_tts_speed),
            speech_tts_gap(speech_tts_gap),
            speech_tts_beep_cmd(speech_tts_beep_cmd),
            speech_tts_think_cmd(speech_tts_think_cmd),
            speech_tts_speak_replacements(speech_tts_speak_replacements),
            speech_stt_voice_recorder_sample_rate(speech_stt_voice_recorder_sample_rate),
            speech_stt_voice_recorder_frames_per_buffer(speech_stt_voice_recorder_frames_per_buffer),
            speech_stt_voice_recorder_buffer_seconds(speech_stt_voice_recorder_buffer_seconds),
            speech_stt_noise_monitor_threshold_pc(speech_stt_noise_monitor_threshold_pc),
            speech_stt_noise_monitor_rmax_decay_pc(speech_stt_noise_monitor_rmax_decay_pc),
            speech_stt_noise_monitor_window(speech_stt_noise_monitor_window),
            speech_stt_transcriber_model(speech_stt_transcriber_model),
            speech_stt_poll_interval_ms(speech_stt_poll_interval_ms)
            // speech(speech)//,
            // speech_interrupt_info_token(speech_interrupt_info_token),
            // speech_amplitude_threshold_pc_setter_token(speech_amplitude_threshold_pc_setter_token)
        {
            if (!model_name.empty()) load_model(true);
        }

        virtual ~User() {
            speech_delete();
        }

        string get_model_file() {
            if (model_name.empty()) throw ERROR("Model name is not set.");
            return basedir + "/models/" + model_name + ".json";
        }

        Model& get_model_ref() {
            return model;
        }

        void set_model_name(const string& model_name) {
            this->model_name = model_name;
        }

        const string& get_model_name_ref() const {
            return model_name;
        }

        bool is_auto_save() const {
            return auto_save;
        }

        // --- commander stuff ----
        
        void set_commands(const vector<void*>& commands) {
            commander.set_commands(commands);
        }

        vector<void*> get_commands_ref() {
            return commander.get_commands_ref();
            // return commands;
        }


        const CompletionMatcher& get_cmatcher_ref() const {
            return commander.get_cmatcher_ref();
            // return cmatcher;
        }

        void exit() {
            commander.exit();   
            exiting = true;
        }

        // --------------------

        void set_mode(mode_t mode) {
            this->mode = mode;
        }

        mode_t get_mode() const {
            return mode;
        }

        void save_model(bool override /*= false*/, bool show_save_succeed = true) {
            string model_file = get_model_file();
            if (!override && file_exists(model_file)) 
                throw ERROR("Model already exists, can not override.");
            string errmsg = model.save(model, model_file);
            if (!errmsg.empty()) cout << "Model save error: " << errmsg << endl;
            else if (show_save_succeed) cout << "Model saved: " << model_name << endl;
        }

        void load_model(bool create /*= true*/) {
            string model_file = get_model_file();
            if (create && !file_exists(model_file)) {
                save_model(false);
                return;
            }
            string errmsg = model.load(model, model_file);
            if (!errmsg.empty()) cout << "Model load error: " << errmsg << endl;
            else {
                model.dump_conversation(commander.get_command_line_ref().get_prompt());
                cout << "Model loaded: " << model_name << endl;
            }
        }

        void show_voice_input(const string& input) {
            commander.get_command_line_ref().show(input + (input.back() == '\n' ? "" : "\n"));
        }

        bool speech_create() {
            if (speech) return true;
            speech = new Speech(
                commander,
                user_lang,
                speech_ignores_rgxs,
                speech_impatient_ms,
                speech_tts_speed,
                speech_tts_gap,
                speech_tts_beep_cmd,
                speech_tts_think_cmd,
                speech_tts_speak_replacements,
                speech_stt_voice_recorder_sample_rate,
                speech_stt_voice_recorder_frames_per_buffer,
                speech_stt_voice_recorder_buffer_seconds,
                speech_stt_noise_monitor_threshold_pc,
                speech_stt_noise_monitor_rmax_decay_pc,
                speech_stt_noise_monitor_window,
                speech_stt_transcriber_model,
                speech_stt_poll_interval_ms            
            );

            if (speech_stall) {
                string hesitros_textfile = basedir + "/hesitors." + user_lang + ".txt";
                if (file_exists(hesitros_textfile)) {
                    speech->set_hesitors(explode("\n", file_get_contents(hesitros_textfile)));
                } else {
                    Model* thinker = (Model*)model.spawn(
                        "You are a linguistic assistant"//,
                        // model.get_conversation_length_max(),
                        // model.get_conversation_loss_ratio(),
                        // model.get_think_steps(),
                        // model.get_think_deep()
                    );
                    speech->set_hesitors(thinker->multiple_str(
                        "I need a list of 'Filler/Stall word' and 'Hesitation markers/sentences'. "
                        "Write one word long to a full sentence and anything in between. "
                        "We need 7 one-word long, 5 mid sentence and 3 very long full sentence. "
                        "We need the list in language: " + user_lang
                    ));
                    model.kill(thinker);
                    for (string& hesitor: speech->get_hesitors_ref()) hesitor = str_replace("\n", " ", hesitor);
                    file_put_contents(hesitros_textfile, implode("\n", speech->get_hesitors_ref()), true, true);
                }
                
                // TODO: do we still need this?
                string repeaters_textfile = basedir + "/repeaters." + user_lang + ".txt";
                if (file_exists(repeaters_textfile)) {
                    speech->set_repeaters(explode("\n", file_get_contents(repeaters_textfile)));
                } else {
                    Model* thinker = (Model*)model.spawn("You are a linguistic assistant");
                    speech->set_repeaters(thinker->multiple_str(
                        "I need a list of 'repeat asking word' and 'small questions'. "
                        "We need 10 few word long, for eg. 'what?', 'sorry can you repeat?', 'what did you say?', 'Ha?', 'tell again?' "
                        "We need the list in language: " + user_lang
                    ));
                    model.kill(thinker);
                    for (string& repeater: speech->get_repeaters_ref()) repeater = str_replace("\n", " ", repeater);
                    file_put_contents(repeaters_textfile, implode("\n", speech->get_repeaters_ref()), true, true);
                }
            }
            return true;
        }

        bool speech_delete() {
            if (!speech) return false;            
            delete speech;
            speech = nullptr;
            return true;
        }


        bool speech_toggle() {
            if (!speech) {
                speech_create();
                return true;
            }
            speech_delete();
            return false;
        }

        static bool stream_callback(void* user_void, const string& inference) {
            if (!user_void) return false;
            User* user = (User*)user_void;
            string inference_to_user = user->model.inference_remove_plugins(inference);
            if (user->speech) user->speech->hide_mic();

            // cout << inference_to_user << endl;

            // Check if the string ends with a newline character
            if (inference_to_user.empty()) return true;
            cout << inference_to_user << (inference_to_user.back() == '\n' ? "" : "\n") << flush;

            if (!user->speech) return false;
            return !user->speech->speak(inference_to_user);
        }

        static void stream_done_callback(void* user_void, const string& fulltext) {
            if (!user_void) return;
            User* user = (User*)user_void;
            if (user->speech) user->speech->beep();
        }

        void start() {
            string input = "";
            string response = "";

            // cout << "[DEBUG] User keyboard input handler thread start..." << endl;
            thread keyboard_input_thread([&]{
                while (!exiting && !commander.is_exiting()) {
                    sleep(1);
                    if (speech && kbhit()) {
                        while (kbhit()) getchar();
                        speech_delete();
                        continue;
                    }
                    if (!speech) {
                        if (stream && streaming) continue;
                        if (requesting) continue;
                        if (exiting) break;
                        input = commander.get_command_line_ref().readln();
                        if (input.empty()) speech_create();
                    }
                }
            });

            while (!exiting && !commander.is_exiting()) {

                if (!stream) {
                    string resp = trim(response);
                    if (!resp.empty()) {
                        cout << "\r" << resp << endl;                  
                        if (speech) speech->speak(resp, true, true);
                    }
                }

                // waiting for the next input:
                input = "";
                while (!exiting && input.empty()) {                    
                    usleep(30000);                    
                    if (speech) input += speech->get_text_input();
                }
                if (speech) speech->rand_speak_hesitate();

                if (model.inference_func_calls.empty() && input.empty()) continue;
                if (!input.empty() && input[0] == '/') {
                    commander.run_command(this, input);
                    continue; 
                }
                
                response = "";
                switch (mode)
                {
                    case MODE_CHAT:
                        if (stream) {
                            streaming = true;
                            model.inference_plugins_reset();
                            response = model.prompt_stream(input, this, stream_callback, stream_done_callback);
                            streaming = false;
                            break;
                        }
                        requesting = true;
                        response = model.prompt(input);
                        requesting = false;
                        break;

                    case MODE_THINK:
                        response = model.think(input);
                        break;

                    case MODE_SOLVE:
                        response = model.solve(input);
                        break;
                
                    default:
                        throw ERROR("Invalid mode");
                }

                if (auto_save && !model_name.empty()) save_model(true, false);
            }
            
            // cout << "[DEBUG] Waiting for keyboard input thread to finish..." << endl;
            keyboard_input_thread.join();
        }

    };

}