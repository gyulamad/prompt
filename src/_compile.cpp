#include <sys/stat.h>
#include <ctime>

#include "tools/utils/Test.hpp"
#include "tools/utils/Arguments.hpp"

#include "tools/_build/_build.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::build;

// Function to print help message
void print_help(const string& executable_name) {
    cerr << "Usage: " << executable_name << " <file> [options]\n"
         << "Options:\n"
         << "  --mode <debug|release>   Set build configuration (default: release)\n"
         << "  --output-dir <dir>       Set output directory (default: ./builds)\n"
         << "  --output-exe <exe_name>  Set executable name (default: derived from file)\n"
         << "  --flags <flags>          Additional compiler flags (e.g., \"-Wall -O2\")\n"
         << "  --help                   Show this help message\n";
}

int safe_main(int argc, char *argv[]) {
    Arguments args(argc, argv);
    try {
        if (args.has("help")) {
            print_help(args.getString(0));
            return 0;
        }

        string main_file = args.getString(1);
        string config_mode = args.has("mode") ? args.getString("mode") : "release";
        if (config_mode != "debug" && config_mode != "release") throw ERROR("Invalid config value");
        string output_dir = args.has("output-dir") ? args.getString("output-dir") : "./builds";
        string output_exe_base = args.has("output-exe") ? args.getString("output-exe") : main_file.substr(0, main_file.find(".cpp"));
        string extra_flags = args.has("flags") ? args.getString("flags") : "";

        // Generate build-specific subfolder and executable name
        string build_hash = generate_build_hash(main_file, config_mode, extra_flags);
        string build_dir = output_dir + "/" + build_hash;
        string output_exe = build_dir + "/" + output_exe_base + (config_mode == "debug" ? ".gdb" : "");

        // Dependency graph setup
        include_registry registry;
        include_scancfg config;
        include_depnode* root_node = nullptr;
        string cache_file = build_dir + "/.dependency_cache";
        time_t cache_timestamp = 0;

        fs::create_directories(build_dir);

        if (load_dependency_cache(registry, cache_file, cache_timestamp)) {
            string abs_main = fs::absolute(main_file).string();
            if (registry.find(abs_main) != registry.end()) {
                root_node = registry[abs_main];
            }
        }

        bool cache_valid = (root_node != nullptr);
        for (auto& [path, node] : registry) {
            struct stat file_stat;
            if (stat(path.c_str(), &file_stat) == 0) {
                node->last_modified = file_stat.st_mtime;
                if (cache_valid && node->last_modified > cache_timestamp) {
                    cache_valid = false;
                }
            } else {
                cache_valid = false;
                break;
            }
        }

        if (!cache_valid || root_node == nullptr) {
            cout << "Cache invalid or missing for build " << build_hash << ", rebuilding dependency graph...\n";
            include_registry_free(registry);
            root_node = scan_includes(main_file, registry, config);
            save_dependency_cache(registry, cache_file);
        }

        // Change detection
        string timestamp_file = build_dir + "/.build_timestamp";
        time_t last_build_time = 0;
        ifstream timestamp_in(timestamp_file);
        if (timestamp_in.is_open()) {
            timestamp_in >> last_build_time;
            timestamp_in.close();
        }

        bool needs_rebuild = (last_build_time == 0);
        if (!needs_rebuild) {
            for (const auto& [path, node] : registry) {
                if (node->last_modified > last_build_time) {
                    needs_rebuild = true;
                    cout << "Change detected: " << path << " (modified: " << node->last_modified << ", last build: " << last_build_time << ")\n";
                    break;
                }
            }
        }

        if (needs_rebuild) {
            cout << "Rebuild required for " << output_exe << "\n";
            build_project(registry, build_dir, output_exe, config_mode, extra_flags, last_build_time);
        } else {
            cout << "No changes detected for " << output_exe << ", skipping build.\n";
        }

        // Print graph
        cout << "Dependency graph:\n";
        set<include_depnode*> seen;
        function<void(include_depnode*, int)> print_graph = 
            [&](include_depnode* node, int depth) {
                if (!node || seen.count(node)) return;
                seen.insert(node);
                string path;
                for (const auto& [p, n] : registry) {
                    if (n == node) { path = p; break; }
                }
                cout << string(depth * 2, ' ') << "- " << path << "\n";
                for (include_depnode* dep : node->dependencies) {
                    print_graph(dep, depth + 1);
                }
            };
        print_graph(root_node, 0);

        include_registry_free(registry);
    } catch (exception &e) {
        cerr << "Error: " << e.what() << endl;
        print_help(args.getString(0));
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    return run_tests() + safe_main(argc, argv);
}