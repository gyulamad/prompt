#pragma once

#include <string>

// #include "ChatPlugin.hpp"
// #include "../../chat/ChatHistory.hpp"
// #include "../../chat/Chatbot.hpp"

using namespace std;

namespace tools::agency::agents::plugins {

    class HistoryPlugin: public ChatPlugin {
    public:
        HistoryPlugin(Owns& owns) : ChatPlugin(), owns(owns) {}

        string processChat(Chatbot* chatbot, const string& sender, const string& text, bool& /*interrupted*/) override {
            ChatHistory* history = (ChatHistory*)safe(chatbot->getHistoryPtr());
            history->append(sender, text);
            return text;
        }

        string processInstructions(Chatbot* /*chatbot*/, const string& instructions) override {
            return instructions;
        }

        string processChunk(Chatbot* /*chatbot*/, const string& chunk) override {
            return chunk;
        }

        string processResponse(Chatbot* /*chatbot*/, const string& response) override {
            return response;
        }

        string processRespond(Chatbot* chatbot, const string& sender, const string& text) override {
            ChatHistory* history = (ChatHistory*)safe(chatbot->getHistoryPtr());
            history->append(sender, text);
            return text;
        }

    private:
        Owns& owns;
    };

}
