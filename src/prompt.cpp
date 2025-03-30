#include <map>
#include <string>
#include <functional>

#include "tools/utils/ERROR.hpp"
#include "tools/utils/Test.hpp"
#include "tools/utils/Factory.hpp"
#include "tools/voice/MicView.hpp"
#include "tools/voice/ESpeakTTSAdapter.hpp"
#include "tools/voice/WhisperTranscriberSTTSwitch.hpp"
#include "tools/containers/in_array.hpp"

#include "tools/agency/Agent.hpp"
#include "tools/agency/Agency.hpp"
#include "tools/agency/agents/ChatbotAgent.hpp"
#include "tools/agency/agents/TalkbotAgent.hpp"
// #include "tools/agency/agents/EchoAgent.hpp"
#include "tools/agency/agents/UserAgent.hpp"
#include "tools/agency/agents/UserAgentInterface.hpp"


#include "tools/cmd/CommandFactory.hpp"
#include "tools/cmd/LinenoiseAdapter.hpp"

#include "tools/agency/agents/commands/HelpCommand.hpp"
#include "tools/agency/agents/commands/ExitCommand.hpp"
#include "tools/agency/agents/commands/ListCommand.hpp"
#include "tools/agency/agents/commands/SpawnCommand.hpp"
#include "tools/agency/agents/commands/KillCommand.hpp"
#include "tools/agency/agents/commands/VoiceCommand.hpp"
#include "tools/agency/agents/commands/TargetCommand.hpp"

#include "tools/ai/GeminiChatbot.hpp"
#include "tools/ai/GeminiTalkbot.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::containers;
using namespace tools::cmd;
using namespace tools::agency;
using namespace tools::agency::agents;
using namespace tools::agency::agents::commands;
using namespace tools::ai;


