#pragma once

#include "tools/strings.hpp"
#include "tools/Logger.hpp"
#include "tools/Process.hpp"
#include "tools/Curl.hpp"

#include "Model.hpp"

using namespace std;
using namespace tools;

namespace prompt {

    class Gemini: public Model {
    private:
        string command;
        JSON response;
        int err_retry_sec;
        int err_attempts;
        int restarts = err_attempts;
        vector<string> sentence_delimiters;
        long stream_request_timeout;
        string tmpfile;

        Logger& logger;
        string secret;

        const vector<string> variants;
        size_t current_variant = 0;
        string variant = variants[current_variant];

        bool next_variant() {
            bool next_variant_found = true;
            current_variant++;
            if (current_variant >= variants.size()) {
                current_variant = 0;
                next_variant_found = false;
            }
            variant = variants[current_variant];
            return next_variant_found;
        }

    protected:

        string message_to_json(const Message& message, const role_name_map_t& role_name_map = {
            { ROLE_NONE, "" },
            { ROLE_INPUT, "user" },
            { ROLE_OUTPUT, "model" },
        }) {
            return tpl_replace({
                { "{{role}}", to_string(message.get_role(), role_name_map) },
                { "{{text}}", json_escape(message.get_text()) },
            }, R"({
                "role": "{{role}}",
                "parts":[{
                    "text": "{{text}}"
                }]
            })");
        }

