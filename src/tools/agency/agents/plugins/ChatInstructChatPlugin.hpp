#pragma once

namespace tools::agency::agents::plugins {

    template<typename T>
    class ChatInstructChatPlugin: public ChatPlugin {
    public:
        ChatInstructChatPlugin(
            const string& lang,
            const string& instruct_persona,
            const string& instruct_stt,
            const string& instruct_tts,
            const string& instruct_lang,
            UserAgentInterface<T>& interface
        ): 
            ChatPlugin(), 
            lang(lang),
            instruct_persona(instruct_persona),
            instruct_stt(instruct_stt),
            instruct_tts(instruct_tts),
            instruct_lang(instruct_lang),
            interface(interface)
        {}
    
        virtual ~ChatInstructChatPlugin() {}
    
        string process(Chatbot* chatbot, const string& instructions) override {
            safe(chatbot);
            return instructions 
                // add instruct_persona
                + instruct_persona + "\n"
    
                // add STT instruction
                + (interface.getSttSwitchRef().is_on() ? instruct_stt : "") + "\n"
    
                // add TTS instruction
                + (chatbot->isTalks() ? instruct_tts : "") + "\n"
    
                // add language instruction
                + tpl_replace({{ "{{lang}}", lang }}, instruct_lang) + "\n";
        }
    
    private:
        string lang;
        string instruct_persona;
        string instruct_stt;
        string instruct_tts;
        string instruct_lang;
        UserAgentInterface<T>& interface;
    };
    

}
