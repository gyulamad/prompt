#pragma once

#include "../chat/Talkbot.hpp"

#include "Gemini.hpp"

using namespace tools::chat;

namespace tools::ai {

    class GeminiTalkbot: public Gemini, public Talkbot {
    public:
        GeminiTalkbot(
            Owns& owns,
            const string& secret, 
            const string& variant,
            long timeout,
            const string& name, // TODO: remove it
            ChatHistory* history, 
            Printer& printer,
            SentenceStream& sentences,
            TTS& tts
        ):
            Gemini(secret, variant, timeout),
            Talkbot(owns, name, history, printer, sentences, tts)
        {}

        virtual ~GeminiTalkbot() {
            // cout << "GeminiTalkbot (" + this->name + ") destruction..." << endl;
        }
    
        // TODO: do we need this function? simply override and call (does nothing special, caller can directly call Gemini's chat method maybe??)
        string chat(const string& sender, const string& text, bool& interrupted) override {
            return Gemini::chat(*this, sender, text, interrupted);
        }
    };

}