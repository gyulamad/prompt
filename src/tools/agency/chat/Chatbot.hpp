#pragma once

#include <string>

#include "../../utils/Printer.hpp"
#include "../../voice/TTS.hpp"
#include "../../voice/SentenceStream.hpp"
#include "ChatHistory.hpp"

using namespace std;
using namespace tools::voice;

namespace tools::agency::chat {

    class Chatbot { //: public JSONSerializable { //  TODO: refact: build up a correct abstraction hierarhy
    public:

        class cancel: public exception {};

        Chatbot(
            Owns& owns,
            const string& name,
            const string& instructions,
            void* history, 
            Printer& printer,

            // talkbot:
            bool talks,
            SentenceStream& sentences,
            TTS& tts
        ):
            // JSONSerializable(),
            owns(owns),
            name(name),
            instructions(instructions),
            history(owns.reserve<ChatHistory>(this, history, FILELN)),
            printer(printer),

            // talkbot:
            talks(talks),
            sentences(sentences),
            tts(tts)
        {}

        virtual ~Chatbot() {
            owns.release(this, history);

            // talkbot:
            tts.speak_stop();
        }

        string getName() const { return name; }

        string getInstructions() { return instructions; } // TODO: remove this

        void setInstructions(const string& instructions) {
            this->instructions = instructions;
        }

        void* getHistoryPtr() { return history; } // TODO: remove this

        virtual bool isTalks() const { return talks; }


        // prompt completion call
        virtual string respond(const string& /*sender*/, const string& /*text*/) {
            if (talks) throw ERROR("Talkbots does not support full completion resonse.");
            else throw ERROR("Chatbots respond needs to be implemented.");
        }

        // stream chat
        virtual string chat(const string& sender, const string& text, bool& interrupted) = 0;

        // on stream chunk recieved
        virtual string chunk(const string& chunk) {
            if (talks) { // talkbot:
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
            throw ERROR("Chatbots chunk needs to be implemented.");
        }

        // on full response recieved
        virtual string response(const string& response) {
            if (talks) {
                sentences.flush();
                tell(sentences.read());
            }
            return response;
        }

        // talkbot:
        bool tell(const string& text) {
            return tts.speak(text);
        }


        // ----- JSON serialization -----

        // void fromJSON(const JSON& json) override {
        // //     DEBUG("Chatbot::fromJSON called"); 
        // //     // Convert pointer to integer type for printing
        // //     uintptr_t history_addr = reinterpret_cast<uintptr_t>(history);
        // //     DEBUG("Checking history pointer address: " + to_string(history_addr)); 
        // //     safe(history); // Check history pointer before use
        // //     DEBUG("History pointer is safe"); 
        // //     history->fromJSON(json.get<JSON>("history")); // Call history's fromJSON
        // //     DEBUG("Called history->fromJSON"); 

        // //     name = json.get<string>("name");
        //     safe(history)->fromJSON(json.get<JSON>("history"));
        // }

        // JSON toJSON() const override {
        //     JSON json;
        //     json.set("history", safe(history)->toJSON());
        // //     json.set("name", name);
        //     return json;
        // }
        

    protected:
        Owns& owns;
        string name; //  TODO: remove this!
        string instructions;
        ChatHistory* history = nullptr;
        Printer& printer;
    
        // talkbot:
        bool talks = true;
        SentenceStream& sentences;
        TTS& tts;
    };

    

}
