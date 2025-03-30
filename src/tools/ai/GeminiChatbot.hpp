#pragma once

#include "../chat/Chatbot.hpp"

#include "Gemini.hpp"

using namespace tools::chat;

namespace tools::ai {

    class GeminiChatbot: public Gemini, public Chatbot {
    public:
        GeminiChatbot(
            const string& secret, 
            const string& variant,
            long timeout,
            const string& name, 
            Factory<ChatHistory>& histories, const string& history_type, // ChatHistory& history
            Printer& printer
        ):
            Gemini(secret, variant, timeout),
            Chatbot(name, histories, history_type/*history*/, printer)
        {}

        virtual ~GeminiChatbot() {
            cout << "GeminiChatbot (" + this->name + ") destruction..." << endl;
        }
    
        string chat(const string& sender, const string& text) override {
            return Gemini::chat(*this, sender, text);
        }
    };    

}
