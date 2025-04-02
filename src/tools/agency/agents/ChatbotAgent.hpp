#pragma once

#include <string>

#include "../../voice/TTS.hpp"
#include "../Agent.hpp"
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
            Owns& owns,
            Worker<T>* agency,
            PackQueue<T>& queue,
            const string& name,
            vector<string> recipients,
            void* chatbot
        ): 
            Agent<T>(owns, agency, queue, name, recipients),
            chatbot(owns.reserve(this, chatbot, FILELN))
        {}

        virtual ~ChatbotAgent() {
            this->owns.release(this, chatbot);
        }

        void* getChatbotPtr() { return chatbot; }


        string type() const override { return "chat"; }

        void handle(const string& sender, const T& item) override {
            this->addRecipients({ sender });
            this->send(((Chatbot*)chatbot)->chat(sender, item));
        }

    private:
        void* chatbot = nullptr;
    };
    
}