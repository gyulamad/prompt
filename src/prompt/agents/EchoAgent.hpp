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
                sleep(2); // TODO: waiting for a bit to emulate some work (for testing only, it need to be removed!!!!)
                string data = event->getInput();
                // IUserAgent& user = event->getUserRef();
                // Commander& commander = user.getCommanderRef();
                // CommandLine& cline = commander.get_command_line_ref();
                // ILineEditor& editor = cline.getEditorRef();
                {
                    lock_guard<mutex> lock(mtx);
                    event->echo(data);
                //     editor.WipeLine(); // hide user input area (clear the actual line)
                //     cout << data << flush;
                //     editor.RefreshLine(); // show the user input prompt (linenoise readln) so that user can continue typing...
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
    EventBus bus(false, logger, queue);
    shared_ptr agent = make_shared<EchoAgent>("echo");
    agent->registerWithEventBus(&bus);

    MockLineEditor editor;
    MockCommandLine cline(editor);
    MockCommander commander(cline);
    IUserAgent user("user", commander, logger);

    editor.resetFlags();

    capture_cout([&](){
        bus.createAndPublishEvent<UserInputEvent>("user", "echo", user, "Test input to echo", true);
        this_thread::sleep_for(chrono::milliseconds(200));
    });
    
    bool wiped = editor.wasWiped();
    bool refreshed = editor.wasRefreshed();

    assert(wiped == true && "EchoAgent should wipe the line on UserInputEvent");
    assert(refreshed == true && "EchoAgent should refresh the line on UserInputEvent");
}

void test_EchoAgent_registerEventInterests_outputsToCout() {
    // TEST_SKIP("TODO: Async queue delivery mechanism needs refact");
    MockLogger logger;
    RingBufferEventQueue queue(100, logger);
    EventBus bus(false, logger, queue);
    shared_ptr agent = make_shared<EchoAgent>("echo");
    agent->registerWithEventBus(&bus);

    MockLineEditor editor;
    MockCommandLine cline(editor);
    MockCommander commander(cline);
    IUserAgent user("user", commander, logger);

    string actualOutput = capture_cout([&]() {
        bus.createAndPublishEvent<UserInputEvent>("user", "echo", user, "Test input to echo", true);
        this_thread::sleep_for(chrono::milliseconds(50)); // Wait for async processing
    });

    assert(actualOutput == "Test input to echo\n" && "EchoAgent should output 'Test echo' followed by newline");
}

void test_EchoAgent_registerEventInterests_handlesEmptyInput() {
    // TEST_SKIP("TODO: Async queue delivery mechanism needs refact");
    MockLogger logger;
    RingBufferEventQueue queue(100, logger);
    EventBus bus(false, logger, queue);
    shared_ptr agent = make_shared<EchoAgent>("echo");
    agent->registerWithEventBus(&bus);

    MockLineEditor editor;
    MockCommandLine cline(editor);
    MockCommander commander(cline);
    IUserAgent user("user", commander, logger);

    string actualOutput = capture_cout([&]() {
        bus.createAndPublishEvent<UserInputEvent>("user", "echo", user, "", true);
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