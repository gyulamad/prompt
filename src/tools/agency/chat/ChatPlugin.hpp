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

        virtual string processInstructions(Chatbot* /*chatbot*/, const string&) = 0;
        virtual string processChunk(Chatbot* /*chatbot*/, const string&) = 0;
        virtual string processResponse(Chatbot* /*chatbot*/, const string&) = 0;
        virtual string processCompletion(Chatbot* /*chatbot*/, const string& /*sender*/, const string& /*text*/) = 0;
        virtual string processChat(Chatbot* /*chatbot*/, const string& /*sender*/, const string& /*text*/, bool& /*interrupted*/) = 0;
    };

    // class ChatPlugins {
    // public:
    //     ChatPlugins(Owns& owns): owns(owns) {}
        
    //     virtual ~ChatPlugins() {
    //         for (void* plug: plugs)
    //             owns.release(this, plug);
    //     }

    //     vector<void*> getPlugs() const { return plugs; }

    //     template<typename T>
    //     void push(void* plug) {
    //         owns.reserve<T>(this, plug, FILELN);
    //         this->plugs.push_back(plug);
    //     }

    // private:
    //     Owns& owns;
    //     vector<void*> plugs;
    // };

} // namespace tools::agency::chat;
