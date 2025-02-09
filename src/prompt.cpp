#include <cassert>

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

// #include "Speech.hpp"
#include "User.hpp"

using namespace std;
using namespace tools;
using namespace tools::llm;

namespace prompt {

    class Terminal {
    private:
        string outputs;
    public:
        // triggered by [SEND-TO-TERMINAL]
        void send(const string& reason, const string& input) {

        }

        // triggered by [RESET-TERMINAL]
        void reset() {

        }
    };

    class Task {
    public:
        enum Status { TODO, IN_PROGRESS, DONE, FAIL };
    private:
        const string id;
        const string objective;  
        Status status;
        vector<string> results;
    public:
        // triggered by [TASK-UPDATE]
        void update(const string& id, Status* status = nullptr, const string* result = nullptr) {

        }
    };

    class Agent {
    private:
        const Agent* owner = nullptr;
        const string name;
        vector<Agent*> childs;
        Task task;
        Terminal terminal;

        // triggered by [SENT-TO-PARENT]
        void report(const string& response) {
            
        }

        // triggered by [SPAWN-ASSISTANT]
        void spawn(const string& name, const Task& task) {

        }

        // triggered by [SENT-TO-ASSISTANT]
        void command(const Agent& children, const string& request) {

        }

        // triggered by [KILL-ASSISTANT]
        void kill(const string& name) {

        }

    public:

    };

    // ----------------------------



