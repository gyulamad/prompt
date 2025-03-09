// #include <iostream>    // for cerr, cout
// #include <unistd.h>    // for sleep()
// #include <algorithm>   // for reverse()
// #include <memory>

#include "tools/utils/utils.hpp"
#include "tools/cmd/cmd.hpp"
#include "tools/events/events.hpp"
#include "tools/voice/voice.hpp"

#include "tools/utils/Test.hpp"

#include "prompt/agents/IUserAgent.hpp"
#include "prompt/commands/ExitCommand.hpp"
#include "prompt/events/UserInputEvent.hpp"
#include "prompt/agents/UserTextInputAgent.hpp"
#include "prompt/agents/EchoAgent.hpp"


using namespace std;
// using namespace tools::utils;
using namespace tools::cmd;
// using namespace tools::events;
// using namespace tools::voice;

using namespace prompt::agents;
using namespace prompt::commands;
using namespace prompt::events;

#ifdef TEST

// #include "tools/utils/tests/MockLogger.hpp"
// #include "tools/cmd/tests/MockCommander.hpp"

#endif

int safe_main(int argc, char *argv[]) {
    run_tests();
    // run_tests("test_SharedPtrFactory_");
    // run_tests("test_SharedPtrFactory_concurrent_create_strict_with_holders");

    try {
        // TODO: use configs:
        size_t capacity = 100;
        bool async = true;
        long ms = 300;

        const string prompt = "> ";

        Logger logger("applog", "app.log");
        RingBufferEventQueue queue(capacity, logger);
        EventBus bus(async, logger, queue);
        
        LinenoiseAdapter editor(prompt);
        CommandLine cline(editor);
        Commander commander(cline);
        // Register commands
        ExitCommand exitCommand;
        vector<void*> commands = { &exitCommand };
        commander.set_commands(commands);

        shared_ptr userTextInput = make_shared<UserTextInputAgent>("user-text-input", commander, logger);
        shared_ptr echo = make_shared<EchoAgent>("echo");
        
        userTextInput->registerWithEventBus(&bus);
        echo->registerWithEventBus(&bus);

        // Start the input loop
        userTextInput->run();
    } catch (exception &e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    return safe_main(argc, argv);
}