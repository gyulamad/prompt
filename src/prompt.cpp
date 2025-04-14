#include <map>
#include <string>
#include <functional>

#include "tools/utils/ERROR.hpp"
#include "tools/utils/Test.hpp"
#include "tools/utils/Owns.hpp"
#include "tools/utils/files.hpp"
#include "tools/utils/Settings.hpp"
#include "tools/utils/JSON.hpp"
#include "tools/str/get_absolute_path.hpp"
#include "tools/str/get_filename_only.hpp"
#include "tools/str/tpl_replace.hpp"
#include "tools/voice/MicView.hpp"
#include "tools/voice/ESpeakTTSAdapter.hpp"
#include "tools/voice/WhisperTranscriberSTTSwitch.hpp"
#include "tools/containers/in_array.hpp"

// #include "tools/agency/Agent.hpp"
#include "tools/agency/Agency.hpp"

#include "tools/agency/agents/UserAgent.hpp"
#include "tools/agency/agents/UserAgentInterface.hpp"
#include "tools/agency/agents/ChatbotAgent.hpp"
// #include "tools/agency/agents/TalkbotAgent.hpp"
// #include "tools/agency/agents/DecidorChatbotAgent.hpp"
// #include "tools/agency/agents/EchoAgent.hpp"


#include "tools/cmd/CommandFactory.hpp"
#include "tools/cmd/LinenoiseAdapter.hpp"

#include "tools/agency/agents/commands/HelpCommand.hpp"
#include "tools/agency/agents/commands/ExitCommand.hpp"
#include "tools/agency/agents/commands/ListCommand.hpp"
#include "tools/agency/agents/commands/SpawnCommand.hpp"
#include "tools/agency/agents/commands/KillCommand.hpp"
#include "tools/agency/agents/commands/VoiceCommand.hpp"
#include "tools/agency/agents/commands/TargetCommand.hpp"
#include "tools/agency/agents/commands/LoadCommand.hpp"
#include "tools/agency/agents/commands/SaveCommand.hpp"

// #include "tools/agency/ai/gemini/GeminiChatbot.hpp"
// #include "tools/agency/ai/gemini/GeminiTalkbot.hpp"
// #include "tools/agency/ai/gemini/GeminiDecidorChatbot.hpp"

#include "tools/agency/agents/plugins/GeminiApiPlugin.hpp"
#include "tools/agency/agents/plugins/ChatbotPlugin.hpp"
#include "tools/agency/agents/plugins/TalkbotPlugin.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::str;
using namespace tools::containers;
using namespace tools::cmd;
using namespace tools::agency;
using namespace tools::agency::agents;
using namespace tools::agency::agents::commands;
using namespace tools::agency::agents::plugins;
// using namespace tools::agency::ai;

// TODO: !@# add coverage -> test prompt -> cover more -> fix todo-s -> add response processing plugins -> add tool use plugins (add agency tool: spawn helpers for eg.; add terminal use; add browser use) -> ... -> add vision -> add visual tool use (computer use for e.g)


