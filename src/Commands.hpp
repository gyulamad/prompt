#pragma once

#include "tools/io.hpp"

#include "User.hpp"

using namespace std;
using namespace tools;

namespace prompt {

    class ExitCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return { "/exit" };
        }

        string run(void* user, const vector<string>& args) override {
            ((User*)user)->exit();
            return "Exiting...";
        }
    };

    class HelpCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return { "/help" };
        }

        string run(void* user, const vector<string>& args) override {
            cout << "Usages:" << endl;
            array_dump(((User*)user)->get_cmatcher_ref().command_patterns, false);
            return "";
        }
    };

    // -------- app specific commands ----------

    class VoiceCommand: public Command {
    private:
    
        // void show_user_voice_stat(Speech* speech) {
        //     if (!speech) {
        //         cout << "No speach loaded." << endl;
        //         return;
        //     }
        //     cout << "Voice input:\t[" << (speech->is_voice_in() ? "On" : "Off") << "]" << endl;
        //     cout << "Voice output:\t[" << (speech->is_voice_out() ? "On" : "Off") << "]" << endl;
        // }

    public:
    
        vector<string> get_patterns() const override {
            return { 
                "/voice",
                "/voice {switch}",
            };
        }

        string run(void* user_void, const vector<string>& args) override {
            User* user = (User*)user_void;
            bool on = false;
            if (args.size() == 1) on = user->speech_toggle();
            else if (args.size() == 2) {
                if (args[1] == "on") on = user->speech_create();
                else if (args[1] == "off") on = !user->speech_delete();
                else return "Invalid argument: " + args[1];
            }
            return on ? "Voice mode [ON]" : "Voice mode [OFF]";
        }
    };

    class SendCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return {
                "/send {filename}",
                "/send {string} {filename}",
                "/send {filename} {number} {number}",
                "/send {string} {filename} {number} {number}",
                "/send-lines {filename}",
                "/send-lines {string} {filename}",
                "/send-lines {filename} {number} {number}",
                "/send-lines {string} {filename} {number} {number}",
            };
        }

        string run(void* user_void, const vector<string>& args) override {
            User* user = (User*)user_void;
            int lnfirst = 0, lnlast = 0;
            if (args.size() == 1) {
                cout << "Filename is missing. Use /send [\"message\"] filename [first-line last-line]" << endl;
                cout << "Note: message and first/last line numbers are optional, line numbers start from line-1th."
                    "\nThe line number is zero (0) means the begin/end of file." << endl;
                return "";
            }
            string message = "", filename;
            if (args.size() == 2) {
                filename = args[1];
            }
            if (args.size() == 3) {
                message = args[1];
                filename = args[2];
            }
            if (args.size() == 4) {
                filename = args[1];
                if (!is_numeric(args[2])) cout << "Invalit first line number: " << args[2] << endl;
                else lnfirst = parse<int>(args[2]);
                if (!is_numeric(args[3])) cout << "Invalit last line number: " << args[3] << endl;
                else lnlast = parse<int>(args[3]);
            }
            if (args.size() == 5) {
                message = args[1];
                filename = args[2];
                if (!is_numeric(args[3])) cout << "Invalit first line number: " << args[3] << endl;
                else lnfirst = parse<int>(args[2]);
                if (!is_numeric(args[4])) cout << "Invalit last line number: " << args[4] << endl;
                else lnlast = parse<int>(args[4]);
            }
            if (args.size() > 5) {
                cout << "Too many arguments" << endl;
                return "";
            }

            if (!file_exists(filename)) {
                cout << "File not found: " << filename << endl;
                return "";
            }

            if (lnfirst < 0 || lnlast < lnfirst) {
                cout << "Line numbers should be greater or equal to 1th and first line should be less or equal to the last line number." << endl;
                return "";
            }

            string contents = file_get_contents(filename);
            if (contents.empty()) contents = "<empty>";
            else if (lnfirst || lnlast) {
                vector<string> lines = explode("\n", contents);
                vector<string> show_lines;
                for (size_t ln = 1; ln <= lines.size(); ln++) {
                    if (
                        (lnfirst == 0 || ln >= lnfirst) &&
                        (lnlast == 0 || ln <= lnlast)
                    ) show_lines.push_back(
                        (args[0] == "/send-lines" ? to_string(ln) + ": " : "") + lines[ln-1]
                    );
                }
                contents = implode("\n", show_lines);
            }
            user->get_model_ref().addContext(message + "\nFile '" + filename + "' contents:\n" + contents);
            cout << "File added to the conversation context: " << filename << endl;
            return "";
        }
    };

    class ModeCommand: public Command {
    private:

        void show_user_mode(User* user) {
            string mode_s = "";
            switch (user->get_mode())
            {
                case MODE_CHAT:
                    mode_s = "chat";
                    break;

                case MODE_THINK:
                    mode_s = "think (steps: " + to_string(user->get_model_ref().think_steps) + ")";
                    break;

                case MODE_SOLVE:
                    mode_s = "solve (steps: " + to_string(user->get_model_ref().think_steps) + ", deep: " + to_string(user->get_model_ref().think_deep) + ")";
                    break;
            
                default:
                    throw ERROR("Invalid mode");
            }

            cout << "Mode: " << mode_s << endl;
        }

    public:
    
        vector<string> get_patterns() const override {
            return { 
                "/mode",
                "/mode chat",
                "/mode think",
                "/mode solve",
            };
        }

        string run(void* user_void, const vector<string>& args) override {
            User* user = (User*)user_void;
            if (args.size() >= 2) {
                if (args[1] == "chat") {
                    user->set_mode(MODE_CHAT);
                    show_user_mode(user);
                }
                else if (args[1] == "think") {
                    user->set_mode(MODE_THINK);
                    show_user_mode(user);
                }
                else if (args[1] == "solve") {
                    user->set_mode(MODE_SOLVE);
                    show_user_mode(user);
                }
                else {
                    cout << "Invalid mode: " << args[1] << endl;
                }
            }
            return "";
        }
    };

    class ThinkCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return { "/think {number}" };
        }

        string run(void* user_void, const vector<string>& args) override {
            User* user = (User*)user_void;
            if (args.size() == 2) {
                if (is_integer(args[1]))
                    user->get_model_ref().think_steps = parse<int>(args[1]);
                else cout << "Invalid parameter." << endl;
            }
            cout << "Extracting steps: " << user->get_model_ref().think_steps << endl;

            return "";
        }
    };

    class SolveCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return { "/solve {number}" };
        }

        string run(void* user_void, const vector<string>& args) override {
            User* user = (User*)user_void;
            if (args.size() == 2) {
                if (is_integer(args[1]))
                    user->get_model_ref().think_deep = parse<int>(args[1]);
                else cout << "Invalid parameter." << endl;
            }
            cout << "Deep thinking solution tree depth max: " << user->get_model_ref().think_deep << endl;

            return "";
        }
    };

    class SaveCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return { "/save {string}" };
        }

        string run(void* user_void, const vector<string>& args) override {
            User* user = (User*)user_void;
            
            if (args.size() > 2) {
                cout << "Invalid parameter counts, use /save {name}" << endl;
                return "";
            }
            if (args.size() == 2) {
                user->set_model_name(args[1]);
                if (file_exists(user->get_model_file())) {
                    cout << "Model already exists: " << user->get_model_name_ref() << endl;
                    if (!confirm("Do you want to override?")) return "";
                }
            }
            user->save_model(true);

            return "";
        }
    };

    class LoadCommand: public Command {
    public:
    
        vector<string> get_patterns() const override {
            return { "/load {string}" };
        }

        string run(void* user_void, const vector<string>& args) override {
            User* user = (User*)user_void;
            
            if (args.size() != 2) {
                cout << "Invalid parameter counts, use /load {name}" << endl;
                return "";
            }
            if (!user->is_auto_save() && 
                confirm("Current model session is: " + user->get_model_name_ref() + 
                        "\nDo you want to save it first?")) user->save_model(true);
            
            user->set_model_name(args[1]);
            user->load_model(false);

            return "";
        }
    };

}