#pragma once

#include "../events/UserInputEvent.hpp"
#include "IUserAgent.hpp"

using namespace prompt::events;

namespace prompt::agents {

    class UserTextInputAgent: public IUserAgent {
    public:
        UserTextInputAgent(const ComponentId& id, Commander& commander, Logger& logger): IUserAgent(id, commander, logger) {}
    
        void run() { // Command loop
            string input;
            // Commander& commander = getCommanderRef();
            CommandLine& cline = commander.get_command_line_ref();
            ILineEditor& editor = cline.getEditorRef();
            while (!commander.is_exiting()) {
                try {
                    if (editor.Readline(input)) input = "/exit"; // Readline => returns true for Ctrl+C
                    if (input.empty()) continue;
                    if (input[0] == '/') commander.run_command(this, input); // Try to execute the input as a command
                    else publishEvent<UserInputEvent>("", *this, input, true); // Publishes to the EventBus
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

}


#ifdef TEST

#include "../../tools/utils/Test.hpp"
#include "../../tools/events/Event.hpp"
#include "../../tools/events/tests/MockEventBus.hpp"

using namespace tools::events;
using namespace prompt::agents;

// Tests for UserTextInputAgent
void test_UserTextInputAgent_run_handlesExitCommand() {
    MockLogger logger;
    MockLineEditor editor;
    MockCommandLine cline(editor);
    MockCommander commander(cline);
    UserTextInputAgent agent("user-text-input", commander, logger);

    editor.queueInput("/exit");
    agent.run(); // Will exit immediately due to /exit

    bool actual = commander.is_exiting();
    assert(actual == true && "UserTextInputAgent::run should exit on '/exit' command");
}

void test_UserTextInputAgent_run_ignoresEmptyInput() {
    MockLogger logger;
    MockLineEditor editor;
    MockCommandLine cline(editor);
    MockCommander commander(cline);
    UserTextInputAgent agent("user-text-input", commander, logger);

    editor.queueInput("");
    editor.queueInput("/exit"); // To break the loop
    agent.run();

    bool actual = commander.is_exiting();
    assert(actual == true && "UserTextInputAgent::run should ignore empty input and only exit on '/exit'");
}

void test_UserTextInputAgent_run_handlesCtrlC() {
    MockLogger logger;
    MockLineEditor editor;
    MockCommandLine cline(editor);
    MockCommander commander(cline);
    UserTextInputAgent agent("user-text-input", commander, logger);

    editor.queueInput("ctrl+c"); // Simulates Ctrl+C
    agent.run();

    bool actual = commander.is_exiting();
    assert(actual == true && "UserTextInputAgent::run should treat Ctrl+C as '/exit'");
}

void test_UserTextInputAgent_run_logsException() {
    string actual = "incorrect";
    string output = capture_cerr([&]() {
        struct BadEditor : ILineEditor {
            BadEditor() : shouldThrow(true) {}
            bool Readline(string& line) override {
                if (shouldThrow) {
                    shouldThrow = false; // Throw only once
                    throw runtime_error("Test exception");
                }
                line = "/exit"; // After throwing, provide exit command
                return false;
            }
            // void setPrompt(const string&) override {}
            void WipeLine() override {}
            void RefreshLine() override {}
        private:
            bool shouldThrow;
        };
        MockLogger logger;
        BadEditor editor;
        MockCommandLine cline(editor);
        MockCommander commander(cline);
        UserTextInputAgent agent("user-text-input", commander, logger);

        agent.run(); // Will throw once, then exit on "/exit"
        actual = logger.getLastError();
    });

    assert(str_contains(actual, "Runtime error: Test exception") && "UserTextInputAgent::run should log exception message");
    assert(str_contains(output, "Runtime error: Test exception") && "UserTextInputAgent::run should cerr message");
}

// Non-command input publishes event
void test_UserTextInputAgent_run_publishesNonCommandInput() {
    MockLogger logger;
    MockLineEditor editor;
    MockCommandLine cline(editor);
    MockCommander commander(cline);
    RingBufferEventQueue queue(100, logger);
    MockEventBus bus(logger, queue);
    UserTextInputAgent agent("user-text-input", commander, logger);
    agent.registerWithEventBus(&bus);
    bus.start();

    editor.queueInput("hello");
    editor.queueInput("/exit");
    agent.run();

    string actual = bus.publishedEvents.empty() ? "" : ((UserInputEvent*)bus.publishedEvents[0])->getInput();
    assert(actual == "hello\n" && "UserTextInputAgent::run should publish non-command input with newline");
}

// Invalid command doesn’t exit
void test_UserTextInputAgent_run_handlesInvalidCommand() {
    MockLogger logger;
    MockLineEditor editor;
    MockCommandLine cline(editor);
    MockCommander commander(cline);
    UserTextInputAgent agent("user-text-input", commander, logger);

    editor.queueInput("/foo"); // Unknown command
    editor.queueInput("/exit");
    agent.run();

    bool actual = commander.is_exiting();
    assert(actual == true && "UserTextInputAgent::run should not exit on invalid command, only on '/exit'");
}

// Multiple inputs in sequence
void test_UserTextInputAgent_run_handlesMultipleInputs() {
    MockLogger logger;
    MockLineEditor editor;
    MockCommandLine cline(editor);
    MockCommander commander(cline);
    RingBufferEventQueue queue(100, logger);
    MockEventBus bus(logger, queue);
    UserTextInputAgent agent("user-text-input", commander, logger);
    agent.registerWithEventBus(&bus);
    bus.start();

    editor.queueInput("hello");
    editor.queueInput("world");
    editor.queueInput("/exit");
    agent.run();

    vector<string> actual;
    for (Event*& event : bus.publishedEvents) {
        actual.push_back(((UserInputEvent*)event)->getInput());
    }
    
    vector<string> expected = {"hello\n", "world\n"};
    assert(actual == expected && "UserTextInputAgent::run should publish multiple non-command inputs in sequence");
}


TEST(test_UserTextInputAgent_run_handlesExitCommand);
TEST(test_UserTextInputAgent_run_ignoresEmptyInput);
TEST(test_UserTextInputAgent_run_handlesCtrlC);
TEST(test_UserTextInputAgent_run_logsException);
TEST(test_UserTextInputAgent_run_publishesNonCommandInput);
TEST(test_UserTextInputAgent_run_handlesInvalidCommand);
TEST(test_UserTextInputAgent_run_handlesMultipleInputs);

#endif