        string conversation_to_json(const string& system, const Conversation& conversation) {
            vector<string> jsons;
            vector<Message> messages = conversation.get_messages_ref();
            for (const Message& message: messages)
                jsons.push_back(message_to_json(message));
            return tpl_replace({
                { "{{system}}", json_escape(system) },
                { "{{conversation}}", implode(",", jsons) },
            }, R"({
                "system_instruction":{
                    "parts":{
                        "text": "{{system}}"
                    }
                },
                "contents":[
                    {{conversation}}
                ]
            })");
        }

        runtime_error handle_request_error(const exception &e) {
            string what = e.what();
            string usrmsg = "Gemini API failure: " + what + "\nSwitching variant from " + variant + " to ";
            string errmsg = 
                "Gemini API (" + variant + ") request failed: " + what;
            bool next_variant_found = next_variant();
            usrmsg += variant + ".";                    
            cerr << usrmsg << endl;
            errmsg += 
                "\nContinue with variant " + variant + 
                "\nRequest was: " + command +
                "\nRequest data: " + str_cut_begin(file_get_contents(tmpfile)) +
                "\nResponse was: " + response.dump();
            logger.warning(errmsg);
            if (!next_variant_found) {
                cerr << "Retry after " << err_retry_sec << " second(s)..." << endl;
                sleep(err_retry_sec);
                restarts--;
            } else sleep(err_retry_sec); // TODO: for api rate limit
            return ERROR("Gemini API error. See more in log...");
        }
        
        virtual string request() override {
            restarts = err_attempts;
            while (restarts) {
                try {
                    // JSON request;
                    // request.set("contents[0].parts[0].text", prompt);
                    // file_put_contents(tmpfile, request.dump(4));
                    string conversation_json = conversation_to_json(system, conversation);
                    if (!file_put_contents(tmpfile, conversation_json)) {
                        throw ERROR("Unable to write: " + tmpfile);
                    }
                    //assert(file_get_contents(tmpfile) == conversation_json);
                    command = "curl -s \"https://generativelanguage.googleapis.com/v1beta/models/" + variant + ":generateContent?key=" + escape(secret) + "\" -H 'Content-Type: application/json' -X POST --data-binary @" + tmpfile;
                    //sleep(3); // TODO: for api rate limit
                    // cout << "[DEBUG] Gemini request..." << endl;
                    response = Process::execute(command);
                    // cout << "[DEBUG] Gemini request [done]" << endl;
                    if (response.isDefined("error") || !response.isDefined("candidates[0].content.parts[0].text"))
                        throw ERROR("Gemini error: " + response.dump());
                    return response.get<string>("candidates[0].content.parts[0].text");    
                } catch (exception &e) {
                    if(restarts == 0) throw handle_request_error(e);
                }
            }
            throw ERROR("Unable to reach Gemini API.");
        }

        string request_stream(
            void* context,
            function<bool(void*, const string&)> cb_response,
            function<void(void*, const string&)> cb_done
        ) override {
            int restarts = err_attempts;

            while (restarts) {
                string full_response;
                string pending_text;
                bool request_success = false;
        
                try {
                    Curl curl;
                    curl.AddHeader("Accept: application/json");
                    curl.SetTimeout(stream_request_timeout);
                    curl.SetVerifySSL(true);
                    // curl.SetFollowRedirects(true);
                    // curl.SetAutoDecompress(true);
                    // curl.SetDNSCaching(300);
        
                    bool interrupted = false;

                    string conversation_json_data = conversation_to_json(system, conversation);
                    // cout << "[DEBUG conversation_json_data]: " << conversation_json_data << endl;
                    string succsess_sentences = "";

                    // cout << "[DEBUG] Gemini request stream..." << endl;
                    request_success = curl.POST(
                        "https://generativelanguage.googleapis.com/v1beta/models/" 
                            + variant + ":streamGenerateContent?alt=sse&key=" + escape(secret),
                        [&](const string& chunk) {
                            // cout << "[DEBUG chunk]: " << chunk << endl;
                            if (interrupted) return;

                            if (is_valid_json(chunk)) {
                                JSON json(chunk);
                                if (json.isDefined("error"))
                                    throw ERROR("Gemini error: " + json.dump());
                                
                                if (!json.isDefined("candidates[0].content.parts[0].text"))
                                    throw ERROR("Gemini error: text is not defined: " + json.dump());
                            }

                            // Process SSE events
                            vector<string> events = explode("\n\n", str_replace("\r", "", chunk));
                            
                            for (const string& event : events) {
                                if (!str_contains(event, "data: ")) continue;
                                
                                vector<string> parts = explode("data: ", event);
                                if (parts.size() < 2) continue;
                                
                                JSON json(parts[1]);
                                if (json.isDefined("error"))
                                    throw ERROR("Gemini error: " + json.dump());
                                
                                if (!json.isDefined("candidates[0].content.parts[0].text"))
                                    throw ERROR("Gemini error: text is not defined: " + json.dump());

                                string text = json.get<string>("candidates[0].content.parts[0].text");
                                full_response += text;
                                pending_text += text;

                                // Split into sentences
                                // vector<string> chars = explode("", pending_text);
                                string current_sentence;
                                
                                for (char c : pending_text) {
                                    string s(1, c);
                                    current_sentence += s;
                                    //if (in_array(s, sentence_delimiters)) {
                                    for (const string& sentence_delimiter: sentence_delimiters) {
                                        if (str_ends_with(current_sentence, sentence_delimiter)) {
                                            // cout << "[DEBUG] resp: " << current_sentence << endl;
                                            if (cb_response(context, current_sentence)) {
                                                interrupted = true;
                                                curl.cancel();
                                                return;
                                            }
                                            succsess_sentences += current_sentence;
                                            current_sentence.clear();
                                        }
                                    }
                                }
                                
                                pending_text = current_sentence;
                                
                            }
                        },
                        conversation_json_data,
                        {
                            "Content-Type: application/json",
                            "Accept: text/event-stream"
                        }
                    );
                    // cout << "[DEBUG] Gemini request stream [done]" << endl;
        
                    if(request_success) {
                        // Flush remaining text
                        if(!interrupted && !pending_text.empty()) {
                            // cout << "[DEBUG] last: " << pending_text << endl;
                            if (!cb_response(context, pending_text))
                                succsess_sentences += pending_text;
                        }
                        // cout << "[DEBUG] done." << endl;
                        cb_done(context, full_response);
                        return succsess_sentences;
                    }
                }
                catch(const exception& e) {
                    handle_request_error(e);
                }
            }
            
            throw ERROR("Unable to reach Gemini API after " + tools::to_string(err_attempts) + " attempts.");
        }
        

    public:
        Gemini(
            Logger& logger,
            const string& secret,
            const vector<string>& variants,
            size_t current_variant,
            int err_retry_sec,
            int err_attempts,
            const vector<string>& sentence_delimiters,
            long stream_request_timeout,
            const string& tmpfile,
            MODEL_ARGS
        ):
            Model(MODEL_ARGS_PASS),
            logger(logger), 
            secret(secret),
            variants(variants),
            current_variant(current_variant),
            err_retry_sec(err_retry_sec),
            err_attempts(err_attempts),
            sentence_delimiters(sentence_delimiters),
            stream_request_timeout(stream_request_timeout),
            tmpfile(tmpfile)
        {}

        // make it as a factory - caller should delete spawned model using kill()
        void* spawn(MODEL_ARGS) override {
            return new Gemini(
                logger, 
                secret, 
                variants, 
                current_variant, 
                err_retry_sec,
                err_attempts,
                sentence_delimiters,
                stream_request_timeout,
                tmpfile,
                MODEL_ARGS_PASS
            );
        }

        void kill(Model* gemini) override { 
            delete (Gemini*)gemini;
        }
    };

}