#pragma once

#include <string>

#include "../../voice/TTS.hpp"
#include "../Agent.hpp"
#include "../chat/Chatbot.hpp"
#include "UserAgentInterface.hpp"

using namespace std;
using namespace tools::agency::chat;

namespace tools::agency::agents {
    
    template<typename T>
    class ChatbotAgent: public Agent<T> {
    public:
        ChatbotAgent(
            Owns& owns,
            Worker<T>* agency,
            PackQueue<T>& queue,
            const string& name,
            void* chatbot,
            void* history,
            UserAgentInterface<T>& interface
        ): 
            Agent<T>(owns, agency, queue, name),
            chatbot(owns.reserve<Chatbot>(this, chatbot, FILELN)),
            history(owns.reserve<ChatHistory>(this, history, FILELN)),
            interface(interface)
        {}

        virtual ~ChatbotAgent() {
            this->owns.release(this, chatbot);
            this->owns.release(this, history);
        }

        void* getChatbotPtr() { return chatbot; } // TODO: remove this


        string type() const override { return "chat"; }

        void handle(const string& sender, const T& item) override {

            CommandLine& cline = interface.getCommanderRef().getCommandLineRef();
            cline.setPromptVisible(false);
            if (chatbot->isTalks()) cline.getEditorRef().wipeLine();

            interface.println();
            bool interrupted;
            string response = safe(chatbot)->chat(sender, item, interrupted);
            interface.println();

            // if (interrupted) {
                //DEBUG("[[[---CHAT INTERRUPTED BY USER---]]] (DBG)"); // TODO interrupt next, now
            // }

            // if (!chatbot->isTalks()) {
                this->addRecipients({ sender });
                this->send(response);
            // }

            cline.setPromptVisible(true);
        }

        void fromJSON(const JSON& json) override {
            Agent<T>::fromJSON(json);

            // role
            if (json.get<string>("role") != type()) // NOTE: We may don't need this check as this function can be extended and the type overrided in derived classes!
                throw ERROR("Type missmatch '" + json.get<string>("role") + "' != '" + type() + "'");

            // chatbot.history.messages
            if (json.has("chatbot.history.messages")) {
                safe(history);
                vector<JSON> jmessages = json.get<vector<JSON>>("chatbot.history.messages");
                for (const JSON& jmessage: jmessages) {
                    history->append(
                        jmessage.get<string>("sender"),
                        jmessage.get<string>("text")
                    );
                }
            }
        } // TODO: !@# basepath for save/load

        JSON toJSON() const override {
            JSON json = Agent<T>::toJSON();

            // role
            json.set("role", this->type());
            
            // chatbot.history.messages
            safe(history);
            vector<ChatMessage> messages = history->getMessages();
            vector<JSON> jmessages;
            for (const ChatMessage& message: messages) {
                JSON jmessage;
                jmessage.set("sender", message.getSender());
                jmessage.set("text", message.getText());
                jmessages.push_back(jmessage);
            }
            json.set("chatbot.history.messages", jmessages);

            return json;
        }

    private:
        Chatbot* chatbot = nullptr;
        ChatHistory* history = nullptr;
        UserAgentInterface<T>& interface;
    };
    
}