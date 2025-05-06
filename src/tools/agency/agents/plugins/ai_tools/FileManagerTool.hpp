#pragma once

#include "../../../../utils/JSON.h"
// #include "../../../../utils/files.hpp"
#include "../../../../utils/Process.hpp"
#include "../../../../str/trim.hpp"
#include "../../../../str/is_valid_filepath.hpp"
#include "../../../../containers/array_keys.hpp"
#include "../../../../files/file_put_contents.hpp"
#include "../../../../files/file_exists.hpp"
#include "../../../../files/remove.hpp"
#include "../../../../files/rename.hpp"
#include "../../../../files/is_dir.hpp"
#include "../../../../files/mkdir.hpp"
#include "../../../../files/is_empty_dir.hpp"
#include "../../UserAgentInterface.hpp"
#include "../Tool.hpp"

using namespace tools::utils;
using namespace tools::files;
using namespace tools::str;
using namespace tools::containers;
using namespace tools::agency::agents;
using namespace tools::agency::agents::plugins;

namespace tools::agency::agents::plugins::ai_tools {

    template<typename T>
    class FileManagerTool: public Tool<T> {
    public:
        
        FileManagerTool(
            UserAgent<T>& user, 
            const string& base_folder
        ): 
            Tool<T>(
                user,
                "file_manager", 
                { 
                    { "action", PARAMETER_TYPE_STRING, true }, // TODO: use get_required_errors to validate Tools before callback()... Note, I found that it's already validated in some way, using the Parameter::is_required()...
                }, 
                callback,
                "*   `action`: Specifies the action to perform: " + actions_to_string(action_map) + "\n"
                "\n"
                "**Action Breakdown:**"
                "\n"
                "*   **create/append:**\n"
                "    *   `filename`: Required.\n"
                "    *   `content`: Required.\n"
                "    *Note: Create attempt overrides if the file already exists.\n"
                "\n"
                "*   **remove:**\n"
                "    *   `filename`: Required.\n"
                "\n"
                "*   **rename:**\n"
                "    *   `old_filename`: Required.\n"
                "    *   `new_filename`: Required.\n"
                "\n"
                "*   **view:**\n"
                "    *   `filename`: Required.\n"
                "    *   `start_line`: Optional (number, default: 1th line).\n"
                "    *   `end_line`: Optional (number, default: last line).\n"
                "\n"
                "*   **edit:**\n"
                "    *   `filename`: Required.\n"
                "    *   `start_line`: Optional (number, default: 1th line).\n"
                "    *   `end_line`: Optional (number, default: last line).\n"
                "    *   `content`: Required. (send empty string to remove lines)\n"
                "\n"
                "*   **exec:**\n"
                "    *   `command`: Required (bash command).\n"
                "    *   `timeout`: Optional (number, in seconds).\n"
                "    *Notes:\n"
                "       You need to avoid long running or blocking commands\n"
                "       that waits for user input to prevent them\n"
                "       from being prematurely terminated when timeouts.\n" 
                "\n"
                "*   **list:**\n"
                "    *   `directory`: Required. Specifies the directory to list.\n"
                "    *   `keyword`: Optional. If provided, only files and folders containing the keyword in their names will be listed.\n"
                "\n"
                "*   **make_dir:**\n"
                "    *   `dirname`: Required.\n"
                "\n"
                "*   **remove_dir:**\n"
                "    *   `dirname`: Required.\n"
                "\n"
                "*   **rename_dir:**\n"
                "    *   `old_dirname`: Required.\n"
                "    *   `new_dirname`: Required.\n"
                "\n"
                "*   **search:**\n"
                "    *   `directory`: Required. Specifies the directory to list.\n" //  TODO
                "    *   `keyword`: Required. Searches for the keyword in files.\n"
                "\n"
            ),
            base_folder(base_folder)
        {}

        static string callback(void* tool_void, /*void* model_void, void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            if (!args.has("action")) return "Parameter 'action' is missing!";
            string action = args.get<string>("action");
            if (trim(action).empty()) return "Parameter 'action' can not be empty!";
            auto it = action_map.find(action);
            if (it == action_map.end()) return "Action '" + it->first + "' does not exists!\nPossible actions are: " + actions_to_string(action_map);
            return it->second(tool_void, /*model_void, user_void,*/ args/*, conf*/);
        }

