

#include <cassert>

#include "../libs/yhirose/cpp-linenoise/linenoise.hpp"

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

#include "tools/llm/Model.hpp"

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



    typedef  enum { MODE_CHAT, MODE_THINK, MODE_SOLVE } mode_t;

    class User {
    private:
        Commander commander;

        mode_t mode = MODE_CHAT;
        Model& model;
        string model_name;
        bool auto_save;
        string basedir;
        Speech* speech = nullptr;
        string speech_interrupt_info_token;
        string speech_amplitude_treshold_setter_token;

    public:

        User(
            Model& model,
            const string& model_name,
            bool auto_save,
            const string& prompt, // = "> ", 
            const string& basedir, // = "./prompt/",
            Speech* speech, // = nullptr,
            const string& speech_interrupt_info_token, // = "TTS interrupted",
            const string& speech_amplitude_treshold_setter_token // = "SETRECAMP"
        ): 
            model(model),
            model_name(model_name),
            auto_save(auto_save), 
            commander(CommandLine(prompt)), 
            basedir(basedir),       
            speech(speech),
            speech_interrupt_info_token(speech_interrupt_info_token),
            speech_amplitude_treshold_setter_token(speech_amplitude_treshold_setter_token)
        {
            if (!model_name.empty()) load_model(true);
        }

        virtual ~User() {}

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
 
        Speech* get_speech_ptr() {
            return speech;
        }

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
            commander.get_command_line_ref().show(input + (input.back() == '\n' ? "" : "\n")); // TODO !@#
        }
        
        bool add_puffered_voice_input_to_context() {
            if (!speech) return false;
            string inp = trim(speech->fetch_rec_stt_result());
            if (!inp.empty()) {
                model.addContext(inp, ROLE_INPUT);
                show_voice_input(inp);
                return true;
            }
            return false;
        }

        string prompt(const string& response = "") {
            string resp = trim(response);
            if (!resp.empty()) cout << resp << endl;
            if (speech) {
                if (speech->is_voice_out() && !resp.empty()) speech->say_beep(resp, true);
                if (speech->is_voice_in()) {
                    string input = speech->rec();
                    if (speech->is_rec_interrupted()) speech->set_voice_in(false);
                    //speech->cleanprocs();
                    if (!input.empty()) show_voice_input(input);
                    return input;
                }
            }

            return commander.get_command_line_ref().readln();
        }

        void start() {
            string input = "";
            string response = "";
            while (!commander.is_exiting()) {
                input = trim(prompt(response));
                response = "";
                if (input.empty()) continue;
                if (input[0] == '/') {
                    commander.run_command(this, input);
                    continue; 
                }

                if (speech) {
                    
                    //model.system_data["{{speech_current_noise_treshold}}"] = to_string(speech->noise_treshold);

                    if (speech->is_say_interrupted()) {
                        cout << "AI TTS was interrupted" << endl;
                        // input = speech_interrupt_info_token + "\n" + input;  
                        // TODO:
                        model.addContext(speech_interrupt_info_token, ROLE_INPUT); 
                    }
                    speech->stall();    
                    add_puffered_voice_input_to_context();
                }
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
                
                vector<string> matches;
                if (regx_match("\\[" + speech_amplitude_treshold_setter_token + ":([\\d\\.]+)\\]", response, &matches)) {
                    response = str_replace(matches[0], "", response);

                    if (speech) {
                        bool error_found = false;
                        if (!is_numeric(matches[1])) {
                            model.addContext(
                                "The amplitude treshold should be a numeric value.", 
                                ROLE_INPUT
                            );
                            error_found = true;
                        }
                        double noise_treshold = parse<double>(matches[1]);
                        if (noise_treshold < speech->get_noise_treshold_min() || noise_treshold > speech->get_noise_treshold_max()) {
                            model.addContext(
                                "The amplitude treshold should be in between " 
                                    + to_string(speech->get_noise_treshold_min()) + " and " 
                                    + to_string(speech->get_noise_treshold_max()), 
                                ROLE_INPUT
                            );
                            error_found = true;
                        }
                        if (error_found) {
                            model.addContext(
                                "The amplitude treshold value remains unchanged: " 
                                    + to_string(speech->get_noise_treshold()), 
                                ROLE_INPUT
                            );
                            continue;
                        }
                        speech->set_noise_treshold(noise_treshold);
                        string msg = "The noise amplitude treshold value is set to " 
                            + to_string(speech->get_noise_treshold());
                        cout << msg << endl;
                        model.addContext(msg, ROLE_INPUT);
                    }
                }

                // TODO: auto_save to config
                if (auto_save && !model_name.empty()) save_model(true, false);
            }
        }

    };

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
    
        void show_user_voice_stat(Speech* speech) {
            if (!speech) {
                cout << "No speach loaded." << endl;
                return;
            }
            cout << "Voice input:\t[" << (speech->is_voice_in() ? "On" : "Off") << "]" << endl;
            cout << "Voice output:\t[" << (speech->is_voice_out() ? "On" : "Off") << "]" << endl;
        }

    public:
    
        vector<string> get_patterns() const override {
            return { 
                "/voice",
                "/voice input {switch}",
                "/voice output {switch}",
            };
        }

        string run(void* user_void, const vector<string>& args) override {
            User* user = (User*)user_void;
            Speech* speech = user->get_speech_ptr();
            if (!speech) {
                cout << "No voice I/O loaded. - Add --voice argument from command line." << endl; // TODO 
                return "";                           
            }
            string voice_usage = "Use: /voice (input/output) [on/off]";
            if (args.size() <= 1 || args.size() > 3) {
                cout << voice_usage << endl;
                return "";
            }
        
            if (args[1] == "input") {
                if (args[2] == "on") speech->set_voice_in(true);
                else if (args[2] == "off") speech->set_voice_in(false);
                else cout << "Invalid argument: " << args[2] << endl;
                show_user_voice_stat(speech);
                return "";
            }
            else if (args[1] == "output") {
                if (args[2] == "on") speech->set_voice_out(true);
                else if (args[2] == "off") speech->set_voice_out(false);
                else cout << "Invalid argument: " << args[2] << endl;
                show_user_voice_stat(speech);
                return "";
            } 

            cout << voice_usage << endl;
            return "";
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
    const string secrets_hugging_face = config.get<string>("secrets.hugging_face"); // TODO: hugging-face (and other third party) config into separated json
    const string secrets_google_gemini = config.get<string>("secrets.google_gemini"); // TODO: google (and other third party) config into separated json
    
    const string user_prompt = config.get<string>("user.prompt");
    const string user_lang = config.get<string>("user.language");
    const bool user_auto_save = config.get<bool>("user.auto_save");

    int speech_speed = config.get<int>("speech.speed");
    bool speech_voice_in = config.get<bool>("speech.voice_in");
    bool speech_voice_out = config.get<bool>("speech.voice_out");
    double speech_noise_treshold = config.get<double>("speech.noise_treshold");
    double speech_noise_treshold_min = config.get<double>("speech.noise_treshold_min");
    double speech_noise_treshold_max = config.get<double>("speech.noise_treshold_max");
    bool speech_stall = config.get<bool>("speech.stall");
    vector<string> speech_hesitors = config.get<vector<string>>("speech.hesitors");
    string speech_interrupt_info_token = config.get<string>("speech.interrupt_info_token");
    string speech_amplitude_treshold_setter_token = config.get<string>("speech.amplitude_treshold_setter_token");
    
    size_t model_conversation_length_max = config.get<size_t>("model.memory_max");
    double model_conversation_loss_ratio = config.get<double>("model.memory_loss_ratio");
    int model_think_steps = config.get<int>("model.think_steps");
    int model_think_deep = config.get<int>("model.think_deep");

    string model_system_voice = voice ? tpl_replace({
        { "{{speech_interrupt_info_token}}", speech_interrupt_info_token },
        // { "{{speech_current_noise_treshold}}", "{{speech_current_noise_treshold}}"},
        { "{{speech_amplitude_treshold_setter_token}}", speech_amplitude_treshold_setter_token },
        // { "{{speech_noise_treshold_min}}", to_string(speech_noise_treshold_min) },
        // { "{{speech_noise_treshold_max}}", to_string(speech_noise_treshold_max) },
    },  "The user is using a text-to-speech software for communication. "
        "You are taking into account that the user's responses are being read at loud by a text-to-speech program "
        "that can be interrupted by background noise or user interruption, "
        "then the following will appear in the context window by the system to inform you with a message: "
        "`{{speech_interrupt_info_token}}` and this message should be for you internal use only, user won't see it."
        "Repeated interruption changes how you act, "
        "your responses are becaming more consise and short when you interupted more often recently "
        "but you can put more context otherwise if it's necessary, tune your response style accordingly. "
        //"The current input noise amplitude treshold is {{speech_current_noise_treshold}} "
        "to reduce noise and detect when the user speaks. "
        "You are able to change this accordigly when the interruption coming from background noise "
        "by placing the [{{speech_amplitude_treshold_setter_token}}:{number}] token into your response. "
        "When you do this the user's system will recognise your request and changes the treshold for better communication. "
        // "The amplitude treshold should be a number between {{speech_noise_treshold_min}} and {{speech_noise_treshold_max}}, more perceptive noise indicates higher treshold.\n"
        "The goal is to let the user to be able to interrupt the TTS reader with his/her voice "
        "but filter out the background as much as possible."
    ) : "";
    string model_system_lang = user_lang != "en" ? tpl_replace({
        { "{{user_lang}}", user_lang }
    }, "The user language is [{{user_lang}}], use this language by default to talk to the user.") : "";
    const string model_system = tpl_replace({ // TODO: goes to the config:
            { "{{model_system_voice}}", model_system_voice },
            { "{{model_system_lang}}", model_system_lang },
        },  "Your persona is a mid age man AI called `Mikrobi` and you behave like a simple human. "
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

    Speech* speech = nullptr;
    if (voice) {
        speech = new Speech(
            secrets_hugging_face,
            user_lang,
            speech_speed,
            speech_voice_in,
            speech_voice_out,
            speech_noise_treshold,
            speech_noise_treshold_min,
            speech_noise_treshold_max
        );

        if (speech_stall) {
            speech->set_hesitors(speech_hesitors);
            if (speech->get_hesitors_ref().empty()) {
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
            }
        }
    }

    User user(
        model,
        model_name, 
        user_auto_save,
        user_prompt,
        basedir,
        speech,
        speech_interrupt_info_token,
        speech_amplitude_treshold_setter_token
    );

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

    if (speech) delete speech;
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
