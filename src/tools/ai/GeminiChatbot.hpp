#pragma once

#include "../chat/Chatbot.hpp"

#include "Gemini.hpp"

using namespace tools::chat;

namespace tools::ai {

    class GeminiChatbot: public Gemini, public Chatbot {
    public:
        GeminiChatbot(
            Owns& owns,
            const string& secret, 
            const string& variant,
            long timeout,
            const string& name, 
            ChatHistory* history,
            Printer& printer
        ):
            Gemini(secret, variant, timeout),
            Chatbot(owns, name, history, printer)
        {}

        virtual ~GeminiChatbot() {
            // cout << "GeminiChatbot (" + this->name + ") destruction..." << endl;
        }
    
        string chat(const string& sender, const string& text, bool& interrupted) override {
            return Gemini::chat(*this, sender, text, interrupted);
        }
    };    

}