    class ExitCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return { "/exit" };
        }

        string run(void* user, const vector<string>& args) override {
            ((User*)user)->exit();
            return "Exiting...";
        }
    };

    class HelpCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return { "/help" };
        }

        string run(void* user, const vector<string>& args) override {
            cout << "Usages:" << endl;
            array_dump(((User*)user)->get_cmatcher_ref().command_patterns, false);
            return "";
        }
    };

    // -------- app specific commands ----------

    class VoiceCommand: public Command {
    private:
    
        // void show_user_voice_stat(Speech* speech) {
        //     if (!speech) {
        //         cout << "No speach loaded." << endl;
        //         return;
        //     }
        //     cout << "Voice input:\t[" << (speech->is_voice_in() ? "On" : "Off") << "]" << endl;
        //     cout << "Voice output:\t[" << (speech->is_voice_out() ? "On" : "Off") << "]" << endl;
        // }

    public:
    
        vector<string> get_patterns() const override {
            return { 
                "/voice",
                // "/voice input {switch}",
                // "/voice output {switch}",
            };
        }

        string run(void* user_void, const vector<string>& args) override {
            User* user = (User*)user_void;
            return user->speech_toggle() ? "Voice mode [ON]" : "Voice mode [OFF]";
            // // Speech* speech = user->get_speech_ptr();
            // // if (!speech) {
            //     cout << "No voice I/O loaded. - Add --voice argument from command line." << endl; // TODO 
            //     return "";                           
            // // }
            // string voice_usage = "Use: /voice (input/output) [on/off]";
            // if (args.size() <= 1 || args.size() > 3) {
            //     cout << voice_usage << endl;
            //     return "";
            // }
        
            // if (args[1] == "input") {
            //     if (args[2] == "on"); // TODO: speech->set_voice_in(true);
            //     else if (args[2] == "off"); // TODO: speech->set_voice_in(false);
            //     else cout << "Invalid argument: " << args[2] << endl;
            //     // TODO: show_user_voice_stat(speech);
            //     return "";
            // }
            // else if (args[1] == "output") {
            //     if (args[2] == "on"); // TODO: speech->set_voice_out(true);
            //     else if (args[2] == "off"); // TODO: speech->set_voice_out(false);
            //     else cout << "Invalid argument: " << args[2] << endl;
            //     // TODO: show_user_voice_stat(speech);
            //     return "";
            // } 

            // cout << voice_usage << endl;
            // return "";
        }
    };

    class SendCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return {
                "/send {filename}",
                "/send {string} {filename}",
                "/send {filename} {number} {number}",
                "/send {string} {filename} {number} {number}",
                "/send-lines {filename}",
                "/send-lines {string} {filename}",
                "/send-lines {filename} {number} {number}",
                "/send-lines {string} {filename} {number} {number}",
            };
        }

        string run(void* user_void, const vector<string>& args) override {
            User* user = (User*)user_void;
            int lnfirst = 0, lnlast = 0;
            if (args.size() == 1) {
                cout << "Filename is missing. Use /send [\"message\"] filename [first-line last-line]" << endl;
                cout << "Note: message and first/last line numbers are optional, line numbers start from line-1th."
                    "\nThe line number is zero (0) means the begin/end of file." << endl;
                return "";
            }
            string message = "", filename;
            if (args.size() == 2) {
                filename = args[1];
            }
            if (args.size() == 3) {
                message = args[1];
                filename = args[2];
            }
            if (args.size() == 4) {
                filename = args[1];
                if (!is_numeric(args[2])) cout << "Invalit first line number: " << args[2] << endl;
                else lnfirst = parse<int>(args[2]);
                if (!is_numeric(args[3])) cout << "Invalit last line number: " << args[3] << endl;
                else lnlast = parse<int>(args[3]);
            }
            if (args.size() == 5) {
                message = args[1];
                filename = args[2];
                if (!is_numeric(args[3])) cout << "Invalit first line number: " << args[3] << endl;
                else lnfirst = parse<int>(args[2]);
                if (!is_numeric(args[4])) cout << "Invalit last line number: " << args[4] << endl;
                else lnlast = parse<int>(args[4]);
            }
            if (args.size() > 5) {
                cout << "Too many arguments" << endl;
                return "";
            }

            if (!file_exists(filename)) {
                cout << "File not found: " << filename << endl;
                return "";
            }

            if (lnfirst < 0 || lnlast < lnfirst) {
                cout << "Line numbers should be greater or equal to 1th and first line should be less or equal to the last line number." << endl;
                return "";
            }

            string contents = file_get_contents(filename);
            if (contents.empty()) contents = "<empty>";
            else if (lnfirst || lnlast) {
                vector<string> lines = explode("\n", contents);
                vector<string> show_lines;
                for (size_t ln = 1; ln <= lines.size(); ln++) {
                    if (
                        (lnfirst == 0 || ln >= lnfirst) &&
                        (lnlast == 0 || ln <= lnlast)
                    ) show_lines.push_back(
                        (args[0] == "/send-lines" ? to_string(ln) + ": " : "") + lines[ln-1]
                    );
                }
                contents = implode("\n", show_lines);
            }
            user->get_model_ref().addContext(message + "\nFile '" + filename + "' contents:\n" + contents);
            cout << "File added to the conversation context: " << filename << endl;
            return "";
        }
    };

    class ModeCommand: public Command {
    private:

        void show_user_mode(User* user) {
            string mode_s = "";
            switch (user->get_mode())
            {
                case MODE_CHAT:
                    mode_s = "chat";
                    break;

                case MODE_THINK:
                    mode_s = "think (steps: " + to_string(user->get_model_ref().think_steps) + ")";
                    break;

                case MODE_SOLVE:
                    mode_s = "solve (steps: " + to_string(user->get_model_ref().think_steps) + ", deep: " + to_string(user->get_model_ref().think_deep) + ")";
                    break;
            
                default:
                    throw ERROR("Invalid mode");
            }

            cout << "Mode: " << mode_s << endl;
        }

    public:
    
        vector<string> get_patterns() const override {
            return { 
                "/mode",
                "/mode chat",
                "/mode think",
                "/mode solve",
            };
        }

        string run(void* user_void, const vector<string>& args) override {
            User* user = (User*)user_void;
            if (args.size() >= 2) {
                if (args[1] == "chat") {
                    user->set_mode(MODE_CHAT);
                    show_user_mode(user);
                }
                else if (args[1] == "think") {
                    user->set_mode(MODE_THINK);
                    show_user_mode(user);
                }
                else if (args[1] == "solve") {
                    user->set_mode(MODE_SOLVE);
                    show_user_mode(user);
                }
                else {
                    cout << "Invalid mode: " << args[1] << endl;
                }
            }
            return "";
        }
    };

    class ThinkCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return { "/think {number}" };
        }

        string run(void* user_void, const vector<string>& args) override {
            User* user = (User*)user_void;
            if (args.size() == 2) {
                if (is_integer(args[1]))
                    user->get_model_ref().think_steps = parse<int>(args[1]);
                else cout << "Invalid parameter." << endl;
            }
            cout << "Extracting steps: " << user->get_model_ref().think_steps << endl;

            return "";
        }
    };

    class SolveCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return { "/solve {number}" };
        }

        string run(void* user_void, const vector<string>& args) override {
            User* user = (User*)user_void;
            if (args.size() == 2) {
                if (is_integer(args[1]))
                    user->get_model_ref().think_deep = parse<int>(args[1]);
                else cout << "Invalid parameter." << endl;
            }
            cout << "Deep thinking solution tree depth max: " << user->get_model_ref().think_deep << endl;

            return "";
        }
    };

    class SaveCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return { "/save {string}" };
        }

        string run(void* user_void, const vector<string>& args) override {
            User* user = (User*)user_void;
            
            if (args.size() > 2) {
                cout << "Invalid parameter counts, use /save {name}" << endl;
                return "";
            }
            if (args.size() == 2) {
                user->set_model_name(args[1]);
                if (file_exists(user->get_model_file())) {
                    cout << "Model already exists: " << user->get_model_name_ref() << endl;
                    if (!confirm("Do you want to override?")) return "";
                }
            }
            user->save_model(true);

            return "";
        }
    };

    class LoadCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return { "/load {string}" };
        }

        string run(void* user_void, const vector<string>& args) override {
            User* user = (User*)user_void;
            
            if (args.size() != 2) {
                cout << "Invalid parameter counts, use /load {name}" << endl;
                return "";
            }
            if (!user->is_auto_save() && 
                confirm("Current model session is: " + user->get_model_name_ref() + 
                        "\nDo you want to save it first?")) user->save_model(true);
            
            user->set_model_name(args[1]);
            user->load_model(false);

            return "";
        }
    };


    // -----------------------------------------
    
    class Gemini: public Model {
    private:
        int err_retry = 10;
        Logger& logger;
        string secret;

        static const vector<string> variants;
        static size_t current_variant;
        static string variant;

        static bool next_variant() {
            bool next_variant_found = true;
            current_variant++;
            if (current_variant >= variants.size()) {
                current_variant = 0;
                next_variant_found = false;
            }
            variant = variants[current_variant];
            return next_variant_found;
        }

    protected:

        string message_to_json(const Message& message, const role_name_map_t& role_name_map = {
            { ROLE_NONE, "" },
            { ROLE_INPUT, "user" },
            { ROLE_OUTPUT, "model" },
        }) {
            return tpl_replace({
                { "{{role}}", to_string(message.get_role(), role_name_map) },
                { "{{text}}", json_escape(message.get_text()) },
            }, R"({
                "role": "{{role}}",
                "parts":[{
                    "text": "{{text}}"
                }]
            })");
        }

        string conversation_to_json(const string& system, const Conversation& conversation) {
            vector<string> jsons;
            vector<Message> messages = conversation.get_messages_ref();
            for (const Message& message: messages)
                jsons.push_back(message_to_json(message));
            return tpl_replace({
                { "{{system}}", json_escape(system) },
                { "{{conversation}}", implode(",", jsons) },
            }, R"({
                "system_instruction":{
                    "parts":{
                        "text": "{{system}}"
                    }
                },
                "contents":[
                    {{conversation}}
                ]
            })");
        }
        
        virtual string request() override {
            int restarts = 2;
            const string tmpfile = "/tmp/temp.json";
            string command;
            JSON response;
            while (restarts) {
                try {
                    // JSON request;
                    // request.set("contents[0].parts[0].text", prompt);
                    // file_put_contents(tmpfile, request.dump(4));
                    string conversation_json = conversation_to_json(system, conversation);
                    if (!file_put_contents(tmpfile, conversation_json)) {
                        throw ERROR("Unable to write: " + tmpfile);
                    }
                    //assert(file_get_contents(tmpfile) == conversation_json);
                    command = "curl -s \"https://generativelanguage.googleapis.com/v1beta/models/" + variant + ":generateContent?key=" + escape(secret) + "\" -H 'Content-Type: application/json' -X POST --data-binary @" + tmpfile;
                    //sleep(3); // TODO: for api rate limit
                    response = Process::execute(command);
                    if (response.isDefined("error") || !response.isDefined("candidates[0].content.parts[0].text"))
                        throw ERROR("Gemini error: " + response.dump());
                    return response.get<string>("candidates[0].content.parts[0].text");    
                } catch (exception &e) {
                    string what = e.what();
                    string usrmsg = "Gemini API failure: " + what + "\nSwitching variant from " + variant + " to ";
                    string errmsg = 
                        "Gemini API (" + variant + ") request failed: " + what;
                    bool next_variant_found = next_variant();
                    usrmsg += variant + ".";                    
                    cerr << usrmsg << endl;
                    errmsg += 
                        "\nContinue with variant " + variant + 
                        "\nRequest was: " + command +
                        "\nRequest data: " + str_cut_begin(file_get_contents(tmpfile)) +
                        "\nResponse was: " + response.dump();
                    logger.warning(errmsg);
                    if (!next_variant_found) {
                        cerr << "Retry after " << err_retry << " second(s)..." << endl;
                        sleep(err_retry);
                        restarts--;
                    } else sleep(3); // TODO: for api rate limit
                }
            }
            throw ERROR("Gemini API error. See more in log...");
        }

    public:
        Gemini(Logger& logger, const string& secret, MODEL_ARGS): Model(MODEL_ARGS_PASS),
            logger(logger), secret(file_exists(secret) ? file_get_contents(secret) : secret)
        {}

        // make it as a factory - caller should delete spawned model using kill()
        void* spawn(MODEL_ARGS) override {
            return new Gemini(logger, secret, MODEL_ARGS_PASS);
        }

        void kill(Model* gemini) override { 
            delete (Gemini*)gemini;
        }
    };
    const vector<string> Gemini::variants = {
        // TODO: to config
        // See all models at: https://ai.google.dev/gemini-api/docs/models/gemini
        "gemini-2.0-flash",
        "gemini-2.0-flash-lite-preview-02-05",
        "gemini-2.0-flash-exp",
        "gemini-1.5-flash-latest",
        "gemini-1.5-flash",
        "gemini-1.5-flash-001",
        "gemini-1.5-flash-002",
        "gemini-1.5-flash-8b-latest",
        "gemini-1.5-flash-8b",
        "gemini-1.5-flash-8b-001",
        "gemini-1.5-pro-latest",
        "gemini-1.5-pro",
        "gemini-1.5-pro-001",
        "gemini-1.5-pro-002",
        // "gemini-1.0-pro-latest", // gemini-1.0-pro models are deprecated on 2/15/2025
        // "gemini-1.0-pro", 
        // "gemini-1.0-pro-001", 
    };
    size_t Gemini::current_variant = 0;
    string Gemini::variant = variants[current_variant];

}

