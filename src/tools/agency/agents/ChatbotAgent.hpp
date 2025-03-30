#pragma once

#include <string>

// #include "../../voice/TTS.hpp"
// #include "../Agent.hpp"
// #include "UserAgent.hpp"
#include "../../chat/Chatbot.hpp"

// using namespace tools::abstracts;
// using namespace tools::voice;
// using namespace tools::agency;
using namespace tools::chat;

using namespace std;

namespace tools::agency::agents {


    template<typename T>
    class ChatbotAgent: public Agent<T> {
    public:
        ChatbotAgent(
            PackQueue<T>& queue,
            const string& name,
            vector<string> recipients,
            Factory<Chatbot>& chatbots, const string& chatbot_type//Chatbot& chatbot
        ): 
            Agent<T>(queue, name, recipients),
            chatbots(chatbots),
            chatbot(chatbots.hold(this, chatbots.create(chatbot_type)))
        {}

        virtual ~ChatbotAgent() {
            cout << "ChatbotAgent (" + this->name + ") destruction..." << endl;
            chatbots.release(this, chatbot);
        }

        string type() const override { return "chat"; }

        void handle(const string& sender, const T& item) override {
            this->addRecipients({ sender });
            this->send(chatbot->chat(sender, item));
        }

    private:
        Factory<Chatbot>& chatbots;
        Chatbot* chatbot = nullptr;
    };
    
}