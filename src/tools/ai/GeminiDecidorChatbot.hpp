#pragma once

#include "../chat/DecidorChatbot.hpp"

#include "Gemini.hpp"

using namespace tools::chat;

namespace tools::ai {

    class GeminiDecidorChatbot: public Gemini, public DecidorChatbot {
    public:
        GeminiDecidorChatbot(
            Owns& owns,
            const string& secret, 
            const string& variant,
            long timeout,
            const string& name, // TODO: remove it
            ChatHistory* history,
            Printer& printer
        ):
            Gemini(secret, variant, timeout),
            DecidorChatbot(owns, name, history, printer)
        {}

        virtual ~GeminiDecidorChatbot() = default;
    

        // string chat(const string& sender, const string& text, bool& interrupted) override {
        //     return Gemini::chat(*this, sender, text, interrupted);
        // }

        string respond(const string& sender, const string& text) override {
            return Gemini::respond(*this, sender, text);
        }

    };

}