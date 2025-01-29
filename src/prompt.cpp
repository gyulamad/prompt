

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

    class User {
    private:
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

        void save_model(bool override /*= false*/) {
            string model_file = get_model_file();
            if (!override && file_exists(model_file)) 
                throw ERROR("Model already exists, can not override.");
            string errmsg = model.save(model, model_file);
            if (!errmsg.empty()) cout << "Model save error: " << errmsg << endl;
            else cout << "Model saved: " << model_name << endl;
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

        // bool exits() {
        //     return commandLine.is_exited();
        // }

        string prompt(const string& response = "") {
            string resp = trim(response);
            if (!resp.empty()) cout << resp << endl;
            if (speech) {
                if (voice_out && !resp.empty()) speech->say(resp, true);
                if (voice_in) {
                    string input = speech->rec();
                    if (speech->is_rec_interrupted()) voice_in = false;
                    // if (!speech->is_rec_interrupted() && !input.empty()) {
                    //     //cout << commandLine.get_prompt() << input << endl; // emulate command line
                    //     commandLine.show(input + "\n");
                    // }
                    speech->cleanprocs();
                    //commandLine.show(input);
                    return input;
                }
            }

            return commandLine.readln();
        }
        

        void start() {
            CompletionMatcher cmatcher;
            cmatcher.command_patterns = {
                "/help",
                "/exit",
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
            commandLine.set_completion_matcher(cmatcher);

            string response = "";
            while (true) {
                string input = prompt(response); 
                response = "";      
                if (commandLine.is_exited()) break;        
                if (input.empty()) continue;
                else if (input[0] == '/') {
                    bool trlspc;
                    vector<string> input_parts = array_filter(cmatcher.parse_input(input, trlspc, false));
                    input = "";

                    if (input_parts[0] == "/exit") break;

                    if (input_parts[0] == "/help") {
                        cout << "Usages:" << endl;
                        array_dump(cmatcher.command_patterns, false);
                        continue;
                    }

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


                    if (input.empty()) {
                        cout << "Invalid command/arguments or syntax: " << input_parts[0] << endl;
                        continue; 
                    }
                    commandLine.show(input + "\n");
                }

                if (speech) {
                    //model.system_data["{{speech_current_noise_treshold}}"] = to_string(speech->noise_treshold);

                    if (speech->is_say_interrupted()) {
                        // cout << "AI TTS was interrupted" << endl;
                        input = speech_interrupt_info_token + "\n" + input;                    
                    }

                    if (speech) speech->stall();
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
                if (auto_save && !model_name.empty()) save_model(true);
            }
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
                    "I need a list of a minimum 10 'Filler/Stall word' "
                    "and 'Discourse/Hesitation markers' in language: " + user_lang +
                    "\nWrite one word long to a full sentence and anything in between."
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

    user.start();

    if (speech) delete speech;
    return 0;
}


    // array_dump(model.options("All the planets of the solar system?"));
    // array_dump(model.options("All the complementer colors?"));

    // vector<string> planets = model.options("All the planets of the solar system?");
    // vector<string> filtered_planets = model.options("Which are gas planets?", planets);
    // int selected_planet = model.choose("Which is the largest?", planets);
    // int selected_filtered_planets = model.choose("Which is the largest?", filtered_planets);

    // cout << "planets:" << endl;
    // array_dump(planets);
    // cout << "selected_planet: " << selected_planet << endl;

    // cout << "filtered_planets:" << endl;
    // array_dump(filtered_planets);
    // cout << "selected_filtered_planets: " << selected_filtered_planets << endl;

    // cout << "summary:" << endl;
    // array_dump(model.options("summarize what just happend step by step"));


    // cout << "!!!!!!!!!!:" << endl;
    // array_dump(model.options("I need a tetris game but this task is complex. To solve the problem, need to break down smaller steps."));
    
    // cout << "--------------------------------------------------" << endl;
    // cout << model.think("How to you calculate the rist revard ratio of an investment and how to improve it?", 3) << endl;
    // cout << "--------------------------------------------------" << endl;
    // cout << model.prompt("Summarize.") << endl;
    // cout << "--------------------------------------------------" << endl;
    // model.amnesia();
    // cout << model.think("(1732.45 * 64438.25) / 34239.34 = ? Answer as precice as possible", 10) << endl;

    // model.solve("5+5=?");

    // model.solve("
    
    // Create a terminal-based text choose your adventure style  multiplayer game. The game should be very simple and minimalistic, with a graph of locations and items, very basic fight system. Simplify every possible technical solutions. Use C++. the basic idea is, server traks the game, players can send commands to interact with the game world. game contains puzzles and quests that can score up the players the last man stands or the best scored player is the winner. the game design is simple, in the bottom line of the terminal an input prompt (cpp-linenoise) and on the screen scrolls up as event happens by other player interactions. players can interact like go/use/take/talk actions and the actions parameters are item/places/players every action a player does, the others in the same room will be notified. I don't need the full implementation, just a simple game design skeleton of classes (be object oriented)
    // once you are done, then dive deeper and create the full source code implementation for server and clients as well. Remember it should be interactive with a non-blocking game loop so that the client users can be notified when a "real-time" event happens. Use simple websocket server/clients custom wrapper library with a chat demo in C++ that works from linux terminal command line (Use the most simplicistic websoket lib, that is header only lightweight, easy to use (e.g wslay or utilize any usefull linux command if are aweare of any) - Do not use Boost library). No need authentication or special errorhandling etc. this will be only a small example/demonstration, focus on simplicity. The client layout is simply a one line user input (no need line editing or text formatting library, just use the standard I/O), when user hits enter the message sent to the server that forwards the message to the other clients. No need any additional info presented on terminal screen for client, this program is just a proof of concept example for communication. Do not use cmake or other build system, only use the g++ command.

    // Create a simple linux terminal based chat app. that use the bottom line for message sending and the screen to print out and scroll the incoming messages. use the minimalistic header only, simple libraris whenever it's possible. for networking the websocket server is preffered, but DO NOT USE BOOST!! keep simple with the libs. for input message you can use cpp-lineoise and for json nholman::json (if needed) - however, if you keep if as simple as possible you may can avoid json at all. - Do not use cmake or other build system, just use g++ command to build the project for now (we have own build system), use header only inlcudes with .hpp extensions

    // Create an arduino application that works as a usb stick but I can store my passowrd on it and act like a keybord, so that I don't have to memorize nor store my password on the computer. I need a cheepest/simplies design, output screen and buttons to manage/generate/delete/use passwords. I really only need an output (number) which password is selected and I can generate and reuse any time. (other settings, like password complexity etc will be through serial monitor but it's out of scope for now, if you have to use config variable just hard-code them in a central place in the code for now) - no need to be secure, it's only a proof-of-concept and wont be used in real life scenarios
    
    // create a step by step development plan for an esp32 usb keyboard emulator for automatic password typing so that I dont have to store my password on the computer and also don't have to keep them in mind. the esp have to store/generate/delete etc. the passwords and when I push a button it just types in like a keyboard. design a project and a main instruction step to create an MVP proof of concept version. Do not create a production ready system just make something that works for testing and for a better understanding what effort would the full project involve. use espressif lib (Do not utilize Arduino related things)

    // Write a simple linux terminal application in python3 that has a server and clients side, the clients can send a websocket text and the server forward each message to the other clients.

    // Write me a simple websocket server/clients custom wrapper library with a chat demo in C++ that works from linux terminal command line (Use the most simplicistic websoket lib, that is header only lightweight, easy to use (e.g wslay or utilize any usefull linux command if are aweare of any) - Do not use Boost library). No need authentication or special errorhandling etc. this will be only a small example/demonstration, focus on simplicity. The client layout is simply a one line user input (no need line editing or text formatting library, just use the standard I/O), when user hits enter the message sent to the server that forwards the message to the other clients. No need any additional info presented on terminal screen for client, this program is just a proof of concept example for communication. Do not use cmake or other build system, only use the g++ command.

    // Write me a simple socket server/clients custom wrapper library with a chat demo in C++ that works from linux terminal command line (Use the most simplicistic socket lib(s) that is header only lightweight, easy to use (or utilize any usefull linux command if you are aweare of any) - Do not use Boost library). No need authentication or special errorhandling etc. this will be only a small example/demonstration, focus on simplicity. The client layout is simply a one line user input (no need line editing or text formatting library, just use the standard I/O), when user hits enter the message sent to the server that forwards the message to the other clients. No need any extre info presented on terminal screen for clients, only show the received message. We need multiple client so I guess the client input should be non-blocking or you will need some sort of thread-management this program is just a proof of concept example for communication. Do not use cmake or other build system, only use the g++ command.

/*
1: 
1: locations are a simple tree (or graph) each location connected with other locations and the user can choose where to go from a list. 2 - A simple system might use string comparisons is how puzzles be defined 3 - Handling disconnections, invalid input, and potential exploits is crucial; 4: mechanism will be used to synchronize game? - simple server - clients broadcast I guess, or the simplest as possible. 5: - libraries or frameworks: minimal libraries, lightweigt, header only libs always prefered - json encoder/decoder we already have implemented, we can re-use that. 6: simple command-line interface - we already has a cpp-linenoise utilized, we can reuse it's wrapper here. 7 - for managing items (inventory, item properties, item interactions): I think classes would be better, we like the object oriented code.

1: player movement and location updates be handled as simple as possible, (erver constantly track and update every player's location I guess). 2: I am not sure What strategies will be used to prevent cheating but my idea is that the users are just sending and receiving simple commands and the server responses and notifies when action happens. everything is managed on the server so the only thing user can not send too many command to the server (ddos prevention, or too many action is taken) so that you can not automate game actions pragmatically to speed up your character for eg, - 3: handle disconnections gracefully: we are developing MVC now, simple soution I think if user disconnect then the game keep it state for a while (configurable) and if the player dont connecting back the "dies/disappear" from the game (which is an other action, close players should be notified by the server) - maybe dead players items are leaving in the location so others can collect. 4 - testing: unit tests are essencial (must), integration tests is a (could), - I don't know what is the system testing you mean? - for now unit testing with simple cassert test will be oke, integration testing also can go with cassert if we emulate a network environment locally, if that is a good idea? - 5: the estimated development: solo dev. --- Note: always choose the more simple soltion in the future, related to anything

------------------
--- Error Handling Depth: lmore basic approach? - any error happens just throw an exception and we will polishing..
--- Concurrency Model (1.1): simpler model (e.g., sequential processing of client requests)
--- Data Structures (1.2 and 1.3): Simple `std::vector`
--- Network Protocol (1.4 and 1.5): simple string or JSON.
--- Testing Strategy (Phase 1): minimal testing first, then more test cases when we find out concrete expectations and bugs
--- Will a mocking framework be used to isolate units under test? - No, so DO NOT USE static or global data but always open the possibility for dependency injection

--- Client-Side Architecture: users should recieve messages from the server to get notified when an action taken or event happens on the server so minimum one extra thread necessary (but I don't see why it would be a big issue)
--- Command Parsing Robustness: simple error messages will suffice (later extends - for testing a simple universal validation error message enough. like "synax error")
--- JSON Data Structure: sumple rest json, {"data":"....."}, or {"error": "....."}
--- Client-Side Update Handling: simply display updates as they arrive. Note. it's just a simpe text game. scroll up the text as it receives while a simple command line sends from the bottom of the screen.
--- Network Latency Considerations: client command messages handled as arrived. eg. two player pick up the same item, first recive a message "item taken to your inventory", second only informed as "There is no item..." (as they are probably in the same location the server sould notify the second anyway, like "X player picked up the Y item." - just almost in the same time, so the second will now he/she wasnt fast enough)
--- Testing Strategy (Phase 2): in this phase a simple unit/integration tests (with minimal localhost server enough)

--- Puzzle Design Specificity && Item Interaction Complexity:: concrete example of the "very simple puzzle" planned for this phase: A user pick up a "key" item and brings it to a location where the exit door is and use it. or, the user collect all the 10 hidden gold. or, users should be able to fight with simple formatted commands like "<action> <item>: <object/player>", for eg "use knife player-xy" just like other items can be used: "use key exit-door" other actions takes other parameters, for eg: "go starways" or "take key" or "drop knife" or some command no need parameters like "search" => and returns "Here is a rope and a knife." etc..
--- Game State Update Mechanics: simple boolean flags. actually what I am planning is the game map contains a certain amount of pussels, each for X score per complexity and the players who get more score until the last puzzle, wins and the game restarts.
--- 

 */


/*
Should the example demonstrate a simple echo server, or a more complex interaction?
A simple echo server (This aligns with the request for a minimalistic example).
What operating system should the example be designed for (e.g., Linux, Windows)?  This impacts socket handling.
POSIX-compliant systems (Linux, macOS, BSD) - This simplifies socket handling and avoids the complexities of Windows sockets.
Should the code include any comments explaining its functionality?
Yes, comments are especially important in this case because the example is simplified and omits many details found in a production-ready WebSocket server.  Comments help clarify what's missing and why.
What is the desired level of code formatting (e.g., use of spaces vs. tabs, consistent indentation)?
Consistent indentation using spaces (e.g., 4 spaces per indentation level). This enhances readability and is generally preferred in C++ code.  It avoids potential issues with tab interpretation across different editors and systems.

> yes
src/prompt.cpp:268 - Current task:
yes
Thinking of a solution... (deep thinking: 2)
This program implements a minimalistic WebSocket echo server using the wslay library.  It handles only one client at a time and includes minimal error handling.  It's designed for POSIX-compliant systems.

```cpp
#include <iostream>
#include <string>
#include <unistd.h> // for close
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <wslay/wslay.h>

// Minimal error handling:  Print to stderr and exit.
void handle_error(const std::string& msg) {
    std::cerr << "Error: " << msg << std::endl;
    exit(1);
}


int main() {
    // Create a socket.
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) handle_error("Could not create socket");

    // Set up the server address.
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(8080); // Choose a port

    // Bind the socket to the address.
    if (bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        handle_error("Could not bind socket");

    // Listen for incoming connections.
    if (listen(sockfd, 1) < 0) handle_error("Could not listen on socket");

    // Accept a connection.  Only handles one client at a time.
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sockfd = accept(sockfd, (sockaddr*)&client_addr, &client_len);
    if (client_sockfd < 0) handle_error("Could not accept connection");

    //Close listening socket after accepting a client.
    close(sockfd);

    // WebSocket handshake (simplified - missing error handling and full handshake logic).
    //  In a real application you'd use a proper WebSocket library to handle this.
    char buffer[1024];
    int bytes_received = recv(client_sockfd, buffer, sizeof(buffer), 0);
    if (bytes_received < 0) handle_error("Could not receive handshake");
    //  ... (WebSocket handshake processing would go here) ...

    //Simple echo loop.
    while (true) {
        bytes_received = recv(client_sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) break; // Connection closed

        //Send the received data back to the client.
        if (send(client_sockfd, buffer, bytes_received, 0) < 0)
            handle_error("Could not send data");
    }


    close(client_sockfd);
    std::cout << "Client disconnected." << std::endl;
    return 0;
}
```

To compile and run this code:

1.  **Install wslay:**  You'll need to install the wslay library.  The installation method depends on your system (e.g., `apt-get install libwslay-dev` on Debian/Ubuntu, `brew install wslay` on macOS with Homebrew).
2.  **Compile:** `g++ -Wall -o websocket_chat websocket_chat.cpp -lwslay`
3.  **Run:** `./websocket_chat`

Then connect to `ws://localhost:8080` with a WebSocket client (like a browser's developer tools or a dedicated WebSocket client).  Remember that this is a drastically simplified example and lacks robust error handling, security measures, and a proper WebSocket handshake implementation.  It's only intended as a minimal demonstration.  For production, use a more mature WebSocket library with complete handshake handling and proper error management.
*/

/*
This client uses the `wslay` library as well, mirroring the server's simplicity.  It's equally minimal and lacks robust error handling and full handshake processing.  Remember that this is for demonstration purposes only;  a production client would require much more comprehensive error handling and features.


```cpp
#include <iostream>
#include <string>
#include <unistd.h> // for close
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <wslay/wslay.h>


void handle_error(const std::string& msg) {
    std::cerr << "Error: " << msg << std::endl;
    exit(1);
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) handle_error("Could not create socket");

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Connect to localhost
    serv_addr.sin_port = htons(8080);

    if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        handle_error("Could not connect to server");

    //Simplified WebSocket handshake (missing crucial parts)
    //In a real application, you'd use a proper WebSocket library to handle this.
    //For now we'll just send a dummy handshake.  This will likely fail with a real server.
    std::string handshake = "GET / HTTP/1.1\r\n"
                            "Host: localhost:8080\r\n"
                            "Upgrade: websocket\r\n"
                            "Connection: Upgrade\r\n"
                            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n" // Replace with a proper key
                            "\r\n";
    send(sockfd, handshake.c_str(), handshake.length(), 0);


    char buffer[1024];
    std::string message;
    while (true) {
        std::cout << "Enter message (or type 'quit' to exit): ";
        std::getline(std::cin, message);
        if (message == "quit") break;

        send(sockfd, message.c_str(), message.length(), 0);

        int bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) break; //Connection closed
        buffer[bytes_received] = '\0'; //Null-terminate the received data
        std::cout << "Received: " << buffer << std::endl;
    }

    close(sockfd);
    return 0;
}
```

**To compile and run:**

1.  Make sure you have `wslay` installed (same as the server).
2.  Compile: `g++ -Wall -o websocket_client websocket_client.cpp -lwslay`
3.  Run: `./websocket_client`

**Important Considerations:**

*   **Incomplete Handshake:** Both the client and server drastically simplify the WebSocket handshake.  A real implementation needs to follow the WebSocket protocol precisely, including key generation, acceptance, and security considerations.
*   **Error Handling:**  The error handling is rudimentary.  A production-ready client needs far more robust error checking and handling.
*   **No Framing:**  The code doesn't handle WebSocket framing (masking, length encoding, etc.).  This is a critical part of the WebSocket protocol.
*   **Security:** The example lacks any security measures (like TLS/SSL).  Never use this code in a production environment without proper security.

This improved client, like the server, is still a minimal example, but it provides a more complete (though still very basic) illustration of how to interact with a WebSocket server using `wslay`.  For anything beyond simple testing, use a mature, well-tested WebSocket library.
 */

/*
The previous examples, while demonstrating basic socket communication, did *not* actually use the `wslay` library for WebSocket handling.  They only used sockets for a rudimentary connection and message passing.  `wslay` handles the complexities of the WebSocket protocol (framing, handshake, etc.), which was entirely missing from the previous code.  Using `wslay` correctly requires a significant rewrite.

Creating a multi-client server with `wslay` that's still relatively simple is challenging. `wslay` itself is not designed for concurrent handling of multiple clients; it's a low-level library.  To manage multiple clients concurrently, you need to incorporate an event loop or multithreading.  Below, I'll provide a basic example using threads.  This is still simplified and lacks robust error handling and some features a production server would need.

**Server (Multi-Client, Threaded, using wslay - Simplified):**

```cpp
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <wslay/wslay.h>

// Error handling (still minimal)
void handle_error(const std::string& msg) {
    std::cerr << "Error: " << msg << std::endl;
    exit(1);
}

//Simplified Echo Handler for a single client connection
void handle_client(int client_sockfd) {
  //This is a VERY simplified example.  Real-world usage requires significantly more error handling and proper WebSocket handling with wslay.

    char buffer[1024];
    while (true) {
        int bytes_received = recv(client_sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) break; // Connection closed

        if (send(client_sockfd, buffer, bytes_received, 0) < 0) {
            handle_error("Could not send data");
        }
    }
    close(client_sockfd);
    std::cout << "Client disconnected." << std::endl;
}


int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) handle_error("Could not create socket");

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(8080);

    if (bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        handle_error("Could not bind socket");

    if (listen(sockfd, 10) < 0) handle_error("Could not listen on socket");


    std::vector<std::thread> clients;
    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sockfd = accept(sockfd, (sockaddr*)&client_addr, &client_len);
        if (client_sockfd < 0) {
            std::cerr << "Error accepting connection" << std::endl; //Don't exit on accept error
            continue;
        }
        clients.emplace_back(handle_client, client_sockfd);
    }

    //This will never be reached in this simplified example.  In a real app you'd need to manage client threads gracefully.
    for (auto& t : clients) {
        t.join();
    }

    close(sockfd);
    return 0;
}
```

**Client (remains largely unchanged - still lacks proper wslay integration):**

The client code from before is still largely appropriate, but *it doesn't use `wslay`*.  Integrating `wslay` into the client would require substantial changes to properly handle the WebSocket handshake and framing. This would make the example significantly more complex.


**Crucial Missing Parts:**

*   **Proper WebSocket Handshake with `wslay`:** The server and client are still missing the proper WebSocket handshake using the `wslay` functions.  This is a *critical* component for WebSocket communication.
*   **WebSocket Framing with `wslay`:**  The code doesn't handle WebSocket framing (masking, opcodes, length encoding) which `wslay` is designed to manage.
*   **Robust Error Handling:** The error handling remains very minimal.  A production server needs far more comprehensive error checks and handling.
*   **Thread Management:** The server's thread management is extremely basic.  In a real application, you need mechanisms to gracefully handle client disconnections and thread cleanup.
*   **Security (TLS/SSL):**  The example lacks any security, which is crucial for any production-level WebSocket server.


This multi-client example provides a *very high-level* sketch of how you could structure a multi-client server.  Using `wslay` effectively for a production-ready WebSocket server requires a much deeper understanding of the library and the WebSocket protocol.  For production, consider using a higher-level WebSocket library that handles the complexities for you.  This example is mainly for illustrative purposes to show the basic threading structure.  Do not deploy this in a production environment.
 */