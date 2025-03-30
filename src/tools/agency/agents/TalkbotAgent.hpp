#pragma once

#include <string>

#include "../../chat/Talkbot.hpp"

using namespace tools::chat;

using namespace std;

namespace tools::agency::agents {


    template<typename T>
    class TalkbotAgent: public Agent<T> {
    public:
        TalkbotAgent(
            PackQueue<T>& queue,
            const string& name,
            vector<string> recipients,
            Talkbot& talkbot
        ): 
            Agent<T>(queue, name, recipients),
            talkbot(talkbot)
        {}

        string type() const override { return "talk"; }

        void handle(const string& sender, const T& item) override {
            this->addRecipients({ sender });
            this->send(talkbot.chat(sender, item));
        }

    private:
        Talkbot& talkbot;
    };
    
}