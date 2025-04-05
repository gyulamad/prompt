#pragma once

#include <string>

#include "../abstracts/JSONSerializable.hpp"

using namespace std;

using namespace tools::abstracts;

namespace tools::chat {

    class ChatMessage: public JSONSerializable {
    public:
        using JSONSerializable::JSONSerializable;

        ChatMessage(
            const string& sender,
            const string& text 
        ):  
            sender(sender),
            text(text) 
        {}    

        virtual ~ChatMessage() {}

        string getSender() const { return sender; }
        string getText() const { return text; }

        // ----- serialization -----

        void fromJSON(const JSON& json) override {
            sender = json.get<string>("sender");
            text = json.get<string>("text");
        }
    
    protected:
        string sender;
        string text;
    };

}