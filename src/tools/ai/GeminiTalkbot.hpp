#pragma once

#include "../chat/Talkbot.hpp"

#include "Gemini.hpp"

using namespace tools::chat;

namespace tools::ai {

    class GeminiTalkbot: public Gemini, public Talkbot {
    public:
        GeminiTalkbot(
            const string& secret, 
            const string& variant,
            long timeout,
            const string& name, 
            ChatHistory& history, 
            Printer& printer,
            SentenceStream& sentences,
            TTS& tts
        ):
            Gemini(secret, variant, timeout),
            Talkbot(name, history, printer, sentences, tts)
        {}
    
        string chat(const string& sender, const string& text) override {
            return Gemini::chat(*this, sender, text);
        }
    };

}