        static map<string, tool_cb> action_map;

        static string actions_to_string(map<string, tool_cb> action_map) {
            return "[\"" + implode("\", \"", array_keys(action_map)) + "\"]";
        }

        string write(/*void* user_void,*/ const JSON& args, /*const JSON& conf,*/ bool append) {
            // NULLCHK(user_void);
            // error_to_user(get_required_error<T>(conf, "base_folder"));
            // string base = conf.get<string>("base_folder");
            
            string errors = this->template get_required_errors<string>(args, { "filename", "content" });
            if (!errors.empty()) return errors;
            string filepath = base_folder + args.get<string>("filename");
            if (!is_valid_filepath(filepath)) return "Filename is invalid: " + filepath;
            string content = args.get<string>("content"); 
            errors += this->get_user_confirm_error(
                [&](const string& prmpt) { 
                    return this->user.confirm(prmpt); 
                }, 
                string("File manager tool wants to ") 
                    + (append ? "append to" : "create a") + " file: " + filepath 
                    + "\nwith contents:\n" + (content.empty() ? "<empty>" : content) 
                    + "\nDo you want to proceed?",
                "User intercepted the file operation", filepath
            );
            if (!errors.empty()) return errors;

            if (!file_put_contents(filepath, content, append))
                return "File write failed: " + filepath; 

            // if (!append) {
            //     try {
            //         chgrp(filepath, group);
            //         chprm(filepath, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
            //     } catch (exception &e) {
            //         return "File write error: " + string(e.what());
            //     }
            // }

            string outp = "File writen: " + filepath;
            cout << outp << "\nContents:\n" + (content.empty() ? "<empty>" : content) << endl;
            return outp;
        }

