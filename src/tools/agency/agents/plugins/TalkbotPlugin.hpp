#pragma once

#include <string>

#include "../../../voice/SentenceStream.hpp"
#include "../../../voice/TTS.hpp"
#include "../../../str/FrameTokenParser.hpp" // Added include

using namespace std;
using namespace tools::voice;
using namespace tools::str; // Added namespace

namespace tools::agency::agents::plugins {


    template<typename T>
    class TalkbotPlugin: public ChatPlugin {
    public:
        TalkbotPlugin(
            Owns& owns,
            const string& instruct_tts,
            UserAgentInterface<T>& interface,

            // talkbot:
            // bool talks,
            // SentenceStream* sentences,
            void* sentences,
            TTS& tts,

            // Tokens for filtering
            const string& tool_start_token, // = "<tool_use>", // TODO: Externalize
            const string& tool_stop_token, // = "</tool_use>", // TODO: Externalize
            const string& code_start_token, // = "```", // TODO: Externalize
            const string& code_stop_token // = "```" // TODO: Externalize
        ):
            ChatPlugin(),
            owns(owns),
            instruct_tts(instruct_tts),
            interface(interface),

            // talkbot:
            // talks(talks),
            // sentences(sentences),
            sentences(owns.reserve<SentenceStream>(this, sentences, FILELN)),
            tts(tts),

            // Store tokens
            tool_start_token(tool_start_token),
            tool_stop_token(tool_stop_token),
            code_start_token(code_start_token),
            code_stop_token(code_stop_token)
        {}

        virtual ~TalkbotPlugin() {
            // talkbot:
            owns.release(this, sentences);
            tts.speak_stop();
        }
    
        string processInstructions(Chatbot* chatbot, const string& instructions) override {
            safe(chatbot);
            return instructions 
                // // add instruct_persona
                // + instruct_persona + "\n"
    
                // // add STT instruction
                // + (interface.getSttSwitchRef().is_on() ? instruct_stt : "") + "\n"
    
                // add TTS instruction
                + (chatbot->isTalks() ? instruct_tts : "") + "\n";
    
                // // add language instruction
                // + tpl_replace({{ "{{lang}}", lang }}, instruct_lang) + "\n";
        }

        string processChunk(Chatbot* chatbot, const string& chunk) override {
            if (safe(chatbot)->isTalks()) { // talkbot:
                // Step 1: Remove tool use content entirely
                string text_without_tool_use = toolUseParser.parse(
                    chunk,
                    tool_start_token,
                    tool_stop_token,
                    [](const string&){ /* Discard tool use content */ }
                );

                // Step 2: Filter out code blocks for speech path only, print them via callback
                string speakable_chunk = codeBlockParser.parse(
                    text_without_tool_use,
                    code_start_token,
                    code_stop_token,
                    // Callback prints the code block content directly
                    [&](const string& code_content) {
                        interface.print(code_start_token + code_content + code_stop_token);
                    }
                );

                // Step 3: Feed speakable part (text outside code blocks) to sentence stream
                safe(sentences)->write(speakable_chunk);

                bool interrupted = tts.is_speaking();
                string sentence = "???"; // Initial non-empty value to enter loop
                while (!sentence.empty()) {
                    sentence = sentences->read();
                    if (sentence.empty()) break; // Exit if no complete sentence read
                    if (interrupted) continue; // Skip speaking/printing if already interrupted

                    // Print the sentence that will be spoken
                    interface.print(sentence); // Kept: Print spoken parts incrementally

                    // Speak the sentence
                    interrupted = !tell(sentence);
                    if (interrupted) continue; // Skip rest of loop if speaking failed/stopped
                    // told += sentence; // Removed: Not needed
                };

                if (interrupted) throw Chatbot::cancel();

                // Step 4: Return text *with* code blocks but *without* tool use for potential final display by caller
                return text_without_tool_use;
            }
            // interface.print(chunk); // Original print for non-talking mode (if needed)
            return chunk; // Return original chunk if not talking
        }

        string processResponse(Chatbot* chatbot, const string& response) override {
            if (safe(chatbot)->isTalks()) {
                sentences->flush();
                string final_sentence = sentences->read(); // Get any remaining partial sentence
                if (!final_sentence.empty()) {
                    // Print and speak the final part
                    interface.print(final_sentence);
                    tell(final_sentence);
                }
                // Reset parsers for the next message
                toolUseParser.reset();
                codeBlockParser.reset();
            }
            return response;
        }

        string processCompletion(Chatbot* chatbot, const string& /*sender*/, const string& text) override {
            if (safe(chatbot)->isTalks()) 
                throw ERROR("Talkbots does not support full completion resonse.");
            return text;
        }
        
        string processChat(Chatbot* /*chatbot*/, const string& /*sender*/, const string& text, bool& /*interrupted*/) override {
            return text;
        }

    protected:

        // talkbot:
        bool tell(const string& text) {
            return tts.speak(text);
        }

    private:
        Owns& owns; 
        string instruct_tts;
        UserAgentInterface<T>& interface;

        // talkbot:
        // bool talks = true;
        SentenceStream* sentences = nullptr;
        TTS& tts;

        // Tokens
        string tool_start_token;
        string tool_stop_token;
        string code_start_token;
        string code_stop_token;

        // Parsers
        FrameTokenParser toolUseParser;
        FrameTokenParser codeBlockParser;
    };
    
}
