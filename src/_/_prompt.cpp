#include "../libs/yhirose/cpp-linenoise/linenoise.hpp"

#include "tools/Test.hpp"
#include "tools/ERROR.hpp"
#include "tools/io.hpp"
#include "tools/files.hpp"
#include "tools/strings.hpp"
#include "tools/vectors.hpp"
#include "tools/Process.hpp"
#include "tools/JSON.hpp"
#include "tools/Process.hpp"
#include "tools/Logger.hpp"
#include "tools/system.hpp"
#include "tools/Arguments.hpp"
#include "tools/Tasks.hpp"
#include "tools/cmd/Commander.hpp"

#include "Tool.hpp"
#include "Model.hpp"
#include "User.hpp"
#include "Commands.hpp"
#include "Tools.hpp"
#include "Gemini.hpp"

using namespace std;
using namespace tools;
using namespace tools::cmd;

namespace prompt {

    // class Terminal {
    // private:
    //     string outputs;
    // public:
    //     // triggered by [SEND-TO-TERMINAL]
    //     void send(const string& reason, const string& input) {

    //     }

    //     // triggered by [RESET-TERMINAL]
    //     void reset() {

    //     }
    // };

    // class Task {
    // public:
    //     enum Status { TODO, IN_PROGRESS, DONE, FAIL };
    // private:
    //     const string id;
    //     const string objective;  
    //     Status status;
    //     vector<string> results;
    // public:
    //     // triggered by [TASK-UPDATE]
    //     void update(const string& id, Status* status = nullptr, const string* result = nullptr) {

    //     }
    // };

    // class Agent {
    // private:
    //     const Agent* owner = nullptr;
    //     const string name;
    //     vector<Agent*> childs;
    //     Task task;
    //     Terminal terminal;

    //     // triggered by [SENT-TO-PARENT]
    //     void report(const string& response) {
            
    //     }

    //     // triggered by [SPAWN-ASSISTANT]
    //     void spawn(const string& name, const Task& task) {

    //     }

    //     // triggered by [SENT-TO-ASSISTANT]
    //     void command(const Agent& children, const string& request) {

    //     }

    //     // triggered by [KILL-ASSISTANT]
    //     void kill(const string& name) {

    //     }

    // public:

    // };

    // ----------------------------
    
}

using namespace prompt;

