#pragma once

#include "../../tools/events/BaseEventAgent.hpp"
#include "../../tools/cmd/Commander.hpp"

using namespace tools::events;
using namespace tools::cmd;

namespace prompt::agents {

    class IUserAgent: public BaseEventAgent {
    public:
        IUserAgent(const ComponentId& id, Commander& commander, Logger& logger): BaseEventAgent(id), commander(commander), logger(logger) {}
        void echo(const string& echo) const { commander.echo(echo); }
        void exit() const { commander.exit(); }
        // Commander& getCommanderRef() { return commander; }
        // Logger& getLoggerRef() { return logger; }
    protected:
        Commander& commander;
        Logger& logger;
    };
    
}