#pragma once

#include <string>

#include "../../../voice/SentenceStream.hpp"
#include "../../../voice/TTS.hpp"

using namespace std;
using namespace tools::voice;

namespace tools::agency::agents::plugins {

    template<typename T>
    class ChatbotPlugin: public ChatPlugin {
    public:
        ChatbotPlugin(
            Owns& owns,
            const string& lang,
            const string& instruct_persona,
            const string& instruct_stt,
            // const string& instruct_tts,
            const string& instruct_lang,
            UserAgentInterface<T>& interface

            // talkbot:
            // bool talks,
            // SentenceStream* sentences,
            // void* sentences,
            // TTS& tts
        ): 
            ChatPlugin(), 
            owns(owns),
            lang(lang),
            instruct_persona(instruct_persona),
            instruct_stt(instruct_stt),
            // instruct_tts(instruct_tts),
            instruct_lang(instruct_lang),
            interface(interface)

            // talkbot:
            // talks(talks),
            // sentences(sentences),
            // sentences(owns.reserve<SentenceStream>(this, sentences, FILELN)),
            // tts(tts)
        {}
    
        virtual ~ChatbotPlugin() {
            // // talkbot:
            // owns.release(this, sentences);
            // tts.speak_stop();
        }

        // virtual bool isTalks() const { return talks; }

        // virtual void setTalks(bool talks) { this->talks = talks; }
    
        string processInstructions(Chatbot* chatbot, const string& instructions) override {
            safe(chatbot);
            return instructions 
                // add instruct_persona
                + instruct_persona + "\n"
    
                // add STT instruction
                + (interface.getSttSwitchRef().is_on() ? instruct_stt : "") + "\n"
    
                // // add TTS instruction
                // + (chatbot->isTalks() ? instruct_tts : "") + "\n"
    
                // add language instruction
                + tpl_replace({{ "{{lang}}", lang }}, instruct_lang) + "\n";
        }

        string processChunk(Chatbot* chatbot, const string& chunk) override {
            // if (safe(chatbot)->isTalks()) { // talkbot:
            //     safe(sentences)->write(chunk);
            //     bool interrupted = tts.is_speaking();
            //     string told = "";
            //     string sentence = "???";
            //     while (!sentence.empty()) {
            //         sentence = sentences->read();
            //         if (interrupted) continue; 
            //         // this->printer.print(sentence);      
            //         interface.print(sentence);    
            //         interrupted = !tell(sentence);
            //         if (interrupted) continue;
            //         told += sentence;
            //     };
            //     if (interrupted) throw Chatbot::cancel();
            //     // return chunk;
            //     return told;
            // }
            if (!safe(chatbot)->isTalks()) interface.print(chunk);
            return chunk;
        }

        string processResponse(Chatbot* /*chatbot*/, const string& response) override {
            // if (safe(chatbot)->isTalks()) {
            //     sentences->flush();
            //     tell(sentences->read());
            // }
            return response;
        }

        string processRespond(Chatbot* /*chatbot*/, const string& /*sender*/, const string& text) override {
            // if (safe(chatbot)->isTalks()) 
            //     throw ERROR("Talkbots does not support full completion resonse.");
            return text;
        }
        
        string processChat(Chatbot* /*chatbot*/, const string& /*sender*/, const string& text, bool& /*interrupted*/) override {
            return text;
        }
    
    private:
        Owns& owns; 
        string lang;
        string instruct_persona;
        string instruct_stt;
        // string instruct_tts;
        string instruct_lang;
        UserAgentInterface<T>& interface;

        // // talkbot:
        // // bool talks = true;
        // SentenceStream* sentences = nullptr;
        // TTS& tts;
    };
    

}
