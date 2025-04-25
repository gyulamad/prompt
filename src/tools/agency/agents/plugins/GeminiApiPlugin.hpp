#pragma once

#include <string>

#include "ChatApiPlugin.hpp"

namespace tools::agency::agents::plugins {

    class GeminiApiPlugin: public ChatApiPlugin {
    public:
        GeminiApiPlugin(
            const string& url,
            const string& secret, 
            const string& variant,
            vector<string> headers,
            long timeout,
            bool verifySSL,
            const string& interruptionFeedback
        ):
            ChatApiPlugin(headers, timeout, verifySSL),
            url(url),
            secret(secret),
            variant(variant),
            // timeout(timeout)
            interruptionFeedback(interruptionFeedback)
        {}

        virtual ~GeminiApiPlugin() {}

        string processInstructions(Chatbot* /*chatbot*/, const string& instructions) override {
            return instructions;
        }

        string processChunk(Chatbot* /*chatbot*/, const string& chunk) override {
            return chunk;
        }
        
        string processResponse(Chatbot* /*chatbot*/, const string& response) override {
            return response;
        }
        
        string processCompletion(Chatbot* /*chatbot*/, const string& /*sender*/, const string& text) override {
            return text;
        }

        string getUrl() {
            return tpl_replace({
                { "{{variant}}", variant },
                { "{{secret}}", escape(secret) },
            }, url);
            // string url = "https://generativelanguage.googleapis.com/v1beta/models/" 
            //     + variant + ":streamGenerateContent?alt=sse&key=" + escape(secret);
            // return url;
        }
        
        string getProtocolData(Chatbot* chatbot) override {
            JSON data; // Create JSON object

            // Add system instruction if provided
            string instructions = chatbot->getInstructions();
            if (!instructions.empty()) {
                data.set(".system_instruction.parts[0].text", instructions);
            }

            // Add message history to contents
            ChatHistory* history = safe((ChatHistory*)chatbot->getHistoryPtr());
            vector<ChatMessage> messages = history->getMessages();
            int content_idx = 0;
            for (const ChatMessage& message: messages) {
                string sender = message.getSender();
                if (sender.empty()) continue;
                string text = message.getText();
                if (text.empty()) continue;

                string name = chatbot->getName();
                string role = (sender == name) ? "model" : "user";
                
                // Use index for array elements
                string base_selector = ".contents[" + ::to_string(content_idx) + "]";
                data.set(base_selector + ".role", role);
                data.set(base_selector + ".parts[0].text", text); // No need to escape, JSON class handles it

                content_idx++;
            }

            // Return formatted JSON string
            return data.dump(4);
        }

        string processSSEEvent(const string& split) {
            string trm = trim(split);
            if (trm.empty()) return "";
            if (!str_starts_with(trm, "data:")) throw ERROR("Invalid SSE response: " + trm);
            vector<string> parts = explode("data: ", trm);
            if (parts.size() < 2) throw ERROR("Invalid SSE data: " + trm);
            JSON json(parts[1]);
            if (json.isDefined("error")) throw ERROR("Gemini error: " + json.dump());
            if (!json.isDefined("candidates[0].content.parts[0].text")) throw ERROR("Gemini error: text is not defined: " + json.dump());
            string text = json.get<string>("candidates[0].content.parts[0].text");

            return text;
        }

        string getInterruptionFeedback(const string& name, const string& sender) override {
            return tpl_replace({
                { "{{name}}", name },
                { "{{sender}}", sender },
            }, interruptionFeedback);
        }

    private:
        string url;
        string secret;
        string variant;
        string interruptionFeedback;
        // long timeout;

        // UserAgentInterface<T>& interface;
    };

}
