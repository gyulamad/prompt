#pragma once

#include <string>
#include <vector>

#include "tools/JSON.hpp"

#include "Message.hpp"

using namespace std;
using namespace tools;

namespace prompt {

    class Conversation {
    private:
        vector<Message> messages;
    public:
        Conversation() {}
        
        virtual ~Conversation() {}

        JSON toJSON() const {
            json jmessages = json::array();
            for (const Message& message: messages)
                jmessages.push_back(message.toJSON().get_json());
            JSON json(jmessages);
            return json;
        }

        void fromJSON(JSON J) {
            messages.clear();
            //DEBUG(J.dump());
            json jmessages = J.get_json();
            for (const auto& jmessage: jmessages) {
                add(jmessage.at("text"), (role_t)jmessage.at("role"));
            }
        }
        
        void add(const string& text, role_t role = ROLE_NONE) {
            messages.push_back({ text, role });
        }

        bool empty() const {
            return messages.empty();
        }

        Message pop() {
            Message last = messages.back();
            messages.pop_back();
            return last;
        }

        void clear() {
            messages.clear();
        }

        const vector<Message>& get_messages_ref() const {
            return messages;
        }

        size_t length() const {
            size_t l = 0;
            for (const Message& message: messages) l += message.length() + 1;
            return l;
        }

    };
    
}