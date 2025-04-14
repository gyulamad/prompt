#pragma once

#include <string>

#include "../../../str/tpl_replace.hpp"
#include "../../../str/FrameTokenParser.hpp"
#include "../../chat/ChatPlugin.hpp"

#include "Tool.hpp"

using namespace std;
using namespace tools::str;
using namespace tools::agency::chat;

namespace tools::agency::agents::plugins {

    class ToolusePlugin: public ChatPlugin {
    public:
        ToolusePlugin(
            const string& instruct_tooluse,
            const string& tooluse_start_token,
            const string& tooluse_stop_token
        ):
            ChatPlugin(),
            instruct_tooluse(instruct_tooluse),
            tooluse_start_token(tooluse_start_token),
            tooluse_stop_token(tooluse_stop_token)
        {}

        virtual ~ToolusePlugin() {}
        
        string processInstructions(Chatbot* /*chatbot*/, const string& instructions) override {
            return instructions + tpl_replace({
                { "{{tools}}", to_string(tools) },
                { "{{tooluse_start_token}}", tooluse_start_token},
                { "{{tooluse_stop_token}}", tooluse_stop_token},
            }, instruct_tooluse);
        }

        string buffer;
        bool in_tokens;
        string inner;
        void reset() {
            buffer = "";
            in_tokens = false;
            inner = "";
        }
        
        string processChunk(Chatbot* /*chatbot*/, const string& chunk) override {
            // TODO: !@# the processChunk proceeds an AI inference stream output.
            // task: we have to puffer the chunks and cut the chunk where it contains a tooluse start token, store afterwards,
            // until a tooluse stop token reached in the chunk (or later recieved chunks)
            // when the tooluse stop received, call the processFunctionCall(string tooluse-content)
            // the returned chunk should not contains the tooluse outputs from the AI

            return parser.parse(chunk, tooluse_start_token, tooluse_stop_token, [this](const string& inner) {
                processFunctionCall(inner);
            });
        }
        
        string processResponse(Chatbot* /*chatbot*/, const string& response) override {
            return response;
        }
        
        string processRespond(Chatbot* /*chatbot*/, const string& /*sender*/, const string& text) override {
            return text;
        }
        
        string processChat(Chatbot* /*chatbot*/, const string& /*sender*/, const string& text, bool& /*interrupted*/) override {
            parser.reset();
            return text;
        }

        virtual void processFunctionCall(const string& functionCall) {// = 0;
            DEBUG(functionCall);
            // TODO: Implement actual tool execution logic here based on functionCall content
        }
        
    private:
        vector<Tool*> tools;
        string instruct_tooluse;
        string tooluse_start_token;
        string tooluse_stop_token;

        FrameTokenParser parser;
    };

}
