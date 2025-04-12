#pragma once

#include "../../../str/json_escape.hpp"
#include "../../../str/str_starts_with.hpp"
#include "../../../utils/Curl.hpp"
#include "../../chat/Chatbot.hpp"

// #include "Gemini.hpp"

using namespace tools::str;
using namespace tools::utils;
using namespace tools::agency::chat;

namespace tools::agency::ai {

    class GeminiChatbot: /*public Gemini,*/ public Chatbot {
    public:
        GeminiChatbot(
            Owns& owns,
            const string& secret, 
            const string& variant,
            long timeout,
            const string& name,  // TODO: remove it
            ChatHistory* history,
            Printer& printer,

            // talkbot:
            bool talks,
            SentenceStream& sentences,
            TTS& tts
        ):
            // Gemini(secret, variant, timeout),
            Chatbot(owns, name, history, printer, talks, sentences, tts),
            secret(secret),
            variant(variant),
            timeout(timeout)
        {}

        virtual ~GeminiChatbot() {
            // cout << "GeminiChatbot (" + this->name + ") destruction..." << endl;
        }

        // string respond(const string& sender, const string& text) override {
        //     return Gemini::respond(*this, sender, text);
        // }
        // TODO: load with curl but no need to show (it's for internal conversations)
        string respond(const string& /*sender*/, const string& /*text*/) override {
            STUB("Needs to be implemented");
            throw ERROR("Unimplemented");
        }
        
        // string chat(const string& sender, const string& text, bool& interrupted) override {
        //     return Gemini::chat(*this, sender, text, interrupted);
        // }


        string chat(const string& sender, const string& text, bool& interrupted) override {
            Curl curl;
            curl.AddHeader("Content-Type: application/json");
            curl.AddHeader("Accept: application/json");
            curl.SetTimeout(timeout);
            curl.SetVerifySSL(true);
            
            string url = "https://generativelanguage.googleapis.com/v1beta/models/" 
                + variant + ":streamGenerateContent?alt=sse&key=" + escape(secret);            
    
            // ChatHistory& history = *safe((ChatHistory*)chatbot.getHistoryPtr());
            
            history->append(sender, text);

            // string data = tpl_replace({
            //     { "{{history}}", json_escape(history->toString()) }, 
            //     { "{{start}}", json_escape(history->startToken(name)) },
            // }, R"({
            //     "contents": [{
            //         "parts": [{
            //             "text": "{{history}}{{start}}"
            //         }]
            //     }]
            // })");
            string data = getProtocolData();
            DEBUG(data);
            
            // TODO: if error happens because the rate limit, check all the variant (from API endpoint), and pick the next suitable
            string response;
            interrupted = false;
            try {
                if (!curl.POST(url, [&](const string& chunk) {
                    // Process SSE events
                    vector<string> splits = explode("\r\n\r\n", chunk);
                    for (const string& split : splits) {
                        string trm = trim(split);
                        if (trm.empty()) continue;
                        if (!str_starts_with(trm, "data:")) throw ERROR("Invalid SSE response: " + trm);
                        vector<string> parts = explode("data: ", trm);
                        if (parts.size() < 2) throw ERROR("Invalid SSE data: " + trm);
                        JSON json(parts[1]);
                        if (json.isDefined("error")) throw ERROR("Gemini error: " + json.dump());
                        if (!json.isDefined("candidates[0].content.parts[0].text")) throw ERROR("Gemini error: text is not defined: " + json.dump());
                        string text = json.get<string>("candidates[0].content.parts[0].text");
        
                        response += this->chunk(text);
                    }
                }, data)) throw ERROR("Error requesting Gemini API"); 
            } catch (Chatbot::cancel&) {
                interrupted = true;
            }

            history->append(name, this->response(response));
            if (interrupted) history->append(sender, "['" + name + "' interrupted by '" + sender + "']");

            return response;
        }

        string chunk(const string& chunk) override {
            if (this->talks) Chatbot::chunk(chunk);
            else printer.print(chunk);
            return chunk;
        }
        
        string response(const string& response) override {
            return response; 
        }

    protected:
        string getProtocolData() const {
            string data;
            for (const ChatMessage& message: history->getMessages()) {
                // TODO ....
            }
            return data;
        }

    private:
        string secret;
        string variant;
        long timeout;
    };    

}
