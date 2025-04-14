#pragma once

#include <vector> // Added for vector
#include "../../../str/str_starts_with.hpp"
#include "../../../utils/Curl.hpp"
#include "../../../utils/JSON.hpp" // Added for JSON
#include "../../chat/Chatbot.hpp"
#include "../../chat/ChatMessage.hpp" // Added for ChatMessage

// #include "Gemini.hpp"

using namespace std;
using namespace tools::str;
using namespace tools::utils;
using namespace tools::agency::chat;

namespace tools::agency::ai {

    template<typename T>
    class _GeminiChatbot: /*public Gemini,*/ public Chatbot {
    public:
        _GeminiChatbot(
            Owns& owns,
            const string& secret, 
            const string& variant,
            long timeout,
            const string& name,  // TODO: remove it
            // const string& instructions,
            ChatHistory* history,
            // Printer& printer,
            // UserAgentInterface<T>& interface,

            // plugins:
            ChatPlugins* plugins,

            // talkbot:
            bool talks
            // SentenceStream& sentences,
            // TTS& tts
        ):
            // Gemini(secret, variant, timeout),
            Chatbot(owns, name, history, plugins, talks),
            secret(secret),
            variant(variant),
            timeout(timeout)
            // interface(interface)
        {}

        virtual ~_GeminiChatbot() {
            // cout << "GeminiChatbot (" + this->name + ") destruction..." << endl;
        }

        // string respond(const string& sender, const string& text) override {
        //     return Gemini::respond(*this, sender, text);
        // }
        // TODO: load with curl but no need to show (it's for internal conversations)
        // string respond(const string& /*sender*/, const string& /*text*/) override {
        //     STUB("Needs to be implemented");
        //     throw ERROR("Unimplemented");
        // }
        
        // string chat(const string& sender, const string& text, bool& interrupted) override {
        //     return Gemini::chat(*this, sender, text, interrupted);
        // }


        string chat(const string& sender, const string& text, bool& interrupted) override {
            Curl curl;
            curl.AddHeader("Content-Type: application/json");
            curl.AddHeader("Accept: application/json");
            curl.SetTimeout(timeout);
            curl.SetVerifySSL(true);
            
            string url = "https://generativelanguage.googleapis.com/v1beta/models/" 
                + variant + ":streamGenerateContent?alt=sse&key=" + escape(secret);            
    
            // ChatHistory& history = *safe((ChatHistory*)chatbot.getHistoryPtr());
            
            history->append(sender, text);

            string data = getProtocolData();
            // DEBUG(data);
            
            // TODO: if error happens because the rate limit, check all the variant (from API endpoint), and pick the next suitable
            string response;
            interrupted = false;
            try {
                if (!curl.POST(url, [&](const string& chunk) {
                    // Process SSE events
                    vector<string> splits = explode("\r\n\r\n", chunk);
                    for (const string& split : splits) {
                        string trm = trim(split);
                        if (trm.empty()) continue;
                        if (!str_starts_with(trm, "data:")) throw ERROR("Invalid SSE response: " + trm);
                        vector<string> parts = explode("data: ", trm);
                        if (parts.size() < 2) throw ERROR("Invalid SSE data: " + trm);
                        JSON json(parts[1]);
                        if (json.isDefined("error")) throw ERROR("Gemini error: " + json.dump());
                        if (!json.isDefined("candidates[0].content.parts[0].text")) throw ERROR("Gemini error: text is not defined: " + json.dump());
                        string text = json.get<string>("candidates[0].content.parts[0].text");
        
                        string output = this->chunk(text);
                        // interface.print(output);
                        response += output;
                    }
                }, data)) throw ERROR("Error requesting Gemini API"); 
            } catch (Chatbot::cancel&) {
                interrupted = true;
            }

            history->append(name, this->response(response));
            if (interrupted) history->append(sender, "['" + name + "' interrupted by '" + sender + "']");

            return response;
        }

        // string chunk(const string& chunk) override {
        //     if (this->talks) Chatbot::chunk(chunk);
        //     else printer.print(chunk);
        //     return chunk;
        // }
        
        // string response(const string& response) override {
        //     return response;
        // }

    protected:
        string getProtocolData() {
            JSON data; // Create JSON object

            // Add system instruction if provided
            string instructions = getInstructions();
            if (!instructions.empty()) {
                data.set(".system_instruction.parts[0].text", instructions);
            }

            // Add message history to contents
            vector<ChatMessage> messages = history->getMessages();
            int content_idx = 0;
            for (const ChatMessage& message: messages) {
                string sender = message.getSender();
                if (sender.empty()) continue;
                string text = message.getText();
                if (text.empty()) continue;

                string role = (sender == name) ? "model" : "user";
                
                // Use index for array elements
                string base_selector = ".contents[" + to_string(content_idx) + "]";
                data.set(base_selector + ".role", role);
                data.set(base_selector + ".parts[0].text", text); // No need to escape, JSON class handles it

                content_idx++;
            }

            // Return formatted JSON string
            return data.dump(4);
        }

    private:
        string secret;
        string variant;
        long timeout;

        // UserAgentInterface<T>& interface;
    };    

}
