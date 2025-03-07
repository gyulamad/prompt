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

class ExitCommand : public Command {
public:
    // Define the command pattern(s) this class will match
    vector<string> get_patterns() const override {
        return {"/exit"};
    }

    // Execute the command logic
    string run(void* user_context, const vector<string>& args) override {
        // Cast the user_context to Commander and signal it to exit
        static_cast<Commander*>(user_context)->exit();
        return "Exiting...";
    }
};


class UserInputEvent : public TypedEvent<UserInputEvent> {
public:
    UserInputEvent(
        ILineEditor& editor, 
        const string& input
    ): 
        TypedEvent<UserInputEvent>(), 
        editor(editor),
        input(input)
    {}

    string getInput() const { return input; }
    ILineEditor& getEditorRef() { return editor; }

private:
    ILineEditor& editor;
    string input;
};

class UserAgent: public BaseEventAgent {
public:
    UserAgent(const ComponentId& id): BaseEventAgent(id) {}

    void run() {
        const string prompt = "> ";

        // Setup CommandLine with a LinenoiseAdapter for input
        LinenoiseAdapter editor(prompt);
        CommandLine command_line(editor);
        Commander commander(command_line);

        // Register commands
        ExitCommand exitCommand;
        vector<void*> commands = { &exitCommand };
        commander.set_commands(commands);

        // Command loop
        string input;
        while (!commander.is_exiting()) {
            editor.Readline(input); // commander.get_command_line_ref().readln(); // TODO: use editor.ReadLine() ???
            if (input.empty()) continue;
            // Try to execute the input as a command
            if (input[0] == '/') commander.run_command(&commander, input);
            else publishEvent(make_shared<UserInputEvent>(editor, input)); // Publishes to the EventBus
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
            sleep(3);
            string data = event->getInput();
            reverse(data.begin(), data.end());
            ILineEditor& editor = event->getEditorRef();
            editor.WipeLine(); // hide user input area (clear the actual line)  
            cout << "Echo: " << data << endl;            
            editor.RefreshLine(); // show the user input prompt (linenoise readln) so that user can continue typing...
        });
    }
};


int main(int argc, char *argv[]) {
    run_tests();
    // run_tests("test_BaseEventProducer_publishEvent_async_bus");
    // run_tests("event");
    // run_tests("test_RingBufferEventQueue");
    // run_tests("test_EventBus");
    // run_tests("test_BaseEventAgent");
    // run_tests("test_BaseEventConsumer");
    // run_tests("test_BaseEventProducer");
    // run_tests("test_SelfMessageFilter");
    // run_tests("test_FilteredEventBus");
    // sleep(3);
    // return 0;

    // TODO: use configs:
    size_t capacity = 100;
    bool async = true;
    long ms = 300;


    Logger logger("logger");
    RingBufferEventQueue queue(capacity, logger);
    EventBus bus(async, logger, queue);
    shared_ptr user = make_shared<UserAgent>("user");
    shared_ptr echo = make_shared<EchoAgent>("echo");
    user->registerWithEventBus(&bus);
    echo->registerWithEventBus(&bus);

    // Start the input loop
    user->run();

    sleep(3);
    return 0;
}