#pragma once

#include <climits>
#include <functional>

#include "../ERROR.hpp"
#include "../strings.hpp"
#include "../vectors.hpp"

using namespace std;

namespace tools::llm {

    #define ANSI_FMT_MODEL_THINKS ANSI_FMT_C_BLACK ANSI_FMT_T_DIM

    class Model {
    public:
        using think_reporter_func_t = function<void(Model* model, const string&)>;
        using think_interruptor_func_t = function<bool(Model* model)>;
    protected:
        const string opt_prefix = "[OPTION:]";
        const string opt_suffix = "[/OPTION]";
        string system;
        bool remember;
        string memory;
        size_t memory_max; 
        double memory_loss_ratio;
        const int think_steps;
        const int think_deep;
        think_reporter_func_t default_think_reporter;
        think_interruptor_func_t default_think_interruptor;

        virtual string request(const string& prompt) { UNIMP }

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


        void compress_memory() {
            Model* helper = (Model*)spawn();
            auto [firstHalf, secondHalf] = str_cut_ratio(memory, memory_loss_ratio);
            memory = helper->prompt("Summarize this:\n" + firstHalf) + secondHalf;
            kill(helper);
        }

    public:

        #define MODEL_ARGS \
            const string& system = "", \
            bool remember = false, \
            const string& memory = "", \
            size_t memory_max = 100000, \
            double memory_loss_ratio = 0.5, \
            int think_steps = 1, \
            int think_deep = 2, \
            think_reporter_func_t default_think_reporter = [](Model*, const string&) { cout << ansi_fmt(ANSI_FMT_MODEL_THINKS, ".") << flush; }, \
            think_interruptor_func_t default_think_interruptor = [](Model*) { cout << ansi_fmt(ANSI_FMT_MODEL_THINKS, ".") << flush; return false; }

        #define MODEL_ARGS_PASS \
            system, \
            remember, \
            memory, \
            memory_max, \
            memory_loss_ratio, \
            think_steps, \
            think_deep, \
            default_think_reporter, \
            default_think_interruptor

        Model(MODEL_ARGS):
            system(system),
            remember(remember),
            memory(memory),
            memory_max(memory_max),
            memory_loss_ratio(memory_loss_ratio),
            think_steps(think_steps), 
            think_deep(think_deep),
            default_think_reporter(default_think_reporter),
            default_think_interruptor(default_think_interruptor)
        {}

        virtual ~Model() {}

        // make it as a factory - caller should delete spawned/cloned model using kill()

        virtual void* spawn(MODEL_ARGS) { UNIMP }
        void* clone() {
            return spawn(MODEL_ARGS_PASS);
        }
        virtual void kill(Model*) { UNIMP }
        

        
        void memorize(const string& memo) {
            memory += "\n" + memo;
            while (memory.size() > memory_max) compress_memory(); 
        }

        void amnesia() {
            memory = "";
        }



        string prompt(const string& prompt, const string& suffix) {
            if (!remember) return request(prompt + "\n" + system + "\n" + suffix);
            memorize(prompt);
            string response = request(memory + "\n" + system + "\n" + suffix);
            memorize(response);
            return response;
        }
        string prompt(const string& prmpt, const char* sffx = nullptr) {
            string suffix(sffx ? sffx : "");
            return prompt(prmpt, suffix);
        }

        int choose(const string& prompt, const vector<string>& options, int defopt = 0) {
            if (options.empty()) ERROR("No options to choose from");
            if (options.size() == 1) return 0;
            Model* helper = (Model*)clone();
            string response = helper->prompt(
                prompt + 
                "\nYour options are the followings: " 
                + implode_options(options) +
                "\nSelect only one! Show your answer in the following format: " + implode_options({ "Your selection here..." }, "")
            );
            vector<string> selections = explode_options(response);
            if (selections.empty()) {
                kill(helper);
                return defopt; // couldn't
                //return -options.size() - 1; // < -options - 1 means couldn't select at all
                // throw ERROR("No selection.\nPrompt: " + prompt);
            }
            if (selections.size() > 1) {
                kill(helper);
                return -selections.size(); // < -1 means multiple selected
                // throw ERROR("Multiple selection, prompt: " + prompt);
            }
            int selection = -1; // -1 means no decision.
            int distance_min = INT_MAX;
            for (size_t nth = 0; nth < options.size(); nth++) {
                int distance = levenshtein(selections[0], options[nth]);
                if (distance <= distance_min) {
                    distance_min = distance;
                    selection = nth;
                }
            }
            
            kill(helper);
            return selection;
        }

        vector<string> options(const string& prompt, const vector<string>& options = {}) {
            Model* helper = (Model*)clone();
            string response = helper->prompt(prompt, 
                (options.empty() ? "" : "Your options to choose from: \n" + implode_options(options)) + 
                "List your response in the following format:" +
                implode_options({ "Your selection..", "Yor other option (if multiple apply)" }) + "etc.."
            );
            kill(helper);
            return explode_options(response);
        }

