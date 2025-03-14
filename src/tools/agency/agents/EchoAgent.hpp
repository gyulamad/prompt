#pragma once

#include "UserAgent.hpp"

namespace tools::agency::agents {
    
    template<typename T>
    class EchoAgent: public Agent<T> {
    public:
        EchoAgent(PackQueue<T>& queue, Agency<T>& agency): Agent<T>(queue, "echo"), agency(agency) {}

        void handle(const string& sender, const T& item) override {
            sleep(2); // emulate some background work;
            Agent<T>& agent = agency.getAgentRef("user");
            if (agent.name != "user") throw ERROR("Invalid user agent, name is '" + agent.name + "'");
            UserAgent<T>& user = (UserAgent<T>&)agent;
            Commander* commander = user.getCommanderPtr();
            NULLCHK(commander);
            CommandLine& cline = commander->get_command_line_ref();
            cline.getEditorRef().WipeLine();
            cout << "Echo: '" << sender << "' -> " << item << endl;
            cline.getEditorRef().RefreshLine();
        }

    private:
        Agency<T>& agency;
    };
    
}