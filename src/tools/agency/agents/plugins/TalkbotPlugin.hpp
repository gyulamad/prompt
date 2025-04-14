#pragma once

#include <string>

#include "../../../voice/SentenceStream.hpp"
#include "../../../voice/TTS.hpp"

using namespace std;
using namespace tools::voice;

namespace tools::agency::agents::plugins {


    template<typename T>
    class TalkbotPlugin: public ChatPlugin {
    public:
        TalkbotPlugin(
            Owns& owns,
            const string& instruct_tts,
            UserAgentInterface<T>& interface,

            // talkbot:
            // bool talks,
            // SentenceStream* sentences,
            void* sentences,
            TTS& tts
        ): 
            ChatPlugin(), 
            owns(owns),
            instruct_tts(instruct_tts),
            interface(interface),

            // talkbot:
            // talks(talks),
            // sentences(sentences),
            sentences(owns.reserve<SentenceStream>(this, sentences, FILELN)),
            tts(tts)
        {}
    
        virtual ~TalkbotPlugin() {
            // talkbot:
            owns.release(this, sentences);
            tts.speak_stop();
        }
    
        string processInstructions(Chatbot* chatbot, const string& instructions) override {
            safe(chatbot);
            return instructions 
                // // add instruct_persona
                // + instruct_persona + "\n"
    
                // // add STT instruction
                // + (interface.getSttSwitchRef().is_on() ? instruct_stt : "") + "\n"
    
                // add TTS instruction
                + (chatbot->isTalks() ? instruct_tts : "") + "\n";
    
                // // add language instruction
                // + tpl_replace({{ "{{lang}}", lang }}, instruct_lang) + "\n";
        }

        string processChunk(Chatbot* chatbot, const string& chunk) override {
            if (safe(chatbot)->isTalks()) { // talkbot:
                safe(sentences)->write(chunk);
                bool interrupted = tts.is_speaking();
                string told = "";
                string sentence = "???";
                while (!sentence.empty()) {
                    sentence = sentences->read();
                    if (interrupted) continue; 
                    // this->printer.print(sentence);      
                    interface.print(sentence);    
                    interrupted = !tell(sentence);
                    if (interrupted) continue;
                    told += sentence;
                };
                if (interrupted) throw Chatbot::cancel();
                // return chunk;
                return told;
            }
            // interface.print(chunk);
            return chunk;
        }

        string processResponse(Chatbot* chatbot, const string& response) override {
            if (safe(chatbot)->isTalks()) {
                sentences->flush();
                tell(sentences->read());
            }
            return response;
        }

        string processRespond(Chatbot* chatbot, const string& /*sender*/, const string& text) override {
            if (safe(chatbot)->isTalks()) 
                throw ERROR("Talkbots does not support full completion resonse.");
            return text;
        }

    protected:

        // talkbot:
        bool tell(const string& text) {
            return tts.speak(text);
        }

    private:
        Owns& owns; 
        string instruct_tts;
        UserAgentInterface<T>& interface;

        // talkbot:
        // bool talks = true;
        SentenceStream* sentences = nullptr;
        TTS& tts;
    };
    
}