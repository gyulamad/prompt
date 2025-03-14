#pragma once

#include <vector>

#include "Command.hpp"

using namespace std;

namespace tools::cmd {

    class CommandFactory {
    public:
        virtual ~CommandFactory() { reset(); }

        template<typename T, typename... Args>
        T& withCommand(Args&&... args) {
            T* command = new T(forward<Args>(args)...);
            commands.push_back(command);
            return *command;
        }

        vector<Command*> getCommands() const {
            return commands;
        }

    protected:

        void reset() {
            for (auto* command : commands) delete command;
            commands.clear();
        }

    private:
        vector<Command*> commands;
    };

} // namespace tools::cmd

// TODO: add tests