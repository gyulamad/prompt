#pragma once

#include "../../chat/Chatbot.hpp"

#include "Gemini.hpp"

using namespace tools::agency::chat;

namespace tools::agency::ai {

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

        string respond(const string& sender, const string& text) override {
            return Gemini::respond(*this, sender, text);
        }
    
        string chat(const string& sender, const string& text, bool& interrupted) override {
            return Gemini::chat(*this, sender, text, interrupted);
        }

        string chunk(const string& chunk) override {
            printer.print(chunk);
            return chunk;
        }
        
        string response(const string& response) override {
            return response; 
        }

    };    

}