template<typename PackT>
int safe_main(int argc, char* argv[]) {
    try {
        const string basedir = get_path(__FILE__); // temp only for dev / get from executable filename
        Arguments args(argc, argv);
        JSON conf(file_get_contents(get_absolute_path( // TODO: better config loader
            basedir + "/" + (
                args.has("config") 
                    ? args.get<string>("config")
                    : get_filename_only(args.get<string>(0))
                ) + ".config.json"
        )));
        Settings settings(args, conf);

        // config overrider
        settings.extends(JSON(file_get_contents(basedir + "/gemini.config.json")));


        // -----------------------------------------------------------
        // -----------------------------------------------------------
        // -----------------------------------------------------------


        Owns owns;

        
        Process process;
        ESpeakTTSAdapter tts(
            settings.get<string>("lang"),
            settings.get<int>("tts.speed"), // tts_speed, //int speed, 
            settings.get<int>("tts.gap"), // tts_gap, //int gap,
            settings.get<string>("tts.beep_cmd"), // tts_beep_cmd, //const string& beep_cmd,
            settings.get<string>("tts.think_cmd"), // tts_think_cmd, //const string& think_cmd,
            settings.get<map<string, string>>("tts.speak_replacements"), // tts_speak_replacements,
            &process //Process* process = nullptr
        );

        // Printer printer;
        
        // BasicSentenceSeparation separator(
        //     settings.get<vector<string>>("chatbot.sentence_separators") // talkbot_sentence_separators
        // );
        // SentenceStream sentences(
        //     separator, 
        //     settings.get<size_t>("chatbot.sentences_max_buffer_size") // talkbot_sentences_max_buffer_size
        // );
        AgentRoleMap roles;
        PackQueue<PackT> queue;

        JSON jagency; // TODO: to config (or default agency json):
        jagency.set("recipients", vector<string>({ "user" })); 
        jagency.set("workers", vector<JSON>({}));
        Agency<PackT> agency(owns, roles, queue, "agency");
        agency.fromJSON(jagency);

        vector<Command*> commands;
        CommandFactory cfactory(commands);
        InputPipeInterceptor interceptor;

        LinenoiseAdapter editor(
            settings.get<string>("prompt"), 
            interceptor.getPipeFileDescriptorAt(STDIN_FILENO)
        );
        CommandLine cline(
            editor,
            settings.get<string>("prompt"), // command_list_prompt,
            settings.get<string>("command_line.history_path"), // command_list_history_path,
            settings.get<bool>("command_line.multi_line"), // command_list_multi_line,
            settings.get<size_t>("command_line.history_max_length") //command_list_history_max_length
        );


        WhisperTranscriberSTTSwitch stt_switch(
            settings.get<string>("whisper.model_path"), // whisper_model_path,
            settings.get<string>("whisper.warmup_audio_path"),
            settings.get<size_t>("whisper.warmup_audio_max_length"),
            settings.get<string>("lang"),
            settings.get<double>("stt.voice_recorder.sample_rate"), // stt_voice_recorder_sample_rate,
            settings.get<unsigned long>("stt.voice_recorder.frames_per_buffer"), // stt_voice_recorder_frames_per_buffer,
            settings.get<size_t>("stt.voice_recorder.buffer_seconds"), // stt_voice_recorder_buffer_seconds,
            settings.get<float>("stt.noise_monitor.threshold_pc"), // stt_noise_monitor_threshold_pc,
            settings.get<float>("stt.noise_monitor.rmax_decay_pc"), // stt_noise_monitor_rmax_decay_pc,
            settings.get<size_t>("stt.noise_monitor.window"), // stt_noise_monitor_window,
            settings.get<long>("stt.poll_interval_ms") // stt_poll_interval_ms
        );
        MicView micView;

        Commander commander(cline, cfactory.getCommandsRef(), settings.get<string>("prefix"));

        UserAgentInterface<PackT> interface(
            tts,
            stt_switch, 
            micView,
            commander, 
            interceptor
        );


        // ChatPlugins plugins;
        // ChatbotPlugin<PackT> chatInstructChatPlugin(
        //     settings.get<string>("lang"),
        //     settings.get<string>("chatbot.instruct_persona"),
        //     settings.get<string>("chatbot.instruct_stt"),
        //     settings.get<string>("chatbot.instruct_tts"),
        //     settings.get<string>("chatbot.instruct_lang"),
        //     interface,
        //     sentences,
        //     tts
        // );
        // plugins.push_back(&chatInstructChatPlugin);

        // Map of role strings to factory functions
        roles["chat"] = [&](const string& name, /*vector<string> recipients,*/ const JSON& json = nullptr) {
            ChatHistory* history = owns.allocate<ChatHistory>(
                settings.get<string>("prompt"),
                settings.get<bool>("chatbot.use_start_token") // chatbot_use_start_token
            );

            ChatPlugins* plugins = owns.allocate<ChatPlugins>(owns);

            GeminiApiPlugin* geminiPlugin = owns.allocate<GeminiApiPlugin>(
                    settings.get<string>("gemini.url"),
                    settings.get<string>("gemini.secret"), // gemini_secret,
                    settings.get<string>("gemini.variant"), // gemini_variant,
                    settings.get<vector<string>>("gemini.headers"),
                    settings.get<long>("gemini.timeout"), // gemini_timeout,
                    settings.get<bool>("gemini.verify_ssl"),
                    settings.get<string>("gemini.interruption_feedback")
            );
            plugins->push<GeminiApiPlugin>(geminiPlugin);

            ChatbotPlugin<PackT>* chatPlugin = owns.allocate<ChatbotPlugin<PackT>>(
                owns,
                settings.get<string>("lang"),
                settings.get<string>("chatbot.instruct_persona"),
                settings.get<string>("chatbot.instruct_stt"),
                // settings.get<string>("chatbot.instruct_tts"),
                settings.get<string>("chatbot.instruct_lang"),
                interface
                // sentences,
                // tts
            );
            plugins->push<ChatbotPlugin<PackT>>(chatPlugin);

            BasicSentenceSeparation* separator = owns.allocate<BasicSentenceSeparation>(
                settings.get<vector<string>>("chatbot.sentence_separators") // talkbot_sentence_separators
            );
            SentenceStream* sentences = owns.allocate<SentenceStream>(
                owns,
                separator, 
                settings.get<size_t>("chatbot.sentences_max_buffer_size") // talkbot_sentences_max_buffer_size
            );
            TalkbotPlugin<PackT>* talkPlugin = owns.allocate<TalkbotPlugin<PackT>>(
                owns,
                // settings.get<string>("lang"),
                // settings.get<string>("chatbot.instruct_persona"),
                // settings.get<string>("chatbot.instruct_stt"),
                settings.get<string>("chatbot.instruct_tts"),
                // settings.get<string>("chatbot.instruct_lang"),
                interface,
                sentences,
                tts
            );
            plugins->push<TalkbotPlugin<PackT>>(talkPlugin);

            // GeminiChatbot<PackT>* chatbot = owns.allocate<GeminiChatbot<PackT>>(
            //     owns,
            //     settings.get<string>("gemini.secret"), // gemini_secret,
            //     settings.get<string>("gemini.variant"), // gemini_variant,
            //     settings.get<long>("gemini.timeout"), // gemini_timeout,
            //     name,
            //     // settings.get<string>("chatbot.instructions"),
            //     history,
            //     // interface,
            //     // printer,
            //     plugins,
            //     settings.get<bool>("chatbot.talks")
            //     // sentences, 
            //     // tts
            // );
            Chatbot* chatbot = owns.allocate<Chatbot>(
                owns,
                name,
                history, 
                plugins,
                settings.get<bool>("chatbot.talks")
            );
            ChatbotAgent<PackT>& agent = agency.template spawn<ChatbotAgent<PackT>>(
                owns,
                &agency,
                queue,
                name,
                chatbot,
                // instructions,
                // history,
                interface
            );
            agent.fromJSON(json);
        };

        // roles["talk"] = [&](const string& name, /*vector<string> recipients,*/ const JSON& json = nullptr) {
        //     ChatHistory* history = owns.allocate<ChatHistory>(
        //         settings.get<string>("prompt"),
        //         settings.get<bool>("talkbot.use_start_token") // talkbot_use_start_token
        //     );
        //     GeminiTalkbot* talkbot = owns.allocate<GeminiTalkbot>(
        //         owns,
        //         settings.get<string>("gemini.secret"), // gemini_secret,
        //         settings.get<string>("gemini.variant"), // gemini_variant,
        //         settings.get<long>("gemini.timeout"), // gemini_timeout,
        //         "gemini",
        //         history,
        //         printer,
        //         sentences, // TODO: create it, do not use the same sentences object at each chatbot!!
        //         tts // TODO: tts also should be separated. (different tone/speed or even different kind of speach syntheser)
        //     );
        //     TalkbotAgent<PackT>& agent = agency.template spawn<TalkbotAgent<PackT>>(
        //         owns,
        //         &agency,
        //         queue,
        //         name,
        //         talkbot,
        //         interface
        //     );
        //     agent.fromJSON(json);
        // };

        // roles["decidor"] = [&](const string& name, /*vector<string> recipients,*/ const JSON& json = nullptr) {
        //     ChatHistory* history = owns.allocate<ChatHistory>(
        //         settings.get<string>("prompt"),
        //         settings.get<bool>("chatbot.use_start_token") // chatbot_use_start_token
        //     );
        //     GeminiDecidorChatbot* decidorChatbot = owns.allocate<GeminiDecidorChatbot>(
        //         owns,
        //         settings.get<string>("gemini.secret"), // gemini_secret,
        //         settings.get<string>("gemini.variant"), // gemini_variant,
        //         settings.get<long>("gemini.timeout"), // gemini_timeout,
        //         "gemini",
        //         history,
        //         printer,
        //         settings.get<bool>("chatbot.talks"), 
        //         sentences, 
        //         tts
        //     );
        //     DecidorChatbotAgent<PackT>& agent = agency.template spawn<DecidorChatbotAgent<PackT>>(
        //         owns,
        //         &agency,
        //         queue,
        //         name,
        //         decidorChatbot
        //     );
        //     agent.fromJSON(json);
        // };

        const vector<string> command_factory_commands = settings.get<vector<string>>("command_line.available_commands");
        if (in_array("help", command_factory_commands)) cfactory.withCommand<HelpCommand<PackT>>(commander.getPrefix());
        if (in_array("exit", command_factory_commands)) cfactory.withCommand<ExitCommand<PackT>>(commander.getPrefix());
        if (in_array("list", command_factory_commands)) cfactory.withCommand<ListCommand<PackT>>(commander.getPrefix());
        if (in_array("spawn", command_factory_commands)) cfactory.withCommand<SpawnCommand<PackT>>(commander.getPrefix(), roles);
        if (in_array("kill", command_factory_commands)) cfactory.withCommand<KillCommand<PackT>>(commander.getPrefix());
        if (in_array("voice", command_factory_commands)) cfactory.withCommand<VoiceCommand<PackT>>(commander.getPrefix());
        if (in_array("target", command_factory_commands)) cfactory.withCommand<TargetCommand<PackT>>(commander.getPrefix());
        if (in_array("load", command_factory_commands)) cfactory.withCommand<LoadCommand<PackT>>(commander.getPrefix(), roles);
        if (in_array("save", command_factory_commands)) cfactory.withCommand<SaveCommand<PackT>>(commander.getPrefix(), roles);
        commander.setupCommands();

        string uname = "user"; // TODO: to config
        vector<string> urecipients = { "chat" }; // TODO: to config
        JSON juser;
        juser.set("recipients", urecipients);
        UserAgent<PackT>& user = agency.template spawn<UserAgent<PackT>>(
            owns,
            &agency,
            queue,
            uname,
            interface
        );
        user.fromJSON(juser);
        if (settings.has("startup.batch")) {
            string bkey = settings.get<string>("startup.batch");
            cout << "Loading startup batch: " << bkey << endl;
            vector<string> batch = settings.get<vector<string>>("startup.batches." + bkey);
            user.batch(batch);
        }
        user.async();

        //cout << "Agency started" << endl;
        agency.sync();        

    } catch (exception &e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    run_tests({
       //"test_Agency_spawn_duplicate"
    });
    
    return safe_main<string>(argc, argv);
}