using namespace prompt;

int main(int argc, char *argv[]) {

    // args
    Arguments args(argc, argv);
    const bool voice = args.getBool("voice");
    const string model_name = args.has("model") ? args.getString("model") : "";

    // configs
    JSON config(file_get_contents("config.json"));

    // logger
    const string basedir = get_exec_path() + "/.prompt";
    mkdir(basedir);
    Logger logger("Prompt-log", basedir + "/prompt.log");
    logger.info("Prompt started");

    // settings
    // const string secrets_hugging_face = config.get<string>("secrets.hugging_face"); // TODO: hugging-face (and other third party) config into separated json
    const string secrets_google_gemini = config.get<string>("secrets.google_gemini"); // TODO: google (and other third party) config into separated json
    
    const string user_prompt = config.get<string>("user.prompt");
    const string user_lang = config.get<string>("user.language");
    const bool user_auto_save = config.get<bool>("user.auto_save");

    // int speech_speed = config.get<int>("speech.speed");
    // bool speech_voice_in = config.get<bool>("speech.voice_in");
    // bool speech_voice_out = config.get<bool>("speech.voice_out");
    // double speech_noise_threshold_pc = config.get<double>("speech.noise_threshold_pc");
    // double speech_noise_threshold_pc_min = config.get<double>("speech.noise_threshold_pc_min");
    // double speech_noise_threshold_pc_max = config.get<double>("speech.noise_threshold_pc_max");
    // double speech_noise_threshold_pc_while_speech = config.get<double>("speech.noise_threshold_pc_while_speech");
    // string speech_stt = config.get<string>("speech.stt");
    // string speech_whisper_model = config.get<string>("speech.whisper_model");
    // int speech_whisper_threads = config.get<int>("speech.whisper_threads");
    // bool speech_stall = config.get<bool>("speech.stall");
    // vector<string> speech_hesitors = config.get<vector<string>>("speech.hesitors");
    // vector<string> speech_repeaters = config.get<vector<string>>("speech.repeaters");
    // string speech_interrupt_info_token = config.get<string>("speech.interrupt_info_token");
    // string speech_amplitude_threshold_pc_setter_token = config.get<string>("speech.amplitude_threshold_pc_setter_token");

    bool speech_stall = config.get<bool>("speech.stall"); // TODO    
    bool speech_tts_voice_out = voice && config.get<bool>("speech.tts_voice_out");
    int speech_tts_speed = config.get<int>("speech.tts.speed");
    int speech_tts_gap = config.get<int>("speech.tts.gap");
    string speech_tts_beep_cmd = config.get<string>("speech.tts.beep_cmd");
    string speech_tts_think_cmd = config.get<string>("speech.tts.think_cmd");
    bool speech_stt_voice_in = voice && config.get<bool>("speech.stt_voice_in");
    double speech_stt_voice_recorder_sample_rate = config.get<double>("speech.stt.voice_recorder.sample_rate");
    unsigned long speech_stt_voice_recorder_frames_per_buffer = config.get<unsigned long>("speech.stt.voice_recorder.frames_per_buffer");
    size_t speech_stt_voice_recorder_buffer_seconds = config.get<size_t>("speech.stt.voice_recorder.buffer_seconds");
    float speech_stt_noise_monitor_threshold_pc = config.get<float>("speech.stt.noise_monitor.threshold_pc");
    size_t speech_stt_noise_monitor_window = config.get<size_t>("speech.stt.noise_monitor.window");
    string speech_stt_transcriber_model = config.get<string>("speech.stt.transcriber.model");
    long speech_stt_poll_interval_ms = config.get<long>("speech.stt.poll_interval_ms");


    size_t model_conversation_length_max = config.get<size_t>("model.memory_max");
    double model_conversation_loss_ratio = config.get<double>("model.memory_loss_ratio");
    int model_think_steps = config.get<int>("model.think_steps");
    int model_think_deep = config.get<int>("model.think_deep");

    string model_system_voice = voice ? tpl_replace({
        // { "{{speech_interrupt_info_token}}", speech_interrupt_info_token },
        // { "{{speech_current_noise_threshold_pc}}", "{{speech_current_noise_threshold_pc}}"},
        // { "{{speech_amplitude_threshold_pc_setter_token}}", speech_amplitude_threshold_pc_setter_token },
        // { "{{speech_noise_threshold_pc_min}}", to_string(speech_noise_threshold_pc_min) },
        // { "{{speech_noise_threshold_pc_max}}", to_string(speech_noise_threshold_pc_max) },
    },  "The user is using a text-to-speech software for communication. "
        "You are taking into account that the user's responses are being read at loud by a text-to-speech program "
        "that can be interrupted by background noise or by the user itself."
        // "then the following will appear in the context window by the system to inform you with a message: "
        // "`{{speech_interrupt_info_token}}` and this message should be for you internal use only, user won't see it."
        "Repeated interruption changes how you act, "
        "your responses are becaming more consise and short when you interupted more often recently "
        "but you can put more context otherwise if it's necessary, tune your response style accordingly. "
        //"The current input noise amplitude threshold_pc is {{speech_current_noise_threshold_pc}} "
        // "to reduce noise and detect when the user speaks. "
        // "You are able to change this accordigly when the interruption coming from background noise "
        // "by placing the [{{speech_amplitude_threshold_pc_setter_token}}:{number}] token into your response. "
        // "When you do this the user's system will recognise your request and changes the threshold_pc for better communication. "
        // "The amplitude threshold_pc should be a number between {{speech_noise_threshold_pc_min}} and {{speech_noise_threshold_pc_max}}, more perceptive noise indicates higher threshold_pc.\n"
        // "The goal is to let the user to be able to interrupt the TTS reader with his/her voice "
        // "but filter out the background as much as possible."
    ) : "";
    string model_system_lang = user_lang != "en" ? tpl_replace({
        { "{{user_lang}}", user_lang }
    }, "The user language is [{{user_lang}}], use this language by default to talk to the user.") : "";
    const string model_system = tpl_replace({ // TODO: goes to the config:
            { "{{model_system_voice}}", model_system_voice },
            { "{{model_system_lang}}", model_system_lang },
        },  "Your persona is a mid age man like AI and you behave like a simple human. "
            "You have a sense of humor, your personality is entertaining. "
            "Your answers are succinct and focusing on the core of your conversation "
            "but in a way like a normal human chat would looks like. "
            "You always helpful but also concise in answers.\n"
            "{{model_system_voice}}\n"
            "{{model_system_lang}}\n"
            // + "\nYou are a creative helper designer who always should came up with the simpliest possible solution no mather what even if you don't know the correct answer you guess."
            // + (voice ?
            //     "\nThe user is using a text-to-speech software for communication. It should be taken into account that the user's responses are being read aloud by a text-to-speech program that can be interrupted by background noise or user interruption, then the following will appear in the context window to inform you about it: " + User::speech_interrupt_info + "\n"
            //     "\nRepeated interruption changes how you act, your responses are becaming more consise if you intterupted more often recently but you can put more context otherwise if it's necessary, tune your response style accordingly."
            //     : "")
            //"The user language is [{{user_lang}}], use this language by default to talk to the user."
    );

    Gemini model(
        logger,
        secrets_google_gemini,
        model_system,
        model_conversation_length_max,
        model_conversation_loss_ratio,
        model_think_steps,
        model_think_deep
    );

    // Speech* speech = nullptr;
    

    User user(
        model,
        model_name, 
        user_lang,
        user_auto_save,
        user_prompt,
        basedir,
        speech_stall,
        speech_tts_speed,
        speech_tts_gap,
        speech_tts_beep_cmd,
        speech_tts_think_cmd,
        speech_stt_voice_in,
        speech_stt_voice_recorder_sample_rate,
        speech_stt_voice_recorder_frames_per_buffer,
        speech_stt_voice_recorder_buffer_seconds,
        speech_stt_noise_monitor_threshold_pc,
        speech_stt_noise_monitor_window,
        speech_stt_transcriber_model,
        speech_stt_poll_interval_ms
        // speech//,
        // speech_interrupt_info_token,
        // speech_amplitude_threshold_pc_setter_token
    );
    
    if (voice) user.speech_create();

    // TODO:
    // "/set volume {number}",
    // "/set voice-input {switch}",
    // "/cat {filename}",
    // "/print {string}",
    // "/print {string} {filename}"
    user.set_commands({
        new ExitCommand,
        new HelpCommand,
        new VoiceCommand,
        new SendCommand,
        new ModeCommand,
        new ThinkCommand,
        new SolveCommand,
        new SaveCommand,
        new LoadCommand,
    });

    user.start();

    for (void* command_void: user.get_commands_ref()) {
        Command* command = (Command*)command_void;
        delete command;
    }

    // user.start({
    //     &testCommand,
    //     &exitCommand,
    //     //...        
    // });

    // if (speech) delete speech;
    return 0;
}

