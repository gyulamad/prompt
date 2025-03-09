#pragma once

#include "../../tools/events/BaseEventAgent.hpp"
#include "../events/UserInputEvent.hpp"
#include "IUserAgent.hpp"

using namespace tools::events;
using namespace prompt::events;

namespace prompt::agents {

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

}


#ifdef TEST

#include "../../tools/utils/Test.hpp"

// Tests for EchoAgent
void test_EchoAgent_registerEventInterests_handlesUserInputEvent() {
    MockLogger logger;
    RingBufferEventQueue queue(100, logger);
    EventBus bus(true, logger, queue);
    shared_ptr agent = make_shared<EchoAgent>("echo");
    agent->registerWithEventBus(&bus);

    MockLineEditor editor;
    MockCommandLine cline(editor);
    MockCommander commander(cline);
    IUserAgent user("user", commander, logger);
    shared_ptr event = make_shared<UserInputEvent>(user, "Test echo", true);

    capture_cout([&](){
        bus.publishEvent(event);
        this_thread::sleep_for(chrono::milliseconds(50)); // Wait for async processing
    });

    bool wiped = editor.wasWiped();
    bool refreshed = editor.wasRefreshed();

    assert(wiped == true && "EchoAgent should wipe the line on UserInputEvent");
    assert(refreshed == true && "EchoAgent should refresh the line on UserInputEvent");
}

void test_EchoAgent_registerEventInterests_outputsToCout() {
    MockLogger logger;
    RingBufferEventQueue queue(100, logger);
    EventBus bus(true, logger, queue);
    shared_ptr agent = make_shared<EchoAgent>("echo");
    agent->registerWithEventBus(&bus);

    MockLineEditor editor;
    MockCommandLine cline(editor);
    MockCommander commander(cline);
    IUserAgent user("user", commander, logger);
    shared_ptr event = make_shared<UserInputEvent>(user, "Test echo", true);

    string actualOutput = capture_cout([&]() {
        bus.publishEvent(event);
        this_thread::sleep_for(chrono::milliseconds(50)); // Wait for async processing
    });

    assert(actualOutput == "Test echo\n" && "EchoAgent should output 'Test echo' followed by newline");
}

void test_EchoAgent_registerEventInterests_handlesEmptyInput() {
    MockLogger logger;
    RingBufferEventQueue queue(100, logger);
    EventBus bus(true, logger, queue);
    shared_ptr agent = make_shared<EchoAgent>("echo");
    agent->registerWithEventBus(&bus);

    MockLineEditor editor;
    MockCommandLine cline(editor);
    MockCommander commander(cline);
    IUserAgent user("user", commander, logger);
    shared_ptr event = make_shared<UserInputEvent>(user, "", true); // Empty input

    string actualOutput = capture_cout([&]() {
        bus.publishEvent(event);
        this_thread::sleep_for(chrono::milliseconds(50));
    });

    bool wiped = editor.wasWiped();
    bool refreshed = editor.wasRefreshed();

    assert(wiped == true && "EchoAgent should wipe the line even with empty input");
    assert(refreshed == true && "EchoAgent should refresh the line even with empty input");
    assert(actualOutput == "\n" && "EchoAgent should output just a newline for empty input");
}

// Register all tests
TEST(test_EchoAgent_registerEventInterests_handlesUserInputEvent);
TEST(test_EchoAgent_registerEventInterests_outputsToCout);
TEST(test_EchoAgent_registerEventInterests_handlesEmptyInput);

#endif