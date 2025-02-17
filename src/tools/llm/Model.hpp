#pragma once

#include <climits>
#include <functional>

#include "../ERROR.hpp"
#include "../strings.hpp"
#include "../vectors.hpp"
#include "../files.hpp"
#include "../Rotary.hpp"
// #include "../Speech.hpp"
#include "../JSON.hpp"
#include "../io.hpp"

using namespace std;

namespace tools::llm {

    #define ANSI_FMT_MODEL_THINKS ANSI_FMT_C_BLACK // ANSI_FMT_T_DIM

    typedef enum { ROLE_NONE = 0, ROLE_INPUT, ROLE_OUTPUT } role_t;
    typedef unordered_map<role_t, const string> role_name_map_t;
    const role_name_map_t default_role_name_map = {
        { ROLE_NONE, "" },
        { ROLE_INPUT, "input" },
        { ROLE_OUTPUT, "output" },
    };

    string to_string(const role_t& role, const role_name_map_t& role_name_map = default_role_name_map) {
        return role_name_map.at(role);
    }
    
    class Message {
    private:
        string text;
        role_t role;
    public:

        Message(
            const string& text, 
            const role_t& role
        ):
            text(text), 
            role(role)
        {}

        virtual ~Message() {}

        JSON toJSON() const {
            JSON json;
            json.set("text", text);
            json.set("role", role);
            return json;
        }

        void fromJSON(JSON json) {
            text = json.get<string>("text");
            role = json.get<role_t>("role");
        }

        string get_text() const {
            return text;
        }

        void set_text(const string& text) {
            this->text = text;
        }

        role_t get_role() const {
            return role;
        }

        void set_role(const role_t& role) {
            this->role = role;
        }

        size_t length() const {
            return dump().size();
        }

        string dump(bool show = false) const {
            string dump = ((role == ROLE_NONE) ? "" : escape(to_string(role)) + ": ") + escape(text);
            return dump;
        }
    };

    class Conversation {
    private:
        vector<Message> messages;
    public:
        Conversation() {}
        
        virtual ~Conversation() {}

        JSON toJSON() const {
            json jmessages = json::array();
            for (const Message& message: messages)
                jmessages.push_back(message.toJSON().get_json());
            JSON json(jmessages);
            return json;
        }

        void fromJSON(JSON J) {
            messages.clear();
            //DEBUG(J.dump());
            json jmessages = J.get_json();
            for (const auto& jmessage: jmessages) {
                add(jmessage.at("text"), (role_t)jmessage.at("role"));
            }
        }
        
        void add(const string& text, role_t role = ROLE_NONE) {
            messages.push_back({ text, role });
        }

        bool empty() const {
            return messages.empty();
        }

        Message pop() {
            Message last = messages.back();
            messages.pop_back();
            return last;
        }

        void clear() {
            messages.clear();
        }

        const vector<Message>& get_messages_ref() const {
            return messages;
        }

        size_t length() const {
            size_t l = 0;
            for (const Message& message: messages) l += message.length() + 1;
            return l;
        }

    };


    
    typedef enum { PARAMETER_TYPE_INTEGER, PARAMETER_TYPE_STRING } parameter_type_t;

    string to_string(parameter_type_t type) {
        switch (type) {
            case PARAMETER_TYPE_INTEGER:
                return "integer";
            case PARAMETER_TYPE_STRING:
                return "string";
            default:
                throw ERROR("Invalid parameter type");
        }
    }

    class Parameter {
    private:
        string name;
        parameter_type_t type;
        bool required;
        string rules;
    public:
        Parameter(
            const string& name, 
            parameter_type_t type, 
            bool required = false, 
            const string& rules = ""
        ): 
            name(name),
            type(type),
            required(required),
            rules(rules)
        {}

        virtual ~Parameter() {}

        string get_name() const { return name; }
        parameter_type_t get_type() const { return type; }
        bool is_required() const { return required; }
        string get_rules() const { return rules; }

    };

    string to_string(bool b) { // TODO to common libs
        return b ? "true" : "false";
    }

