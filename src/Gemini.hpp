#pragma once

#include "tools/llm/Model.hpp"
#include "tools/Logger.hpp"
#include "tools/Process.hpp"

using namespace std;
using namespace tools;
using namespace tools::llm;

namespace prompt {

    class Gemini: public Model {
    private:
        int err_retry = 10;
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
        
        virtual string request() override {
            int restarts = 2;
            const string tmpfile = "/tmp/temp.json";
            string command;
            JSON response;
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
                }
            }
            throw ERROR("Gemini API error. See more in log...");
        }

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