#pragma once 

#include "../voice/TTS.hpp"
#include "../voice/SentenceStream.hpp"

#include "Chatbot.hpp"

using namespace tools::voice;

namespace tools::chat {

    class Talkbot: public Chatbot {
    public:

        Talkbot(
            Owns& owns,
            const string& name, // TODO: do we need name here?
            void* history, 
            Printer& printer,
            SentenceStream& sentences,
            TTS& tts
        ):
            Chatbot(owns, name, history, printer),
            sentences(sentences), tts(tts)
        {}

        virtual ~Talkbot() {
            tts.speak_stop();
        }
    
        string chunk(const string& chunk) override { 
            // Chatbot::chunk(chunk);
            sentences.write(chunk);
            bool interrupted = tts.is_speaking();
            string told = "";
            string sentence = "***";
            while (!(sentence).empty()) {
                sentence = sentences.read();
                if (interrupted) continue; 
                this->printer.print(sentence);          
                interrupted = !tell(sentence);
                if (interrupted) continue;
                told += sentence;
            };
            if (interrupted) throw cancel();
            // return chunk;
            return told;
        }
    
        string response(const string& response) override {
            sentences.flush();
            tell(sentences.read());
            return response;
        }
    
        bool tell(const string& text) {
            return tts.speak(text);
        }

        string respond(const string& /*sender*/, const string& /*text*/) override {
            throw ERROR("Talkbots does not support full completion resonse.");
        }

        // ----- JSON serialization -----

        // void fromJSON(const JSON& json) override {
        //     DEBUG("Talkbot::fromJSON called");
        //     Chatbot::fromJSON(json); // Call base class implementation
        //     DEBUG("Talkbot::fromJSON finished");
        // }

        // JSON toJSON() const override {
        //     return Chatbot::toJSON();
        // }
    
    private:
        SentenceStream& sentences;
        TTS& tts;
    };        

}
