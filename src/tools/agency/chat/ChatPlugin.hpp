#pragma once

#include <map>
#include <string>
#include <vector>

using namespace std;

namespace tools::agency::chat {

    class Chatbot;

    class ChatPlugin {
    public:
        ChatPlugin() {}
        virtual ~ChatPlugin() {}

        virtual string process(Chatbot* /*chatbot*/, const string&) = 0;
    };

    enum ChatPlugType { INSTRUCT };

    using ChatPlugins = map<ChatPlugType, vector<ChatPlugin*>>;

} // namespace tools::agency::chat;