        bool decide(const string& prompt, const vector<string>& options = {"Yes", "No"}) {
            int resp = choose(prompt, options, -1);
            if (resp == 0) return true;
            if (resp == 1) return false;
            throw ERROR("Model was unable to decide. Prompt: " + str_cut_end(prompt));
        }

        string think(
            const string& prompt, //int think_steps = 1,
            think_reporter_func_t think_reporter = nullptr,
            think_interruptor_func_t think_interruptor = nullptr
        ) {
            if (!think_reporter) think_reporter = default_think_reporter;
            if (!think_interruptor) think_interruptor = default_think_interruptor;
            Model* helper = (Model*)clone();
            helper->remember = true;
            string response = helper->prompt(prompt);
            for (int step = 0; step < think_steps; step++) {
                response = helper->prompt("Think deeply and refine the response.");
                if (think_interruptor(this)) break;
            }
            kill(helper);
            think_reporter(this, response);
            return response;
        }
        
        string solve(
            const string& task, //int think_steps = 1, int think_deep = 2,
            think_reporter_func_t think_reporter = nullptr,
            think_interruptor_func_t think_interruptor = nullptr,
            int think_deeper = -1
        ) {
            if (think_deeper == -1) think_deeper = think_deep;
            if (!think_reporter) think_reporter = default_think_reporter;
            if (!think_interruptor) think_interruptor = default_think_interruptor;
            // DEBUG("Current task:\n" + task + "\nThinking of a solution... (deep thinking: " + to_string(think_deep) + ")");
            if (think_deeper <= 0 || think_interruptor(this)) return prompt(task);
            think_deeper--;
            Model* helper = (Model*)clone();
            helper->remember = true;
            if (!helper->decide("Is it a task?:\n" + task)) {
                // It's not even a task:
                kill(helper);
                return prompt(task);
            }
            // it's a valid task...

            if (helper->decide("Do you have any questions about the task?")) {
                vector<string> questions = helper->options("What are your most important/relevant questions? (if any)");
                if (!questions.empty()) {
                    // there are questions:

                    // DEBUG(questions.size() > 1 ? 
                    //     "There are several questions:\n" + implode("\n", questions) + "\n" :
                    //     "There there is only one questions:\n" + questions[0] + "\n"
                    // );
                    string answers;
                    for (const string& question: questions) {
                        think_reporter(this, question);
                        answers += question + "\n";
                        vector<string> options = helper->options("Offer possible answers to the following question: " + question);
                        if (!options.empty()) {
                            // DEBUG(options.size() > 1 ? 
                            //     "There are several possible answer:\n" + implode("\n", options) + "\n" :
                            //     "There is only one option:\n" + options[0] + "\n"
                            // );
                            if (options.size() > 1) {
                                // there are next question, there are options:
                                helper->think(
                                    task + question + implode("\n", options) //, think_reporter, think_interruptor
                                );
                                int choise = helper->choose(task + "\n" + question, options);
                                if (choise >= 0) answers += options[choise] + "\n"; // do not store answer if couldn't choose
                            } else answers += options[0] + "\n"; // there are next question, only one option
                        }; // else DEBUG("I don't have an answer :(");
                        if (think_interruptor(this)) break;
                    }
                    if (!answers.empty()) {
                        // there are questions, there are options, we got answers:
                        kill(helper);
                        memorize(task);
                        memorize(answers);
                        return answers;
                    }
                    // there are questions, there are options, no answers...

                    string results = implode("\n", questions); // our results with only questions..
                    kill(helper);
                    memorize(task);
                    memorize(results);
                    return results;
                }
                // no question...

                // kill(helper);
                // return prompt(task);
                //return helper->solve(answers, think_steps, think_deep);
                //return implode("\n", questions);
            }
            // no question...

            if (helper->decide("Is the task too complex, so have to be break down smaller steps?")) {
                vector<string> small_steps = helper->options("What are smaller steps to get this task done?");
                if (!small_steps.empty()) {
                    // DEBUG("Task too complex, have to be break down smaller steps:\n" + implode("\n", small_steps) + "\n");                    
                    string results;
                    for (const string& step: small_steps) {
                        think_reporter(this, step);
                        string result = step + "\n" + helper->solve(
                            task + "\n" + step, nullptr, nullptr, think_deeper //, think_reporter, think_interruptor
                        ) + "\n";
                        think_reporter(this, result);
                        results += result;
                        if (think_interruptor(this)) break;
                    }
                    kill(helper);
                    memorize(results);
                    return results;
                }
                // no question, no smaller steps...

            }
            // no question, no smaller steps...

            kill(helper);
            return prompt(task);
        }
    };

}