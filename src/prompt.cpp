#include "tools/utils/Test.hpp"

#include "tools/utils/ERROR.hpp"

#include "tools/utils/utils.hpp"
#include "tools/cmd/cmd.hpp"
#include "tools/voice/voice.hpp"
#include "tools/agency/agency.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::cmd;
using namespace tools::agency;
using namespace tools::agency::agents;
using namespace tools::agency::agents::commands;
using namespace tools::voice;


template<typename PackT>
int safe_main(int , char *[]) {
    try {
        const string prompt = "> ";
        const string lang = "hu";


        const string command_list_history_path = "cline_history.log";
        const bool command_list_multi_line = true;
        const size_t command_list_history_max_length = 0;
        const vector<string> command_factory_commands = { "help", "exit", "list", "spawn", "kill", "voice" };

        const string whisper_model_path = "libs/ggerganov/whisper.cpp/models/ggml-base-q8_0.bin";

        const double stt_voice_recorder_sample_rate = 16000;
        const unsigned long stt_voice_recorder_frames_per_buffer = 512;
        const size_t stt_voice_recorder_buffer_seconds = 5;
        const float stt_noise_monitor_threshold_pc = 0.1;
        const float stt_noise_monitor_rmax_decay_pc = 0.0;
        const size_t stt_noise_monitor_window = 16384;
        const long stt_poll_interval_ms = 30;

        // ----
        const string linenoise_prompt = prompt;
        const string command_list_prompt = prompt;
        const string whisper_lang = lang;


        // Map of role strings to factory functions
        map<string, function<Agent<PackT>&(Agency<PackT>&)>> roles = {
            { "echo", [](Agency<PackT>& agency) -> Agent<string>& { return agency.template spawn<EchoAgent<PackT, WhisperAdapter>>(agency); } },
        };

        CommandFactory cfactory;

        if (in_array("help", command_factory_commands)) cfactory.withCommand<HelpCommand<PackT, WhisperAdapter>>();
        if (in_array("exit", command_factory_commands)) cfactory.withCommand<ExitCommand<PackT>>();
        if (in_array("list", command_factory_commands)) cfactory.withCommand<ListCommand<PackT>>();
        if (in_array("spawn", command_factory_commands)) cfactory.withCommand<SpawnCommand<PackT>>(roles);
        if (in_array("kill", command_factory_commands)) cfactory.withCommand<KillCommand<PackT>>();
        if (in_array("voice", command_factory_commands)) cfactory.withCommand<VoiceCommand<PackT, WhisperAdapter>>();

        InputPipeInterceptor interceptor;

        LinenoiseAdapter editor(linenoise_prompt, interceptor.getPipeFileDescriptorAt(STDIN_FILENO));
        CommandLine cline(
            editor,
            command_list_prompt,
            command_list_history_path,
            command_list_multi_line,
            command_list_history_max_length
        );
        Commander commander(cline, cfactory.getCommands());

        WhisperAdapter whisper(whisper_model_path, whisper_lang.c_str());
        STT<WhisperAdapter> stt(
            whisper,
            stt_voice_recorder_sample_rate,
            stt_voice_recorder_frames_per_buffer,
            stt_voice_recorder_buffer_seconds,
            stt_noise_monitor_threshold_pc,
            stt_noise_monitor_rmax_decay_pc,
            stt_noise_monitor_window,
            stt_poll_interval_ms
        );

        PackQueue<PackT> queue;
        Agency<PackT> agency(queue);
        agency.template spawn<EchoAgent<PackT, WhisperAdapter>>(agency).async();
        agency.template spawn<UserAgent<PackT, WhisperAdapter>>(agency, &commander, &stt, &interceptor).async();
        agency.sync();

    } catch (exception &e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    run_tests({
        //"test_json_selector_empty"
    });
    
    return safe_main<string>(argc, argv);
}