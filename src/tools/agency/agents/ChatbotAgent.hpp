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
    class ChatbotAgentConfig: public AgentConfig<T> {
    public:
        ChatbotAgentConfig(
            Owns& owns,
            void* agency,
            PackQueue<T>& queue,
            const string& name,
            vector<string> recipients,
            void* chatbot
        ):
            AgentConfig<T>(owns, agency, queue, name, recipients),
            chatbot(chatbot)
        {}
        
        virtual ~ChatbotAgentConfig() {}

        void* getChatbotPtr() { return chatbot; }

    private:
        void* chatbot;
    };

    template<typename T>
    class ChatbotAgent: public Agent<T> {
    public:
        ChatbotAgent(ChatbotAgentConfig<T>& config
            // Owns& owns,
            // PackQueue<T>& queue,
            // const string& name,
            // vector<string> recipients,
            // Chatbot* chatbot
        ): 
            owns(config.getOwnsRef()),
            Agent<T>(config),
            chatbot(owns.reserve(this, config.getChatbotPtr(), FILELN))
        {}

        virtual ~ChatbotAgent() {
            owns.release(this, chatbot);
        }

        string type() const override { return "chat"; }

        void handle(const string& sender, const T& item) override {
            this->addRecipients({ sender });
            this->send(((Chatbot*)chatbot)->chat(sender, item));
        }

    private:
        Owns& owns;
        void* chatbot = nullptr;
    };
    
}