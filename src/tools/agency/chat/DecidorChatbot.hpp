#pragma once

#include "Chatbot.hpp"

namespace tools::agency::chat {

    class DecidorChatbot: public Chatbot {
    public:

        DecidorChatbot(
            Owns& owns,
            const string& name, // TODO: do we need name here?
            void* history, 
            Printer& printer,

            // talkbot:
            bool talks,
            SentenceStream& sentences,
            TTS& tts
        ):
            Chatbot(owns, name, history, printer, talks, sentences, tts)
        {}

        virtual ~DecidorChatbot() {}

        string chunk(const string& chunk) override {
            printer.print(chunk);
            return chunk;
        }
    
        string response(const string& response) override {
            return response;
        }

        // TODO: add chat/stream mode to the chatbot (avoid talkbot concept separation)
        string chat(const string& /*sender*/, const string& /*text*/, bool& /*interrupted*/) override {
            throw ERROR("DecidorChatbot does not support chat stream resonse.");
        }

        // string respond(const string& sender, const string& text) override {
        //     STUB("Needs to be implemented");
        //     return "";
        // }
        
        // ----- decision making -----
        enum Decision { YES, NO, CANCEL };
        // virtual bool confirm(const string& sender, const string& text) = 0;
        // virtual Decision decide(const string& sender, const string& text) = 0;
        // virtual int choose(const string& sender, const string& text, const vector<string>& options, bool optional = false) = 0;
        // virtual vector<int> select(const string& sender, const string& text, const vector<string>& options, bool optional = false) = 0;



        virtual bool confirm(const string& /*sender*/, const string& /*text*/) {
            STUB("Needs to be implemented");
            return false;
        }

        virtual Decision decide(const string& /*sender*/, const string& /*text*/) {
            STUB("Needs to be implemented");
            return NO;
        }

        virtual int choose(const string& /*sender*/, const string& /*text*/, const vector<string>& /*options*/, bool /*optional*/ = false) {
            STUB("Needs to be implemented");
            return -1;
        }

        virtual vector<int> select(const string& /*sender*/, const string& /*text*/, const vector<string>& /*options*/, bool /*optional*/ = false) {
            STUB("Needs to be implemented");
            return {};
        }
    };

}