/*

**Cím:** "Mikrobi: Egy mesterséges intelligencia, aki segít a programozásban"

**Bevezetés:**

*   Bemutatkozás: A nevem "Mikrobi", és egy mesterséges intelligencia vagyok.
*   Rövid bemutatás a képességeimről:  A szövegértés, a válaszadás, a beszélgetés, a memóriám, a zajszűrés, a nyelvváltás.

**Részek:**

1.  **Memória teszt:**
    *   Egyszerű számtani feladatok megoldása.
    *   Korábbi beszélgetések részleteinek felidézése.
2.  **Zajszűrés teszt:**
    *   A zajszűrés szintjének beállítása különböző szinteken.
    *   A zajszűrés hatékonyságának bemutatása.
3.  **Szövegértés és válaszadás:**
    *   Egyszerű kérdésekre válaszolás.
    *   Összetettebb kérdésekre válaszolás.
    *   **Átvitt értelmű kérdésekre válaszolás:**
        *   "Milyen a helyzet a jég hátán?" (Kérdező: Mit értesz ez alatt a kérdés alatt?)
        *   "Ég a ház?" (Kérdező: Mi a jelentése ennek a kérdésnek?)
4.  **Beszélgetés:**
    *   Természetesen folytatott párbeszéd.
    *   A párbeszéd menetének követése.
    *   A korábbi mondatokra való emlékezés.
5.  **Nyelvváltás:**
    *   A nyelvváltás bemutatása angolról magyarra.
    *   A nyelvváltás bemutatása magyarról angolra.

**Zárás:**

*   Rövid összegzés a képességeimről: 
    *   "Jól láttátok, hogy képes vagyok a szöveg megértésére, a válaszadásra, a beszélgetésre, a memóriák tárolására, a zajszűrésre, és a nyelvváltásra."
*   Lehetséges jövőbeli fejlesztések:
    *   "A jövőben szeretnénk továbbfejleszteni a memóriámat és a nyelvi képességeimet."
    *   "A 'terminál-asszisztens' funkcióval a programozók munkáját segíthetem a hibakeresésben és a terminál-parancsok megértésében."
    *   "A 'jegyzetfüzet' funkció segíthet a felhasználóknak a gondolataik és ötleteik rendezésében."
    *   "A 'több én' koncepció lehetővé teszi a hatékonyabb feladatmegoldást."
 */
