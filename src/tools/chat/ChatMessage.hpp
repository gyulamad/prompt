#pragma once

#include <string>

using namespace std;

namespace tools::chat {

    class ChatMessage {
    public:
        ChatMessage(
            const string& sender,
            const string& text 
        ):  
            sender(sender),
            text(text) 
        {}    
    
    public:
        const string sender;
        const string text;
    };

}