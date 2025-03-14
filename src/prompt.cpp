#include "tools/utils/Test.hpp"

#include "tools/utils/ERROR.hpp"

// #include "tools/utils/utils.hpp"
#include "tools/cmd/cmd.hpp"
// #include "tools/voice/voice.hpp"
#include "tools/agency/agency.hpp"

using namespace std;
// using namespace tools::utils;
using namespace tools::cmd;
using namespace tools::agency;
using namespace tools::agency::agents;
using namespace tools::agency::agents::commands;
// using namespace tools::voice;


int safe_main(int , char *[]) {
    try {
        const string prompt = "> ";
        const string history_path = "";
        const bool multi_line = true;
        const size_t history_max_length = 0;
        const vector<string> commands = { "exit", "list", "spawn", "kill" };

        CommandFactory cfactory;

        if (in_array("exit", commands)) cfactory.withCommand<ExitCommand<string>>();
        if (in_array("list", commands)) cfactory.withCommand<ListCommand<string>>();
        if (in_array("spawn", commands)) cfactory.withCommand<SpawnCommand<string>>();
        if (in_array("kill", commands)) cfactory.withCommand<KillCommand<string>>();

        LinenoiseAdapter editor(prompt);
        CommandLine cline(
            editor,
            prompt,
            history_path,
            multi_line,
            history_max_length
        );
        Commander commander(cline, cfactory.getCommands());

        PackQueue<string> queue;
        Agency<string> agency(queue);
        agency.spawn<EchoAgent<string>>(agency).async();
        agency.spawn<UserAgent<string>>(agency, &commander).async();
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
    
    return safe_main(argc, argv);
}