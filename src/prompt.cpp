#include "tools/utils/Test.hpp"

#include "tools/utils/ERROR.hpp"

// #include "tools/utils/utils.hpp"
#include "tools/cmd/cmd.hpp"
#include "tools/voice/voice.hpp"
#include "tools/agency/agency.hpp"

using namespace std;
// using namespace tools::utils;
using namespace tools::cmd;
using namespace tools::agency;
using namespace tools::agency::agents;
using namespace tools::agency::agents::commands;
using namespace tools::voice;


template<typename PackT, typename TranscriberT>
int safe_main(int , char *[]) {
    try {
        const string prompt = "> ";
        const string history_path = "";
        const bool multi_line = true;
        const size_t history_max_length = 0;
        const vector<string> commands = { "exit", "list", "spawn", "kill", "voice" };

        // Map of role strings to factory functions
        map<string, function<Agent<PackT>&(Agency<PackT>&)>> roles = {
            { "echo", [](Agency<PackT>& agency) -> Agent<string>& { return agency.template spawn<EchoAgent<PackT, TranscriberT>>(agency); } },
        };

        CommandFactory cfactory;

        if (in_array("exit", commands)) cfactory.withCommand<ExitCommand<PackT>>();
        if (in_array("list", commands)) cfactory.withCommand<ListCommand<PackT>>();
        if (in_array("spawn", commands)) cfactory.withCommand<SpawnCommand<PackT>>(roles);
        if (in_array("kill", commands)) cfactory.withCommand<KillCommand<PackT>>();
        if (in_array("voice", commands)) cfactory.withCommand<VoiceCommand<PackT, TranscriberT>>();

        LinenoiseAdapter editor(prompt);
        CommandLine cline(
            editor,
            prompt,
            history_path,
            multi_line,
            history_max_length
        );
        Commander commander(cline, cfactory.getCommands());

        PackQueue<PackT> queue;
        Agency<PackT> agency(queue);
        agency.template spawn<EchoAgent<PackT, TranscriberT>>(agency).async();
        agency.template spawn<UserAgent<PackT, TranscriberT>>(agency, &commander).async();
        agency.sync();


    } catch (exception &e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    run_tests({
        
    });
    
    return safe_main<string, WhisperAdapter>(argc, argv);
}