

#include <cassert>

#include "../libs/yhirose/cpp-linenoise/linenoise.hpp"

#include "tools/ERROR.hpp"
#include "tools/io.hpp"
#include "tools/files.hpp"
#include "tools/strings.hpp"
#include "tools/vectors.hpp"
#include "tools/Process.hpp"
#include "tools/JSON.hpp"
#include "tools/CommandLine.hpp"
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

    class Command {
    public:
        virtual ~Command() {}

        virtual vector<string> get_patterns() const { UNIMP; }
        
        /**
         * @brief 
         * 
         * @param user 
         * @param args 
         * @return string 
         */
        virtual string run(void*, const vector<string>&) { UNIMP; }
    };

    class User {
    private:
        bool exiting = false;
        CompletionMatcher cmatcher;
        vector<void*> commands;

        enum Mode {MODE_CHAT, MODE_THINK, MODE_SOLVE};
        Mode mode = MODE_CHAT;
        Model& model;
        string model_name;
        bool auto_save;
        string basedir;
        CommandLine commandLine;
        Speech* speech = nullptr;
        bool voice_in, voice_out;
        string speech_interrupt_info_token;
        string speech_amplitude_treshold_setter_token;

        string get_model_file() {
            if (model_name.empty()) throw ERROR("Model name is not set.");
            return basedir + "models/" + model_name + ".json";
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
                model.dump_conversation(commandLine.get_prompt());
                cout << "Model loaded: " << model_name << endl;
            }
        }

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
            commandLine(prompt), 
            basedir(basedir),       
            speech(speech),
            voice_in(speech),
            voice_out(speech),
            speech_interrupt_info_token(speech_interrupt_info_token),
            speech_amplitude_treshold_setter_token(speech_amplitude_treshold_setter_token)
        {
            if (!model_name.empty()) load_model(true);
        }

        ~User() {}
        
        void set_commands(const vector<void*>& commands) {
            this->commands = commands;
        }

        vector<void*> get_commands_ref() {
            return commands;
        }

        void exit() {
            exiting = true;
        }

        const CompletionMatcher& get_cmatcher_ref() const {
            return cmatcher;
        }

        string prompt(const string& response = "") {
            string resp = trim(response);
            if (!resp.empty()) cout << resp << endl;
            if (speech) {
                if (voice_out && !resp.empty()) speech->say(resp, true);
                if (voice_in) {
                    string input = speech->rec();
                    if (speech->is_rec_interrupted()) voice_in = false;
                    speech->cleanprocs();
                    if (!input.empty()) commandLine.show(input + (input.back() == '\n' ? "" : "\n")); // TODO !@#
                    return input;
                }
            }

            return commandLine.readln();
        }
        

        void start() {

            cmatcher.command_patterns = {
                // "/help",
                // "/exit",
                "/voice",
                "/voice input {switch}",
                "/voice output {switch}",
                "/send {filename}",
                "/send {string} {filename}",
                "/send {filename} {number} {number}",
                "/send {string} {filename} {number} {number}",
                "/mode",
                "/mode chat",
                "/mode think",
                "/mode solve",
                "/think {number}",
                "/solve {number}",
                "/save {string}",
                "/load {string}",
                // "/set volume {number}",
                // "/set voice-input {switch}",
                // "/cat {filename}",
                // "/print {string}",
                // "/print {string} {filename}"
            };

            for (const void* command: commands)
                cmatcher.command_patterns = array_merge(
                    cmatcher.command_patterns, 
                    ((Command*)command)->get_patterns()
                );
            
            commandLine.set_completion_matcher(cmatcher);

            string response = "";
            while (!exiting) {
                string input = prompt(response); 
                response = "";      
                if (commandLine.is_exited()) break;        
                if (input.empty()) continue;
                else if (input[0] == '/') {
                    bool trlspc;
                    vector<string> input_parts = array_filter(cmatcher.parse_input(input, trlspc, false));
                    input = "";

                    bool command_found = false;
                    bool command_arguments_matches = false;
                    for (void* command_void: commands) {
                        Command* command = (Command*)command_void;
                        for (const string& command_pattern: command->get_patterns()) {
                            vector<string> command_pattern_parts = array_filter(explode(" ", command_pattern));
                            if (input_parts[0] == command_pattern_parts[0]) {
                                command_found = true;
                                if (input_parts.size() == command_pattern_parts.size()) {
                                    command_arguments_matches = true;
                                    cout << command->run(this, input_parts) << endl;
                                }
                            }
                        }
                    }
                    if (!command_found) cout << "Command not found: " << input_parts[0] << endl;
                    else if (!command_arguments_matches) cout << "Invalid argument(s)." << endl;


                    // if (input_parts[0] == "/help") {
                    //     cout << "Usages:" << endl;
                    //     array_dump(cmatcher.command_patterns, false);
                    //     continue;
                    // }

                    if (input_parts[0] == "/voice") {
                        if (!speech) {
                            cout << "No voice I/O loaded. - Add --voice argument from command line." << endl; // TODO 
                            continue;                           
                        }
                        if (input_parts.size() > 1) {
                            string voice_usage = "Use: /voice (input/output) [on/off]";
                            if (input_parts.size() < 3) cout << voice_usage << endl;
                            else if (input_parts[1] == "input") {
                                if (input_parts[2] == "on") voice_in = true;
                                else if (input_parts[2] == "off") voice_in = false;
                                else cout << "Invalid argument: " << input_parts[2] << endl;
                            }
                            else if (input_parts[1] == "output") {
                                if (input_parts[2] == "on") voice_out = true;
                                else if (input_parts[2] == "off") voice_out = false;
                                else cout << "Invalid argument: " << input_parts[2] << endl;
                            } else cout << voice_usage << endl;
                        }

                        cout << "Voice input:\t[" << (voice_in ? "On" : "Off") << "]" << endl;
                        cout << "Voice output:\t[" << (voice_out ? "On" : "Off") << "]" << endl;
                        continue;
                    }

                    if (input_parts[0] == "/send" || input_parts[0] == "/send-lines") {
                        int lnfirst = 0, lnlast = 0;
                        if (input_parts.size() == 1) {

                // "/send {filename}",
                // "/send {string} {filename}",
                // "/send {filename} {number} {number}",
                // "/send {string} {filename} {number} {number}",
                            cout << "Filename is missing. Use /send [\"message\"] filename [first-line last-line]" << endl;
                            cout << "Note: message and first/last line numbers are optional, line numbers start from line-1th."
                                "\nThe line number is zero (0) means the begin/end of file." << endl;
                            continue;
                        }
                        string message = "", filename;
                        if (input_parts.size() == 2) {
                            filename = input_parts[1];
                        }
                        if (input_parts.size() == 3) {
                            message = input_parts[1];
                            filename = input_parts[2];
                        }
                        if (input_parts.size() == 4) {
                            filename = input_parts[1];
                            if (!is_numeric(input_parts[2])) cout << "Invalit first line number: " << input_parts[2] << endl;
                            else lnfirst = parse<int>(input_parts[2]);
                            if (!is_numeric(input_parts[3])) cout << "Invalit last line number: " << input_parts[3] << endl;
                            else lnlast = parse<int>(input_parts[3]);
                        }
                        if (input_parts.size() == 5) {
                            message = input_parts[1];
                            filename = input_parts[2];
                            if (!is_numeric(input_parts[3])) cout << "Invalit first line number: " << input_parts[3] << endl;
                            else lnfirst = parse<int>(input_parts[2]);
                            if (!is_numeric(input_parts[4])) cout << "Invalit last line number: " << input_parts[4] << endl;
                            else lnlast = parse<int>(input_parts[4]);
                        }
                        if (input_parts.size() > 5) {
                            cout << "Too many arguments" << endl;
                            continue;
                        }

                        if (!file_exists(filename)) {
                            cout << "File not found: " << filename << endl;
                            continue;
                        }

                        if (lnfirst < 0 || lnlast < lnfirst) {
                            cout << "Line numbers should be greater or equal to 1th and first line should be less or equal to the last line number." << endl;
                            continue;
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
                                    (input_parts[0] == "/send-lines" ? to_string(ln) + ": " : "") + lines[ln-1]
                                );
                            }
                            contents = implode("\n", show_lines);
                        }
                        input = message + "\nFile '" + filename + "' contents:\n" + contents;
                    }

                    if (input_parts[0] == "/mode") {
                        if (input_parts.size() >= 2) {
                            if (input_parts[1] == "chat") {
                                mode = MODE_CHAT;
                            }
                            else if (input_parts[1] == "think") {
                                mode = MODE_THINK;
                            }
                            else if (input_parts[1] == "solve") {
                                mode = MODE_SOLVE;
                            }
                            else {
                                cout << "Invalid mode: " << input_parts[1] << endl;
                                continue;
                            }
                        }
                        string mode_s = "";
                        switch (mode)
                        {
                            case MODE_CHAT:
                                mode_s = "chat";
                                break;

                            case MODE_THINK:
                                mode_s = "think (steps: " + to_string(model.think_steps) + ")";
                                break;

                            case MODE_SOLVE:
                                mode_s = "solve (steps: " + to_string(model.think_steps) + ", deep: " + to_string(model.think_deep) + ")";
                                break;
                        
                            default:
                                throw ERROR("Invalid mode");
                        }

                        cout << "Mode: " << mode_s << endl;
                        continue;
                    }

                    if (input_parts[0] == "/think") {
                        if (input_parts.size() == 2) {
                            if (is_integer(input_parts[1]))
                                model.think_steps = parse<int>(input_parts[1]);
                            else cout << "Invalid parameter." << endl;
                        }
                        cout << "Extracting steps: " << model.think_steps << endl;
                        continue;
                    }

                    if (input_parts[0] == "/solve") {
                        if (input_parts.size() == 2) {
                            if (is_integer(input_parts[1]))
                                model.think_deep = parse<int>(input_parts[1]);
                            else cout << "Invalid parameter." << endl;
                        }
                        cout << "Deep thinking solution tree depth max: " << model.think_deep << endl;
                        continue;
                    }

                    if (input_parts[0] == "/save") {
                        if (input_parts.size() > 2) {
                            cout << "Invalid parameter counts, use /save {name}" << endl;
                            continue;
                        }
                        if (input_parts.size() == 2) {
                            model_name = input_parts[1];
                            if (file_exists(get_model_file())) {
                                cout << "Model already exists: " << model_name << endl;
                                if (!confirm("Do you want to override?")) continue;
                            }
                        }
                        save_model(true);
                        continue;
                    }

                    if (input_parts[0] == "/load") {
                        if (input_parts.size() != 2) {
                            cout << "Invalid parameter counts, use /load {name}" << endl;
                            continue;
                        }
                        if (!auto_save && 
                            confirm("Current model session is: " + model_name + 
                                    "\nDo you want to save it first?")) save_model(true);
                        
                        model_name = input_parts[1];
                        load_model(false);
                        continue;
                    }


                    // if (input.empty()) {
                    //     cout << "Invalid command/arguments or syntax: " << input_parts[0] << endl;
                        continue; 
                    // }
                    // commandLine.show(input + "\n");
                }

                if (speech) {
                    //model.system_data["{{speech_current_noise_treshold}}"] = to_string(speech->noise_treshold);

                    if (speech->is_say_interrupted()) {
                        // cout << "AI TTS was interrupted" << endl;
                        // input = speech_interrupt_info_token + "\n" + input;  
                        // TODO: model.addContext(speech_interrupt_info_token, ROLE_INPUT);                 
                    }

                    speech->stall();
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
                //response = model.prompt(input); //model.solve(input);
                // cout << response << endl;
                vector<string> matches;
                if (regx_match("\\[" + speech_amplitude_treshold_setter_token + ":([\\d\\.]+)\\]", response, &matches)) {
                    response = str_replace(matches[0], "", response);

                    if (speech) {
                        bool error_found = false;
                        if (!is_numeric(matches[1])) {
                            model.addContext("The amplitude treshold should be a numeric value.", ROLE_INPUT);
                            error_found = true;
                        }
                        double noise_treshold = parse<double>(matches[1]);
                        if (noise_treshold < 0.2 || noise_treshold > 0.8) {
                            model.addContext("The amplitude treshold should be in between 0.2 and 0.8.", ROLE_INPUT);
                            error_found = true;
                        }
                        if (error_found) {
                            model.addContext("The amplitude treshold value remains unchanged: " + to_string(speech->noise_treshold), ROLE_INPUT);
                            continue;
                        }
                        speech->noise_treshold = noise_treshold;
                        model.addContext("The amplitude treshold value is set to " + to_string(speech->noise_treshold), ROLE_INPUT);
                    }
                }

                // TODO: auto_save to config
                if (auto_save && !model_name.empty()) save_model(true, false);
            }
        }

    };

    class TestCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return {
                "/test",
                "/test echo {string}",
            };
        }

        string run(void* user, const vector<string>& args) override {
            if (args.size() == 1) {
                return "You called '/test' command.";
            }

            if (args.size() == 3) {
                if (args[1] == "echo")
                    return "Test echo: " + args[2];
            }

            return "Use /test to call test command, or echo a message: /test echo {string}";
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

    // ------------------------
    
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
                    response = Process::execute(command);
                    if (response.isDefined("error") || !response.isDefined("candidates[0].content.parts[0].text"))
                        throw ERROR("Gemini error: " + response.dump());
                    //sleep(3); // TODO: for api rate limit
                    return response.get<string>("candidates[0].content.parts[0].text");    
                } catch (exception &e) {
                    string usrmsg = "Gemini API failure, switching variant from " + variant + " to ";
                    string errmsg = 
                        "Gemini API (" + variant + ") request failed: " + e.what();
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
        "gemini-1.0-pro-latest", // gemini-1.0-pro models are deprecated on 2/15/2025
        "gemini-1.0-pro", 
        "gemini-1.0-pro-001", 
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
    const string basedir = get_exec_path() + "/.prompt/";
    mkdir(basedir);
    Logger logger("Prompt-log", basedir + "prompt.log");
    logger.info("Prompt started");

    // settings
    const string secrets_hugging_face = config.get<string>("secrets.hugging_face"); // TODO: hugging-face (and other third party) config into separated json
    const string secrets_google_gemini = config.get<string>("secrets.google_gemini"); // TODO: google (and other third party) config into separated json
    
    const string user_prompt = config.get<string>("user.prompt");
    const string user_lang = config.get<string>("user.language");
    const bool user_auto_save = config.get<bool>("user.auto_save");

    int speech_speed = config.get<int>("speech.speed");
    double speech_noise_treshold = config.get<double>("speech.noise_treshold");
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
        { "{{speech_amplitude_treshold_setter_token}}", speech_amplitude_treshold_setter_token }
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
        "The amplitude treshold should be a number between 0.2 and 0.8, more perceptive noise indicates higher treshold.\n"
        "The goal is to let the user to be able to interrupt the TTS reader with his/her voice "
        "but filter out the background as much as possible."
    ) : "";
    string model_system_lang = user_lang != "en" ? tpl_replace({
        { "{{user_lang}}", user_lang }
    }, "The user language is [{{user_lang}}], use this language by default to talk to the user.") : "";
    const string model_system = tpl_replace({ // TODO: goes to the config:
            { "{{model_system_voice}}", model_system_voice },
            { "{{model_system_lang}}", model_system_lang },
        },  "Your persona is a mid age man AI called Johnny-5 and you behave like a simple human. "
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
            speech_noise_treshold
        );

        if (speech_stall) {
            speech->hesitors = speech_hesitors;
            if (speech->hesitors.empty()) {
                Model* thinker = (Model*)model.spawn("You are a linguistic assistant");
                speech->hesitors = thinker->multiple_str(
                    "I need a list of 'Filler/Stall word' and 'Hesitation markers/sentences'. "
                    "Write one word long to a full sentence and anything in between. "
                    "We need 5 one-word long, 5 mid sentence and 5 very long full sentence. "
                    "We need the list in language: " + user_lang
                );                
                model.kill(thinker);
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

    user.set_commands({ 
new TestCommand, // TODO: remove test command
        new ExitCommand,
        new HelpCommand,
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