    string to_string(const Parameter& parameter) {
        return 
            "Parameter name: " + parameter.get_name() + "\n" + 
            "Type: " + to_string(parameter.get_type()) + "\n" + 
            "Required: " + to_string(parameter.is_required()) + (
                parameter.get_rules().empty() ? "" : ("\nRules: " + parameter.get_rules())
            );
    }

    string to_string(const vector<Parameter>& parameters) {
        vector<string> results;
        for (const Parameter& parameter: parameters)
            results.push_back(to_string(parameter));
        return implode("\n", results);
    }
    
    class Tool {
    private:
        string name;
        vector<Parameter> parameters;
        string description;
        function<string(void*, const JSON&)> callback;

    public:
        
        Tool(
            const string& name,
            const vector<Parameter>& parameters,
            function<string(void*, const JSON&)> callback,
            const string& description = ""
        ):
            name(name),
            parameters(parameters),
            callback(callback),
            description(description)
        {}

        string get_name() const { return name; }
        string get_description() const { return description; }
        const vector<Parameter>& get_parameters_cref() const { return parameters; }

        string call(const JSON& args) {
            return callback(this, args);
        };
    };

    string to_string(const Tool& tool) {
        return "Function name: " + tool.get_name()
            + (tool.get_description().empty() ? "" : ("\nDescription: " + tool.get_description()))
            + (tool.get_parameters_cref().empty() ? "" : ("\nParameters:\n" + to_string(tool.get_parameters_cref())));
    }


    string to_string(const vector<Tool>& tools) {
        vector<string> results;
        for (const Tool& tool: tools) 
            results.push_back(to_string(tool));
        return implode("\n\n", results);
    }

    class Model {
    // public:
        // using think_reporter_func_t = function<void(Model* model, const string&)>;
        // using think_interruptor_func_t = function<bool(Model* model)>;
    private:
        static Rotary rotary;
    protected:
        const string opt_prefix = "[OPTION-START]";
        const string opt_suffix = "[OPTION-END]";
        string system;
        Conversation conversation;
        // bool remember;
        // string memory;
        // size_t memory_max; 
        size_t conversation_length_max;
        double conversation_loss_ratio;
        // think_reporter_func_t default_think_reporter;
        // think_interruptor_func_t default_think_interruptor;

        // --------- tools -----------

        vector<Tool> tools;

        string inference_full;
        bool inference_stat_in_func_call;
        string inference_next_func_call;
        vector<string> inference_func_calls;

        // ---------------------------


        virtual void think_reporter(const string& thought = "") {
            cout << "\r\033[2K";
            if (!thought.empty()) cout << ANSI_FMT(ANSI_FMT_MODEL_THINKS, thought) << endl;
            rotary.tick();
        }

        virtual bool think_interruptor() {
            rotary.tick();
            return kbhit();
        }

        virtual string request() UNIMP
        virtual string request_stream(
            void*, // context
            function<bool(void*, const string&)>, // inference chunk recieved
            function<void(void*, const string&)> // full response recieved
        ) UNIMP

        vector<string> explode_options(const string& response) {
            vector<string> splits = explode(opt_prefix, response);
            array_shift(splits);
            vector<string> options;
            for (const string& split: splits) options.push_back(explode(opt_suffix, split)[0]);
            return options;
        }

        string implode_options(const vector<string>& options, string sep = "\n") {
            return sep + opt_prefix + implode(opt_suffix + sep + opt_prefix, options) + opt_suffix + sep;
        }


        bool in_compressing_conversation = false;
        bool compress_conversation() {
            if (!in_compressing_conversation) in_compressing_conversation = true;
            else return false;
            size_t size = conversation.get_messages_ref().size();
            size_t cut_at = size * conversation_loss_ratio;
            if (!cut_at) return false;
            vector<string> conversation_first_part;
            for (size_t n = 0; n < cut_at; n++) 
                conversation_first_part.push_back(conversation.get_messages_ref().at(n).dump());
            
            Model* summariser = (Model*)spawn(
                "You are a summariser."//, 
                // conversation_length_max, 
                // conversation_loss_ratio, 
                // think_steps, 
                // think_deep
            );            
            string summary = summariser->prompt(
                "Summarise the following conversation: " + implode(",", conversation_first_part)
            );
            kill(summariser);

            Conversation summarised;
            summarised.add("Earlier conversation summary: " + summary, ROLE_INPUT);
            for (size_t n = cut_at; n < size; n++) 
                summarised.add(
                    conversation.get_messages_ref().at(n).get_text(),
                    conversation.get_messages_ref().at(n).get_role()
                );
            conversation = summarised;

            // TODO
            // Model* helper = (Model*)spawn();
            // auto [firstHalf, secondHalf] = str_cut_ratio(memory, memory_loss_ratio);
            // memory = helper->prompt("system", "Summarize this:\n" + firstHalf) + secondHalf;
            // kill(helper);
            in_compressing_conversation = false;
            return true;
        }

