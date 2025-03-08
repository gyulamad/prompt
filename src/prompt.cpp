#include <iostream>    // for cerr, cout
#include <unistd.h>    // for sleep()
#include <algorithm>   // for reverse()
#include <memory>

#include "tools/utils/utils.hpp"
#include "tools/cmd/cmd.hpp"
#include "tools/events/events.hpp"
#include "tools/voice/voice.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::cmd;
using namespace tools::events;
using namespace tools::voice;


class IUserAgent: public BaseEventAgent {
public:
    IUserAgent(const ComponentId& id, Commander& commander, Logger& logger): BaseEventAgent(id), commander(commander), logger(logger) {}
    Commander& getCommanderRef() { return commander; }
    Logger& getLoggerRef() { return logger; }
protected:
    Commander& commander;
    Logger& logger;
};


class ExitCommand : public Command {
public:
    // Define the command pattern(s) this class will match
    vector<string> get_patterns() const override {
        return {"/exit"};
    }

    // Execute the command logic
    string run(void* user_context, const vector<string>& args) override {
        IUserAgent& user = dref<IUserAgent>(user_context);
        Commander& commander = user.getCommanderRef();
        commander.exit();
        return "Exiting...";
    }
};


class UserInputEvent : public TypedEvent<UserInputEvent> {
public:
    UserInputEvent(
        IUserAgent& user, 
        const string& input,
        bool newln
    ): 
        TypedEvent<UserInputEvent>(), 
        user(user),
        input(input + (newln ? "\n" : ""))
    {}

    string getInput() const { return input; }
    IUserAgent& getUserRef() { return user; }

private:
    IUserAgent& user;
    string input;
};

class UserTextInputAgent: public IUserAgent {
public:
    UserTextInputAgent(const ComponentId& id, Commander& commander, Logger& logger): IUserAgent(id, commander, logger) {}

    void run() { // Command loop
        string input;
        Commander& commander = getCommanderRef();
        CommandLine& cline = commander.get_command_line_ref();
        ILineEditor& editor = cline.getEditorRef();
        while (!commander.is_exiting()) {
            try {
                if (editor.Readline(input)) input = "/exit"; // Readline => returns true for Ctrl+C
                if (input.empty()) continue;
                if (input[0] == '/') commander.run_command(this, input); // Try to execute the input as a command
                else publishEvent(make_shared<UserInputEvent>(*this, input, true)); // Publishes to the EventBus
            } catch (exception &e) {
                string errmsg = "Runtime error: " + string(e.what());
                cerr << errmsg << endl;
                logger.err(errmsg);
            }
        }
    }

protected:
    // TODO ...
    // Implement the pure virtual function
    void registerEventInterests() override {
        // Your implementation here
        // You might need to register for specific events
    }

};

class EchoAgent: public BaseEventAgent {
public:
    EchoAgent(const ComponentId& id): BaseEventAgent(id) {}

protected:

    void registerEventInterests() override {
        registerHandler<UserInputEvent>([this](shared_ptr<UserInputEvent> event) {
            string data = event->getInput();
            IUserAgent& user = event->getUserRef();
            Commander& commander = user.getCommanderRef();
            CommandLine& cline = commander.get_command_line_ref();
            ILineEditor& editor = cline.getEditorRef();
            {
                lock_guard<mutex> lock(mtx);
                editor.WipeLine(); // hide user input area (clear the actual line)  
                cout << data << flush;
                editor.RefreshLine(); // show the user input prompt (linenoise readln) so that user can continue typing...
            }
        });
    }

private:
    static mutex mtx;
};
mutex EchoAgent::mtx;


int main(int argc, char *argv[]) {
    run_tests();

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