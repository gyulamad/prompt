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
            Factory<Talkbot>& talkbots, const string& talkbot_type //Talkbot& talkbot
        ): 
            Agent<T>(queue, name, recipients),
            talkbots(talkbots),
            talkbot(talkbots.hold(this, talkbots.create(talkbot_type))/*talkbot*/)
        {}

        virtual ~TalkbotAgent() {
            cout << "TalkbotAgent (" + this->name + ") destruction..." << endl;
            talkbots.release(this, talkbot);
        }

        string type() const override { return "talk"; }

        void handle(const string& sender, const T& item) override {
            this->addRecipients({ sender });
            this->send(talkbot->chat(sender, item));
        }

    private:
        Factory<Talkbot>& talkbots;
        Talkbot* talkbot = nullptr;
    };
    
}