        // string get_system_with_data() {
        //     return tpl_replace(system_data, system);
        // }

    public:
        int think_steps;
        int think_deep;
        // map<string, string> system_data;

        #define MODEL_ARGS \
            const string& system, \
            /*Conversation& conversation,*/ \
            /*bool remember = false,*/ \
            /*const string& memory = "",*/ \
            /*size_t memory_max = 100000,*/ \
            size_t conversation_length_max /*= 500000*/, \
            double conversation_loss_ratio /*= 0.5*/, \
            int think_steps /*= 1*/, \
            int think_deep /*= 2*/
            
        #define MODEL_ARGS_PASS \
            system, \
            /*conversation,*/ \
            /*remember,*/ \
            /*memory,*/ \
            /*memory_max,*/ \
            conversation_length_max, \
            conversation_loss_ratio, \
            think_steps, \
            think_deep
            
        Model(MODEL_ARGS):
            system(system),
            // conversation(conversation),
            // remember(remember),
            // memory(memory),
            // memory_max(memory_max),
            conversation_length_max(conversation_length_max),
            conversation_loss_ratio(conversation_loss_ratio),
            think_steps(think_steps), 
            think_deep(think_deep)
        {
            //cout << "DEBUG[" << conversation_length_max << "]" << endl;
        }

        virtual ~Model() {}

        // make it as a factory - caller should delete spawned/cloned model using kill()
        virtual void* spawn(const string& system) {
            return spawn(MODEL_ARGS_PASS);
        }
        virtual void* spawn(MODEL_ARGS) UNIMP
        void* clone() {
            return spawn(MODEL_ARGS_PASS);
        }
        virtual void kill(Model*) UNIMP

