#pragma once

#include <climits>
#include <functional>

#include "../ERROR.hpp"
#include "../strings.hpp"
#include "../vectors.hpp"
#include "../Rotary.hpp"
#include "../Speech.hpp"

using namespace std;

namespace tools::llm {

    #define ANSI_FMT_MODEL_THINKS ANSI_FMT_C_BLACK // ANSI_FMT_T_DIM

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
        bool remember;
        string memory;
        size_t memory_max; 
        double memory_loss_ratio;
        // think_reporter_func_t default_think_reporter;
        // think_interruptor_func_t default_think_interruptor;

        virtual void think_reporter(const string& thought = "") {
            cout << "\r\033[2K";
            if (!thought.empty()) cout << ANSI_FMT(ANSI_FMT_MODEL_THINKS, thought) << endl;
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
            memory = helper->prompt("system", "Summarize this:\n" + firstHalf) + secondHalf;
            kill(helper);
        }

    public:
        int think_steps;
        int think_deep;

        #define MODEL_ARGS \
            const string& system = "", \
            bool remember = false, \
            const string& memory = "", \
            size_t memory_max = 100000, \
            double memory_loss_ratio = 0.5, \
            int think_steps = 1, \
            int think_deep = 2
            
        #define MODEL_ARGS_PASS \
            system, \
            remember, \
            memory, \
            memory_max, \
            memory_loss_ratio, \
            think_steps, \
            think_deep
            
        Model(MODEL_ARGS):
            system(system),
            remember(remember),
            memory(memory),
            memory_max(memory_max),
            memory_loss_ratio(memory_loss_ratio),
            think_steps(think_steps), 
            think_deep(think_deep)
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



        string prompt(string from, const string& prmpt/*, const string& suffix*/) {
            const string suffix = "";
            if (!from.empty()) from += ": ";
            if (!remember) return request(from + prmpt + "\n" + system + "\n" + suffix);
            memorize(from + prmpt);
            string response = request(memory + "\n" + system + "\n" + suffix);
            memorize(response);
            return response;
        }
        // string prompt(const string& prmpt, const char* sffx = nullptr) {
        //     string suffix(sffx ? sffx : "");
        //     return prompt(prmpt, suffix);
        // }

        string choose_str(const string& prmpt, const vector<string>& options) {
            if (options.empty()) ERROR("No options to choose from.");
            if (options.size() == 1) return options[0];
            
            string selection = prompt("system", 
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
                selection = prompt("system", "To many. Select only ONE!");
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
            
            string response = prompt("system", 
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
            string response = prompt("system", 
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

        string think(const string& from, const string& prmpt) {
            think_reporter();

            Model* thinker = (Model*)clone();

            string response = thinker->prompt(from, prmpt);
            think_reporter();
            for (int step = 0; step < think_steps; step++) {
                response = thinker->prompt("system", "Think deeply and refine the response.");
                think_reporter();
                if (think_interruptor()) break;
            }

            kill(thinker);

            memorize(prmpt);
            memorize(response);
            return response;
        }

        // string deep_think(const string& task) {
        //     Model* solver = (Model*)clone();
        //     string result = solver->solve(task);
        //     kill(solver);
        //     return result;
        // }
        
        string solve(const string& from, const string& task, int think_deeper = -1) {
            think_reporter();
            if (!remember) return prompt(from, task);
            if (think_deeper == -1) think_deeper = think_deep;
            if (think_deeper <= 0 || think_interruptor()) return prompt(from, task);
            think_deeper--;

            Model* helper = (Model*)clone();

            think_reporter();
            if (!helper->decide("Is it even a task?:\n" + task)) {
                // It's not even a task:
                kill(helper);
                think_reporter();
                return prompt(from, task);
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
                            
                            string thoughts = thinker->think("system",
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
                        memorize(task);
                        memorize(results);
                        return results;
                    }
                    // there are questions, there are options, we got answers:
                    
                    memorize(task);
                    memorize(answers);
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
                        string solution = helper->solve("system", task + "\n" + step, think_deeper);
                        // think_reporter(solution);
                        results += step + "\n" + solution + "\n";
                        if (think_interruptor()) break;
                    }
                    
                    kill(helper);
                    memorize(task);
                    memorize(results);
                    return results;
                }
                // no question, no smaller steps...

            }
            // no question, no smaller steps...

            kill(helper);
            think_reporter();
            return prompt(from, task);
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