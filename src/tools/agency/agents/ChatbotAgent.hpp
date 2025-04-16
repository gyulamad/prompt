#pragma once

#include <string>

#include "../../voice/TTS.hpp"
#include "../Agent.hpp"
#include "../chat/Chatbot.hpp"
#include "../chat/ChatHistory.hpp"
#include "UserAgentInterface.hpp"
#include "plugins/HistoryPlugin.hpp" // Add this line

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
            // void* instructions,
            // void* history,
            UserAgentInterface<T>& interface
        ): 
            Agent<T>(owns, agency, queue, name),
            chatbot(owns.reserve<Chatbot>(this, chatbot, FILELN)),
            // instructions(owns.reserve<string>(this, instructions, FILELN)),
            // history(owns.reserve<ChatHistory>(this, history, FILELN)),
            interface(interface)
        {}

        virtual ~ChatbotAgent() {
            this->owns.release(this, chatbot);
            // this->owns.release(this, instructions);
            // this->owns.release(this, history);
        }

        void* getChatbotPtr() { return chatbot; } // TODO: remove this


        string type() const override { return "chat"; }

        void setTalks(bool talks) {
            safe(chatbot)->setTalks(talks);
        }

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

            // talks
            if (json.has("talks"))
                chatbot->setTalks(json.get<bool>("chatbot.talks"));

            // // instructions
            // if (json.has("chatbot.instructions"))
            //     chatbot->setInstructions(json.get<string>("chatbot.instructions"));

            // chatbot.history.messages
            ChatHistory* history = (ChatHistory*)safe(chatbot->getHistoryPtr());
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

            // talks
            json.set("talks", chatbot->isTalks());

            // instructions
            json.set("instructions", chatbot->getInstructions());
            
            // chatbot.history.messages
            ChatHistory* history = (ChatHistory*)safe(chatbot->getHistoryPtr());
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
        // string* instructions = nullptr;
        // ChatHistory* history = nullptr;
        UserAgentInterface<T>& interface;
    };
    
}

#ifdef TEST

#include "../../utils/Test.hpp" //  TODO: fix paths everywhere AI hardcode absulutes
#include "../../cmd/LinenoiseAdapter.hpp"
#include "plugins/HistoryPlugin.hpp"
#include "../tests/MockChatbot.hpp"

using namespace std;
using namespace tools::voice;
using namespace tools::cmd;
using namespace tools::utils;
using namespace tools::agency::agents;
using namespace tools::agency::agents::plugins;

void test_ChatbotAgent_constructor() {
    Owns owns;
    PackQueue<string> queue;
    string name = "test_agent";
    string instructions = "test_instructions";
    ChatHistory* history = owns.allocate<ChatHistory>("> ", false);
    ChatPlugins* plugins = owns.allocate<ChatPlugins>(owns);
    Chatbot* chatbot = owns.allocate<Chatbot>(owns, instructions, history, plugins, false);
    TTS tts("", 0, 0, "", "", {});
    STTSwitch sttSwitch;
    MicView micView;
    LinenoiseAdapter lineEditor("> ");
    CommandLine commandLine(lineEditor, "", "", false, 10);
    vector<Command*> commands;
    Commander commander(commandLine, commands, "");
    InputPipeInterceptor inputPipeInterceptor;
    UserAgentInterface<string> interface(tts, sttSwitch, micView, commander, inputPipeInterceptor);

    ChatbotAgent<string> agent(owns, nullptr, queue, name, chatbot, interface);

    assert(agent.getName() == name && "Agent name should be initialized correctly");
    assert(agent.type() == "chat" && "Agent type should be 'chat'");
    assert(agent.getChatbotPtr() == chatbot && "Chatbot pointer should be initialized correctly");
}

