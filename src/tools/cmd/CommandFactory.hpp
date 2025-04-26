#pragma once

#include <vector>

#include "Command.hpp"

using namespace std;

namespace tools::cmd {

    class CommandFactory {
    public:
        CommandFactory(vector<Command*>& commands): commands(commands) {}

        virtual ~CommandFactory() { reset(); }

        template<typename T, typename... Args>
        T& withCommand(Args&&... args) {
            T* command = new T(forward<Args>(args)...);
            commands.push_back(command);
            return *command;
        }

        vector<Command*>& getCommandsRef() const {
            return commands;
        }

    protected:

        void reset() {
            for (Command*& command : commands) {
                delete command;
                command = nullptr;
            }
            commands.clear();
        }

    private:
        vector<Command*>& commands;
    };

} // namespace tools::cmd


#ifdef TEST

#include "tests/MockCommandFactory.hpp"
#include "tests/TestCommand.hpp"

using namespace tools::cmd;

void test_CommandFactory_withCommand() {
    vector<Command*> commands;
    CommandFactory factory(commands);
    auto& cmd = factory.withCommand<TestCommand>();
    assert(&cmd == commands[0] && "Command not added to vector");
    assert(commands.size() == 1 && "Size not incremented");
}

void test_CommandFactory_reset() {
    vector<Command*> commands;
    MockCommandFactory factory(commands);
    factory.withCommand<TestCommand>();
    factory.public_reset();
    assert(commands.empty() && "Vector not cleared");
}

void test_CommandFactory_getCommandsRef() {
    vector<Command*> commands;
    CommandFactory factory(commands);
    auto& commands_ref = factory.getCommandsRef();
    factory.withCommand<TestCommand>();
    
    assert(commands.size() == 1 && "Reference not pointing to correct vector");
    assert(commands_ref.size() == 1 && "Reference not pointing to correct vector");
    assert(commands[0] == commands_ref[0] && "Command not added through reference");
}

TEST(test_CommandFactory_withCommand);
TEST(test_CommandFactory_reset);
TEST(test_CommandFactory_getCommandsRef);

#endif