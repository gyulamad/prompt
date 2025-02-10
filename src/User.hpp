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

    class User {
    private:
        Commander commander;

        mode_t mode = MODE_CHAT;
        Model& model;
        string model_name;
        string user_lang;
        bool auto_save;
        string basedir;

        // ----------- speech -----------
        bool speech_stall;
        // long long speech_speak_wait_ms;
        vector<string> speech_ignores_rgxs;
        int speech_tts_speed;
        int speech_tts_gap;
        string speech_tts_beep_cmd;
        string speech_tts_think_cmd;
        bool speech_stt_voice_in;
        double speech_stt_voice_recorder_sample_rate;
        unsigned long speech_stt_voice_recorder_frames_per_buffer;
        size_t speech_stt_voice_recorder_buffer_seconds;
        float speech_stt_noise_monitor_threshold_pc;
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
            const string& prompt, // = "> ", 
            const string& basedir, // = "./prompt/",,
            bool speech_stall,
            // long long speech_speak_wait_ms,
            const vector<string>& speech_ignores_rgxs,
            int speech_tts_speed,
            int speech_tts_gap,
            const string& speech_tts_beep_cmd,
            const string& speech_tts_think_cmd,
            bool speech_stt_voice_in,
            double speech_stt_voice_recorder_sample_rate,
            unsigned long speech_stt_voice_recorder_frames_per_buffer,
            size_t speech_stt_voice_recorder_buffer_seconds,
            float speech_stt_noise_monitor_threshold_pc,
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
            commander(CommandLine(prompt)), 
            basedir(basedir),
            speech_stall(speech_stall),
            // speech_speak_wait_ms(speech_speak_wait_ms),
            speech_ignores_rgxs(speech_ignores_rgxs),
            speech_tts_speed(speech_tts_speed),
            speech_tts_gap(speech_tts_gap),
            speech_tts_beep_cmd(speech_tts_beep_cmd),
            speech_tts_think_cmd(speech_tts_think_cmd),
            speech_stt_voice_recorder_sample_rate(speech_stt_voice_recorder_sample_rate),
            speech_stt_voice_recorder_frames_per_buffer(speech_stt_voice_recorder_frames_per_buffer),
            speech_stt_voice_recorder_buffer_seconds(speech_stt_voice_recorder_buffer_seconds),
            speech_stt_noise_monitor_threshold_pc(speech_stt_noise_monitor_threshold_pc),
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
            // exiting = true;
        }

        // --------------------

        void set_mode(mode_t mode) {
            this->mode = mode;
        }

        mode_t get_mode() const {
            return mode;
        }
 
        // Speech* get_speech_ptr() {
        //     return speech;
        // }

        // void set_voice_in(bool voice_in) {
        //     this->voice_in = voice_in;
        // }

        // bool is_voice_in() const {
        //     return voice_in;
        // }

        // void set_voice_out(bool voice_in) {
        //     this->voice_out = voice_out;
        // }

        // bool is_voice_out() const {
        //     return voice_out;   
        // }

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
        
        // bool add_puffered_voice_input_to_context() {
        //     if (!speech) return false;
        //     string inp = trim(speech->fetch_rec_stt_result());
        //     if (!inp.empty()) {
        //         model.addContext(inp, ROLE_INPUT);
        //         show_voice_input(inp);
        //         return true;
        //     }
        //     return false;
        // }

        bool speech_create() {
            if (speech) return false;
            speech = new Speech(
                commander,
                user_lang,
                // speech_speak_wait_ms,
                speech_ignores_rgxs,
                speech_tts_speed,
                speech_tts_gap,
                speech_tts_beep_cmd,
                speech_tts_think_cmd,
                speech_stt_voice_recorder_sample_rate,
                speech_stt_voice_recorder_frames_per_buffer,
                speech_stt_voice_recorder_buffer_seconds,
                speech_stt_noise_monitor_threshold_pc,
                speech_stt_noise_monitor_window,
                speech_stt_transcriber_model,
                speech_stt_poll_interval_ms            
            );

            if (speech_stall) {
                string hesitros_textfile = basedir + "/hesitors." + user_lang + ".txt";
                if (file_exists(hesitros_textfile)) {
                    speech->set_hesitors(explode("\n", file_get_contents(hesitros_textfile)));
                } else {
                    Model* thinker = (Model*)model.spawn("You are a linguistic assistant");
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

        string prompt(/*const string& response = ""*/) {
            // string resp = trim(response);
            // if (!resp.empty()) cout << resp << endl;
            // if (speech) {
            //     if (speech->is_voice_out() && !resp.empty()) speech->say_beep(resp, true);
            //     if (speech->is_voice_in()) {
            //         string input = speech->rec();
            //         if (speech->is_rec_interrupted()) speech->set_voice_in(false);
            //         //speech->cleanprocs();
            //         if (!input.empty()) show_voice_input(input);
            //         return input;
            //     }
            // }

            return commander.get_command_line_ref().readln();
        }

        void start() {
            string input = "";
            string response = "";

            cout << "DEBUG: User keyboard input handler thread start..." << endl;
            thread keyboard_input_thread([&]{
                while (!commander.is_exiting()) {
                    input = commander.get_command_line_ref().readln();
                    //if (speech) speech_delete();
                }
            });

            while (!commander.is_exiting()) {

                string resp = trim(response);
                if (!resp.empty()) {
                    cout << "\r" << resp << endl;
                    commander.get_command_line_ref().show();                    
                    if (speech) speech->speak_beep(resp);
                }

                // waiting for the next input:
                input = "";
                while (input.empty()) {                    
                    usleep(30000);                    
                    if (speech) input += speech->get_text_input();
                }
                if (speech) speech->rand_speak_hesitate();

                // input = trim(prompt(/*response*/));
                response = "";
                if (input.empty()) continue;
                if (input[0] == '/') {
                    commander.run_command(this, input);
                    continue; 
                }

                // if (speech) {
                    
                //     //model.system_data["{{speech_current_noise_threshold_pc}}"] = to_string(speech->noise_threshold_pc);

                //     if (speech->is_say_interrupted()) {
                //         cout << "AI TTS was interrupted" << endl;
                //         // input = speech_interrupt_info_token + "\n" + input;  
                //         // TODO:
                //         vector<Message> messages;
                //         while (true) {                           
                //             if (!model.hasContext()) break;
                //             Message message = model.popContext();
                //             messages.push_back(message);
                //             if (message.get_role() != ROLE_OUTPUT) continue;
                //             message.set_text(message.get_text() + "..." + speech_interrupt_info_token);
                //             //model.addContext(message);
                //             //messages = array_reverse(messages);
                //             for (const Message& message: messages)
                //                 model.addContext(message.get_text(), message.get_role());
                //         }
                //         // model.addContext(speech_interrupt_info_token, ROLE_INPUT); 

                //         if (!add_puffered_voice_input_to_context()) speech->stall(); 
                //     }   
                // }
                switch (mode)
                {
                    case MODE_CHAT:
                        response = model.prompt(input);
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
                
                // TODO:
                // vector<string> matches;
                // if (regx_match("\\[" + speech_amplitude_threshold_pc_setter_token + ":([\\d\\.]+)\\]", response, &matches)) {
                //     response = str_replace(matches[0], "", response);

                    // if (speech) {
                    //     bool error_found = false;
                    //     if (!is_numeric(matches[1])) {
                    //         model.addContext(
                    //             "The amplitude threshold_pc should be a numeric value.", 
                    //             ROLE_INPUT
                    //         );
                    //         error_found = true;
                    //     }
                    //     double noise_threshold_pc = parse<double>(matches[1]);
                    //     if (noise_threshold_pc < speech->get_noise_threshold_pc_min() || noise_threshold_pc > speech->get_noise_threshold_pc_max()) {
                    //         model.addContext(
                    //             "The amplitude threshold_pc should be in between " 
                    //                 + to_string(speech->get_noise_threshold_pc_min()) + " and " 
                    //                 + to_string(speech->get_noise_threshold_pc_max()), 
                    //             ROLE_INPUT
                    //         );
                    //         error_found = true;
                    //     }
                    //     if (error_found) {
                    //         model.addContext(
                    //             "The amplitude threshold_pc value remains unchanged: " 
                    //                 + to_string(speech->get_noise_threshold_pc()), 
                    //             ROLE_INPUT
                    //         );
                    //         continue;
                    //     }
                    //     speech->set_noise_threshold_pc(noise_threshold_pc);
                    //     string msg = "The noise amplitude threshold_pc value is set to " 
                    //         + to_string(speech->get_noise_threshold_pc());
                    //     cout << msg << endl;
                    //     model.addContext(msg, ROLE_INPUT);
                    // }
                // }

                if (auto_save && !model_name.empty()) save_model(true, false);
            }

            while(!keyboard_input_thread.joinable());
            keyboard_input_thread.join();
        }

    };

}