        JSON toJSON() const {
            string s = tpl_replace({
                { "{{system}}", json_escape(system) },
                { "{{conversation}}", conversation.toJSON().dump() },
                { "{{conversation_length_max}}", ::to_string(conversation_length_max) },
                { "{{conversation_loss_ratio}}", ::to_string(conversation_loss_ratio) },
            },  
                R"({
                    "system": "{{system}}",
                    "conversation": {{conversation}},
                    "conversation_length_max": {{conversation_length_max}},
                    "conversation_loss_ratio": {{conversation_loss_ratio}}
                })"
            );
            JSON json(s);
            return json;
        }

        void fromJSON(JSON json) {
            system = json.get<string>("system");
            JSON jconversation = json.get_json().at("conversation");
            conversation.fromJSON(jconversation);
            conversation_length_max = json.get<size_t>("conversation_length_max");
            conversation_loss_ratio = json.get<double>("conversation_loss_ratio");
        }
        
        string save(Model& model, const string& path) const {
            if (!file_put_contents(path, model.toJSON().dump(4), false, false))
                return "Unable to save model to file: " + path;
            return "";
        }
        
        string load(Model& model, const string& path) {
            try {
                model.fromJSON(file_get_contents(path));
                return "";
            } catch(exception &e) {
                return "Unable to load model from file: " + path + "\nReason: " + string(e.what());
            }
        }

        void dump_conversation(const string& input_prompt) const {
            for (const Message& message: conversation.get_messages_ref()) {
                if (message.get_role() == ROLE_INPUT) cout << input_prompt;
                string text = message.get_text();
                cout << text + (text.back() == '\n' ? "" : "\n")  << flush;
            }
        }

        bool hasContext() const {
            return conversation.empty();
        }

        void addContext(const string& info, role_t role = ROLE_NONE) {
            conversation.add(info, role);
        }

        Message popContext() {
            return conversation.pop();
        }

        
        void memorize(const string& prmpt, role_t role = ROLE_INPUT) {
            conversation.add(prmpt, role); 
            // check the length of conversation and cut if too long
            if (conversation.length() > conversation_length_max) 
                if (!compress_conversation()); // break;

        //     // TODO: check the length of conversation and cut if too long:
        //     // memory += "\n" + memo;
        //     // while (memory.size() > memory_max) compress_memory(); 
        }

        // void amnesia() { 
        //     // memory = "";
        //     conversation.clear();
        // }

        string prompt_stream(
            const string& prmpt, 
            void* context,
            function<bool(void*, const string&)> cb_response, 
            function<void(void*, const string&)> cb_done
        ) {
            memorize(prmpt, ROLE_INPUT);
            //string response = "";
            string non_interrupted_responses = request_stream(
                context, 
                [&](void* ctx, const string& text) { 
                    if (cb_response(ctx, text)) {
                        //response += text;
                        return true;
                    }
                    return false;
                }, cb_done
            );
            if (!non_interrupted_responses.empty())
                memorize(non_interrupted_responses, ROLE_OUTPUT);

            if (!inference_func_calls.empty()) {
                string tool_output = inference_handle_tools();
                if (!tool_output.empty()) {
                    inference_tools_reset();
                    non_interrupted_responses += prompt_stream(tool_output, context, cb_response, cb_done);
                }
            }
            return non_interrupted_responses;
        }

        string prompt(const string& prmpt/*, const string& suffix*/) {
            // const string suffix = "";
            // if (!from.empty()) from += ": ";
            // if (!remember) return request(from + prmpt + "\n" + system + "\n" + suffix);
            // memorize(from + prmpt);
            // string response = request(memory + "\n" + system + "\n" + suffix);
            // memorize(response);
            // return response;

            inference_tools_reset();


            memorize(prmpt, ROLE_INPUT);
            string response = request();
            memorize(response, ROLE_OUTPUT);

            return inference_tools_process_response(response);
        }
        // string prompt(const string& prmpt, const char* sffx = nullptr) {
        //     string suffix(sffx ? sffx : "");
        //     return prompt(prmpt, suffix);
        // }

        string choose_str(const string& prmpt, const vector<string>& options) {
            if (options.empty()) ERROR("No options to choose from.");
            if (options.size() == 1) return options[0];
            
            string selection = prompt(
                prmpt + "\n" + 
                "Your options are the followings:\n" +           
                " - " + implode("\n - ", options) + "\n" +
                "You MUST select one but ONLY one!\n" // + 
                //"Show your answer in the following format: " + implode_options({ "Your selection here..." }, "")
            );
            while(true) {
                vector<string> selections = explode_options(selection);
                if (selections.size() == 0) break;
                if (selections.size() == 1) {
                    selection = selections[0];
                    break;
                }
                selection = prompt("To many. Select only ONE!");
            }

            return selection;
        }

        // return 0 if it's same, return smaller if the distance small, larger if less similar...
        int compare(const string& s1, const string& s2) {
            return levenshtein(s1, s2); // for string comparison the levenshtein can be replacable by user overrides (for eg embedding)
        }

        int which(const string& option, const vector<string>& options) {
            int selection = -1;
            int distance_min = INT_MAX;
            for (size_t nth = 0; nth < options.size(); nth++) {
                int distance = compare(option, options[nth]);
                if (distance == 0) return nth; // exact match (can not be better)
                if (distance <= distance_min) {
                    distance_min = distance;
                    selection = nth;
                }
            }
            return selection;
        }

        int choose(const string& prmpt, const vector<string>& options) {
            if (options.empty()) ERROR("No options to choose from.");
            if (options.size() == 1) return 0;

            string choice = choose_str(prmpt, options);
            
            return which(choice, options);
        }

        vector<string> multiple_str(const string& prmpt, const vector<string>& options = {}) {
            if (options.empty()) ERROR("No options to choose from.");
            if (options.size() == 1) return { options[0] };
            
            string response = prompt(
                prmpt + "\n" + 
                (
                    options.empty() ? "" :
                        "Your options are the followings:\n" +
                        implode_options(options)
                ) + "\n"
                "Choose your answer and show in the following format:\n" + 
                implode_options({ "Your selection", "Another, if multiple apply..." })
            );

            vector<string> selections = explode_options(response);
            if (selections.empty()) selections = options; // show all if there is none

            return selections;
        }

        vector<int> multiple(const string& prmpt, const vector<string>& options) {
            if (options.empty()) ERROR("No options to choose from.");
            if (options.size() == 1) return { 0 };

            vector<string> selections = multiple_str(prmpt, options);
            if (selections.size() == 0) return {};
            if (selections.size() == 1) return { 0 };

            vector<int> results;
            for (const string& selection: selections) {
                results.push_back(which(selection, options));
            }
            return array_unique(results);
        }

        vector<string> choices(const string& prmpt) {
            string response = prompt(
                prmpt + "\n"
                "List your response in the following format:\n" +
                implode_options({ "Your selection..", "Other option (if multiple apply)..." })
            );
            return explode_options(response);
        }

        bool decide(const string& prmpt, const vector<string>& options = {"Yes", "No"}) {
            int resp = choose(prmpt, options);
            if (resp == 0) return true;
            if (resp == 1) return false;
            throw ERROR("Model was unable to decide. Prompt: " + str_cut_end(prmpt));
        }

        string think(const string& prmpt) {

            inference_tools_reset(); 

            think_reporter();
            memorize(prmpt, ROLE_INPUT);

            Model* thinker = (Model*)clone();

            string response = thinker->prompt(prmpt);
            think_reporter();
            for (int step = 0; step < think_steps; step++) {
                response = thinker->prompt("Think deeply and refine the response.");
                think_reporter();
                if (think_interruptor()) break;
            }

            kill(thinker);

            memorize(response, ROLE_OUTPUT);

            return inference_tools_process_response(response);
            // return response;
        }

        // string deep_think(const string& task) {
        //     Model* solver = (Model*)clone();
        //     string result = solver->solve(task);
        //     kill(solver);
        //     return result;
        // }
        
        string solve(const string& task, int think_deeper = -1) {

            inference_tools_reset(); 

            think_reporter();
            // if (!remember) return prompt(from, task);
            if (think_deeper == -1) think_deeper = think_deep;
            if (think_deeper <= 0 || think_interruptor()) return prompt(task);
            think_deeper--;
            cout << "Thinking progress: " << think_deeper << endl;

            Model* helper = (Model*)clone();

            think_reporter();
            if (!helper->decide("Is it even a task?:\n" + task)) {
                // It's not even a task:
                kill(helper);
                think_reporter();
                return prompt(task);
            }
            // it's a valid task...

            think_reporter();
            if (helper->decide("Do you have any questions about the task?")) {
                think_reporter();
                vector<string> questions = helper->choices("What are your most important/relevant questions? (if any)");
                think_reporter();
                if (!questions.empty()) {
                    // there are questions:

                    string answers;
                    for (const string& question: questions) {
                        if (think_interruptor()) break;
                        think_reporter(question);
                        answers += question + "\n";

                        Model* creative = (Model*)helper->clone();

                        vector<string> options = creative->choices(
                            "Because the task:\n" + task + "\n" + 
                            "The main question now:\n" + question + "\n" + 
                            "Offer a possible answer(s)/resolution(s) for the following question/problem:\n" + 
                            question + "\n"
                            "Offer fewer options if you can not give precise/related solution and more if you have multiple possibly good and relevant idea. We interested only in relevant and best solutions but if you can only guess we still want to hear you."
                        );
                        think_reporter();

                        kill(creative);

                        if (options.empty()) continue;
                        if (options.size() == 1) {
                            answers += options[0] + "\n"; // there are next question, only one option
                            continue;
                        }
                        // there are next question, there are options:
                        
                        string previous_option = options[0];
                        string previous_thoughts;
                        int best_option = 0, nth = 0;
                        for (const string& option: options) {

                            Model* thinker = (Model*)helper->clone();
                            
                            string thoughts = thinker->think(
                                "Because the task:\n" + task + "\n" + 
                                "The main question now:\n" + question + "\n" + 
                                "One possible solution is:\n" + option + "\n" +
                                "Your current task is to think about this solution!"
                            );  
                            think_reporter();                         

                            kill(thinker);

                            if (previous_thoughts.empty()) best_option = nth;
                            else {
                                Model* chooser = (Model*)helper->clone();
                                
                                int choice = chooser->decide(
                                    "Because the task:\n" + task + "\n" + 
                                    "The main question now:\n" + question + "\n" + 
                                    "Now we have to decide between these two option:\n" +
                                    "\n"
                                    " - " + previous_option + "\n" +
                                    "\n"
                                    " - " + option + "\n" +
                                    "\n"
                                    "In the following we have detailed thoughts for both possibilies:\n" +
                                    "\n" +
                                    previous_thoughts + "\n" +
                                    "\n" +
                                    thoughts + "\n" +
                                    "\n" +
                                    "Now your task is to help us to decide which is the better solution?\b",
                                    { previous_option, thoughts }
                                );
                                think_reporter();

                                kill(chooser);

                                if (choice == 1) best_option = nth;
                                nth++;
                            }

                            previous_option = option;
                            previous_thoughts = thoughts;
                            if (think_interruptor()) break;
                        }
                        answers += options[best_option] + "\n";

                        // int choice = helper->choose(task + "\n" + question, options);
                        // if (choice >= 0) answers += options[choice] + "\n"; // do not store answer if couldn't choose
                    
                        
                        if (think_interruptor()) break;
                    }
                    if (answers.empty()) {
                        // there are questions, there are options, no answers...
                        string results = implode("\n", questions); // our results with only questions..
                        kill(helper);
                        memorize(task, ROLE_INPUT);
                        memorize(results, ROLE_OUTPUT);

                        return inference_tools_process_response(results);
                        // return results;
                    }
                    // there are questions, there are options, we got answers:
                    
                    memorize(task, ROLE_INPUT);
                    memorize(answers, ROLE_OUTPUT);
                }
                // no question...

            }
            // no question...

            think_reporter();
            if (helper->decide("Is the task too complex, so have to be break down smaller steps?")) {
                vector<string> small_steps = helper->multiple_str("What are smaller steps to get this task done?");
                if (!small_steps.empty()) {
                    string results;
                    for (const string& step: small_steps) {
                        think_reporter(step);
                        string solution = helper->solve(task + "\n" + step, think_deeper);
                        // think_reporter(solution);
                        results += step + "\n" + solution + "\n";
                        if (think_interruptor()) break;
                    }
                    
                    kill(helper);
                    memorize(task, ROLE_INPUT);
                    memorize(results, ROLE_OUTPUT);


                    return inference_tools_process_response(results);
                    // return results;
                }
                // no question, no smaller steps...

            }
            // no question, no smaller steps...

            kill(helper);
            think_reporter();
            return prompt(task);
        }


        // ----------------- TOOLS -------------------
        


        void set_tools(const vector<Tool> tools) {
            this->tools = tools;
        }

        const vector<string>& get_inference_func_calls_cref() const {
            return inference_func_calls;
        }


        string inference_tools_process_response(const string& response_orig) {
            string response = inference_remove_tools(response_orig);

            if (!inference_func_calls.empty()) {
                string tool_output = inference_handle_tools();
                if (!tool_output.empty())
                    response += prompt(tool_output);
            }

            return response;
        }

        void inference_tools_reset() {
            inference_full = "";
            inference_stat_in_func_call = false;
            inference_next_func_call = "";
            inference_func_calls.clear();
        }

        string inference_remove_tools(const string& inference) {
            string result = "";
            for (size_t i = 0; i < inference.length(); i++) {
                inference_full += inference[i];
                if (inference_stat_in_func_call) inference_next_func_call += inference[i];
                else result += inference[i];
                if (str_ends_with(inference_full, "[FUNCTION-CALLS-START]")) {
                    inference_stat_in_func_call = true;
                    inference_next_func_call = "";                    
                }
                else if (str_ends_with(inference_full, "[FUNCTION-CALLS-STOP]")) {
                    inference_stat_in_func_call = false;
                    inference_func_calls.push_back(str_replace({
                        { "[FUNCTION-CALLS-START]", "" }, 
                        { "[FUNCTION-CALLS-STOP]", "" }, 
                    }, inference_next_func_call));
                    inference_next_func_call = "";
                }
            }            
            return str_replace({
                { "[FUNCTION-CALLS-START]", "" }, 
                { "[FUNCTION-CALLS-STOP]", "" }, 
            }, result);
        }

        string inference_handle_tools() {
            string output = "";
            for (const string& inference_func_call: inference_func_calls) {
                if (!is_valid_json(inference_func_call)) {
                    output += 
                        "Invalid JSON syntax for function call:\n" 
                        + inference_func_call 
                        + "\n";
                    continue;
                }
                JSON fcall_all(inference_func_call);
                if (!fcall_all.has("function_calls")) {
                    output += 
                        "`function_calls` key is missing:\n" 
                        + inference_func_call 
                        + "\n";
                    continue;
                }
                if (!fcall_all.isArray("function_calls")) {
                    output += 
                        "`function_calls` is not an array:\n" 
                        + inference_func_call 
                        + "\n";
                    continue;
                }
                vector<JSON> fcalls(fcall_all.get<vector<JSON>>("function_calls"));
                for (const JSON& fcall: fcalls) {
                    string function_name = fcall.get<string>("function_name");
                    bool found = false;
                    for (Tool& tool: tools) {
                        if (tool.get_name() == function_name) {
                            found = true;
                            bool invalid = false;
                            for (const Parameter& parameter: tool.get_parameters_cref()) {
                                if (!fcall.has(parameter.get_name())) {
                                    if (parameter.is_required()) {
                                        output += 
                                            "A required parameter is missing in function call: `" 
                                            + function_name + "." + parameter.get_name() +
                                            + "`\n";
                                        invalid = true;
                                        continue;
                                    }
                                }                                
                            }
                            if (!invalid) {
                                string result;
                                try {
                                    result = tool.call(fcall);
                                } catch (exception &e) {
                                    result = "Error in function `" + function_name + "`: " + e.what();
                                }
                                output += 
                                    "Function output `" + function_name + "`:\n"
                                    + result + "\n";
                            }
                        }
                    }
                    if (!found) {
                        output += "Function is not found: `" + function_name + "`\n";
                    }
                }
            }
            return output;
        }
    };

    Rotary Model::rotary = Rotary({
        RotaryFrames({
            "🤔", // Thinking Face
            "🧠", // Brain
            "💭", // Thought Balloon
            "😊", // Smiling Face
            "😎", // Smiling Face with Sunglasses
            "🤖", // Robot Face
            "🤨", // Face with Raised Eyebrow
            "🤯", // Exploding Head (for "mind-blown" moments)
            "🤓", // Nerd Face (for deep thinking or studying)
            "🤷", // Shrug (for "I don't know" moments)
            "💡"  // Light Bulb (perfect for representing ideas or inspiration)
        }, 4),     // Emojis
        RotaryFrames({ 
            "[ " + ANSI_FMT(ANSI_FMT_MODEL_THINKS, "Thinking     "), 
            "[ " + ANSI_FMT(ANSI_FMT_MODEL_THINKS, "Thinking.    "), 
            "[ " + ANSI_FMT(ANSI_FMT_MODEL_THINKS, "Thinking..   "), 
            "[ " + ANSI_FMT(ANSI_FMT_MODEL_THINKS, "Thinking...  "), 
            "[ " + ANSI_FMT(ANSI_FMT_MODEL_THINKS, "Thinking.... "), 
            "[ " + ANSI_FMT(ANSI_FMT_MODEL_THINKS, "Thinking....."), 
        }, 1),     // Dots
        RotaryFrames({ 
           ANSI_FMT(ANSI_FMT_MODEL_THINKS, "-") + " ]", 
           ANSI_FMT(ANSI_FMT_MODEL_THINKS, "\\") + " ]", 
           ANSI_FMT(ANSI_FMT_MODEL_THINKS, "|") + " ]", 
           ANSI_FMT(ANSI_FMT_MODEL_THINKS, "/") + " ]",
        }, 1),     // Sticks
    });
}