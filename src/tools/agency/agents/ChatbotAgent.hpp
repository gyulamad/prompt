#pragma once

#include <string>

#include "../../voice/TTS.hpp"
#include "../Agent.hpp"
// #include "UserAgent.hpp"
#include "../../chat/Chatbot.hpp"
// #include "../../abstracts/JSONSerializable.hpp"
#include "UserAgentInterface.hpp"

using namespace std;

// using namespace tools::abstracts;
// using namespace tools::voice;
// using namespace tools::agency;
using namespace tools::chat;
// using namespace tools::abstracts;

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
            void* chatbot,
            UserAgentInterface<T>& interface
        ): 
            Agent<T>(owns, agency, queue, name, recipients),
            chatbot(owns.reserve<Chatbot>(this, chatbot, FILELN)),
            interface(interface)
        {}

        virtual ~ChatbotAgent() {
            this->owns.release(this, chatbot);
        }

        void* getChatbotPtr() { return chatbot; } // TODO: remove this


        string type() const override { return "chat"; }

        void handle(const string& sender, const T& item) override {

            interface.getCommanderRef().getCommandLineRef().setPromptVisible(false);

            bool interrupted;
            string response = safe(chatbot)->chat(sender, item, interrupted);

            if (interrupted) {
                DEBUG("[[[---CHAT INTERRUPTED BY USER---]]] (DBG)"); // TODO interrupt next, now
            }

            this->addRecipients({ sender });
            this->send(response);

            interface.getCommanderRef().getCommandLineRef().setPromptVisible(true);
        }

    private:
        Chatbot* chatbot = nullptr;
        UserAgentInterface<T>& interface;
    };
    
}