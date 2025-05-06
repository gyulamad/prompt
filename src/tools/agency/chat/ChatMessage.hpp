#pragma once

#include <string>

#include "../../abstracts/JSONSerializable.h"

using namespace std;

using namespace tools::abstracts;

namespace tools::agency::chat {

    class ChatMessage { //: public JSONSerializable {
    public:
        // using JSONSerializable::JSONSerializable;

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

        // ----- JSON serialization -----

        // void fromJSON(const JSON& json) override {
        //     sender = json.get<string>("sender");
        //     text = json.get<string>("text");
        // }

        // JSON toJSON() const override {
        //     JSON json;
        //     json.set("sender", sender);
        //     json.set("text", text);
        //     return json;
        // }
    
    protected:
        string sender;
        string text;
    };

}