        string remove(/*void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            // NULLCHK(user_void);
            // error_to_user(get_required_error<T>(conf, "base_folder"));
            // string base = conf.get<string>("base_folder");
            
            string errors = this->template get_required_error<string>(args, "filename");
            if (!errors.empty()) return errors;
            string filepath = base_folder + args.get<string>("filename");
            if (!is_valid_filepath(filepath)) return "Filename is invalid: " + filepath;
            if (!file_exists(filepath)) return "File not exists: " + filepath;

            errors += this->get_user_confirm_error(
                [&](const string& prmpt) { 
                    return this->user.confirm(prmpt); 
                }, 
                string("File manager tool wants to delete file:") + filepath 
                    + "\nDo you want to proceed?",
                "User intercepted the file operation", filepath
            );
            if (!errors.empty()) return errors;

            if (!files::remove(filepath, false))
                return "File remove failed: " + filepath; 

            string ret = "File removed: " + filepath;
            cout << ret << endl;
            return ret;
        }

        string rename(/*void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            // NULLCHK(user_void);
            // error_to_user(get_required_error<T>(conf, "base_folder"));
            // string base = conf.get<string>("base_folder");
            
            string errors = this->template get_required_errors<string>(args, { "old_filename", "new_filename" });
            if (!errors.empty()) return errors;
            string old_filepath = base_folder + args.get<string>("old_filename");
            if (!is_valid_filepath(old_filepath)) return "Old filename is invalid: " + old_filepath;
            string new_filepath = base_folder + args.get<string>("new_filename");
            if (!is_valid_filepath(new_filepath)) return "New filename is invalid :" + new_filepath;
            if (old_filepath == new_filepath) return "Files can not be the same: " + old_filepath;
            if (!file_exists(old_filepath)) return "File not exists: " + old_filepath;
            if (file_exists(new_filepath)) return "File already exists: " + new_filepath + "\nYou have to delete first!";

            string to = old_filepath + " to " + new_filepath;
            errors += this->get_user_confirm_error(
                [&](const string& prmpt) { 
                    return this->user.confirm(prmpt); 
                }, 
                string("File manager tool wants to rename file: ") + to
                    + "\nDo you want to proceed?",
                "User intercepted the file operation", to
            );
            if (!errors.empty()) return errors;

            if (!files::rename(old_filepath, new_filepath, false))
                return "File rename failed: " + to; 

            string ret = "File renamed: " + to;
            cout << ret << endl;
            return ret;
        }

        string view(/*void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            // error_to_user(get_required_error<T>(conf, "base_folder"));
            // string base = conf.get<string>("base_folder");
            string errors = this->template get_required_error<string>(args, "filename");
            if (!errors.empty()) return errors;
            string filepath = base_folder + args.get<string>("filename");
            if (!is_valid_filepath(filepath)) return "Filename is invalid: " + filepath;
            if (!file_exists(filepath)) return "File not exists: " + filepath;
            vector<string> lines;
            try {
                lines = explode("\n", file_get_contents(filepath));
            } catch (ios_base::failure &e) {
                return "IO operation failed on file read '" + filepath + "': " + e.what();
            }
            if (lines.empty()) return "File '" + filepath + "' is empty.";
            int start_line = args.has("start_line") ? args.get<int>("start_line") - 1 : 0;
            int end_line = args.has("end_line") ? args.get<int>("end_line") - 1 : lines.size();
            if (start_line < 0) return "Start line can not be less than 1!";
            if (end_line < 0) return "End line can not be less than 1!";
            string outp = "";
            for (int i = start_line; i < end_line; i++)
                outp += ::to_string(i+1) + "|" + lines[i] + "\n";
            outp = "Contents from file: " + filepath + "\n" + (outp.empty() ? "<empty>" : outp) + "\n";
            cout << outp << flush;
            return outp;
        }

        string edit(/*void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            // error_to_user(get_required_error<T>(conf, "base_folder"));
            // string base = conf.get<string>("base_folder");
            string errors = this->template get_required_errors<string>(args, { "filename", "content" });
            if (!errors.empty()) return errors;
            string filepath = base_folder + args.get<string>("filename");
            if (!is_valid_filepath(filepath)) return "Filename is invalid: " + filepath;
            string content = args.get<string>("content"); 
            if (!file_exists(filepath)) return "File not exists: " + filepath;
            vector<string> lines;
            try {
                lines = explode("\n", file_get_contents(filepath));
            } catch (ios_base::failure &e) {
                return "IO operation failed on file read '" + filepath + "': " + e.what();
            }
            
            errors += this->get_user_confirm_error(
                [&](const string& prmpt) { 
                    return this->user.confirm(prmpt); 
                }, 
                string("File manager tool wants to replace in file: ") + filepath
                    + "\nDo you want to proceed?",
                "User intercepted the file operation", filepath
            );
            if (!errors.empty()) return errors;

            int start_line = args.has("start_line") ? args.get<int>("start_line") - 1 : 0;
            int end_line = args.has("end_line") ? args.get<int>("end_line") - 1 : lines.size();
            if (start_line < 0) return "Start line can not be less than 1!";
            if (end_line < 0) return "End line can not be less than 1!";
            vector<string> before;
            for (int i = 0; i < start_line; i++)
                before.push_back(lines[i]);
            vector<string> after;
            for (size_t i = end_line + 1; i < lines.size(); i++)
                after.push_back(lines[i]);
            string new_content = implode("\n", before) + (content.empty() ? "" : "\n" + content + "\n") + implode("\n", after);
            if (!file_put_contents(filepath, new_content))
                return "File content modification failed: " + filepath; 

            string ret = "File updated: " + filepath;
            cout << ret + "\nNew contents:\n" + (new_content.empty() ? "<empty>" : new_content) << endl;
            return ret;
        }

        string exec(/*void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            // error_to_user(get_required_error<T>(conf, "base_folder"));
            // string base = conf.get<string>("base_folder");

            string errors = this->template get_required_error<string>(args, { "command" });
            if (!errors.empty()) return errors;

            string command = args.get<string>("command");
            int timeout = args.has("timeout") ? args.get<int>("timeout") : 10;
            
            errors += this->get_user_confirm_error(
                [&](const string& prmpt) { 
                    return this->user.confirm(prmpt); 
                }, // solve tools::to_string(..) conflict
                string("File manager tool wants to execute a command with timeout ") + ::to_string(timeout) + "s:\n" + command
                    + "\nDo you want to proceed?",
                "User intercepted the command execution", command
            );
            if (!errors.empty()) return errors;
            
            auto start_time = chrono::steady_clock::now();
            string output = "";
            try {
                string full_command = "cd " + base_folder + " && " + (timeout ? string("timeout ") + ::to_string(timeout) + "s " : "") + command + " 2>&1";
                output = Process::execute(full_command.c_str());
            } catch (const runtime_error& e) {
                output = "Command execution failed: " + string(e.what());
            }
            auto current_time = chrono::steady_clock::now();
            auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(current_time - start_time).count();

            // Display and return results
            string elapsed = "Command execution time was: " + ::to_string(elapsed_time) + "ms";
            cout << "\nOutput:\n" << output + "\n" + elapsed << endl;
            return 
                "Results from command execution:\n" + trim(command) + "\n" + elapsed +
                "\nOutput:\n" + (output.empty() ? "<empty>" : output);
        }

        string search(const JSON& args) {
            string errors = this->template get_required_error<string>(args, { "keyword" });
            if (!errors.empty()) return errors;
            
            string keyword = args.get<string>("keyword");
            string command = "grep -rn " + keyword + " " + base_folder;
            string output = "";
            try {
                output = Process::execute(command.c_str());
            } catch (const runtime_error& e) {
                output = "Command execution failed: " + string(e.what());
            }
            cout << output << endl;
            return output;
        }

        string list(const JSON& args) {
            string directory = args.has("directory") ? args.get<string>("directory") : "";
            string keyword = args.has("keyword") ? args.get<string>("keyword") : "";
            vector<string> files_and_folders;
            for (const auto& entry : filesystem::directory_iterator(base_folder + directory)) {
                if (entry.is_directory() || entry.is_regular_file()) {
                    string full_path = entry.path().string();
                    string item_name = entry.path().filename().string();
                    if (keyword.empty() || item_name.find(keyword) != string::npos) {
                        files_and_folders.push_back(item_name);
                    }
                }
            }

            string output = "Files and folders in " + base_folder + directory + (keyword.empty() ? "" : " (matching '" + keyword + "')") + ":\n";
            for (const auto& item_name : files_and_folders) {
                string full_path = base_folder + directory + item_name;
                string type_indicator = files::is_dir(full_path) ? "[DIR] " : "[FILE] ";
                output += type_indicator + item_name + "\n";
            }
            cout << output << flush;
            return output;
        }

        string mkdir(const JSON& args) {
            string errors = this->template get_required_error<string>(args, "dirname");
            if (!errors.empty()) return errors;
            string dirpath = base_folder + args.get<string>("dirname");
            if (!is_valid_filepath(dirpath)) return "Dirname is invalid: " + dirpath;
            if (file_exists(dirpath)) return "Dir already exists: " + dirpath;

            errors += this->get_user_confirm_error(
                [&](const string& prmpt) { 
                    return this->user.confirm(prmpt); 
                }, 
                string("File manager tool wants to create dir: ") + dirpath
                    + "\nDo you want to proceed?",
                "User intercepted the file operation", dirpath
            );
            if (!errors.empty()) return errors;

            if (!files::mkdir(dirpath, true))
                return "Dir create failed: " + dirpath; 

            string ret = "Dir created: " + dirpath;
            cout << ret << endl;
            return ret;
        }

        string rmdir(const JSON& args) {
            string errors = this->template get_required_error<string>(args, "dirname");
            if (!errors.empty()) return errors;
            string dirpath = base_folder + args.get<string>("dirname");
            if (!is_valid_filepath(dirpath)) return "Dirname is invalid: " + dirpath;
            if (!file_exists(dirpath)) return "Dir not exists: " + dirpath;
            if (!files::is_empty_dir(dirpath)) return "Dir is not empty: " + dirpath;

            errors += this->get_user_confirm_error(
                [&](const string& prmpt) { 
                    return this->user.confirm(prmpt); 
                }, 
                string("File manager tool wants to delete dir: ") + dirpath
                    + "\nDo you want to proceed?",
                "User intercepted the file operation", dirpath
            );
            if (!errors.empty()) return errors;

            if (!files::remove(dirpath))
                return "Dir remove failed: " + dirpath; 

            string ret = "Dir removed: " + dirpath;
            cout << ret << endl;
            return ret;
        }

        string rndir(const JSON& args) {
            string errors = this->template get_required_errors<string>(args, { "old_dirname", "new_dirname" });
            if (!errors.empty()) return errors;
            string old_dirpath = base_folder + args.get<string>("old_dirname");
            if (!is_valid_filepath(old_dirpath)) return "Old dirname is invalid: " + old_dirpath;
            string new_dirpath = base_folder + args.get<string>("new_dirname");
            if (!is_valid_filepath(new_dirpath)) return "New dirname is invalid :" + new_dirpath;
            if (old_dirpath == new_dirpath) return "Dirs can not be the same: " + old_dirpath;
            if (!file_exists(old_dirpath)) return "Dir not exists: " + old_dirpath;
            if (file_exists(new_dirpath)) return "Dir already exists: " + new_dirpath + "\nYou have to delete first!";

            string to = old_dirpath + " to " + new_dirpath;
            errors += this->get_user_confirm_error(
                [&](const string& prmpt) { 
                    return this->user.confirm(prmpt); 
                }, 
                string("File manager tool wants to rename dir: ") + to
                    + "\nDo you want to proceed?",
                "User intercepted the file operation", to
            );
            if (!errors.empty()) return errors;

            if (!files::rename(old_dirpath, new_dirpath, true))
                return "Dir rename failed: " + to; 

            string ret = "Dir renamed: " + to;
            cout << ret << endl;
            return ret;
        }

        static string create_cb(void* tool_void, /*void* model_void, void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->write(/*user_void,*/ args, /*conf,*/ false);
        }

        static string append_cb(void* tool_void, /*void* model_void, void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->write(/*user_void,*/ args, /*conf,*/ true);
        }

        static string remove_cb(void* tool_void, /*void* model_void, void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->remove(/*user_void,*/ args/*, conf*/);
        }

        static string rename_cb(void* tool_void, /*void* model_void, void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->rename(/*user_void,*/ args/*, conf*/);
        }

        static string view_cb(void* tool_void, /*void* model_void, void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->view(/*user_void,*/ args/*, conf*/);
        }

        static string edit_cb(void* tool_void, /*void* model_void, void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->edit(/*user_void,*/ args/*, conf*/);
        }

        static string exec_cb(void* tool_void, /*void* model_void, void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->exec(/*user_void,*/ args/*, conf*/);
        }

        static string list_cb(void* tool_void, /*void* model_void, void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->list(args);
        }

        static string search_cb(void* tool_void, /*void* model_void, void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->search(args);
        }

        static string mkdir_cb(void* tool_void, /*void* model_void, void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->mkdir(args);
        }

        static string rmdir_cb(void* tool_void, /*void* model_void, void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->rmdir(args);
        }

        static string rndir_cb(void* tool_void, /*void* model_void, void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            NULLCHK(tool_void);
            FileManagerTool* tool = (FileManagerTool*)tool_void;
            return tool->rndir(args);
        }

        string base_folder;

    };

    template<typename T>
    map<string, tool_cb> FileManagerTool<T>::action_map = {
        { "create", FileManagerTool<T>::create_cb },
        { "append", FileManagerTool<T>::append_cb },
        { "remove", FileManagerTool<T>::remove_cb },
        { "rename", FileManagerTool<T>::rename_cb },
        { "view", FileManagerTool<T>::view_cb },
        { "edit", FileManagerTool<T>::edit_cb },
        { "exec", FileManagerTool<T>::exec_cb },
        { "list", FileManagerTool<T>::list_cb },
        { "make_dir", FileManagerTool<T>::mkdir_cb}, // create directory
        { "remove_dir", FileManagerTool<T>::rmdir_cb}, // delete directory (only if empty)
        { "rename_dir", FileManagerTool<T>::rndir_cb}, // rename directory
        { "search", FileManagerTool<T>::search_cb}, // search in files ("keyword" parameter is required, list all file:line where the keyword found - similar to grep (you can utilize the grep command using Process::execute))
    };

}
