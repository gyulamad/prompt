#pragma once

#include <climits>
#include <functional>

#include "../ERROR.hpp"
#include "../strings.hpp"
#include "../vectors.hpp"
#include "../Rotary.hpp"

using namespace std;

namespace tools::llm {

    #define ANSI_FMT_MODEL_THINKS ANSI_FMT_C_BLACK ANSI_FMT_T_DIM

    class Model {
    // public:
        // using think_reporter_func_t = function<void(Model* model, const string&)>;
        // using think_interruptor_func_t = function<bool(Model* model)>;
    private:
        static Rotary rotary;
    protected:
        const string opt_prefix = "[[OPTION-START]]";
        const string opt_suffix = "[[OPTION-END]]";
        string system;
        bool remember;
        string memory;
        size_t memory_max; 
        double memory_loss_ratio;
        const int think_steps;
        const int think_deep;
        // think_reporter_func_t default_think_reporter;
        // think_interruptor_func_t default_think_interruptor;

        virtual void think_reporter(const string& thought) {
            cout << ansi_fmt(ANSI_FMT_MODEL_THINKS, thought) << endl;
            rotary.tick();
        }

        virtual bool think_interruptor() {
            rotary.tick();
            return false;
        }

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
            int think_deep = 2

            // think_reporter_func_t default_think_reporter = [](Model*, const string&) { cout << ansi_fmt(ANSI_FMT_MODEL_THINKS, ".") << flush; }, 
            // think_interruptor_func_t default_think_interruptor = [](Model*) { cout << ansi_fmt(ANSI_FMT_MODEL_THINKS, ".") << flush; return false; }

        #define MODEL_ARGS_PASS \
            system, \
            remember, \
            memory, \
            memory_max, \
            memory_loss_ratio, \
            think_steps, \
            think_deep

            // default_think_reporter, 
            // default_think_interruptor

        Model(MODEL_ARGS):
            system(system),
            remember(remember),
            memory(memory),
            memory_max(memory_max),
            memory_loss_ratio(memory_loss_ratio),
            think_steps(think_steps), 
            think_deep(think_deep)
            // default_think_reporter(default_think_reporter),
            // default_think_interruptor(default_think_interruptor)
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



        string prompt(const string& prmpt, const string& suffix) {
            if (!remember) return request(prmpt + "\n" + system + "\n" + suffix);
            memorize(prmpt);
            string response = request(memory + "\n" + system + "\n" + suffix);
            memorize(response);
            return response;
        }
        string prompt(const string& prmpt, const char* sffx = nullptr) {
            string suffix(sffx ? sffx : "");
            return prompt(prmpt, suffix);
        }

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

            string choise = choose_str(prmpt, options);
            
            return which(choise, options);
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
                //return defopt; // couldn't
                //return -options.size() - 1; // < -options - 1 means couldn't select at all
                // throw ERROR("No selection.\nPrompt: " + prompt);
                
            // if (selections.size() > 1) {
            //     kill(helper);
            //     return -selections.size(); // < -1 means multiple selected
            //     // throw ERROR("Multiple selection, prompt: " + prompt);
            // }
            // int selection = -1; // -1 means no decision.
            // int distance_min = INT_MAX;
            // for (size_t nth = 0; nth < options.size(); nth++) {
            //     int distance = levenshtein(selections[0], options[nth]);
            //     if (distance <= distance_min) {
            //         distance_min = distance;
            //         selection = nth;
            //     }
            // }
            
            // kill(helper);
            // return selection;
        }

        vector<string> choises(const string& prmpt) {
            string response = prompt(
                prmpt + "\n"
                "List your response in the following format:\n" +
                implode_options({ "Your selection..", "Other option (if multiple apply)..." })
            );
            return explode_options(response);
        }
        // vector<string> options(const string& prmpt, const vector<string>& options = {}) {
        //     Model* helper = (Model*)clone();
        //     string response = helper->prompt(prmpt, 
        //         (options.empty() ? "" : "Your options to choose from: \n" + implode_options(options)) + 
        //         "List your response in the following format:" +
        //         implode_options({ "Your selection..", "Other option (if multiple apply)" }) + "etc.."
        //     );
        //     kill(helper);
        //     return explode_options(response);
        // }

        bool decide(const string& prmpt, const vector<string>& options = {"Yes", "No"}) {
            int resp = choose(prmpt, options);
            if (resp == 0) return true;
            if (resp == 1) return false;
            throw ERROR("Model was unable to decide. Prompt: " + str_cut_end(prmpt));
        }

