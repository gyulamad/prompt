#pragma once

#include "tools/llm/Model.hpp"
#include "tools/Logger.hpp"
#include "tools/Process.hpp"
#include "tools/Curl.hpp"

using namespace std;
using namespace tools;
using namespace tools::llm;

namespace prompt {

    class Gemini: public Model {
    private:
        const string tmpfile = "/tmp/temp.json";
        string command;
        JSON response;
        int err_retry = 10; // TODO: param??
        const int attempts = 2; // TODO: param
        int restarts = attempts;

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
                cerr << "Retry after " << err_retry << " second(s)..." << endl;
                sleep(err_retry);
                restarts--;
            } else sleep(3); // TODO: for api rate limit
            return ERROR("Gemini API error. See more in log...");
        }
        
        virtual string request() override {
            restarts = attempts;
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
                    response = Process::execute(command);
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
            int restarts = attempts;
            const vector<string> sentence_delimiters = {".", "!", "?"}; // TODO: config

            while (restarts) {
                string full_response;
                string pending_text;
                bool request_success = false;
        
                try {
                    Curl curl;
                    curl.AddHeader("Accept: application/json");
                    curl.SetTimeout(30000); // TODO: config
                    curl.SetVerifySSL(true);
                    // curl.SetFollowRedirects(true);
                    // curl.SetAutoDecompress(true);
                    // curl.SetDNSCaching(300);
        
                    bool interrupted = false;

                    string conversation_json_data = conversation_to_json(system, conversation);
                    // cout << "[DEBUG conversation_json_data]: " << conversation_json_data << endl;
                    string succsess_sentences = "";
                    request_success = curl.POST(
                        "https://generativelanguage.googleapis.com/v1beta/models/" 
                            + variant + ":streamGenerateContent?alt=sse&key=" + escape(secret),
                        [&](const string& chunk) {
                            // cout << "[DEBUG chunk]: " << chunk << endl;
                            if (interrupted) return;

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
                                    continue;

                                string text = json.get<string>("candidates[0].content.parts[0].text");
                                full_response += text;
                                pending_text += text;

                                // Split into sentences
                                // vector<string> chars = explode("", pending_text);
                                string current_sentence;
                                
                                for (char c : pending_text) {
                                    string s(1, c);
                                    current_sentence += s;
                                    if (in_array(s, sentence_delimiters)) {
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
                                
                                pending_text = current_sentence;
                                
                            }
                        },
                        conversation_json_data,
                        {
                            "Content-Type: application/json",
                            "Accept: text/event-stream"
                        }
                    );
        
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
                    throw handle_request_error(e);
                }
            }
            
            throw ERROR("Unable to reach Gemini API after " + to_string(attempts) + " attempts.");
        }

        // void request_stream(
        //     void* context,
        //     function<void(void*, const string&)> cb_response,
        //     function<void(void*, const string&)> cb_done
        // ) override {
        //     Process proc;
        //     restarts = attempts;
        //     while (restarts) {
        //         try {
        //             string conversation_json = conversation_to_json(system, conversation);
        //             if (!file_put_contents(tmpfile, conversation_json)) {
        //                 throw ERROR("Unable to write: " + tmpfile);
        //             }
        //             command = "curl -s \"https://generativelanguage.googleapis.com/v1beta/models/" + variant + ":streamGenerateContent?alt=sse&key=" + escape(secret) + "\" -H 'Content-Type: application/json' --no-buffer --data-binary @" + tmpfile;
        //             proc.writeln(command);
        //             string resp = "";
        //             string text = "";
        //             string last = "";
        //             while(true) {
        //                 if (!proc.ready()) continue;
        //                 resp = proc.read(1000); // TODO: config
        //                 if (resp.empty()) break;
        //                 vector<string> chunks = explode("\n", resp);
        //                 for (const string& chunk: chunks) {
        //                     if (trim(chunk).empty()) continue;
        //                     cout << resp << endl;
        //                     response = resp;
        //                     if (response.isDefined("error") || !response.isDefined("candidates[0].content.parts[0].text"))
        //                         throw ERROR("Gemini error: " + response.dump());
        //                     string text = response.get<string>("candidates[0].content.parts[0].text");
        //                     vector<string> pieces = explode(".", text);
        //                     while (pieces.size() > 1) {
        //                         string piece = array_shift(pieces) + ".";
        //                         if (!last.empty()) piece = last + piece;
        //                         last = "";
        //                         cb_response(context, piece);
        //                         text += piece;
        //                     }
        //                     last = pieces[0];
        //                 }
        //             }
        //             cb_response(context, last);
        //             text += last;
        //             cb_done(context, text);
        //         } catch (exception &e) {
        //             throw handle_request_error(e);
        //         }
        //     }
        // }

    public:
        Gemini(
            Logger& logger,
            const string& secret,
            const vector<string>& variants,
            size_t current_variant,
            MODEL_ARGS
        ):
            Model(MODEL_ARGS_PASS),
            logger(logger), 
            secret(secret),
            variants(variants)
        {}

        // make it as a factory - caller should delete spawned model using kill()
        void* spawn(MODEL_ARGS) override {
            return new Gemini(
                logger, 
                secret, 
                variants, 
                current_variant, 
                MODEL_ARGS_PASS
            );
        }

        void kill(Model* gemini) override { 
            delete (Gemini*)gemini;
        }
    };

}