#pragma once

#include <string>

#include "../../../str/tpl_replace.hpp"
#include "../../../str/implode.hpp"
#include "../../../str/FrameTokenParser.hpp"
#include "../../chat/ChatPlugin.hpp"

#include "Tool.hpp"

using namespace std;
using namespace tools::str;
using namespace tools::agency::chat;

namespace tools::agency::agents::plugins {

    template<typename T>
    class ToolusePlugin: public ChatPlugin {
    public:
        ToolusePlugin(
            Owns& owns,
            void* tools,
            // Worker<T>* agency,
            const string& name,
            const string& instruct_tooluse,
            const string& tooluse_start_token,
            const string& tooluse_stop_token
        ):
            ChatPlugin(),
            owns(owns),
            tools(owns.reserve<OList>(this, tools, FILELN)),
            // agency(agency),
            name(name),
            instruct_tooluse(instruct_tooluse),
            tooluse_start_token(tooluse_start_token),
            tooluse_stop_token(tooluse_stop_token)
        {}

        virtual ~ToolusePlugin() {
            owns.release(this, tools);
        }
        
        string processInstructions(Chatbot* /*chatbot*/, const string& instructions) override {
            string tools_helps;
            for (void* tool: tools->getPlugs())
                tools_helps += ((Tool<T>*)safe(tool))->help() + "\n\n"; 
            return instructions + tpl_replace({
                { "{{tools}}", tools_helps.empty() ? "<no-available-tools>" : tools_helps },
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

            parser.parse(chunk, tooluse_start_token, tooluse_stop_token, [&](const string& inner) {
                functionCalls.push_back(inner);
            });

            return chunk;
        }
        
        string processResponse(Chatbot* /*chatbot*/, const string& response) override {
            return response;
        }
        
        string processCompletion(Chatbot* chatbot, const string& /*sender*/, const string& text) override {
            if (!functionCalls.empty()) safe(chatbot)->completion("tool", callbackFunctionCalls());
            parser.reset();
            return text;
        }
        
        string processChat(Chatbot* chatbot, const string& sender, const string& text, bool& interrupted) override {
            // DEBUG(__FUNC__);
            // DEBUG(sender + ":" + text);
            if (!functionCalls.empty()) safe(chatbot)->chat("tool", callbackFunctionCalls(), interrupted);
            parser.reset();
            // DEBUG(text);
            return text;
        }

    protected:
        string callbackFunctionCalls() {
            vector<string> outputs;
            for (const string& functionCall: functionCalls) {
                // TODO: Implement actual tool execution logic here based on functionCall content
                if (!is_valid_json(functionCall)) {
                    outputs.push_back( 
                        "Invalid JSON syntax for function call:\n" + functionCall + "\n"
                    );
                    continue;
                }
                JSON fcall(functionCall);
                if (!fcall.has("function_name")) {
                    outputs.push_back( 
                        "`function_name` key is missing:\n" + functionCall + "\n"
                    );
                    continue;
                }
                string fname = fcall.get<string>("function_name");
                string callid = fcall.has("call_id") ? fcall.get<string>("call_id") : "<undefined-call-id>";
                
                bool found = false;
                for (void* tool_void: tools->getPlugs()) {
                    Tool<T>* tool = (Tool<T>*)safe(tool_void);
                    if (tool->get_name() == fname) {
                        found = true;
                        bool invalid = false;
                        for (const Parameter& parameter: tool->get_parameters_cref()) {
                            string pname = parameter.get_name();
                            if (!fcall.has(pname)) {
                                if (parameter.is_required()) {
                                    outputs.push_back( 
                                        "A required parameter is missing in function call: `" 
                                        + fname + "." + pname +
                                        + "` (Call ID was: `" + callid + "`)\n"
                                    );
                                    invalid = true;
                                    continue;
                                }
                            }                                
                        }
                        if (!invalid) {
                            string result;
                            try {
                                // result = tool.call(
                                //     this, 
                                //     user_void, 
                                //     fcall, 
                                //     tools_config.get<JSON>(tool.get_name())
                                // );
                                outputs.push_back(
                                    "Call ID: " + callid + " (funtion: `" + fname + "`)\nResult(s):\n" 
                                        + tool->call(fcall)
                                );
                            } catch (exception &e) {
                                outputs.push_back(
                                    "Error in function `" + fname + "` (Call ID was `" + callid + "`): " + e.what()
                                );
                            }
                        }
                    }
                }
                if (!found) {
                    outputs.push_back("Function is not found: `" + fname + "` (Call ID was: `" + callid + "`)\n");
                }
            }
            functionCalls.clear();

            return implode("\n\n", outputs);
        }
        
    private:
        Owns& owns;
        OList* tools = nullptr;
        // Worker<T>* agency = nullptr;
        string name;
        string instruct_tooluse;
        string tooluse_start_token;
        string tooluse_stop_token;

        FrameTokenParser parser;

        vector<string> functionCalls;
    };

}
