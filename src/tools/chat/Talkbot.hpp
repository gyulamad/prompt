#pragma once 

#include "../voice/TTS.hpp"
#include "../voice/SentenceStream.hpp"

#include "Chatbot.hpp"

using namespace tools::voice;

namespace tools::chat {

    class Talkbot: public Chatbot {
    public:
        Talkbot(
            const string& name, 
            Factory<ChatHistory>& histories, const string& history_type, //ChatHistory& history, 
            Printer& printer,
            SentenceStream& sentences,
            TTS& tts
        ):
            Chatbot(name, histories, history_type /*history*/, printer),
            sentences(sentences), tts(tts)
        {}

        virtual ~Talkbot() {
            cout << "Talkbot (" + this->name + ") destruction..." << endl;
        }
    
        string chunk(const string& chunk) override { 
            Chatbot::chunk(chunk);
            sentences.write(chunk);
            while (sentences.available()) tell(sentences.read());
            return chunk;
        }
    
        string response(const string& response) override { 
            Chatbot::response(response);
            sentences.flush();
            tell(sentences.read());
            return response;
        }
    
        bool tell(const string& text) {
            return tts.speak(text);
        }
    
    private:
        SentenceStream& sentences;
        TTS& tts;
    };        

}