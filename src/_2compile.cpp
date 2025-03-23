#include <iostream>
#include <set>

#include "tools/utils/Test.hpp"
#include "tools/utils/Arguments.hpp"
#include "tools/utils/files.hpp"
#include "tools/utils/JSON.hpp"
#include "tools/utils/files.hpp"
#include "tools/utils/execute.hpp"

using namespace std;
using namespace tools::utils;


struct include_depnode {
    vector<include_depnode*> dependencies; // Raw pointers to dependencies
    bool is_built = false;
    time_t last_modified = 0;

    include_depnode() = default; // No file_path, so default constructor is fine
    virtual ~include_depnode() = default; // No cleanup needed
};

// using include_registry = unordered_map<string, include_depnode*>;

// void include_registry_free(include_registry& registry) {
//     for (auto& [path, node] : registry) delete node;
//     registry.clear();
// }

class include_registry {
public:
    include_registry() = default;

    virtual ~include_registry() {
        for (auto& [path, node] : items) if (node) delete node;
    }

    include_depnode* set(const string& abs_path) {
        if (!array_key_exists(abs_path, items)) items[abs_path] = new include_depnode();
        return items[abs_path];
    }

    include_depnode* get(const string& abs_path) {
        if (!array_key_exists(abs_path, items)) return nullptr;
        return items[abs_path];
    }

    const unordered_map<string, include_depnode*>& get() const { return items; }

private:
    unordered_map<string, include_depnode*> items;
};

struct include_scancfg {
    string include_regex = R"(^\s*#\s*include\s*\"\s*([^\"]+)\s*\")";
    string base_dir;
};

include_depnode* scan_includes(
    const string& filename,
    include_registry& registry,
    ms_t& last_depmod,
    const include_scancfg& config = include_scancfg(),
    vector<string>* visited = nullptr
) {
    vector<string> local_visited;
    if (!visited) visited = &local_visited;

    string abs_path = get_absolute_path(filename);
    
    if (in_array(abs_path, *visited)) return registry.get(abs_path);
    visited->push_back(abs_path);

    ms_t fmtime = filemtime_ms(abs_path);
    if (last_depmod < fmtime) last_depmod = fmtime;
    
    include_depnode& node = *registry.set(abs_path);

    ifstream file(filename);
    if (!file.is_open()) throw ERROR("Could not open file: " + filename);
    cout << "scanning: " << filename << endl;

    string line;

    while (getline(file, line)) {
        vector<string> matches;
        if (!regx_match_all(config.include_regex, line, &matches) || matches.size() < 2) continue;
        string include_name = matches[1];
        // Always resolve relative to the current file’s directory
        // Use config.base_dir only if it’s explicitly set and non-empty
        string base_path = (config.base_dir.empty() && config.base_dir != fs::current_path().string())
            ? get_path(filename)
            : config.base_dir;
        string full_path = fix_path(base_path + "/" + include_name);

        include_depnode* dep_node = scan_includes(full_path, registry, last_depmod, config, visited);
        node.dependencies.push_back(dep_node);
    }

    file.close();
    return &node;
}

// =============================================================
// =============================================================
// =============================================================

string get_main_input_file(const Arguments& args) {
    return get_absolute_path(args.getString(1));
}

string get_main_input_path(const Arguments& args) {
    string main_input_file = get_main_input_file(args);
    return get_path(main_input_file);
}

string get_config_file(const Arguments& args, const string& def_config_file = "compile.conf.json") {
    string main_input_path = get_main_input_path(args);
    return fix_path(main_input_path + "/" + def_config_file);
}

JSON get_config(const Arguments& args, const string& def_config_file = "compile.conf.json") {
    string config_file = get_config_file(args, def_config_file);
    JSON config(file_get_contents(config_file));
    return config;

}

string exec_show(const string& cmd) {
    cout << cmd << endl;
    string output = execute(cmd);
    cout << output << endl;
    return output;
}

string get_hash(const string& str) {
    return to_string(hash<string>{}(str));
}

string get_hash(Arguments args, JSON config) {
    return get_hash("__ARGUMENTS__:" + implode(" ", args.getArgsCRef()) + "\n__CONFIG__:" + config.dump());
} 

int safe_main(int argc, char *argv[]) {
    try {
        Arguments args(argc, argv);
        JSON config = get_config(args);
        string hash = get_hash(args, config);

        string mode = args.getString("mode", "release");
        JSON config_mode = config.get<JSON>(mode);

        string workspaceFolder = get_absolute_path(get_main_input_path(args) + "/" + config.get<string>("workspaceFolder"));
        string buildsFolder = get_absolute_path(get_main_input_path(args) + "/" + config.get<string>("buildsFolder") + "/");
        mkdir(buildsFolder);
        string buildsFolderHash = fix_path(buildsFolder + "/" + hash);
        mkdir(buildsFolderHash);

        string main_input_file = get_main_input_file(args);
        string main_output_file = fix_path(buildsFolderHash + "/" + remove_exetension(main_input_file));
        string main_output_file_nohash = fix_path(buildsFolder + "/" + remove_exetension(main_input_file));
        if (mode == "debug") main_output_file += ".gdb";

        string command = config_mode.get<string>("command");
        string arguments = implode(" ", config_mode.get<vector<string>>("args"));

        include_registry registry;
        ms_t last_depmod;
        // include_depnode* root_node = 
        scan_includes(main_input_file, registry, last_depmod); // TODO: load/save/validate a cache for include dependencies!!
        // include_registry_free(registry);

        // Print graph
        cout << "Dependencies:" << endl;
        vector<string> keys = array_keys(registry.get());
        foreach(keys, [](const string& key) {
            cout << key << endl;
        });

        string output = "";
        if (!file_exists(main_output_file) || last_depmod > filemtime_ms(main_output_file)) {
            string compile_cmd = str_replace("${workspaceFolder}", workspaceFolder, command + " " + main_input_file + " -o " + main_output_file + " " + arguments);
            output = exec_show(compile_cmd);
        }
        if (!output.empty()) return 1; // TODO: need more sophisticated error catcher. g++ is not showing any output on success but it's not necessary always true!

        exec_show("cp " + main_output_file + " " + main_output_file_nohash);
    } catch (exception &e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}


int main(int argc, char *argv[]) {
    return run_tests() + safe_main(argc, argv);
}