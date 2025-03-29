#pragma once

#include <string>

#include "../utils/Printer.hpp"
#include "../utils/Curl.hpp"
#include "../utils/JSON.hpp"
#include "../str/trim.hpp"
#include "../str/json_escape.hpp"
#include "../str/explode.hpp"
#include "../str/str_starts_with.hpp"
#include "../chat/Talkbot.hpp"
#include "../chat/ChatHistory.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::chat;
using namespace tools::str;

namespace tools::ai {

    class Gemini: public Talkbot {
    public:
    
        Gemini(
            const string& name, 
            ChatHistory& history, 
            Printer& printer,
            SentenceStream& sentences,
            TTS& tts,
            const string& secret, 
            const string& variant,
            long timeout
        ): 
            Talkbot(name, history, printer, sentences, tts),
            secret(secret),
            variant(variant),
            timeout(timeout)
        {}
    
        string chat(const string& sender, const string& text) override {
            Curl curl;
            curl.AddHeader("Content-Type: application/json");
            curl.AddHeader("Accept: application/json");
            curl.SetTimeout(timeout);
            curl.SetVerifySSL(true);
            
            string url = "https://generativelanguage.googleapis.com/v1beta/models/" 
                + variant + ":streamGenerateContent?alt=sse&key=" + escape(secret);            
    
            history.append(sender, text);
            string data = tpl_replace({
                { "{{history}}", json_escape(history.toString()) }, 
                { "{{start}}", json_escape(history.startToken(name)) },
            }, R"({
                "contents": [{
                    "parts": [{
                        "text": "{{history}}{{start}}"
                    }]
                }]
            })");
            
            // TODO: if error happens because the rate limit, check all the variant (from API endpoint), and pick the next suitable
            string response;
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
            history.append(name, this->response(response));
            return response;
        }
    
    private:
        string secret;
        string variant;
        long timeout;
    };

}