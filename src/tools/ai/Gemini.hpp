#pragma once

// #include <string>

// #include "../utils/Printer.hpp"
// #include "../chat/ChatHistory.hpp"

// using namespace std;

#include "../str/explode.hpp"
#include "../str/trim.hpp"
#include "../str/str_starts_with.hpp"
#include "../str/json_escape.hpp"
#include "../utils/Curl.hpp"
#include "../utils/JSON.hpp"
#include "../chat/Talkbot.hpp"

using namespace tools::str;
using namespace tools::utils;
using namespace tools::chat;

namespace tools::ai {

    class Gemini {
    public:
        Gemini(
            const string& secret, 
            const string& variant,
            long timeout
        ):
            secret(secret),
            variant(variant),
            timeout(timeout)
        {}

        virtual ~Gemini() {}

    protected:

        string chat(Chatbot& chatbot, const string& sender, const string& text, bool& interrupted) {
            Curl curl;
            curl.AddHeader("Content-Type: application/json");
            curl.AddHeader("Accept: application/json");
            curl.SetTimeout(timeout);
            curl.SetVerifySSL(true);
            
            string url = "https://generativelanguage.googleapis.com/v1beta/models/" 
                + variant + ":streamGenerateContent?alt=sse&key=" + escape(secret);            
    
            ChatHistory& history = *safe((ChatHistory*)chatbot.getHistoryPtr());
            
            history.append(sender, text);

            string data = tpl_replace({
                { "{{history}}", json_escape(history.toString()) }, 
                { "{{start}}", json_escape(history.startToken(chatbot.getName())) },
            }, R"({
                "contents": [{
                    "parts": [{
                        "text": "{{history}}{{start}}"
                    }]
                }]
            })");
            
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
        
                        response += chatbot.chunk(text);
                    }
                }, data)) throw ERROR("Error requesting Gemini API"); 
            } catch (Talkbot::cancel&) {
                interrupted = true;
            }

            history.append(chatbot.getName(), chatbot.response(response));
            if (interrupted) history.append(sender, "['" + chatbot.getName() + "' interrupted by '" + sender + "']");

            return response;
        }

        // TODO: load with curl but no need to show (it's for internal conversations)
        string respond(Chatbot& /*chatbot*/, const string& /*sender*/, const string& /*text*/) {
            STUB("Needs to be implemented");
            throw ERROR("Unimplemented");
        }

    private:
        string secret;
        string variant;
        long timeout;
    };    

}
