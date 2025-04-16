#pragma once


#include "../../../utils/Curl.hpp"
#include "../../chat/ChatPlugin.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::agency::chat;

namespace tools::agency::agents::plugins {

    class ChatApiPlugin: public ChatPlugin {
    public:
        ChatApiPlugin(
            vector<string> headers,
            long timeout,
            bool verifySSL
        ):
            ChatPlugin(),
            headers(headers), 
            timeout(timeout),
            verifySSL(verifySSL)
        {}

        virtual ~ChatApiPlugin() {}

        string processChat(Chatbot* chatbot, const string& sender, const string& text, bool& interrupted) override {
            if (interrupted) return text;

            // TODO: Move history appending logic to HistoryPlugin
            ChatHistory* history = safe((ChatHistory*)chatbot->getHistoryPtr());
            history->append(sender, text);

            string response = stream(chatbot, interrupted);

            string name = chatbot->getName();
            response = chatbot->response(response);
            history->append(name, response);
            if (interrupted) history->append(sender, getInterruptionFeedback(name, sender));

            return response;
        }

        string stream(Chatbot* chatbot, bool& interrupted) {
            Curl curl;
            for (const string& header: headers) curl.AddHeader(header);
            // curl.AddHeader("Content-Type: application/json");
            // curl.AddHeader("Accept: application/json");
            curl.SetTimeout(timeout);
            curl.SetVerifySSL(verifySSL); //true
            
            string url = getUrl();

            string data = getProtocolData(chatbot);
            // DEBUG(data);
            
            // TODO: if error happens because the rate limit, check all the variant (from API endpoint), and pick the next suitable
            string response;
            interrupted = false;
            try {
                if (!curl.POST(url, [&](const string& chunk) {
                    // Process SSE events
                    vector<string> splits = explode("\r\n\r\n", chunk);
                    for (const string& split : splits) {
                        string text = processSSEEvent(split);
                        if (!text.empty()) {
                            string output = chatbot->chunk(text);
                            response += output;
                        }
                    }
                }, data)) throw ERROR("Error requesting chat API"); 
            } catch (Chatbot::cancel&) {
                interrupted = true;
            }

            return response;
        }

        virtual string getUrl() = 0;
        virtual string getProtocolData(Chatbot* chatbot) = 0;
        virtual string processSSEEvent(const string& split) = 0;
        virtual string getInterruptionFeedback(const string& name, const string& sender) = 0;

    protected:
        vector<string> headers;
        long timeout;
        bool verifySSL;
    };    

}