        string think(
            const string& prmpt//, //int think_steps = 1,
            // think_reporter_func_t think_reporter = nullptr,
            // think_interruptor_func_t think_interruptor = nullptr
        ) {
            // if (!think_reporter) think_reporter = default_think_reporter;
            // if (!think_interruptor) think_interruptor = default_think_interruptor;

            string response = prompt(prmpt);
            for (int step = 0; step < think_steps; step++) {
                response = prompt("Think deeply and refine the response.");
                if (think_interruptor()) break;
            }

            think_reporter(response);
            return response;
        }
        
        string solve(
            const string& task, //int think_steps = 1, int think_deep = 2,
            // think_reporter_func_t think_reporter = nullptr,
            // think_interruptor_func_t think_interruptor = nullptr,
            int think_deeper = -1
        ) {
            if (!remember) return prompt(task);
            if (think_deeper == -1) think_deeper = think_deep;
            // if (!think_reporter) think_reporter = default_think_reporter;
            // if (!think_interruptor) think_interruptor = default_think_interruptor;
            // DEBUG("Current task:\n" + task + "\nThinking of a solution... (deep thinking: " + to_string(think_deep) + ")");
            if (think_deeper <= 0 || think_interruptor()) 
                return prompt(task);
            think_deeper--;

            Model* helper = (Model*)clone();
            // helper->remember = true;

            if (!helper->decide("Is it even a task?:\n" + task)) {
                // It's not even a task:
                kill(helper);
                return prompt(task);
            }
            // it's a valid task...

            if (helper->decide("Do you have any questions about the task?")) {
                vector<string> questions = helper->choises("What are your most important/relevant questions? (if any)");
                if (!questions.empty()) {
                    // there are questions:

                    // DEBUG(questions.size() > 1 ? 
                    //     "There are several questions:\n" + implode("\n", questions) + "\n" :
                    //     "There there is only one questions:\n" + questions[0] + "\n"
                    // );
                    string answers;
                    for (const string& question: questions) {
                        if (think_interruptor()) break;
                        think_reporter(question);
                        answers += question + "\n";

                        Model* creative = (Model*)helper->clone();
                        // creative->remember = true;
                        // creative->amnesia();

                        vector<string> options = creative->choises(
                            "Because the task:\n" + task + "\n" + 
                            "The main question now:\n" + question + "\n" + 
                            "Offer a possible answer(s)/resolution(s) for the following question/problem:\n" + 
                            question + "\n"
                            "Offer fewer options if you can not give precise/related solution and more if you have multiple possibly good and relevant idea. We interested only in relevant and best solutions but if you can only guess we still want to hear you."
                        );

                        kill(creative);

                        if (options.empty()) continue;
                        if (options.size() == 1) {
                            answers += options[0] + "\n"; // there are next question, only one option
                            continue;
                        }
                        // there are next question, there are options:
                        
                        // DEBUG(options.size() > 1 ? 
                        //     "There are several possible answer:\n" + implode("\n", options) + "\n" :
                        //     "There is only one option:\n" + options[0] + "\n"
                        // );
                        string previous_option = options[0];
                        string previous_thoughts;
                        int best_option = 0, nth = 0;
                        for (const string& option: options) {
                            nth++;

                            Model* thinker = (Model*)helper->clone();
                            // thinker->remember = true;
                            // thinker->amnesia();

                            string thoughts = thinker->think(
                                "Because the task:\n" + task + "\n" + 
                                "The main question now:\n" + question + "\n" + 
                                "One possible solution is:\n" + option + "\n" +
                                "Your current task is to think about this solution!"
                            );                            

                            kill(thinker);

                            if (previous_thoughts.empty()) best_option = nth;
                            else {
                                Model* chooser = (Model*)helper->clone();
                                // chooser->remember = true;
                                // chooser->amnesia();

                                int choise = chooser->decide(
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

                                kill(chooser);

                                if (choise == 1) best_option = nth;
                            }

                            previous_option = option;
                            previous_thoughts = thoughts;
                            if (think_interruptor()) break;
                        }
                        answers += options[best_option] + "\n";

                        // int choise = helper->choose(task + "\n" + question, options);
                        // if (choise >= 0) answers += options[choise] + "\n"; // do not store answer if couldn't choose
                    
                        
                        if (think_interruptor()) break;
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
                vector<string> small_steps = helper->multiple_str("What are smaller steps to get this task done?");
                if (!small_steps.empty()) {
                    // DEBUG("Task too complex, have to be break down smaller steps:\n" + implode("\n", small_steps) + "\n");                    
                    string results;
                    for (const string& step: small_steps) {
                        think_reporter(step);
                        string solution = helper->solve(task + "\n" + step, think_deeper);
                        think_reporter(solution);
                        results += step + "\n" + solution + "\n";
                        if (think_interruptor()) break;
                    }
                    // results = helper->prompt("Summarize the followings: \b" + results);
                    kill(helper);
                    memorize(task);
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
            "    ", 
            ".   ", 
            "..  ", 
            "... ",
        }, 1)     // Dots
    });
}