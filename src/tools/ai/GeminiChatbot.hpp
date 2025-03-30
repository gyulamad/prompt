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
            ChatHistory& history, 
            Printer& printer
        ):
            Gemini(secret, variant, timeout),
            Chatbot(name, history, printer)
        {}
    
        string chat(const string& sender, const string& text) override {
            return Gemini::chat(*this, sender, text);
        }
    };    

}
