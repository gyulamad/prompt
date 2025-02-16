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

        // ------------------------------
        vector<Plugin> plugins;
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

        string inference_full;
        bool inference_stat_in_func_call;
        string inference_next_func_call;
        vector<string> inference_func_calls;

        void inference_plugins_reset() {
            inference_full = "";
            inference_stat_in_func_call = false;
            inference_next_func_call = "";
            inference_func_calls.clear();
        }

        string inference_remove_plugins(const string& inference) {
            string result = "";
            for (size_t i = 0; i < inference.length(); i++) {
                inference_full += inference[i];
                if (inference_stat_in_func_call) inference_next_func_call += inference[i];
                else result += inference[i];
                if (str_ends_with(inference_full, "[FUNCTION-CALLS-START]")) {
                    inference_stat_in_func_call = true;
                    inference_next_func_call = "";                    
                }
                else if (str_ends_with(inference_full, "[FUNCTION-CALLS-STOP]")) {
                    inference_stat_in_func_call = false;
                    inference_func_calls.push_back(str_replace({
                        { "[FUNCTION-CALLS-START]", "" }, 
                        { "[FUNCTION-CALLS-STOP]", "" }, 
                    }, inference_next_func_call));
                    inference_next_func_call = "";
                }
            }            
            return str_replace({
                { "[FUNCTION-CALLS-START]", "" }, 
                { "[FUNCTION-CALLS-STOP]", "" }, 
            }, result);
        }

        string inference_handle_plugins() {
            string output = "";
            for (const string& inference_func_call: inference_func_calls) {
                if (!is_valid_json(inference_func_call)) {
                    output += 
                        "Invalid JSON syntax for function call:\n" 
                        + inference_func_call 
                        + "\n";
                    continue;
                }
                JSON fcall_all(inference_func_call);
                if (!fcall_all.has("function_calls")) {
                    output += 
                        "`function_calls` key is missing:\n" 
                        + inference_func_call 
                        + "\n";
                    continue;
                }
                if (!fcall_all.isArray("function_calls")) {
                    output += 
                        "`function_calls` is not an array:\n" 
                        + inference_func_call 
                        + "\n";
                    continue;
                }
                vector<JSON> fcalls(fcall_all.get<vector<JSON>>("function_calls"));
                for (const JSON& fcall: fcalls) {
                    string function_name = fcall.get<string>("function_name");
                    bool found = false;
                    for (Plugin& plugin: plugins) {
                        if (plugin.get_name() == function_name) {
                            found = true;
                            bool invalid = false;
                            for (const Parameter& parameter: plugin.get_parameters_cref()) {
                                if (!fcall.has(parameter.get_name())) {
                                    if (parameter.is_required()) {
                                        output += 
                                            "A required parameter is missing in function call: `" 
                                            + function_name + "." + parameter.get_name() +
                                            + "`\n";
                                        invalid = true;
                                        continue;
                                    }
                                }                                
                            }
                            if (!invalid) {
                                string result;
                                try {
                                    result = plugin.call(fcall);
                                } catch (exception &e) {
                                    result = "Error in function `" + function_name + "`: " + e.what();
                                }
                                output += 
                                    "Function output `" + function_name + "`:\n"
                                    + result + "\n";
                            }
                        }
                    }
                    if (!found) {
                        output += "Function is not found: `" + function_name + "`\n";
                    }
                }
            }
            return output;
        }

        static bool stream_callback(void* user_void, const string& inference) {
            if (!user_void) return false;
            User* user = (User*)user_void;
            string inference_to_user = user->inference_remove_plugins(inference);
            if (user->speech) user->speech->hide_mic();
            cout << inference_to_user << endl;
            if (!user->speech) return false;
            return !user->speech->speak(inference_to_user);
        }

        static void stream_done_callback(void* user_void, const string& fulltext) {
            if (!user_void) return;
            User* user = (User*)user_void;
            if (user->speech) user->speech->beep();
        }



        void set_plugins(const vector<Plugin> plugins) {
            this->plugins = plugins;
        }

        void start() {
            string input = "";
            string response = "";

            cout << "DEBUG: User keyboard input handler thread start..." << endl;
            thread keyboard_input_thread([&]{
                while (!commander.is_exiting()) {
                    input = prompt(); //commander.get_command_line_ref().readln();
                    //if (speech) speech_delete();
                    sleep(1);
                }
            });

            while (!commander.is_exiting()) {

                if (!stream) {
                    string resp = trim(response);
                    if (!resp.empty()) {
                        cout << "\r" << resp << endl;
                        commander.get_command_line_ref().show();                    
                        if (speech) speech->speak(resp, true, true);
                    }
                }

                // waiting for the next input:
                input = "";
                while (input.empty()) {                    
                    usleep(30000);                    
                    if (speech) input += speech->get_text_input();
                }
                if (speech) speech->rand_speak_hesitate();

                if (input.empty()) continue;
                if (input[0] == '/') {
                    commander.run_command(this, input);
                    continue; 
                }


                if (!plugins.empty()) {
                    Model* plugin_selector_model = (Model*)model.clone();
                    vector<string> plugin_usages;
                    for (const Plugin& plugin: plugins) {
                        plugin_usages.push_back(to_string(plugin));
                        // JSON plugin_usage_json;
                        // plugin_usage_json.set("function_name", plugin->get_function_name());
                        // plugin_usage_json.set("description", plugin->get_description());
                        // vector<string> plugin_parameters_json_as_strings;
                        // for (const Parameter& parameter: plugin->get_parameters_cref()) {
                        //     JSON plugin_parameter_json;
                        //     plugin_parameter_json.set("name", parameter.get_name());
                        //     plugin_parameter_json.set("type", parameter.get_type());
                        //     plugin_parameter_json.set("required", parameter.is_required());
                        //     plugin_parameter_json.set("rules", parameter.get_rules());
                        //     plugin_parameters_json_as_strings.push_back(plugin_parameter_json.dump(4));
                        // }
                        // plugin_usage_json.set("parameters", implode(",", plugin_parameters_json_as_strings);
                        // string plugin_usage = plugin_usage_json.dump(4);
                        // plugin_usages.push_back(plugin_usage);
                    }
                    vector<string> plugin_selections = plugin_selector_model->multiple_str(
                        "Do you need to use any function call at the moment? "
                        "Select which ever you need to use or select the 'Nothing' plugin if you don't need function call at the moment. "
                        "In your response show the selected function call in a JSON format!",
                        plugin_usages
                    );
                    model.kill(plugin_selector_model);
                }
                
                
                response = "";
                inference_plugins_reset();
                switch (mode)
                {
                    case MODE_CHAT:
                        if (stream) {
                            // thread model_prompt_thread([&](){
                                response = model.prompt_stream(input, this, stream_callback, stream_done_callback);
                            // });
                            //cout << "[DEBUG] Waiting for model prompt thread to finish..." << endl;
                            // model_prompt_thread.join();   
                            inference_plugins_reset();                         
                            break;
                        }
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

                response = inference_remove_plugins(response);

                if (!inference_func_calls.empty())
                    model.addContext(inference_handle_plugins(), ROLE_INPUT);
                
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

            //while(!keyboard_input_thread.joinable());
            cout << "[DEBUG] Waiting for keyboard input thread to finish..." << endl;
            keyboard_input_thread.join();
        }

    };

}