template<typename PackT>
int safe_main(int , char *[]) {
    try {
        const string prompt = "> ";
        const string lang = "hu";


        const string command_list_history_path = "cline_history.log";
        const bool command_list_multi_line = true;
        const size_t command_list_history_max_length = 0;
        const vector<string> command_factory_commands = { 
            "help", "exit", "list", "spawn", "kill", "voice", "target"
        };

        const string whisper_model_path = "libs/ggerganov/whisper.cpp/models/ggml-base-q8_0.bin";

        const double stt_voice_recorder_sample_rate = 16000;
        const unsigned long stt_voice_recorder_frames_per_buffer = 512;
        const size_t stt_voice_recorder_buffer_seconds = 5;
        const float stt_noise_monitor_threshold_pc = 0.1;
        const float stt_noise_monitor_rmax_decay_pc = 0.0;
        const size_t stt_noise_monitor_window = 16384;
        const long stt_poll_interval_ms = 30;

        const bool chatbot_use_start_token = false;
        const vector<string> chatbot_sentence_separators = {".", "!", "?"};
        const long chatbot_sentences_max_buffer_size = 1024*1024;

        const string gemini_secret = "AIzaSyDabZvXQNSyDYAcivoaKhSWhRmu9Q6hMh4";
        const string gemini_variant = "gemini-1.5-flash-8b";
        const long gemini_timeout = 30000;

        // ----
        const string linenoise_prompt = prompt;
        const string command_list_prompt = prompt;
        const string chatbot_prompt = prompt;
        const string whisper_lang = lang;


        Process process;
        ESpeakTTSAdapter tts(
            lang,
            200, //int speed, 
            0, //int gap,
            "sox -v 0.03 beep.wav -t wav - | aplay -q -N", //const string& beep_cmd,
            "find sounds/r2d2/ -name \"*.wav\" | shuf -n 1 | xargs -I {} bash -c 'sox -v 0.01 \"{}\" -t wav - | aplay -q -N'", //const string& think_cmd,
            {
                { "...", "\n.\n.\n.\n" },
                { "***", "\n.\n.\n.\n" },
                { "**", "\n.\n.\n" },
                { "*", "\n.\n" },
                { "'", "" },
            }, //const map<string, string>& speak_replacements,
            &process //Process* process = nullptr
        );

        Printer printer;
        
        BasicSentenceSeparation separator(
            chatbot_sentence_separators
        );
        SentenceStream sentences(
            separator, 
            chatbot_sentences_max_buffer_size
        );

        PackQueue<PackT> queue;
        

        Factory<ChatHistory> histories;
        histories.registry("ChatHistory", [&](void*) -> ChatHistory* {
            return new ChatHistory(
                chatbot_prompt, 
                chatbot_use_start_token
            );
        });

        Factory<Chatbot> chatbots;
        chatbots.registry("GeminiChatbot", [&](void*) -> GeminiChatbot* {
            ChatHistory* history = histories.create("ChatHistory");
            return new GeminiChatbot(
                gemini_secret,
                gemini_variant,
                gemini_timeout,
                "gemini",
                *history,
                printer
            );
        });

        Factory<Talkbot> talkbots;
        talkbots.registry("GeminiTalkbot", [&](void*) -> GeminiTalkbot* {
            ChatHistory* history = histories.create("ChatHistory");
            return new GeminiTalkbot(
                gemini_secret,
                gemini_variant,
                gemini_timeout,
                "gemini",
                *history,
                printer,
                sentences,
                tts
            );
        });

        Agency<PackT> agency(
            queue, "agency", { "user" }, 
            chatbots, histories
        );

        CommandFactory cfactory;
        InputPipeInterceptor interceptor;

        LinenoiseAdapter editor(linenoise_prompt, interceptor.getPipeFileDescriptorAt(STDIN_FILENO));
        CommandLine cline(
            editor,
            command_list_prompt,
            command_list_history_path,
            command_list_multi_line,
            command_list_history_max_length
        );


        WhisperTranscriberSTTSwitch stt_switch(
            whisper_model_path,
            whisper_lang,
            stt_voice_recorder_sample_rate,
            stt_voice_recorder_frames_per_buffer,
            stt_voice_recorder_buffer_seconds,
            stt_noise_monitor_threshold_pc,
            stt_noise_monitor_rmax_decay_pc,
            stt_noise_monitor_window,
            stt_poll_interval_ms
        );
        MicView micView;
        // UserAgentInterface<PackT>* interface_ptr;

        ChatHistory* history = histories.create("ChatHistory");
        Talkbot* talkbot = talkbots.create("GeminiTalkbot");
        // GeminiTalkbot talkbot(
        //     gemini_secret,
        //     gemini_variant,
        //     gemini_timeout,
        //     "gemini",
        //     history,
        //     printer,
        //     sentences,
        //     tts
        // );

        // Map of role strings to factory functions
        AgentRoleMap<PackT> roles = {
            // {   // TODO: this one we dont need here it's just an experimental example until we add more agent
            //     "echo", [&](Agency<PackT>& agency, const string& name, vector<string> recipients) -> Agent<PackT>& {
            //         return agency.template spawn<EchoAgent<PackT>>(name, recipients, *interface_ptr);
            //     }
            // },

            {
                "chat", [&](Agency<PackT>& agency, const string& name, vector<string> recipients) -> Agent<PackT>& {
                    ChatHistory* history = agency.histories.create("ChatHistory");
                    Chatbot* chatbot = agency.chatbots.create("GeminiChatbot");
                    return agency.template spawn<ChatbotAgent<PackT>>(name, recipients, *chatbot);
                },

            },
            {
                "talk", [&](Agency<PackT>& agency, const string& name, vector<string> recipients) -> Agent<PackT>& {
                    return agency.template spawn<TalkbotAgent<PackT>>(name, recipients, *talkbot);
                },
            },
        };
        if (in_array("help", command_factory_commands)) cfactory.withCommand<HelpCommand<PackT>>();
        if (in_array("exit", command_factory_commands)) cfactory.withCommand<ExitCommand<PackT>>();
        if (in_array("list", command_factory_commands)) cfactory.withCommand<ListCommand<PackT>>();
        if (in_array("spawn", command_factory_commands)) cfactory.withCommand<SpawnCommand<PackT>>(roles);
        if (in_array("kill", command_factory_commands)) cfactory.withCommand<KillCommand<PackT>>();
        if (in_array("voice", command_factory_commands)) cfactory.withCommand<VoiceCommand<PackT>>();
        if (in_array("target", command_factory_commands)) cfactory.withCommand<TargetCommand<PackT>>();
        Commander commander(cline, cfactory.getCommands());
        UserAgentInterface<PackT> interface(
            tts,
            stt_switch, 
            micView,
            commander, 
            interceptor
        );
        // interface_ptr = &interface;

        agency.template spawn<TalkbotAgent<PackT>>("talk", { "user" }, *talkbot).async();
        agency.template spawn<UserAgent<PackT>>("user", { "talk" }, agency, interface).async();
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
        // "test_Agency_kill_basic"
    });
    
    return safe_main<string>(argc, argv);
}
