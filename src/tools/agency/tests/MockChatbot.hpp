#pragma once

#include "../chat/Chatbot.hpp"

using namespace std;
using namespace tools::agency::chat;

class MockChatbot : public Chatbot {
public:
    MockChatbot(
        Owns& owns,
        const string& name,
        const string& response,
        void* history,
        void* plugins,
        bool talks
    ) : Chatbot(owns, name, history, plugins, talks), response(response) {}

    virtual ~MockChatbot() {}

    virtual string chat(const string& sender, const string& text, bool& /*interrupted*/) override {
        ChatHistory* history = (ChatHistory*)safe(getHistoryPtr());
        history->append(sender, text);
        history->append(getName(), response);
        return response;
    }

private:
    string response;
};
