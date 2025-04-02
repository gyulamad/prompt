#pragma once

#include <string>

#include "../str/tpl_replace.hpp"
#include "../utils/foreach.hpp"
#include "../utils/Owns.hpp"

#include "ChatMessage.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::chat {

    class ChatHistory {
    public:
        ChatHistory(
            const string& prompt,
            bool use_start_token
        ):
            prompt(prompt), 
            use_start_token(use_start_token)
        {}

        virtual ~ChatHistory() {}
    
        void append(const string& sender, const string& text) {        
            ChatMessage message(sender, text);
            messages.push_back(message);
            // TODO: check if full length is more that the bot context window and partially summarise by a customizable context overflow handler
        }
    
        string startToken(const string& prefix) {
            return use_start_token 
                ? tpl_replace({
                        { "{{prefix}}", prefix },
                        { "{{prompt}}", prompt },
                    }, "\n{{prefix}}{{prompt}}")
                : "";
        }
    
        string toString() {
            string serialized = "";
            foreach (messages, [&](const ChatMessage& message) {
                serialized += tpl_replace({
                    { "{{start}}", startToken(message.sender) },
                    { "{{text}}", message.text },
                }, "\n{{start}}{{text}}");
            });
            return serialized;
        }
    
    private:    
        string prompt;
        bool use_start_token;
        // Factory<ChatMessage> messages;
        vector<ChatMessage> messages;
    };

}