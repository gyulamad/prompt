#pragma once

#include "../../tools/events/TypedEvent.hpp"
#include "../agents/IUserAgent.hpp"

using namespace tools::events;
using namespace prompt::agents;

namespace prompt::events {

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

        void echo(const string& echo) const {
            user.echo(echo);
        }
    
        string getInput() const { return input; }
        // IUserAgent& getUserRef() { return user; }
    
    private:
        IUserAgent& user;
        string input;
    };
    
}


#ifdef TEST

#include "../../tools/utils/Test.hpp"

using namespace prompt::events;

// Tests for UserInputEvent
void test_UserInputEvent_constructor_appendsNewline() {
    MockLogger logger;
    MockLineEditor editor;
    MockCommandLine cline(editor);
    MockCommander commander(cline);
    IUserAgent user("user", commander, logger);

    UserInputEvent event(user, "test", true);
    string actual = event.getInput();

    assert(actual == "test\n" && "UserInputEvent constructor should append newline when newln is true");
}

void test_UserInputEvent_constructor_noNewline() {
    MockLogger logger;
    MockLineEditor editor;
    MockCommandLine cline(editor);
    MockCommander commander(cline);
    IUserAgent user("user", commander, logger);

    UserInputEvent event(user, "test", false);
    string actual = event.getInput();

    assert(actual == "test" && "UserInputEvent constructor should not append newline when newln is false");
}

void test_UserInputEvent_getInput_returnsCorrectString() {
    MockLogger logger;
    MockLineEditor editor;
    MockCommandLine cline(editor);
    MockCommander commander(cline);
    IUserAgent user("user", commander, logger);

    UserInputEvent event(user, "hello", true);
    string actual = event.getInput();

    assert(actual == "hello\n" && "UserInputEvent::getInput should return input with newline");
}

// void test_UserInputEvent_getUserRef_returnsSameUser() {
//     MockLogger logger;
//     MockLineEditor editor;
//     MockCommandLine cline(editor);
//     MockCommander commander(cline);
//     IUserAgent user("user", commander, logger);

//     UserInputEvent event(user, "test", true);
//     IUserAgent& actual = event.getUserRef();

//     assert(&actual == &user && "UserInputEvent::getUserRef should return the same user reference");
// }

void test_UserInputEvent_constructor_handlesEmptyInput() {
    MockLogger logger;
    MockLineEditor editor;
    MockCommandLine cline(editor);
    MockCommander commander(cline);
    IUserAgent user("user", commander, logger);

    UserInputEvent eventWithNewln(user, "", true);
    UserInputEvent eventNoNewln(user, "", false);

    string actualWithNewln = eventWithNewln.getInput();
    string actualNoNewln = eventNoNewln.getInput();

    assert(actualWithNewln == "\n" && "UserInputEvent constructor should handle empty input with newline");
    assert(actualNoNewln == "" && "UserInputEvent constructor should handle empty input without newline");
}

// Register tests
TEST(test_UserInputEvent_constructor_appendsNewline);
TEST(test_UserInputEvent_constructor_noNewline);
TEST(test_UserInputEvent_getInput_returnsCorrectString);
// TEST(test_UserInputEvent_getUserRef_returnsSameUser);
TEST(test_UserInputEvent_constructor_handlesEmptyInput);

#endif