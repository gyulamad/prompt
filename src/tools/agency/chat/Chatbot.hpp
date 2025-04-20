#pragma once

#include <string>

// #include "../../utils/Printer.hpp"
// #include "../../voice/TTS.hpp"
// #include "../../voice/SentenceStream.hpp"
#include "ChatHistory.hpp"
#include "ChatPlugin.hpp"

using namespace std;
using namespace tools::voice;

namespace tools::agency::chat {

    class Chatbot { //: public JSONSerializable { //  TODO: refact: build up a correct abstraction hierarhy
    public:

        class cancel: public exception {};

        Chatbot(
            Owns& owns,
            const string& name,
            // const string& instructions,
            void* history, 
            // Printer& printer,

            // plugins:
            void* plugins,

            // talkbot:
            bool talks
            // SentenceStream& sentences,
            // TTS& tts
        ):
            // JSONSerializable(),
            owns(owns),
            name(name),
            // instructions(instructions),
            history(owns.reserve<ChatHistory>(this, history, FILELN)),
            // printer(printer),

            // plugins:
            plugins(owns.reserve<OList>(this, plugins, FILELN)),

            // talkbot:
            talks(talks)
            // sentences(sentences),
            // tts(tts)
        {}

        virtual ~Chatbot() {
            owns.release(this, history);
            owns.release(this, plugins);

            // // talkbot:
            // tts.speak_stop();
        }

        string getName() const { return name; }

        virtual string getInstructions() { 
            string proceed = "";// this->instructions;
            for (void* plugin: plugins->getPlugs())
                proceed = ((ChatPlugin*)safe(plugin))->processInstructions(this, proceed);
            return proceed; 
        }

        // void setInstructions(const string& instructions) {
        //     this->instructions = instructions;
        // }

        void* getHistoryPtr() { return safe(history); } // TODO: remove this

        virtual bool isTalks() const { return talks; }

        virtual void setTalks(bool talks) { this->talks = talks; }


        // prompt completion call
        virtual string completion(const string& sender, const string& text) {
            string proceed = text;
            for (void* plugin: plugins->getPlugs())
                proceed = ((ChatPlugin*)safe(plugin))->processCompletion(this, sender, proceed);
            return proceed; 

            // if (talks) throw ERROR("Talkbots does not support full completion resonse.");
            // else throw ERROR("Chatbots completion needs to be implemented.");
        }

        // stream chat // TODO: add as plugins!! <- gemini/request for chunks stream plugin can be used
        virtual string chat(const string& sender, const string& text, bool& interrupted) {
            string proceed = text;
            for (void* plugin: plugins->getPlugs())
                proceed = ((ChatPlugin*)safe(plugin))->processChat(this, sender, proceed, interrupted);
            return proceed; 
        }

        // on stream chunk recieved
        virtual string chunk(const string& chunk) {
            string proceed = chunk;
            for (void* plugin: plugins->getPlugs())
                proceed = ((ChatPlugin*)safe(plugin))->processChunk(this, proceed);
            return proceed; 

            // if (talks) { // talkbot:
            //     sentences.write(chunk);
            //     bool interrupted = tts.is_speaking();
            //     string told = "";
            //     string sentence = "***";
            //     while (!(sentence).empty()) {
            //         sentence = sentences.read();
            //         if (interrupted) continue; 
            //         this->printer.print(sentence);          
            //         interrupted = !tell(sentence);
            //         if (interrupted) continue;
            //         told += sentence;
            //     };
            //     if (interrupted) throw cancel();
            //     // return chunk;
            //     return told;
            // }
            // throw ERROR("Chatbots chunk needs to be implemented.");
        }

        // on full response recieved
        virtual string response(const string& response) {
            string proceed = response;
            for (void* plugin: plugins->getPlugs())
                proceed = ((ChatPlugin*)safe(plugin))->processResponse(this, proceed);
            return proceed; 

            // if (talks) {
            //     sentences.flush();
            //     tell(sentences.read());
            // }
            // return response;
        }

        // // talkbot:
        // bool tell(const string& text) {
        //     return tts.speak(text);
        // }


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
        // string instructions;
        ChatHistory* history = nullptr;
        // Printer& printer;

        // plugins:
        OList* plugins = nullptr;
    
        // talkbot:
        bool talks = true;
        // SentenceStream& sentences;
        // TTS& tts;
    };

    

}