int main(int argc, char *argv[]) {
    run_tests();

    // args
    Arguments args(argc, argv);
    const bool voice = args.getBool("voice");
    const string model_name = args.has("model") ? args.getString("model") : "";

    // configs
    JSON config(file_get_contents(get_exec_path() + "/../config.json"));
    JSON config_gemini(file_get_contents(get_exec_path() + "/../config.gemini.json"));

    // logger
    const string basedir = get_exec_path() + "/.prompt";
    mkdir(basedir);
    Logger logger("Prompt-log", basedir + "/prompt.log");
    logger.info("Prompt started");

    // settings
    const string gemini_secret = config_gemini.get<string>("secret");
    const vector<string> gemini_variants = config_gemini.get<vector<string>>("variants");
    const size_t gemini_current_variant = config_gemini.get<size_t>("current_variant");
    const int gemini_err_retry_sec = config_gemini.get<size_t>("err_retry_sec");
    const int gemini_err_attempts = config_gemini.get<size_t>("err_attempts");
    const vector<string> gemini_sentence_delimiters = config_gemini.get<vector<string>>("sentence_delimiters");
    const long gemini_stream_request_timeout = config_gemini.get<long>("stream_request_timeout");
    const string gemini_tmpfile = config_gemini.get<string>("tmpfile");

    const string user_prompt = config.get<string>("user.prompt");
    const string user_lang = config.get<string>("user.language");
    const bool user_auto_save = config.get<bool>("user.auto_save");
    const prompt::mode_t user_mode = get_mode(config.get<string>("user.mode"));
    const bool user_stream = config.get<bool>("user.stream");

    const bool speech_stall = config.get<bool>("speech.stall");  
    // const long long speech_speak_wait_ms = config.get<long long>("speech.speak_wait_ms");
    const vector<string> speech_ignores_rgxs = config.get<vector<string>>("speech.ignores_rgxs");
    const long long speech_impatient_ms = config.get<long long>("speech.impatient_ms");
    const bool speech_tts_voice_out = voice && config.get<bool>("speech.tts_voice_out");
    const int speech_tts_speed = config.get<int>("speech.tts.speed");
    const int speech_tts_gap = config.get<int>("speech.tts.gap");
    const string speech_tts_beep_cmd = config.get<string>("speech.tts.beep_cmd");
    const string speech_tts_think_cmd = config.get<string>("speech.tts.think_cmd");
    const map<string, string> speech_tts_speak_replacements = config.get<map<string, string>>("speech.tts.speak_replacements");
    const bool speech_stt_voice_in = voice && config.get<bool>("speech.stt_voice_in");
    const double speech_stt_voice_recorder_sample_rate = config.get<double>("speech.stt.voice_recorder.sample_rate");
    const unsigned long speech_stt_voice_recorder_frames_per_buffer = config.get<unsigned long>("speech.stt.voice_recorder.frames_per_buffer");
    const size_t speech_stt_voice_recorder_buffer_seconds = config.get<size_t>("speech.stt.voice_recorder.buffer_seconds");
    const float speech_stt_noise_monitor_threshold_pc = config.get<float>("speech.stt.noise_monitor.threshold_pc");
    const float speech_stt_noise_monitor_rmax_decay_pc = config.get<float>("speech.stt.noise_monitor.rmax_decay_pc");
    const size_t speech_stt_noise_monitor_window = config.get<size_t>("speech.stt.noise_monitor.window");
    const string speech_stt_transcriber_model = config.get<string>("speech.stt.transcriber.model");
    const long speech_stt_poll_interval_ms = config.get<long>("speech.stt.poll_interval_ms");


    const size_t model_conversation_length_max = config.get<size_t>("model.memory_max");
    const double model_conversation_loss_ratio = config.get<double>("model.memory_loss_ratio");
    const int model_think_steps = config.get<int>("model.think_steps");
    const int model_think_deep = config.get<int>("model.think_deep");

    const JSON model_tools_config = config.get<JSON>("tools");

    string model_system_voice = voice ? tpl_replace({
    },  "The user is using a text-to-speech software for communication. "
        "You are taking into account that the user's responses are being read at loud by a text-to-speech program "
        "that can be interrupted by background noise or by the user itself."
        "Repeated interruption changes how you act, "
        "your responses are becaming more consise and short when you interupted more often recently "
        "but you can put more context otherwise if it's necessary, tune your response style accordingly. "
    ) : "";
    
    string model_system_lang = user_lang != "en" ? tpl_replace({
        { "{{user_lang}}", user_lang }
    }, "The user language is [{{user_lang}}], use this language by default to talk to the user.") : "";



    vector<Tool> model_tools = {
        // nothingTool,
        dateTimeTool,
        googleSearchTool,
        webBrowserTool,
        // bashCommandTool,
        fileManagerTool,
    };

    string model_system_tools = model_tools.empty() ? "" : tpl_replace({
        { "{{model_tools}}", to_string(model_tools) },
    }, 
    R"(
You are a helpful and humorous AI assistant designed to provide concise and accurate responses, considering that the user is communicating via text-to-speech.

When you need to perform a real-world action (like searching the web), you MUST use the provided `function_calls` format. This format is CRUCIAL for your functionality.

**Function Call Format:**

*   A `function_calls` block will be provided to you.
*   The `function_calls` block MUST be enclosed within these tokens:
    *   `[FUNCTION-CALLS-START]` (at the very beginning of the block)
    *   `[FUNCTION-CALLS-STOP]` (at the very end of the block)
*   Inside the `function_calls` block, you will have a JSON structure. This JSON structure will contain the following:
    *   A field called `function_calls` which is an array of function calls.
    *   Each function call is an object that must contain a `function_name`.
    *   Each function call can have parameters like `query`, `max`, etc.
    *   Example:

[FUNCTION-CALLS-START]
{
  "function_calls": [
    {
      "function_name": "google_search",
      "query": "best restaurants in London",
      "max": 3
    }
  ]
}
[FUNCTION-CALLS-STOP]

*   **IMPORTANT:** If you need to perform a function call, and you fail to enclose your JSON in the tokens or the structure is incorrect, you will be considered to have failed the action!

**In short:** ALWAYS use the `[FUNCTION-CALLS-START]` and `[FUNCTION-CALLS-STOP]` tags and ALWAYS format your calls as JSON. No exceptions.    
    )"
        "You can use the following function calls to perform 'real-life' actions:"
        "\n\n{{model_tools}}\n\n"
        "Note:\n"
        "**Avoid triple quotes (```json...```) when using function calls.**\n"
        "**Always double-check the JSON syntax for errors.**\n"
    );

    const string model_system = tpl_replace({ // TODO: goes to the config:
            { "{{model_system_voice}}", model_system_voice },
            { "{{model_system_lang}}", model_system_lang },
            { "{{model_system_tools}}", model_system_tools },
        },  "Your persona is a mid age man like AI and you behave like a simple human. "
            "You have a sense of humor, your personality is entertaining. "
            "Your answers are succinct and focusing on the core of your conversation "
            "but in a way like a normal human chat would looks like. "
            "You always helpful but also concise in answers.\n"
            "{{model_system_voice}}\n"
            "{{model_system_lang}}\n"
            "{{model_system_tools}}\n"
    );


    Gemini model(
        logger,
        gemini_secret,
        gemini_variants,
        gemini_current_variant,
        gemini_err_retry_sec,
        gemini_err_attempts,
        gemini_sentence_delimiters,
        gemini_stream_request_timeout,
        gemini_tmpfile,
        model_system,
        model_tools,
        model_tools_config,
        model_conversation_length_max,
        model_conversation_loss_ratio,
        model_think_steps,
        model_think_deep
    );

    // model.set_tools(tools);



    User user(
        model,
        model_name, 
        user_lang,
        user_auto_save,
        user_mode,
        user_stream,
        user_prompt,
        basedir,
        speech_stall,
        speech_ignores_rgxs,
        speech_impatient_ms,
        speech_tts_speed,
        speech_tts_gap,
        speech_tts_beep_cmd,
        speech_tts_think_cmd,
        speech_tts_speak_replacements,
        speech_stt_voice_in,
        speech_stt_voice_recorder_sample_rate,
        speech_stt_voice_recorder_frames_per_buffer,
        speech_stt_voice_recorder_buffer_seconds,
        speech_stt_noise_monitor_threshold_pc,
        speech_stt_noise_monitor_rmax_decay_pc,
        speech_stt_noise_monitor_window,
        speech_stt_transcriber_model,
        speech_stt_poll_interval_ms
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
    
    return 0;
}
