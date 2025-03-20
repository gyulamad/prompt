#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <set>
#include <filesystem>

#include "../utils/ERROR.hpp"

using namespace std;
using namespace tools::utils;

namespace fs = filesystem;

namespace tools::build {

    struct include_depnode {
        vector<include_depnode*> dependencies; // Raw pointers to dependencies
        bool is_built = false;
        time_t last_modified = 0;

        include_depnode() = default; // No file_path, so default constructor is fine
        ~include_depnode() = default; // No cleanup needed
    };

    using include_registry = unordered_map<string, include_depnode*>;

    void include_registry_free(include_registry& registry) {
        for (auto& [path, node] : registry) delete node;
        registry.clear();
    }

    struct include_scancfg {
        string include_regex = R"(^\s*#\s*include\s*\"\s*([^\"]+)\s*\")";
        string base_dir;
    };

    include_depnode* scan_includes(
        const string& filename,
        include_registry& registry,
        const include_scancfg& config = include_scancfg(),
        set<string>* visited = nullptr
    ) {
        set<string> local_visited;
        if (!visited) visited = &local_visited;

        string abs_path;
        try {
            abs_path = fs::absolute(filename).string();
        } catch (const fs::filesystem_error &e) {
            throw ERROR("Cannot resolve absolute path for " + filename + ": " + e.what());
        }

        if (visited->find(abs_path) != visited->end()) return registry[abs_path];
        visited->insert(abs_path);

        if (registry.find(abs_path) == registry.end()) registry[abs_path] = new include_depnode();
        include_depnode *node = registry[abs_path];

        ifstream file(filename);
        if (!file.is_open()) throw ERROR("Could not open file: " + filename);

        string line;
        regex include_pattern(config.include_regex);

        while (getline(file, line)) {
            smatch matches;
            if (regex_search(line, matches, include_pattern) && matches.size() >= 2) {
                string include_name = matches[1].str();
                string full_path;

                try {
                    // Always resolve relative to the current file’s directory
                    fs::path base_path = fs::path(filename).parent_path();
                    // Use config.base_dir only if it’s explicitly set and non-empty
                    if (!config.base_dir.empty() && config.base_dir != fs::current_path().string()) base_path = fs::path(config.base_dir);
                    full_path = fs::canonical(base_path / include_name).string();
                } catch (const fs::filesystem_error &e) {
                    // TODO: we may have to file.close() and throw ERROR here?!
                    cerr << "Warning: Skipping include " << include_name << " - " << e.what() << "\n";
                    continue;
                }

                include_depnode *dep_node = scan_includes(full_path, registry, config, visited);
                if (dep_node) node->dependencies.push_back(dep_node);
            }
        }

        file.close();
        return node;
    }

    void save_dependency_cache(const include_registry& registry, const string& cache_file) {
        ofstream out(cache_file);
        if (!out.is_open()) {
            cerr << "Warning: Could not write cache file: " << cache_file << "\n";
            return;
        }
    
        // Write timestamp
        out << time(nullptr) << "\n";
    
        // Write number of nodes
        out << registry.size() << "\n";
    
        // Map nodes to indices for dependency links
        unordered_map<include_depnode*, size_t> node_to_index;
        size_t index = 0;
        for (const auto& [path, node] : registry) {
            node_to_index[node] = index++;
            out << path << "\n";
        }
    
        // Write dependencies
        for (const auto& [path, node] : registry) {
            out << node_to_index[node] << ":";
            for (include_depnode* dep : node->dependencies) {
                out << " " << node_to_index[dep];
            }
            out << "\n";
        }
    
        out.close();
    }

    bool load_dependency_cache(include_registry& registry, const string& cache_file, time_t& cache_timestamp) {
        ifstream in(cache_file);
        if (!in.is_open()) return false;
    
        // Read timestamp
        in >> cache_timestamp;
        if (in.fail()) return false;
    
        // Read number of nodes
        size_t node_count;
        in >> node_count;
        if (in.fail()) return false;
        in.ignore(numeric_limits<streamsize>::max(), '\n');
    
        // Read paths and create nodes
        vector<include_depnode*> nodes(node_count);
        string line;
        for (size_t i = 0; i < node_count && getline(in, line); ++i) {
            nodes[i] = new include_depnode();
            registry[line] = nodes[i];
        }
    
        // Read dependencies
        for (size_t i = 0; i < node_count && getline(in, line); ++i) {
            size_t colon_pos = line.find(':');
            if (colon_pos == string::npos) return false;
            
            string deps_str = line.substr(colon_pos + 1);
            stringstream ss(deps_str);
            size_t dep_index;
            while (ss >> dep_index) {
                if (dep_index >= node_count) return false;
                nodes[i]->dependencies.push_back(nodes[dep_index]);
            }
        }
    
        in.close();
        return true;
    }