void test_ChatbotAgent_type() {
    Owns owns;
    PackQueue<string> queue;
    string name = "test_agent";
    string instructions = "test_instructions";
    ChatHistory* history = owns.allocate<ChatHistory>("> ", false);
    ChatPlugins* plugins = owns.allocate<ChatPlugins>(owns);
    Chatbot* chatbot = owns.allocate<Chatbot>(owns, instructions, history, plugins, false);
    TTS tts("", 0, 0, "", "", {});
    STTSwitch sttSwitch;
    MicView micView;
    LinenoiseAdapter lineEditor("> ");
    CommandLine commandLine(lineEditor, "", "", false, 10);
    vector<Command*> commands;
    Commander commander(commandLine, commands, "");
    InputPipeInterceptor inputPipeInterceptor;
    UserAgentInterface<string> interface(tts, sttSwitch, micView, commander, inputPipeInterceptor);

    ChatbotAgent<string> agent(owns, nullptr, queue, name, chatbot, interface);

    string type = agent.type();
    assert(type == "chat" && "Agent type should be 'chat'");
}

void test_ChatbotAgent_setTalks() {
    Owns owns;
    PackQueue<string> queue;
    string name = "test_agent";
    string instructions = "test_instructions";
    ChatHistory* history = owns.allocate<ChatHistory>("> ", false);
    ChatPlugins* plugins = owns.allocate<ChatPlugins>(owns);
    Chatbot* chatbot = owns.allocate<Chatbot>(owns, instructions, history, plugins, false);
    TTS tts("", 0, 0, "", "", {});
    STTSwitch sttSwitch;
    MicView micView;
    LinenoiseAdapter lineEditor("> ");
    CommandLine commandLine(lineEditor, "", "", false, 10);
    vector<Command*> commands;
    Commander commander(commandLine, commands, "");
    InputPipeInterceptor inputPipeInterceptor;
    UserAgentInterface<string> interface(tts, sttSwitch, micView, commander, inputPipeInterceptor);

    ChatbotAgent<string> agent(owns, nullptr, queue, name, chatbot, interface);

    agent.setTalks(true);
    assert(safe(chatbot)->isTalks() == true && "setTalks(true) should set talks to true");

    agent.setTalks(false);
    assert(safe(chatbot)->isTalks() == false && "setTalks(false) should set talks to false");
}

void test_ChatbotAgent_handle() {
    Owns owns;
    PackQueue<string> queue;
    string name = "test_agent";
    string instructions = "test_instructions";
    string chatbotName = "test_chatbot";
    string sender = "test_sender";
    string item = "test_item";
    string chatbotResponse = "test_response";

    ChatHistory* history = owns.allocate<ChatHistory>("> ", false);
    ChatPlugins* plugins = owns.allocate<ChatPlugins>(owns);
    HistoryPlugin* historyPlugin = owns.allocate<HistoryPlugin>(owns);
    plugins->push<HistoryPlugin>(historyPlugin);
    MockChatbot* chatbot = owns.allocate<MockChatbot>(owns, chatbotName, chatbotResponse, history, plugins, false);
    //Chatbot* chatbot = owns.allocate<Chatbot>(owns, chatbotName, history, plugins, false);
    TTS tts("", 0, 0, "", "", {});
    STTSwitch sttSwitch;
    MicView micView;
    LinenoiseAdapter lineEditor("> ");
    CommandLine commandLine(lineEditor, "", "", false, 10);
    vector<Command*> commands;
    Commander commander(commandLine, commands, "");
    InputPipeInterceptor inputPipeInterceptor;
    UserAgentInterface<string> interface(tts, sttSwitch, micView, commander, inputPipeInterceptor);

    ChatbotAgent<string> agent(
        owns, 
        nullptr, 
        queue, 
        name, 
        chatbot, 
        interface
    );

    agent.handle(sender, item);

    // Verify that the chatbot's chat function was called and the response was sent
    ChatHistory* chatHistory = (ChatHistory*)safe(chatbot->getHistoryPtr());
    vector<ChatMessage> messages = chatHistory->getMessages();
    assert(messages.size() == 2 && "Chatbot should have two messages in history");
    assert(messages[0].getSender() == sender && "Sender should match");
    assert(messages[0].getText() == item && "First message should be the user's message");
    assert(messages[1].getSender() == chatbotName && "Second sender should be the chatbot");
    assert(messages[1].getText() == chatbotResponse && "Second message should be the chatbot's response");
}


TEST(test_ChatbotAgent_constructor);
TEST(test_ChatbotAgent_type);
TEST(test_ChatbotAgent_setTalks);
TEST(test_ChatbotAgent_handle);

#endif
