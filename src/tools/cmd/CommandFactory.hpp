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
            for (Command* command : commands) delete command;
            commands.clear();
        }

    private:
        vector<Command*>& commands;
    };

} // namespace tools::cmd

// TODO: add tests