    string generate_build_hash(const string& main_file, const string& config_mode, const string& extra_flags) {
        string combined = main_file + ":" + config_mode + ":" + extra_flags;
        hash<string> hasher;
        size_t hash_val = hasher(combined);
        return to_string(hash_val);
    }

    string get_compile_command(
        const string& source_file, const string& obj_file, 
        const vector<string>& include_dirs, 
        const string& config_flags, const string& extra_flags
    ) {
        string cmd = "g++ -c " + source_file + " -o " + obj_file;
        for (const string& dir : include_dirs) {
            cmd += " -I" + dir;
        }
        cmd += " " + config_flags + " " + extra_flags;
        return cmd;
    }

    string get_link_command(
        const string& output_exe, const vector<string>& object_files, 
        const string& config_flags, const string& extra_flags
    ) {
        string cmd = "g++ -o " + output_exe;
        for (const string& obj : object_files) {
            cmd += " " + obj;
        }
        cmd += " " + config_flags + " " + extra_flags;
        return cmd;
    }

    void build_project(
        include_registry& registry, 
        const string& build_dir, 
        const string& output_exe, 
        const string& config_mode, 
        const string& extra_flags, 
        time_t last_build_time
    ) {
        // Configuration flags
        string config_flags = (config_mode == "debug") ? "-g -DDEBUG" : "-O3 -DNDEBUG";

        // Create object directory
        string obj_dir = build_dir + "/obj";
        fs::create_directories(obj_dir);

        // Collect unique include directories
        set<string> include_dirs;
        for (const auto& [path, node] : registry) {
            if (path.ends_with(".h") || path.ends_with(".hpp")) {
                include_dirs.insert(fs::path(path).parent_path().string());
            }
        }

        // Collect files to compile
        set<string> files_to_compile;
        for (const auto& [path, node] : registry) {
            if (path.ends_with(".cpp") || path.ends_with(".c")) {
                if (last_build_time == 0 || node->last_modified > last_build_time) {
                    files_to_compile.insert(path); // Direct .cpp/.c include
                }
            } else if (path.ends_with(".h") || path.ends_with(".hpp")) {
                string stem = fs::path(path).stem().string();
                string dir = fs::path(path).parent_path().string();
                string cpp_path = dir + "/" + stem + ".cpp";
                string c_path = dir + "/" + stem + ".c";
                
                if (registry.count(cpp_path) && (last_build_time == 0 || registry[cpp_path]->last_modified > last_build_time)) {
                    files_to_compile.insert(cpp_path);
                } else if (registry.count(c_path) && (last_build_time == 0 || registry[c_path]->last_modified > last_build_time)) {
                    files_to_compile.insert(c_path);
                }
            }
        }

        // Compile to object files
        vector<string> object_files;
        for (const string& source_file : files_to_compile) {
            string obj_file = obj_dir + "/" + fs::path(source_file).stem().string() + ".o";
            string cmd = get_compile_command(source_file, obj_file, vector<string>(include_dirs.begin(), include_dirs.end()), 
                                            config_flags, extra_flags);
            cout << "Compiling: " << cmd << "\n";
            if (system(cmd.c_str()) != 0) {
                throw ERROR("Compilation failed for " + source_file);
            }
            object_files.push_back(obj_file);
            registry[source_file]->is_built = true;
        }

        // Link object files
        if (!object_files.empty()) {
            string cmd = get_link_command(output_exe, object_files, config_flags, extra_flags);
            cout << "Linking: " << cmd << "\n";
            if (system(cmd.c_str()) != 0) {
                throw ERROR("Linking failed for " + output_exe);
            }
        } else {
            cout << "No files to compile.\n";
        }

        // Update build timestamp
        string timestamp_file = build_dir + "/.build_timestamp";
        ofstream timestamp_out(timestamp_file);
        if (timestamp_out.is_open()) {
            timestamp_out << time(nullptr);
            timestamp_out.close();
        } else {
            cerr << "Warning: Could not write timestamp file: " << timestamp_file << "\n";
        }
    }

}