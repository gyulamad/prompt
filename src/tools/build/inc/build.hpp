#pragma once

#include <map>
#include <string>
#include <thread>

#include "../../str/get_absolute_path.hpp"
#include "../../str/get_path.hpp"
#include "../../str/replace_extension.hpp"
#include "../../str/remove_path.hpp"
#include "../../str/str_replace.hpp"
#include "../../str/explode.hpp"
#include "../../str/parse.hpp"
#include "../../regx/regx_match.hpp"
#include "../../containers/array_keys.hpp"
#include "../../containers/array_merge.hpp"
#include "../../containers/array_key_exists.hpp"
#include "../../containers/array_unique.hpp"
#include "../../utils/foreach.hpp"
#include "../../utils/files.hpp"
#include "../../utils/execute.hpp"
#include "../../utils/Arguments.hpp"
#include "../../utils/JSON.hpp"
#include "../../utils/Settings.hpp"

using namespace std;
using namespace tools::str;
using namespace tools::regx;
using namespace tools::containers;
using namespace tools::utils;

namespace tools::build {

    struct build_config {
        bool verbose = false;
        bool buildcache = true;
        bool depcache = false;
        // bool make_temp_source = true;
        // string temp_source_extension = ".cpp";
        string build_folder = "";
        string output_extension = "";
        string object_extension = ".o";
        vector<string> source_extensions = {".cpp", ".c"};
        vector<string> flags = {};
        vector<string> libs = {};
        vector<string> include_path = {};
        vector<string> source_path = {};

        string input_file;
        string input_path;
        string output_file;
        string hash;

        build_config(Arguments& args, const string& fsuffix = "compile.json"): 
            verbose(args.get<bool>("verbose"))
        {
            if (args.has("buildcache")) buildcache = parse<bool>(args.get<string>("buildcache"));

            if (verbose) cout << "Setup configuration..." << endl;
            if (verbose && buildcache) cout << "(with buildcache)" << endl;

            if (!args.has(1)) throw ERROR("Input file argument is missing.");
            input_file = get_absolute_path(args.get<string>(1));
            if (!file_exists(input_file)) throw ERROR("Input file not found: " + input_file);
            if (verbose) cout << "Input file found: " << input_file << endl;

            input_path = get_absolute_path(get_path(input_file));
            build_folder = input_path;

            string exename = get_absolute_path(args.get<string>(0));
            apply_cfg(args, exename + "/" + fsuffix);

            apply_cfg(args, input_path + "/" + fsuffix);
            apply_cfg(args, replace_extension(input_file, "." + fsuffix));
            if (args.has("config")) {
                vector<string> cfgfiles = explode(",", args.get<string>("config"));
                for (const string& cfgfile: cfgfiles) {
                    apply_cfg(args, cfgfile);
                    apply_cfg(args, cfgfile + "." + fsuffix);
                    apply_cfg(args, replace_extension(input_file, "." + cfgfile + "." + fsuffix));
                }
            }

            // if (coverage) flags = array_merge(flags, {"--coverage", "-fprofile-arcs", "-ftest-coverage"}); // TODO: make it configurable

            string outfname = replace_extension(remove_path(input_file), output_extension);
            output_file = get_absolute_path(build_folder + "/" + outfname);

            hash = get_hash(hash);
        }

    private:
        void apply_cfg(Arguments& args, const string& cfgfile) {
            if (verbose) cout << "Looking for configuration candidate at file: " + cfgfile + " ..." << flush;
            if (!file_exists(cfgfile)) {
                if (verbose) cout << " [not found]" << endl;
                return;
            }
            if (is_dir(cfgfile)) {
                if (verbose) cout << " [not found] - (it's a directory)" << endl;
                return;
            }
            
            if (verbose) cout << " [found] loading into..." << flush;
            
            JSON cfg(str_replace("{{input-path}}", input_path, file_get_contents(cfgfile)));
            Settings settings(args, cfg);
            verbose = settings.get<bool>("verbose", verbose);

            #define SHOWCFG_BOOL(var, label) if (verbose) cout << "\n\t" << label << ": " << (var ? "ON" : "OFF") << flush;

            #define SHOWCFG_STRING(var, label) if (verbose) cout << "\n\t" << label << ": " << var << flush;

            #define SHOWCFG_VECTOR(var, label) \
            if (verbose) { \
                cout << "\n\t" << label << ":" << flush; \
                for (const string& s: var) cout << "\n\t\t" << s << flush; \
            }

            #define LOADCFG_BOOL_BUT_KEEPS_TRUE(var, name, label) \
                if (settings.get<bool>(name, var)) var = true;

            #define LOADCFG_BOOL(var, name, label) \
                var = settings.get<bool>(name, var);

            #define LOADCFG_STRING(var, name, label) \
                var = settings.get<string>(name, var); \

            #define LOADCFG_STRING_BUT_KEEPS_NONEMPTY(var, name, label) \
                if (settings.has(name) && !settings.get<string>(name, var).empty()) var = settings.get<string>(name, var); \


            #define LOADCFG_STRING_PATH_BUT_KEEPS_NONEMPTY(var, name, label) \
                LOADCFG_STRING_BUT_KEEPS_NONEMPTY(var, name, label) \
                var = str_starts_with(var, "/") ? var : get_absolute_path(input_path + "/" + var);

            #define LOADCFG_VECTOR(var, name, label) \
                var = array_merge(var, settings.get<vector<string>>(name, var)); \
                // SHOWCFG_VECTOR(var, label)

            #define LOADCFG_VECTOR_PATH(var, name, label) \
                LOADCFG_VECTOR(var, name, label) \
                foreach (var, [&](string& path) { path = str_starts_with(path, "/") ? path : get_absolute_path(input_path + "/" + path); });


            #define LOADSHOWCFG_BOOL_BUT_KEEPS_TRUE(var, name, label) \
                LOADCFG_BOOL_BUT_KEEPS_TRUE(var, name, label) \
                SHOWCFG_BOOL(var, label)

            #define LOADSHOWCFG_BOOL(var, name, label) \
                LOADCFG_BOOL(var, name, label) \
                SHOWCFG_BOOL(var, label)


            #define LOADSHOWCFG_STRING_PATH_BUT_KEEPS_NONEMPTY(var, name, label) \
                LOADCFG_STRING_PATH_BUT_KEEPS_NONEMPTY(var, name, label) \
                SHOWCFG_STRING(var, label)

            #define LOADSHOWCFG_STRING(var, name, label) \
                LOADCFG_STRING(var, name, label) \
                SHOWCFG_STRING(var, label)


            #define LOADSHOWCFG_VECTOR(var, name, label) \
                LOADCFG_VECTOR(var, name, label) \
                SHOWCFG_VECTOR(var, label)

            #define LOADSHOWCFG_VECTOR_PATH(var, name, label) \
                LOADCFG_VECTOR_PATH(var, name, label) \
                SHOWCFG_VECTOR(var, label)


            LOADSHOWCFG_BOOL_BUT_KEEPS_TRUE(verbose, "verbose", "Verbose")
            LOADSHOWCFG_BOOL(depcache, "depcache", "Dependency cache")
            // LOADSHOWCFG_BOOL(make_temp_source, "make-temp-source", "Make temp source")
            // LOADSHOWCFG_STRING(temp_source_extension, "temp-source-extension", "Temp source extension")
            LOADSHOWCFG_STRING_PATH_BUT_KEEPS_NONEMPTY(build_folder, "build-folder", "Build folder")
            LOADSHOWCFG_STRING(output_extension, "output-extension", "Output extension")
            LOADSHOWCFG_STRING(object_extension, "object-extension", "Object extension")
            LOADSHOWCFG_VECTOR(source_extensions, "source-extensions", "Source extensions")
            LOADSHOWCFG_VECTOR(flags, "flags", "flags")
            LOADSHOWCFG_VECTOR(libs, "libs", "libs")
            LOADSHOWCFG_VECTOR_PATH(include_path, "include-path", "Include path")
            LOADSHOWCFG_VECTOR_PATH(source_path, "source-path", "Source path")

            hash += settings.hash();

            if (verbose) cout << endl << cfgfile << " [applied]" << endl;
        }
    };

    typedef map<string, map<string, string>> depmap_t;

    // =========================================================================
    // ========================== RECURSION DETECTION ==========================
    // =========================================================================

    // Get dependencies (headers and impls) for a file
    vector<pair<string, string>> get_deps(const depmap_t& depmap, const string& file) {
        vector<pair<string, string>> deps;
        if (array_key_exists(file, depmap)) {
            const auto& inner_map = depmap.at(file);
            foreach(inner_map, [&deps](const string& impl, const string& header) {
                deps.push_back({header, impl});
            });
        }
        return deps;
    }

    // Check for cycle and extend path with a dependency
    bool check_and_extend_path(const string& dep, const vector<string>& path, vector<string>& new_path, vector<string>& cycle) {
        new_path = path;
        if (in_array(dep, new_path)) {
            new_path.push_back(dep);
            cycle = new_path;
            return false;
        }
        new_path.push_back(dep);
        return true;
    }
    
    bool process_single_dep(const pair<string, string>& dep, const vector<string>& path,
                           vector<vector<string>>& new_paths, vector<string>& cycle, bool& extended) {
        const string& header = dep.first;
        const string& impl = dep.second;
    
        vector<string> header_path;
        if (!check_and_extend_path(header, path, header_path, cycle)) {
            new_paths.push_back(header_path);
            return false;
        }
        new_paths.push_back(header_path);
        extended = true;
    
        if (!impl.empty()) {
            vector<string> impl_path;
            if (!check_and_extend_path(impl, path, impl_path, cycle)) {
                new_paths.push_back(impl_path);
                return false;
            }
            new_paths.push_back(impl_path);
            extended = true;
        }
    
        return true;
    }
    
    bool extend_paths_with_deps(const vector<pair<string, string>>& deps, const vector<string>& path, 
                                vector<vector<string>>& new_paths, vector<string>& cycle) {
        bool extended = false;
        bool no_cycle = true;
    
        foreach(deps, [&](const pair<string, string>& dep) -> bool {
            if (!process_single_dep(dep, path, new_paths, cycle, extended)) {
                no_cycle = false;
                return FE_BREAK;
            }
            return FE_CONTINUE;
        });
    
        return extended && no_cycle;
    }

    // Extend all paths with their dependencies
    bool add_dep_paths(const depmap_t& depmap, vector<vector<string>>& paths, vector<string>& cycle) {
        bool extended = false;
        vector<vector<string>> new_paths;

        bool no_cycle = true;
        foreach(paths, [&](const vector<string>& path) -> bool {
            const string& last_file = path.back();
            vector<pair<string, string>> deps = get_deps(depmap, last_file);

            if (deps.empty()) {
                new_paths.push_back(path);
                return FE_CONTINUE;
            }

            if (extend_paths_with_deps(deps, path, new_paths, cycle)) {
                extended = true;
            } else {
                no_cycle = false;
                return FE_BREAK; // Stop on cycle
            }
            return FE_CONTINUE;
        });

        paths = move(new_paths);
        return extended && no_cycle; // True if extended and no cycle
    }

    // Main function with optional throw on cycle detection
    vector<string> find_circulation(const depmap_t& depmap, bool throw_on_cycle = false) {
        if (depmap.empty()) return {};
    
        vector<vector<string>> paths;
        vector<string> keys = array_keys(depmap);
        foreach(keys, [&paths](const string& file) {
            paths.push_back({file});
        });
    
        vector<string> cycle;
        while (add_dep_paths(depmap, paths, cycle)) {
            // Continue until cycle found or no more extensions
        }
    
        if (!cycle.empty() && throw_on_cycle) {
            throw ERROR("circular dependency found:\n" + implode("\n=>", cycle));
        }
    
        return cycle;
    }

    // =========================================================================
    // =========================================================================
    // =========================================================================

    string find_impfile(const string& incfile, const vector<string>& srcdirs, const string& impext) {
        string base = remove_path(incfile); // e.g., "a.h"
        string impfile = replace_extension(base, impext); // e.g., "a.cpp"
        string incpath = get_path(incfile); // e.g., "include/"
        vector<string> tries;
    
        // First, try the directory of the header (next to incfile)
        string local_candidate = get_absolute_path(incpath + "/" + impfile); // e.g., "include/a.cpp"
        tries.push_back(local_candidate);
        if (file_exists(local_candidate)) return local_candidate; // Found next to header
    
        // Then try srcdirs
        for (const string& srcdir : srcdirs) {
            string candidate = get_absolute_path(srcdir + "/" + impfile); // e.g., "src/a.cpp"
            tries.push_back(candidate);
            if (file_exists(candidate)) return candidate;
        }

        // Not found
        return "";
    }

    string find_impfile(const string& incfile, const vector<string>& srcdirs, const vector<string>& impexts) {
        string impfile = "";
        foreach (impexts, [&](const string& impext) {
            impfile = find_impfile(incfile, srcdirs, impext);
            if (!impfile.empty()) return FE_BREAK;
            return FE_CONTINUE;
        });
        return impfile;
    }
    
    // float roll = 0, spd = 0.01;
    void scan_includes(
        mutex& mtx,
        const string& srcfile, 
        depmap_t& depmap, 
        const vector<string>& idirs, 
        const vector<string>& sdirs, 
        // ms_t& lstmtime, 
        // map<string, string>& impmap, 
        const vector<string>& impexts,
        vector<string>& visited,
        bool verbose
    ) {
        // Check if already visited
        {
            lock_guard<mutex> lock(mtx);
            if (in_array(srcfile, visited)) return;
            visited.emplace_back(srcfile);
        }
    
        // if (verbose) cout << "scan: " + srcfile << endl;
        // cout << "\r\033[K[ ] scan: " << srcfile << flush;
        // ms_t _lstmtime = filemtime_ms(srcfile);
        // if (_lstmtime > lstmtime) lstmtime = _lstmtime;
    
        string srccode = file_get_contents(srcfile);
        vector<string> srclines = explode("\n", srccode);
        vector<string> matches;
        string srcpath = get_path(srcfile);
        vector<string> lookup;
        foreach(srclines, [&](const string& srcline, size_t line) {
            // cout << "\r[" << "/-\\|"[(long)(roll += spd)%4] << "\r" << flush;
            if (srcline.empty() || !regx_match(R"(^\s*#)", srcline) || !regx_match(R"(^\s*#\s*include\s*\"\s*([^\"]+)\s*\")", srcline, &matches)) return;
            string incfile = str_starts_with(matches[1], "/") 
                ? matches[1] 
                : get_absolute_path(srcpath + "/" + matches[1]);
            vector<string> tries = { incfile };
            if (!file_exists(incfile)) foreach (idirs, [&](const string& idir) {
                incfile = get_absolute_path(idir + "/" + remove_path(incfile));
                tries.push_back(incfile);
                if (file_exists(incfile)) return FE_BREAK;
                return FE_CONTINUE;
            });
            if (!file_exists(incfile))
                throw ERROR("Dependency include file not found, following are tried:\n\t" + implode("\n\t", tries) + "\nIncluded in " + ANSI_FMT_FILE_LINE(srcfile, line + 1));
            //cout << "found: " << incfile << " (in " << srcfile << ")" << endl;
            string impfile = find_impfile(incfile, sdirs, impexts);
            {
                lock_guard<mutex> lock(mtx);
                depmap[srcfile][incfile] = file_exists(impfile) ? impfile : "";
                if (!array_key_exists(incfile, depmap)) {
                    depmap[incfile] = {};
                    lookup.push_back(incfile);
                }
            }
        });
        // cout << "\r[✔]" << flush;
        vector<string> errors;
        vector<thread> threads;
        foreach(lookup, [&](const string& incfile) {
            threads.emplace_back([&]() {
                try {
                    scan_includes(mtx, incfile, depmap, idirs,/* lstmtime,*/ sdirs, impexts, visited, verbose);
                } catch (exception &e) {
                    lock_guard<mutex> lock(mtx);
                    errors.push_back(e.what());
                }
            });
        });
        for (thread& t: threads) { if (t.joinable()) t.join(); }
        if (!errors.empty()) throw ERROR("Include error(s) detected:\n" + implode("\n", errors));
    }
    void scan_includes(
        mutex& mtx,
        const string& incfile, 
        depmap_t& depmap, 
        const vector<string>& idirs, 
        const vector<string>& sdirs, 
        const vector<string>& impexts,
        bool verbose
    ) {
        vector<string> visited;
        scan_includes(mtx, incfile, depmap, idirs, sdirs, impexts, visited, verbose);
    }
    
    void save_depcache(const string& filename, const depmap_t& depmap) {
        ostringstream oss;
        foreach (depmap, [&](const map<string, string>& deps, const string& srcf) {
            oss << srcf << "\n";
            foreach(deps, [&](const string& depimpf, const string& depincf) {
                oss << "- " << depincf << ":" << depimpf << "\n";
            });
        });
        file_put_contents(filename, oss.str(), false, true);
    }
    
    depmap_t load_depcache(const string& filename) {
        depmap_t depmap;
        string content = file_get_contents(filename);
        vector<string> lines = explode("\n", content);
    
        string current_srcf;
        int lno = 0;
        foreach(lines, [&](const string& line) {
            ++lno;
            if (line.empty()) return; // Skip empty lines
            if (line.substr(0, 2) == "- ") { // Dependency line
                if (current_srcf.empty()) return; // No source file yet, skip
                string dep = line.substr(2); // Remove "- "
                vector<string> splits = explode(":", dep);
                if (splits.size() != 2) throw ERROR("Invalid line in dependency cache: " + filename + ":" + to_string(lno));
                depmap[current_srcf][splits[0]] = splits[1];
            } else { // Source file line
                current_srcf = line; // Set new source file
                depmap[current_srcf]; // Initialize empty vector if not already present
            }
        });
    
        return depmap;
    }
    
    vector<string> get_all_files(const depmap_t& depmap) {
        vector<string> files;
        foreach (depmap, [&](const map<string, string>& deps, const string& file) {
            if (!in_array(file, files)) files.push_back(file);
            foreach (deps, [&](const string& impf, const string& incf) {
                if (!in_array(incf, files)) files.push_back(incf);
                if (!impf.empty() && !in_array(impf, files)) files.push_back(impf);
            });
        });
        return files;
    }
    
    ms_t get_lstmtime(const vector<string> filenames) {
        ms_t lstmtime = 0;
        foreach (filenames, [&](const string& filename) {
            ms_t _lstmtime = filemtime_ms(filename);
            if (_lstmtime > lstmtime) lstmtime = _lstmtime;
        });
        return lstmtime;
    }
    
    ms_t get_lstmtime(const depmap_t& depmap) {
        return get_lstmtime(get_all_files(depmap));
    }
    
    [[nodiscard]]
    pair<string, string> compile_objfile(const vector<string>& flags, const string& srcfile, const string& outfile, const vector<string>& idirs, bool verbose) {
        string outpath = get_path(outfile);
        if (!mkdir(outpath, true)) throw ERROR("Unable to create folder: " + outpath);
        string cmd = "g++" 
            + (!flags.empty() ? " " + implode(" ", flags) : "") 
            + " -c " + srcfile 
            + " -o " + outfile 
            + (!idirs.empty() ? " -I" + implode(" -I", idirs) : "");
        if (verbose) cout << cmd << endl;
        string output = execute(cmd + " 2>&1");
        //if (!output.empty()) throw ERROR("Compile error at " + srcfile + "\n" + cmd + "\n" + output);
        return pair(cmd, output);
    } 
    
    [[nodiscard]]
    pair<string, string> link_outfile(const vector<string>& flags, const string& outfile, const vector<string>& objfiles, const vector<string>& libs, bool verbose) {
        string outpath = get_path(outfile);
        if (!mkdir(outpath, true)) throw ERROR("Unable to create folder: " + outpath);
        string cmd = "g++" 
            + (!flags.empty() ? " " + implode(" ", flags) : "") 
            + (!objfiles.empty() ? " " + implode(" ", objfiles) : "")
            + " -o " + outfile
            + (!libs.empty() ? " " + implode(" ", libs) : "");
        if (verbose) cout << cmd << endl;
        string output = execute(cmd + " 2>&1");
        //if (!output.empty()) throw ERROR("Link error: " + cmd + "\n" + output);
        return pair(cmd, output);
    }
    
    
    vector<string> get_alldeps(const string& filename, const depmap_t& depmap, vector<string>& visited) {
        vector<string> deps = { filename };
    
        if (in_array(filename, visited)) return deps;
        visited.push_back(filename);
    
        foreach (depmap, [&](const map<string, string>& depfiles, const string& fname) {
            if (fname == filename) {
                foreach (depfiles, [&](const string& impf, const string& depf) {
                    if (!depf.empty()) {
                        deps.push_back(depf);
                        deps = array_unique(array_merge(deps, get_alldeps(depf, depmap, visited)));
                    }
                    if (!impf.empty()) {
                        deps.push_back(impf);
                        deps = array_unique(array_merge(deps, get_alldeps(impf, depmap, visited)));
                    }
                });
                return FE_BREAK;
            }
            return FE_CONTINUE;
        });
    
        return deps;
    }
    vector<string> get_alldeps(const string& filename, const depmap_t& depmap) {
        vector<string> visited; // Track visited files to handle circular dependencies
        return get_alldeps(filename, depmap, visited);
    }
    
    ms_t get_alldepfmtime(const string& filename, const depmap_t& depmap) {
        ms_t fmtime = filemtime_ms(filename);
        vector<string> alldeps = get_alldeps(filename, depmap);
        foreach (alldeps, [&](const string& depf) {
            ms_t depfmtime = filemtime_ms(depf);
            if (depfmtime > fmtime) fmtime = depfmtime;
        });
        return fmtime;
    }
    
}

#ifdef TEST

#include "../../str/str_contains.hpp"
#include "../../utils/Test.hpp"
#include "../../containers/sort.hpp"
#include "../tests/test_fs.hpp"

using namespace tools::build;
using namespace tools::containers;
using namespace test_fs;

// Test: File found next to header
void test_build_find_impfile_found_next_to_header() {
    test_fs::cleanup("/tmp/find_impfile_test/"); // Ensure clean slate
    test_fs::setup_dirs({"/tmp/find_impfile_test/include"});
    test_fs::create_file("/tmp/find_impfile_test/include/a.cpp");

    string incfile = "/tmp/find_impfile_test/include/a.h";
    vector<string> srcdirs = {"/tmp/find_impfile_test/src"};
    string impext = ".cpp";
    string result = find_impfile(incfile, srcdirs, impext);

    test_fs::cleanup("/tmp/find_impfile_test/");

    assert(result == "/tmp/find_impfile_test/include/a.cpp" && "Should find file next to header");
}

// Test: File found in source directory
void test_build_find_impfile_found_in_srcdir() {
    test_fs::cleanup("/tmp/find_impfile_test/");
    test_fs::setup_dirs({"/tmp/find_impfile_test/include", "/tmp/find_impfile_test/src"});
    test_fs::create_file("/tmp/find_impfile_test/src/a.cpp"); // Only in src, not include

    // Debug: Check file existence before running
    bool include_exists = file_exists("/tmp/find_impfile_test/include/a.cpp");
    bool src_exists = file_exists("/tmp/find_impfile_test/src/a.cpp");

    string incfile = "/tmp/find_impfile_test/include/a.h";
    vector<string> srcdirs = {"/tmp/find_impfile_test/src"};
    string impext = ".cpp";
    string result = find_impfile(incfile, srcdirs, impext);

    test_fs::cleanup("/tmp/find_impfile_test/");

    assert(!include_exists && "include/a.cpp should not exist");
    assert(src_exists && "src/a.cpp should exist");
    assert(result == "/tmp/find_impfile_test/src/a.cpp" && "Should find file in source directory");
}

// Test: File not found
void test_build_find_impfile_not_found() {
    test_fs::cleanup("/tmp/find_impfile_test/");
    test_fs::setup_dirs({"/tmp/find_impfile_test/include", "/tmp/find_impfile_test/src"});
    // No a.cpp created anywhere

    string incfile = "/tmp/find_impfile_test/include/a.h";
    vector<string> srcdirs = {"/tmp/find_impfile_test/src"};
    string impext = ".cpp";
    string result = find_impfile(incfile, srcdirs, impext);

    test_fs::cleanup("/tmp/find_impfile_test/");

    assert(result.empty() && "Should return empty string when file not found");
}

// Test: Multiple source directories, prioritizes first match
void test_build_find_impfile_multiple_srcdirs() {
    test_fs::cleanup("/tmp/find_impfile_test/");
    test_fs::setup_dirs({"/tmp/find_impfile_test/include", "/tmp/find_impfile_test/src1", "/tmp/find_impfile_test/src2"});
    test_fs::create_file("/tmp/find_impfile_test/src1/a.cpp");
    test_fs::create_file("/tmp/find_impfile_test/src2/a.cpp");

    string incfile = "/tmp/find_impfile_test/include/a.h";
    vector<string> srcdirs = {"/tmp/find_impfile_test/src1", "/tmp/find_impfile_test/src2"};
    string impext = ".cpp";
    string result = find_impfile(incfile, srcdirs, impext);

    test_fs::cleanup("/tmp/find_impfile_test/");

    assert(result == "/tmp/find_impfile_test/src1/a.cpp" && "Should find file in first matching source directory");
}

// Test: Basic include detection
void test_build_scan_includes_basic() {
    test_fs::cleanup("/tmp/scan_includes_test/");
    test_fs::setup_dirs({"/tmp/scan_includes_test/src", "/tmp/scan_includes_test/include"});
    test_fs::create_file("/tmp/scan_includes_test/src/main.cpp", "#include \"a.h\"\n");
    test_fs::create_file("/tmp/scan_includes_test/include/a.h");

    mutex mtx;
    depmap_t depmap;
    vector<string> idirs = {"/tmp/scan_includes_test/include"};
    vector<string> sdirs = {"/tmp/scan_includes_test/src"};
    vector<string> impexts = {".cpp"};
    vector<string> visited;
    scan_includes(mtx, "/tmp/scan_includes_test/src/main.cpp", depmap, idirs, sdirs, impexts, visited, false);

    test_fs::cleanup("/tmp/scan_includes_test/");

    assert(depmap.size() == 2 && "Should have main.cpp and a.h in depmap");
    assert(depmap["/tmp/scan_includes_test/src/main.cpp"].size() == 1 && "main.cpp should include a.h");
    assert(depmap["/tmp/scan_includes_test/src/main.cpp"]["/tmp/scan_includes_test/include/a.h"] == "" && "a.h has no .cpp");
}

// Test: Include with implementation
void test_build_scan_includes_with_implementation() {
    test_fs::cleanup("/tmp/scan_includes_test/");
    test_fs::setup_dirs({"/tmp/scan_includes_test/src", "/tmp/scan_includes_test/include"});
    test_fs::create_file("/tmp/scan_includes_test/src/main.cpp", "#include \"a.h\"\n");
    test_fs::create_file("/tmp/scan_includes_test/include/a.h");
    test_fs::create_file("/tmp/scan_includes_test/src/a.cpp");

    mutex mtx;
    depmap_t depmap;
    vector<string> idirs = {"/tmp/scan_includes_test/include"};
    vector<string> sdirs = {"/tmp/scan_includes_test/src"};
    vector<string> impexts = {".cpp"};
    vector<string> visited;
    scan_includes(mtx, "/tmp/scan_includes_test/src/main.cpp", depmap, idirs, sdirs, impexts, visited, false);

    test_fs::cleanup("/tmp/scan_includes_test/");

    assert(depmap.size() == 2 && "Should have main.cpp and a.h in depmap");
    assert(
        depmap["/tmp/scan_includes_test/src/main.cpp"]["/tmp/scan_includes_test/include/a.h"] == "/tmp/scan_includes_test/src/a.cpp" 
        && "a.h should link to a.cpp");
}

// Test: Nested includes
void test_build_scan_includes_nested() {
    test_fs::cleanup("/tmp/scan_includes_test/");
    test_fs::setup_dirs({"/tmp/scan_includes_test/src", "/tmp/scan_includes_test/include"});
    test_fs::create_file("/tmp/scan_includes_test/src/main.cpp", "#include \"a.h\"\n");
    test_fs::create_file("/tmp/scan_includes_test/include/a.h", "#include \"b.h\"\n");
    test_fs::create_file("/tmp/scan_includes_test/include/b.h");

    mutex mtx;
    depmap_t depmap;
    vector<string> idirs = {"/tmp/scan_includes_test/include"};
    vector<string> sdirs = {"/tmp/scan_includes_test/src"};
    vector<string> impexts = {".cpp"};
    vector<string> visited;
    scan_includes(mtx, "/tmp/scan_includes_test/src/main.cpp", depmap, idirs, sdirs, impexts, visited, false);

    test_fs::cleanup("/tmp/scan_includes_test/");

    assert(depmap.size() == 3 && "Should have main.cpp, a.h, and b.h in depmap");
    assert(depmap["/tmp/scan_includes_test/src/main.cpp"]["/tmp/scan_includes_test/include/a.h"] == "" && "main.cpp includes a.h");
    assert(depmap["/tmp/scan_includes_test/include/a.h"]["/tmp/scan_includes_test/include/b.h"] == "" && "a.h includes b.h");
}

// Test: Missing include file (exception)
void test_build_scan_includes_missing_include() {
    test_fs::cleanup("/tmp/scan_includes_test/");
    test_fs::setup_dirs({"/tmp/scan_includes_test/src", "/tmp/scan_includes_test/include"});
    test_fs::create_file("/tmp/scan_includes_test/src/main.cpp", "#include \"a.h\"\n"); // a.h doesn’t exist

    mutex mtx;
    depmap_t depmap;
    vector<string> idirs = {"/tmp/scan_includes_test/include"};
    vector<string> sdirs = {"/tmp/scan_includes_test/src"};
    vector<string> impexts = {".cpp"};
    vector<string> visited;

    bool thrown = false;
    try {
        scan_includes(mtx, "/tmp/scan_includes_test/src/main.cpp", depmap, idirs, sdirs, impexts, visited, false);
    } catch (exception &e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Dependency include file not found") && "Exception should report missing include");
    }

    test_fs::cleanup("/tmp/scan_includes_test/");

    assert(thrown && "Should throw when include file is missing");
}

// Test: Circular includes
void test_build_scan_includes_circular() {
    test_fs::cleanup("/tmp/scan_includes_test/");
    test_fs::setup_dirs({"/tmp/scan_includes_test/include"});
    test_fs::create_file("/tmp/scan_includes_test/include/a.h", "#include \"b.h\"\n");
    test_fs::create_file("/tmp/scan_includes_test/include/b.h", "#include \"a.h\"\n");

    mutex mtx;
    depmap_t depmap;
    vector<string> idirs = {"/tmp/scan_includes_test/include"};
    vector<string> sdirs = {"/tmp/scan_includes_test/src"};
    vector<string> impexts = {".cpp"};
    vector<string> visited;
    scan_includes(mtx, "/tmp/scan_includes_test/include/a.h", depmap, idirs, sdirs, impexts, visited, false);

    test_fs::cleanup("/tmp/scan_includes_test/");

    assert(depmap.size() == 2 && "Should have a.h and b.h in depmap");
    assert(depmap["/tmp/scan_includes_test/include/a.h"]["/tmp/scan_includes_test/include/b.h"] == "" && "a.h includes b.h");
    assert(depmap["/tmp/scan_includes_test/include/b.h"]["/tmp/scan_includes_test/include/a.h"] == "" && "b.h includes a.h");
    assert(visited.size() == 2 && "Should visit each file once despite circularity");
}

// Test: Empty depmap
void test_build_save_depcache_empty() {
    test_fs::cleanup("/tmp/save_depcache_test/");
    test_fs::setup_dirs({"/tmp/save_depcache_test"});
    string filename = "/tmp/save_depcache_test/cache.txt";
    depmap_t depmap;

    save_depcache(filename, depmap);
    string content = file_get_contents(filename);

    test_fs::cleanup("/tmp/save_depcache_test/");

    assert(content.empty() && "Empty depmap should write an empty file");
}

// Test: Single file, no dependencies
void test_build_save_depcache_single_no_deps() {
    test_fs::cleanup("/tmp/save_depcache_test/");
    test_fs::setup_dirs({"/tmp/save_depcache_test"});
    string filename = "/tmp/save_depcache_test/cache.txt";
    depmap_t depmap = { {"/src/main.cpp", {}} };

    save_depcache(filename, depmap);
    string content = file_get_contents(filename);

    test_fs::cleanup("/tmp/save_depcache_test/");

    string expected = "/src/main.cpp\n";
    assert(content == expected && "Single file with no deps should write just the filename");
}

// Test: Single file with dependencies
void test_build_save_depcache_single_with_deps() {
    test_fs::cleanup("/tmp/save_depcache_test/");
    test_fs::setup_dirs({"/tmp/save_depcache_test"});
    string filename = "/tmp/save_depcache_test/cache.txt";
    depmap_t depmap = { 
        {"/src/main.cpp", {{"/include/a.h", "/src/a.cpp"}}}
    };

    save_depcache(filename, depmap);
    string content = file_get_contents(filename);

    test_fs::cleanup("/tmp/save_depcache_test/");

    string expected = "/src/main.cpp\n- /include/a.h:/src/a.cpp\n";
    assert(content == expected && "Single file with deps should write filename and dependency");
}

// Test: Multiple files with dependencies
void test_build_save_depcache_multiple_files() {
    test_fs::cleanup("/tmp/save_depcache_test/");
    test_fs::setup_dirs({"/tmp/save_depcache_test"});
    string filename = "/tmp/save_depcache_test/cache.txt";
    depmap_t depmap = { 
        {"/src/main.cpp", {{"/include/a.h", "/src/a.cpp"}}},
        {"/include/a.h", {{"/include/b.h", ""}}}
    };

    save_depcache(filename, depmap);
    string content = file_get_contents(filename);

    test_fs::cleanup("/tmp/save_depcache_test/");

    string expected = 
        "/include/a.h\n"
        "- /include/b.h:\n"
        "/src/main.cpp\n"
        "- /include/a.h:/src/a.cpp\n";
    assert(content == expected && "Multiple files with deps should write all entries correctly in sorted order");
}
    
// Test: Empty file
void test_build_load_depcache_empty_file() {
    test_fs::cleanup("/tmp/load_depcache_test/");
    test_fs::setup_dirs({"/tmp/load_depcache_test"});
    test_fs::create_file("/tmp/load_depcache_test/cache.txt", "");

    depmap_t depmap = load_depcache("/tmp/load_depcache_test/cache.txt");

    test_fs::cleanup("/tmp/load_depcache_test/");

    assert(depmap.empty() && "Empty file should result in empty depmap");
}

// Test: Single file, no dependencies
void test_build_load_depcache_single_file_no_deps() {
    test_fs::cleanup("/tmp/load_depcache_test/");
    test_fs::setup_dirs({"/tmp/load_depcache_test"});
    test_fs::create_file("/tmp/load_depcache_test/cache.txt", "/src/main.cpp\n");

    depmap_t depmap = load_depcache("/tmp/load_depcache_test/cache.txt");

    test_fs::cleanup("/tmp/load_depcache_test/");

    assert(depmap.size() == 1 && "Should have one entry");
    assert(depmap["/src/main.cpp"].empty() && "main.cpp should have no dependencies");
}

// Test: Single file with header
void test_build_load_depcache_single_file_with_header() {
    test_fs::cleanup("/tmp/load_depcache_test/");
    test_fs::setup_dirs({"/tmp/load_depcache_test"});
    test_fs::create_file("/tmp/load_depcache_test/cache.txt", 
        "/src/main.cpp\n"
        "- /include/a.h:\n"
    );

    depmap_t depmap = load_depcache("/tmp/load_depcache_test/cache.txt");

    test_fs::cleanup("/tmp/load_depcache_test/");

    assert(depmap.size() == 1 && "Should have one entry");
    assert(depmap["/src/main.cpp"].size() == 1 && "main.cpp should have one dependency");
    assert(depmap["/src/main.cpp"]["/include/a.h"] == "" && "a.h should have no implementation");
}

// Test: Single file with header and implementation
void test_build_load_depcache_single_file_with_impl() {
    test_fs::cleanup("/tmp/load_depcache_test/");
    test_fs::setup_dirs({"/tmp/load_depcache_test"});
    test_fs::create_file("/tmp/load_depcache_test/cache.txt", 
        "/src/main.cpp\n"
        "- /include/a.h:/src/a.cpp\n"
    );

    depmap_t depmap = load_depcache("/tmp/load_depcache_test/cache.txt");

    test_fs::cleanup("/tmp/load_depcache_test/");

    assert(depmap.size() == 1 && "Should have one entry");
    assert(depmap["/src/main.cpp"].size() == 1 && "main.cpp should have one dependency");
    assert(depmap["/src/main.cpp"]["/include/a.h"] == "/src/a.cpp" && "a.h should link to a.cpp");
}

// Test: Multiple files with dependencies
void test_build_load_depcache_multiple_files() {
    test_fs::cleanup("/tmp/load_depcache_test/");
    test_fs::setup_dirs({"/tmp/load_depcache_test"});
    test_fs::create_file("/tmp/load_depcache_test/cache.txt", 
        "/src/main.cpp\n"
        "- /include/a.h:/src/a.cpp\n"
        "/include/a.h\n"
        "- /include/b.h:\n"
    );

    depmap_t depmap = load_depcache("/tmp/load_depcache_test/cache.txt");

    test_fs::cleanup("/tmp/load_depcache_test/");

    assert(depmap.size() == 2 && "Should have two entries");
    assert(depmap["/src/main.cpp"].size() == 1 && "main.cpp should have one dependency");
    assert(depmap["/src/main.cpp"]["/include/a.h"] == "/src/a.cpp" && "main.cpp links a.h to a.cpp");
    assert(depmap["/include/a.h"].size() == 1 && "a.h should have one dependency");
    assert(depmap["/include/a.h"]["/include/b.h"] == "" && "a.h links to b.h with no impl");
}

// Test: Invalid format (exception)
void test_build_load_depcache_invalid_format() {
    test_fs::cleanup("/tmp/load_depcache_test/");
    test_fs::setup_dirs({"/tmp/load_depcache_test"});
    test_fs::create_file("/tmp/load_depcache_test/cache.txt", 
        "/src/main.cpp\n"
        "- invalid_line\n" // Missing colon
    );

    bool thrown = false;
    try {
        depmap_t depmap = load_depcache("/tmp/load_depcache_test/cache.txt");
    } catch (exception &e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Invalid line in dependency cache") && "Exception should report invalid format");
    }

    test_fs::cleanup("/tmp/load_depcache_test/");

    assert(thrown && "Should throw for invalid format");
}

// Test: Empty depmap
void test_build_get_all_files_empty() {
    depmap_t depmap;
    vector<string> result = get_all_files(depmap);

    assert(result.empty() && "Empty depmap should return empty vector");
}

// Test: Single file, no dependencies
void test_build_get_all_files_single_no_deps() {
    depmap_t depmap = { {"/src/main.cpp", {}} };
    vector<string> result = get_all_files(depmap);

    vector<string> expected = {"/src/main.cpp"};
    assert(result.size() == expected.size() && "Should return one file");
    assert(result == expected && "Should contain only the source file");
}

// Test: Single file with header
void test_build_get_all_files_single_with_header() {
    depmap_t depmap = { {"/src/main.cpp", {{"/include/a.h", ""}}} };
    vector<string> result = get_all_files(depmap);

    vector<string> expected = {"/src/main.cpp", "/include/a.h"};
    assert(result.size() == expected.size() && "Should return two files");
    assert(result == expected && "Should contain source and header");
}

// Test: Single file with header and implementation
void test_build_get_all_files_single_with_impl() {
    depmap_t depmap = { {"/src/main.cpp", {{"/include/a.h", "/src/a.cpp"}}} };
    vector<string> result = get_all_files(depmap);

    vector<string> expected = {"/src/main.cpp", "/include/a.h", "/src/a.cpp"};
    assert(result.size() == expected.size() && "Should return three files");
    assert(result == expected && "Should contain source, header, and implementation");
}

// Test: Multiple files with dependencies
void test_build_get_all_files_multiple_files() {
    depmap_t depmap = {
        {"/src/main.cpp", {{"/include/a.h", "/src/a.cpp"}}},
        {"/include/a.h", {{"/include/b.h", ""}}},
        {"/src/another.cpp", {{"/include/a.h", "/src/a.cpp"}}}
    };
    vector<string> result = get_all_files(depmap);

    vector<string> expected = {"/src/main.cpp", "/include/a.h", "/src/a.cpp", "/include/b.h", "/src/another.cpp"};
    assert(result.size() == expected.size() && "Should return five unique files");

    // Sort both for order-independent comparison
    sort(result.begin(), result.end());
    sort(expected.begin(), expected.end());
    assert(result == expected && "Should contain all unique files regardless of order");
}

// Test: Empty vector
void test_build_get_lstmtime_empty_vector() {
    vector<string> filenames;
    ms_t result = get_lstmtime(filenames);

    assert(result == 0 && "Empty vector should return 0");
}

// Test: Single file (vector)
void test_build_get_lstmtime_single_file_vector() {
    const string base_dir = "/tmp/get_lstmtime_test";
    test_fs::cleanup(base_dir);
    test_fs::setup_dirs({base_dir});
    test_fs::create_file(base_dir + "/main.cpp");
    ms_t set_time = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 1000;
    test_fs::set_file_time(base_dir + "/main.cpp", set_time);

    vector<string> filenames = {base_dir + "/main.cpp"};
    ms_t result = get_lstmtime(filenames);

    test_fs::cleanup(base_dir);

    ms_t diff = abs(result - set_time);
    assert(diff < 100 && "Should return the single file's mtime within tolerance");
}

// Test: Multiple files (vector)
void test_build_get_lstmtime_multiple_files_vector() {
    const string base_dir = "/tmp/get_lstmtime_test";
    test_fs::cleanup(base_dir);
    test_fs::setup_dirs({base_dir});
    test_fs::create_file(base_dir + "/main.cpp");
    test_fs::create_file(base_dir + "/a.cpp");

    ms_t t1 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 2000;
    ms_t t2 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 1000;
    test_fs::set_file_time(base_dir + "/main.cpp", t1);
    test_fs::set_file_time(base_dir + "/a.cpp", t2);

    vector<string> filenames = {base_dir + "/main.cpp", base_dir + "/a.cpp"};
    ms_t result = get_lstmtime(filenames);

    test_fs::cleanup(base_dir);

    ms_t diff = abs(result - t2);
    assert(diff < 100 && "Should return the latest mtime within tolerance");
    assert(result > t1 && "Result should be greater than the older time");
}

// Test: Missing file (vector)
void test_build_get_lstmtime_missing_file_vector() {
    const string base_dir = "/tmp/get_lstmtime_test";
    test_fs::cleanup(base_dir);
    test_fs::setup_dirs({base_dir});
    test_fs::create_file(base_dir + "/main.cpp");
    ms_t set_time = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 1000;
    test_fs::set_file_time(base_dir + "/main.cpp", set_time);

    vector<string> filenames = {base_dir + "/main.cpp", "/nonexistent/a.cpp"};
    bool thrown = false;
    try {
        get_lstmtime(filenames);
    } catch (...) {
        thrown = true;
    }

    test_fs::cleanup(base_dir);

    assert(thrown && "Should throw when a file is missing");
}

// Test: Empty depmap
void test_build_get_lstmtime_empty_depmap() {
    depmap_t depmap;
    ms_t result = get_lstmtime(depmap);

    assert(result == 0 && "Empty depmap should return 0");
}

// Test: Multiple files (depmap) - All files exist
void test_build_get_lstmtime_multiple_files_depmap() {
    const string base_dir = "/tmp/get_lstmtime_test";
    test_fs::cleanup(base_dir);
    test_fs::setup_dirs({base_dir + "/src", base_dir + "/include"});
    test_fs::create_file(base_dir + "/src/main.cpp");
    test_fs::create_file(base_dir + "/include/a.h");
    test_fs::create_file(base_dir + "/src/a.cpp");
    test_fs::create_file(base_dir + "/src/another.cpp"); // Ensure all source files exist

    ms_t t1 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 2000;
    ms_t t2 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 1000;
    ms_t t3 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 500;
    ms_t t4 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 1500;
    test_fs::set_file_time(base_dir + "/src/main.cpp", t1);
    test_fs::set_file_time(base_dir + "/include/a.h", t2);
    test_fs::set_file_time(base_dir + "/src/a.cpp", t3);
    test_fs::set_file_time(base_dir + "/src/another.cpp", t4);

    depmap_t depmap = {
        {base_dir + "/src/main.cpp", {{base_dir + "/include/a.h", base_dir + "/src/a.cpp"}}},
        {base_dir + "/src/another.cpp", {{base_dir + "/include/a.h", ""}}} // All files exist
    };
    ms_t result = get_lstmtime(depmap);

    test_fs::cleanup(base_dir);

    ms_t diff = abs(result - t3);
    assert(diff < 100 && "Should return the latest mtime from depmap files");
    assert(result > t1 && result > t2 && result > t4 && "Result should be the most recent");
}

// Test: Multiple files (depmap) - Missing file throws
void test_build_get_lstmtime_missing_file_depmap() {
    const string base_dir = "/tmp/get_lstmtime_test";
    test_fs::cleanup(base_dir);
    test_fs::setup_dirs({base_dir + "/src", base_dir + "/include"});
    test_fs::create_file(base_dir + "/src/main.cpp");
    test_fs::create_file(base_dir + "/include/a.h");
    test_fs::create_file(base_dir + "/src/a.cpp");

    ms_t t1 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 2000;
    ms_t t2 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 1000;
    ms_t t3 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 500;
    test_fs::set_file_time(base_dir + "/src/main.cpp", t1);
    test_fs::set_file_time(base_dir + "/include/a.h", t2);
    test_fs::set_file_time(base_dir + "/src/a.cpp", t3);

    depmap_t depmap = {
        {base_dir + "/src/main.cpp", {{base_dir + "/include/a.h", base_dir + "/src/a.cpp"}}},
        {base_dir + "/src/another.cpp", {{"/nonexistent/b.h", ""}}} // Missing file
    };

    bool thrown = false;
    try {
        get_lstmtime(depmap);
    } catch (const exception& e) {
        thrown = true;
        string err = e.what();
        assert(err.find("File does not exist") != string::npos && "Exception should indicate missing file");
    }

    test_fs::cleanup(base_dir);

    assert(thrown && "Should throw when a file is missing in depmap");
}

// Test: Successful compilation
void test_build_compile_objfile_success() {
    const string base_dir = "/tmp/compile_objfile_test";
    test_fs::cleanup(base_dir);
    test_fs::setup_dirs({base_dir + "/src", base_dir + "/obj"});
    string srcfile = base_dir + "/src/main.cpp";
    string outfile = base_dir + "/obj/main.o";
    test_fs::create_file(srcfile, "int main() { return 0; }"); // Valid C++ code

    pair<string, string> p = compile_objfile({}, srcfile, outfile, {}, false);

    assert(fs::exists(outfile) && "Object file should be created on successful compilation");

    test_fs::cleanup(base_dir);
}

// Test: Compilation with flags and include dirs
void test_build_compile_objfile_with_flags_and_idirs() {
    const string base_dir = "/tmp/compile_objfile_test";
    test_fs::cleanup(base_dir);
    test_fs::setup_dirs({base_dir + "/src", base_dir + "/obj", base_dir + "/include"});
    string srcfile = base_dir + "/src/main.cpp";
    string outfile = base_dir + "/obj/main.o";
    test_fs::create_file(srcfile, "#include \"header.h\"\nint main() { return 0; }");
    test_fs::create_file(base_dir + "/include/header.h", "int foo();");

    vector<string> flags = {"-g", "-O0"};
    vector<string> idirs = {base_dir + "/include"};
    pair<string, string> p = compile_objfile(flags, srcfile, outfile, idirs, false);

    assert(fs::exists(outfile) && "Object file should be created with flags and include dirs");

    test_fs::cleanup(base_dir);
}

// Test: Directory creation failure
void test_build_compile_objfile_dir_creation_failure() {
    const string base_dir = "/tmp/compile_objfile_test";
    test_fs::cleanup(base_dir);
    test_fs::setup_dirs({base_dir + "/src"});
    string srcfile = base_dir + "/src/main.cpp";
    string outfile = "/nonexistent/dir/main.o"; // Invalid dir
    test_fs::create_file(srcfile, "int main() { return 0; }");

    bool thrown = false;
    try {
        pair<string, string> p = compile_objfile({}, srcfile, outfile, {}, false);
    } catch (const exception& e) {
        thrown = true;
        string err = e.what();
        assert(err.find("Unable to create folder") != string::npos && "Exception should indicate dir creation failure");
    }

    test_fs::cleanup(base_dir);

    assert(thrown && "Should throw when output directory cannot be created");
}

// Test: Compilation error
void test_build_compile_objfile_compilation_error() {
    const string base_dir = "/tmp/compile_objfile_test";
    test_fs::cleanup(base_dir);
    test_fs::setup_dirs({base_dir + "/src", base_dir + "/obj"});
    string srcfile = base_dir + "/src/main.cpp";
    string outfile = base_dir + "/obj/main.o";
    test_fs::create_file(srcfile, "int main() { return 0; // Syntax error"); // Invalid C++

    // bool thrown = false;
    // try {
        pair<string, string> p = compile_objfile({}, srcfile, outfile, {}, false);
    // } catch (const exception& e) {
    //     thrown = true;
    //     string err = e.what();
    //     assert(err.find("Compile error") != string::npos && "Exception should indicate compilation failure");
    // }

    assert(!fs::exists(outfile) && "Object file should not exist on compilation failure");
    test_fs::cleanup(base_dir);
    assert(str_contains(p.second, "error:"));
    // assert(thrown && "Should throw when compilation fails");
    // assert(!fs::exists(outfile) && "Object file should not exist on compilation failure");
}

// Test: Successful linking
void test_build_link_outfile_success() {
    const string base_dir = "/tmp/link_outfile_test";
    test_fs::cleanup(base_dir);
    test_fs::setup_dirs({base_dir + "/src", base_dir + "/obj", base_dir + "/bin"});
    string srcfile = base_dir + "/src/main.cpp";
    string objfile = base_dir + "/obj/main.o";
    string outfile = base_dir + "/bin/main";
    test_fs::create_file(srcfile, "int main() { return 0; }");
    pair<string, string> p = compile_objfile({}, srcfile, objfile, {}, false); // Create valid object file

    p = link_outfile({}, outfile, {objfile}, {}, false);

    assert(fs::exists(outfile) && "Executable should be created on successful linking");

    test_fs::cleanup(base_dir);
}

// Test: Linking with flags and libraries
void test_build_link_outfile_with_flags_and_libs() {
    const string base_dir = "/tmp/link_outfile_test";
    test_fs::cleanup(base_dir);
    test_fs::setup_dirs({base_dir + "/src", base_dir + "/obj", base_dir + "/bin"});
    string srcfile = base_dir + "/src/main.cpp";
    string objfile = base_dir + "/obj/main.o";
    string outfile = base_dir + "/bin/main";
    test_fs::create_file(srcfile, "#include <thread>\nvoid foo() { std::thread t([](){}); t.join(); }\nint main() { foo(); return 0; }");
    pair<string, string> p = compile_objfile({}, srcfile, objfile, {}, false);

    vector<string> flags = {"-g"};
    vector<string> libs = {"-pthread"};
    p = link_outfile(flags, outfile, {objfile}, libs, false);

    assert(fs::exists(outfile) && "Executable should be created with flags and libraries");

    test_fs::cleanup(base_dir);
}

// Test: Directory creation failure
void test_build_link_outfile_dir_creation_failure() {
    const string base_dir = "/tmp/link_outfile_test";
    test_fs::cleanup(base_dir);
    test_fs::setup_dirs({base_dir + "/src", base_dir + "/obj"});
    string srcfile = base_dir + "/src/main.cpp";
    string objfile = base_dir + "/obj/main.o";
    string outfile = "/nonexistent/dir/main"; // Invalid dir
    test_fs::create_file(srcfile, "int main() { return 0; }");
    pair<string, string> p = compile_objfile({}, srcfile, objfile, {}, false);

    bool thrown = false;
    try {
        pair<string, string> p = link_outfile({}, outfile, {objfile}, {}, false);
    } catch (const exception& e) {
        thrown = true;
        string err = e.what();
        assert(err.find("Unable to create folder") != string::npos && "Exception should indicate dir creation failure");
    }

    test_fs::cleanup(base_dir);

    assert(thrown && "Should throw when output directory cannot be created");
}

// Test: Linking error
void test_build_link_outfile_linking_error() {
    const string base_dir = "/tmp/link_outfile_test";
    test_fs::cleanup(base_dir);
    test_fs::setup_dirs({base_dir + "/bin"});
    string outfile = base_dir + "/bin/main";
    string objfile = base_dir + "/obj/nonexistent.o"; // Missing object file

    // bool thrown = false;
    // try {
        pair<string, string> p = link_outfile({}, outfile, {objfile}, {}, false);
    // } catch (const exception& e) {
    //     thrown = true;
    //     string err = e.what();
    //     assert(err.find("Link error") != string::npos && "Exception should indicate linking failure");
    // }


    // assert(thrown && "Should throw when linking fails");
    assert(!fs::exists(outfile) && "Executable should not exist on linking failure");
    test_fs::cleanup(base_dir);
    assert(str_contains(p.second, "error:"));
}

// Test: Simple dependencies
void test_build_get_alldeps_simple() {
    depmap_t depmap = {
        {"main.cpp", {{"a.cpp", "a.h"}}}
    };
    vector<string> result = get_alldeps("main.cpp", depmap);

    vector<string> expected = {"main.cpp", "a.cpp", "a.h"};
    assert(vector_equal(result, expected) && "Should return direct dependencies");
}

// Test: Transitive dependencies
void test_build_get_alldeps_transitive() {
    depmap_t depmap = {
        {"main.cpp", {{"a.cpp", "a.h"}}},
        {"a.cpp", {{"b.cpp", "b.h"}}},
        {"b.cpp", {{ "", "c.h"}}}
    };
    vector<string> result = get_alldeps("main.cpp", depmap);

    vector<string> expected = {"main.cpp", "a.cpp", "a.h", "b.cpp", "b.h", "c.h"};
    sort(result);
    sort(expected);
    assert(vector_equal(result, expected) && "Should return transitive dependencies");
}

// Test: Circular dependencies
void test_build_get_alldeps_circular() {
    depmap_t depmap = {
        {"main.cpp", {{"a.cpp", "a.h"}}},
        {"a.cpp", {{"main.cpp", "b.h"}}}
    };
    vector<string> result = get_alldeps("main.cpp", depmap);

    vector<string> expected = {"main.cpp", "a.cpp", "a.h", "b.h"};
    sort(result);
    sort(expected);
    assert(vector_equal(result, expected) && "Should handle circular dependencies without infinite loop");
}

// Test: No dependencies
void test_build_get_alldeps_no_deps() {
    depmap_t depmap = {
        {"a.cpp", {{"b.cpp", "b.h"}}}
    };
    vector<string> result = get_alldeps("main.cpp", depmap);

    vector<string> expected = {"main.cpp"};
    assert(vector_equal(result, expected) && "Should return only the file if no dependencies exist");
}

// Test: Empty depmap
void test_build_get_alldeps_empty_depmap() {
    depmap_t depmap;
    vector<string> result = get_alldeps("main.cpp", depmap);

    vector<string> expected = {"main.cpp"};
    assert(vector_equal(result, expected) && "Should return only the file for empty depmap");
}

// Test: File with no dependencies
void test_build_get_alldepfmtime_no_deps() {
    const string base_dir = "/tmp/get_alldepfmtime_test";
    test_fs::cleanup(base_dir);
    test_fs::setup_dirs({base_dir});
    string filename = base_dir + "/main.cpp";
    test_fs::create_file(filename);
    ms_t set_time = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 1000;
    test_fs::set_file_time(filename, set_time);

    depmap_t depmap;
    ms_t result = get_alldepfmtime(filename, depmap);

    test_fs::cleanup(base_dir);

    assert(abs(result - set_time) < 100 && "Should return the file's own mtime when no dependencies");
}

// Test: File with direct dependencies
void test_build_get_alldepfmtime_direct_deps() {
    const string base_dir = "/tmp/get_alldepfmtime_test";
    test_fs::cleanup(base_dir);
    test_fs::setup_dirs({base_dir});
    string filename = base_dir + "/main.cpp";
    string dep1 = base_dir + "/a.cpp";
    string dep2 = base_dir + "/a.h";
    test_fs::create_file(filename);
    test_fs::create_file(dep1);
    test_fs::create_file(dep2);

    ms_t t1 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 2000;
    ms_t t2 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 1000;
    ms_t t3 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 500;
    test_fs::set_file_time(filename, t1);
    test_fs::set_file_time(dep1, t2);
    test_fs::set_file_time(dep2, t3);

    depmap_t depmap = {
        {filename, {{dep1, dep2}}}
    };
    ms_t result = get_alldepfmtime(filename, depmap);

    test_fs::cleanup(base_dir);

    assert(abs(result - t3) < 100 && "Should return the latest mtime of direct dependencies");
    assert(result > t1 && result > t2 && "Result should be the most recent");
}

// Test: File with transitive dependencies
void test_build_get_alldepfmtime_transitive_deps() {
    const string base_dir = "/tmp/get_alldepfmtime_test";
    test_fs::cleanup(base_dir);
    test_fs::setup_dirs({base_dir});
    string filename = base_dir + "/main.cpp";
    string dep1 = base_dir + "/a.cpp";
    string dep2 = base_dir + "/a.h";
    string dep3 = base_dir + "/b.cpp";
    test_fs::create_file(filename);
    test_fs::create_file(dep1);
    test_fs::create_file(dep2);
    test_fs::create_file(dep3);

    ms_t t1 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 3000;
    ms_t t2 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 2000;
    ms_t t3 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 1000;
    ms_t t4 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 500;
    test_fs::set_file_time(filename, t1);
    test_fs::set_file_time(dep1, t2);
    test_fs::set_file_time(dep2, t3);
    test_fs::set_file_time(dep3, t4);

    depmap_t depmap = {
        {filename, {{dep1, dep2}}},
        {dep1, {{dep3, ""}}}
    };
    ms_t result = get_alldepfmtime(filename, depmap);

    test_fs::cleanup(base_dir);

    assert(abs(result - t4) < 100 && "Should return the latest mtime of transitive dependencies");
    assert(result > t1 && result > t2 && result > t3 && "Result should be the most recent");
}

// Test: Missing dependency
void test_build_get_alldepfmtime_missing_dep() {
    const string base_dir = "/tmp/get_alldepfmtime_test";
    test_fs::cleanup(base_dir);
    test_fs::setup_dirs({base_dir});
    string filename = base_dir + "/main.cpp";
    string dep1 = base_dir + "/a.cpp";
    string dep2 = base_dir + "/nonexistent.h"; // Missing file
    test_fs::create_file(filename);
    test_fs::create_file(dep1);

    ms_t t1 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 2000;
    ms_t t2 = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - 1000;
    test_fs::set_file_time(filename, t1);
    test_fs::set_file_time(dep1, t2);

    depmap_t depmap = {
        {filename, {{dep1, dep2}}}
    };

    bool thrown = false;
    try {
        get_alldepfmtime(filename, depmap);
    } catch (const exception& e) {
        thrown = true;
        string err = e.what();
        assert(err.find("File does not exist") != string::npos && "Exception should indicate missing file");
    }

    test_fs::cleanup(base_dir);

    assert(thrown && "Should throw when a dependency file is missing");
}
    
// Test constructor with missing input file argument
void test_build_config_constructor_missing_input() {
    char* argv[] = {(char*)"program"};
    Arguments args(1, argv);
    bool thrown = false;
    string what;

    try {
        build_config config(args);
    } catch (exception& e) {
        thrown = true;
        what = e.what();
        assert(str_contains(what, "Input file argument is missing") && "Exception message should indicate missing input");
    }

    assert(thrown && "Constructor should throw when input argument is missing");
}

// Test constructor with non-existent input file
void test_build_config_constructor_input_not_found() {
    char* argv[] = {(char*)"program", (char*)"nonexistent.cpp"};
    Arguments args(2, argv);
    bool thrown = false;
    string what;

    try {
        build_config config(args);
    } catch (exception& e) {
        thrown = true;
        what = e.what();
        assert(str_contains(what, "Input file not found: ") && "Exception message should indicate file not found");
    }

    assert(thrown && "Constructor should throw when input file doesn’t exist");
}

// Test constructor with valid input, no config files
void test_build_config_constructor_valid_input_no_config() {
    // Create a temporary input file
    string input_file = "test_input.cpp";
    ofstream ofs(input_file);
    ofs << "// Dummy file" << endl;
    ofs.close();

    char* argv[] = {(char*)"program", (char*)input_file.c_str()};
    Arguments args(2, argv);
    build_config config(args);

    string expected_input_file = get_absolute_path(input_file);
    string expected_input_path = get_path(expected_input_file);
    string expected_output = get_absolute_path(config.build_folder + "/" + replace_extension(remove_path(input_file), config.output_extension));
    vector<string> expected_source_extensions = {".cpp", ".c"};

    assert(config.input_file == expected_input_file && "Input file should be absolute path");
    assert(config.input_path == expected_input_path && "Input path should be absolute directory of input file");
    assert(config.output_file == expected_output && "Output file should be absolute path with default extension");
    assert(config.verbose == false && "Verbose should remain false (default) without config");
    assert(config.source_extensions == expected_source_extensions && "Source extensions should remain default without config");

    // Clean up
    fs::remove(input_file);
}

// Test constructor with config file that exists
void test_build_config_constructor_with_config() {
    string input_file = "test_input.cpp";
    ofstream ofs(input_file);
    ofs << "// Dummy file" << endl;
    ofs.close();

    string config_file = "test_input.compile.json";
    ofstream cfg(config_file);
    cfg << R"({
        "verbose": false,
        "build-folder": "build",
        "output-extension": ".out",
        "source-extensions": [".cxx"]
    })" << endl;
    cfg.close();

    char* argv[] = {(char*)"program", (char*)input_file.c_str(), (char*)"--verbose"};
    Arguments args(3, argv); // Fixed: 3 arguments to include --verbose
    build_config* config = nullptr;
    capture_cout([&](){
        config = new build_config(args);
    });    
    assert(config);

    string expected_input_file = get_absolute_path(input_file);
    string expected_input_path = get_path(expected_input_file);
    string expected_build_folder = get_absolute_path(expected_input_path + "/build");
    string expected_output = get_absolute_path(expected_build_folder + "/" + replace_extension(remove_path(input_file), ".out"));
    vector<string> expected_source_extensions = {".cpp", ".c", ".cxx"};

    assert(config->input_file == expected_input_file && "Input file should be absolute path");
    assert(config->verbose == true && "Verbose should stay true if set to true already, especially by argument --verbose, ignoring config false");
    assert(config->build_folder == expected_build_folder && "Build folder should be set from config as absolute path");
    assert(config->output_extension == ".out" && "Output extension should be set from config");
    assert(vector_equal(config->source_extensions, expected_source_extensions) && "Source extensions should merge with config values");
    assert(config->output_file == expected_output && "Output file should reflect config settings");

    delete config;

    fs::remove(input_file);
    fs::remove(config_file);
}

// Test constructor with named config argument
void test_build_config_constructor_with_named_config() {
    // Create a temporary input file
    string input_file = "test_input.cpp";
    ofstream ofs(input_file);
    ofs << "// Dummy file" << endl;
    ofs.close();

    // Create a temporary config file
    string config_file = "custom_config.json";
    ofstream cfg(config_file);
    cfg << R"({
        "verbose": false,
        "flags": ["-O2"]
    })" << endl;
    cfg.close();

    char* argv[] = {(char*)"program", (char*)input_file.c_str(), (char*)"--config=custom_config.json"};
    Arguments args(3, argv);
    build_config config(args);

    string expected_input = get_absolute_path(input_file);
    string expected_output = get_absolute_path(config.build_folder + "/" + replace_extension(remove_path(input_file), config.output_extension));

    assert(config.input_file == expected_input && "Input file should be absolute path");
    assert(config.verbose == false && "Verbose should be overridden by named config");
    assert(vector_equal(config.flags, vector<string>{"-O2"}) && "Flags should be set from named config");
    assert(config.output_file == expected_output && "Output file should use default extension with empty build_folder");

    // Clean up
    fs::remove(input_file);
    fs::remove(config_file);
}

// Test hash uniqueness with different configs
void test_build_config_constructor_hash_uniqueness() {
    string input_file = "test_input.cpp";
    ofstream ofs(input_file);
    ofs << "// Dummy file" << endl;
    ofs.close();

    // First instance with default config
    char* argv1[] = {(char*)"program", (char*)input_file.c_str()};
    Arguments args1(2, argv1);
    build_config config1(args1);
    string hash1 = config1.hash;

    // Second instance with a config file
    string config_file = "test_input.compile.json"; // Fixed: Match input_file name
    ofstream cfg(config_file);
    cfg << R"({
        "flags": ["-O2"]
    })" << endl;
    cfg.close();

    char* argv2[] = {(char*)"program", (char*)input_file.c_str()};
    Arguments args2(2, argv2);
    build_config config2(args2);
    string hash2 = config2.hash;

    assert(hash1 != hash2 && "Hash should differ with different config settings");

    fs::remove(input_file);
    fs::remove(config_file);
}

void test_get_deps_multiple_dependencies() {
    depmap_t depmap = {
        {"file1.cpp", {{"header1.h", "header1.cpp"}, {"header2.h", "header2.cpp"}}}
    };
    auto actual = get_deps(depmap, "file1.cpp");
    vector<pair<string, string>> expected = {{"header1.h", "header1.cpp"}, {"header2.h", "header2.cpp"}};
    assert(actual == expected && "Should return all header-impl pairs for file with multiple dependencies");
}

void test_get_deps_no_implementations() {
    depmap_t depmap = {
        {"file1.cpp", {{"header1.h", ""}, {"header2.h", ""}}}
    };
    auto actual = get_deps(depmap, "file1.cpp");
    vector<pair<string, string>> expected = {{"header1.h", ""}, {"header2.h", ""}};
    assert(actual == expected && "Should return headers with empty impls for file with no implementations");
}

void test_get_deps_file_not_in_depmap() {
    depmap_t depmap = {
        {"file1.cpp", {{"header1.h", "header1.cpp"}}}
    };
    auto actual = get_deps(depmap, "file2.cpp");
    vector<pair<string, string>> expected = {};
    assert(actual == expected && "Should return empty vector for file not in depmap");
}

void test_get_deps_empty_depmap() {
    depmap_t depmap = {};
    auto actual = get_deps(depmap, "file1.cpp");
    vector<pair<string, string>> expected = {};
    assert(actual == expected && "Should return empty vector for empty depmap");
}

void test_get_deps_single_dependency() {
    depmap_t depmap = {
        {"file1.cpp", {{"header1.h", "header1.cpp"}}}
    };
    auto actual = get_deps(depmap, "file1.cpp");
    vector<pair<string, string>> expected = {{"header1.h", "header1.cpp"}};
    assert(actual == expected && "Should return single header-impl pair for file with one dependency");
}

void test_check_and_extend_path_no_cycle_empty_path() {
    vector<string> path = {};
    vector<string> new_path;
    vector<string> cycle;
    auto actual = check_and_extend_path("file1.cpp", path, new_path, cycle);
    vector<string> expected_new_path = {"file1.cpp"};
    vector<string> expected_cycle = {};
    assert(actual == true && new_path == expected_new_path && cycle == expected_cycle && 
           "Should extend empty path with no cycle");
}

void test_check_and_extend_path_no_cycle_non_empty_path() {
    vector<string> path = {"file1.cpp", "header1.h"};
    vector<string> new_path;
    vector<string> cycle;
    auto actual = check_and_extend_path("header2.h", path, new_path, cycle);
    vector<string> expected_new_path = {"file1.cpp", "header1.h", "header2.h"};
    vector<string> expected_cycle = {};
    assert(actual == true && new_path == expected_new_path && cycle == expected_cycle && 
           "Should extend non-empty path with no cycle");
}

void test_check_and_extend_path_cycle_detected() {
    vector<string> path = {"file1.cpp", "header1.h", "header2.h"};
    vector<string> new_path;
    vector<string> cycle;
    auto actual = check_and_extend_path("header1.h", path, new_path, cycle);
    vector<string> expected_new_path = {"file1.cpp", "header1.h", "header2.h", "header1.h"};
    vector<string> expected_cycle = {"file1.cpp", "header1.h", "header2.h", "header1.h"};
    assert(actual == false && new_path == expected_new_path && cycle == expected_cycle && 
           "Should detect cycle and set cycle when dependency is already in path");
}

void test_check_and_extend_path_cycle_at_start() {
    vector<string> path = {"file1.cpp"};
    vector<string> new_path;
    vector<string> cycle;
    auto actual = check_and_extend_path("file1.cpp", path, new_path, cycle);
    vector<string> expected_new_path = {"file1.cpp", "file1.cpp"};
    vector<string> expected_cycle = {"file1.cpp", "file1.cpp"};
    assert(actual == false && new_path == expected_new_path && cycle == expected_cycle && 
           "Should detect cycle when adding the starting element again");
}

void test_check_and_extend_path_single_element_no_cycle() {
    vector<string> path = {"file1.cpp"};
    vector<string> new_path;
    vector<string> cycle;
    auto actual = check_and_extend_path("header1.h", path, new_path, cycle);
    vector<string> expected_new_path = {"file1.cpp", "header1.h"};
    vector<string> expected_cycle = {};
    assert(actual == true && new_path == expected_new_path && cycle == expected_cycle && 
           "Should extend single-element path with no cycle");
}

void test_extend_paths_with_deps_no_dependencies() {
    vector<pair<string, string>> deps = {};
    vector<string> path = {"file1.cpp"};
    vector<vector<string>> new_paths;
    vector<string> cycle;
    auto actual = extend_paths_with_deps(deps, path, new_paths, cycle);
    vector<vector<string>> expected_new_paths = {};
    vector<string> expected_cycle = {};
    assert(actual == false && new_paths == expected_new_paths && cycle == expected_cycle && 
           "Should not extend path with empty dependencies");
}

void test_extend_paths_with_deps_single_header_no_cycle() {
    vector<pair<string, string>> deps = {{"header1.h", ""}};
    vector<string> path = {"file1.cpp"};
    vector<vector<string>> new_paths;
    vector<string> cycle;
    auto actual = extend_paths_with_deps(deps, path, new_paths, cycle);
    vector<vector<string>> expected_new_paths = {{"file1.cpp", "header1.h"}};
    vector<string> expected_cycle = {};
    assert(actual == true && new_paths == expected_new_paths && cycle == expected_cycle && 
           "Should extend path with single header, no cycle");
}

void test_extend_paths_with_deps_header_with_cycle() {
    vector<pair<string, string>> deps = {{"file1.cpp", ""}};
    vector<string> path = {"file1.cpp"};
    vector<vector<string>> new_paths;
    vector<string> cycle;
    auto actual = extend_paths_with_deps(deps, path, new_paths, cycle);
    vector<vector<string>> expected_new_paths = {{"file1.cpp", "file1.cpp"}};
    vector<string> expected_cycle = {"file1.cpp", "file1.cpp"};
    assert(actual == false && new_paths == expected_new_paths && cycle == expected_cycle && 
           "Should detect cycle with header and stop");
}

void test_extend_paths_with_deps_header_and_impl_no_cycle() {
    vector<pair<string, string>> deps = {{"header1.h", "impl1.cpp"}};
    vector<string> path = {"file1.cpp"};
    vector<vector<string>> new_paths;
    vector<string> cycle;
    auto actual = extend_paths_with_deps(deps, path, new_paths, cycle);
    vector<vector<string>> expected_new_paths = {{"file1.cpp", "header1.h"}, {"file1.cpp", "impl1.cpp"}};
    vector<string> expected_cycle = {};
    assert(actual == true && new_paths == expected_new_paths && cycle == expected_cycle && 
           "Should extend path with header and impl, no cycle");
}

void test_extend_paths_with_deps_impl_with_cycle() {
    vector<pair<string, string>> deps = {{"header1.h", "file1.cpp"}};
    vector<string> path = {"file1.cpp"};
    vector<vector<string>> new_paths;
    vector<string> cycle;
    auto actual = extend_paths_with_deps(deps, path, new_paths, cycle);
    vector<vector<string>> expected_new_paths = {{"file1.cpp", "header1.h"}, {"file1.cpp", "file1.cpp"}};
    vector<string> expected_cycle = {"file1.cpp", "file1.cpp"};
    assert(actual == false && new_paths == expected_new_paths && cycle == expected_cycle && 
           "Should detect cycle with impl and stop");
}

void test_extend_paths_with_deps_self_loop_ignored() {
    vector<pair<string, string>> deps = {{"header1.h", "file1.cpp"}};
    vector<string> path = {"file1.cpp"};
    vector<vector<string>> new_paths;
    vector<string> cycle;
    auto actual = extend_paths_with_deps(deps, path, new_paths, cycle);
    vector<vector<string>> expected_new_paths = {{"file1.cpp", "header1.h"}, {"file1.cpp", "file1.cpp"}};
    vector<string> expected_cycle = {"file1.cpp", "file1.cpp"};
    assert(actual == false && new_paths == expected_new_paths && cycle == expected_cycle && 
           "Should detect cycle with impl even if it's the last_file");
}

void test_extend_paths_with_deps_multiple_headers_no_cycle() {
    vector<pair<string, string>> deps = {{"header1.h", ""}, {"header2.h", ""}};
    vector<string> path = {"file1.cpp"};
    vector<vector<string>> new_paths;
    vector<string> cycle;
    auto actual = extend_paths_with_deps(deps, path, new_paths, cycle);
    vector<vector<string>> expected_new_paths = {{"file1.cpp", "header1.h"}, {"file1.cpp", "header2.h"}};
    vector<string> expected_cycle = {};
    assert(actual == true && new_paths == expected_new_paths && cycle == expected_cycle && 
           "Should extend path with multiple headers, no cycle");
}

void test_process_single_dep_header_only_no_cycle() {
    pair<string, string> dep = {"header1.h", ""};
    vector<string> path = {"file1.cpp"};
    vector<vector<string>> new_paths;
    vector<string> cycle;
    bool extended = false;
    auto actual = process_single_dep(dep, path, new_paths, cycle, extended);
    vector<vector<string>> expected_new_paths = {{"file1.cpp", "header1.h"}};
    vector<string> expected_cycle = {};
    assert(actual == true && new_paths == expected_new_paths && cycle == expected_cycle && extended == true &&
           "Should extend with header only, no cycle");
}

void test_process_single_dep_header_with_cycle() {
    pair<string, string> dep = {"file1.cpp", ""};
    vector<string> path = {"file1.cpp"};
    vector<vector<string>> new_paths;
    vector<string> cycle;
    bool extended = false;
    auto actual = process_single_dep(dep, path, new_paths, cycle, extended);
    vector<vector<string>> expected_new_paths = {{"file1.cpp", "file1.cpp"}};
    vector<string> expected_cycle = {"file1.cpp", "file1.cpp"};
    assert(actual == false && new_paths == expected_new_paths && cycle == expected_cycle && extended == false &&
           "Should detect cycle with header and stop");
}

void test_process_single_dep_header_and_impl_no_cycle() {
    pair<string, string> dep = {"header1.h", "impl1.cpp"};
    vector<string> path = {"file1.cpp"};
    vector<vector<string>> new_paths;
    vector<string> cycle;
    bool extended = false;
    auto actual = process_single_dep(dep, path, new_paths, cycle, extended);
    vector<vector<string>> expected_new_paths = {{"file1.cpp", "header1.h"}, {"file1.cpp", "impl1.cpp"}};
    vector<string> expected_cycle = {};
    assert(actual == true && new_paths == expected_new_paths && cycle == expected_cycle && extended == true &&
           "Should extend with header and impl, no cycle");
}

void test_process_single_dep_impl_with_cycle() {
    pair<string, string> dep = {"header1.h", "file1.cpp"};
    vector<string> path = {"file1.cpp"};
    vector<vector<string>> new_paths;
    vector<string> cycle;
    bool extended = false;
    auto actual = process_single_dep(dep, path, new_paths, cycle, extended);
    vector<vector<string>> expected_new_paths = {{"file1.cpp", "header1.h"}, {"file1.cpp", "file1.cpp"}};
    vector<string> expected_cycle = {"file1.cpp", "file1.cpp"};
    assert(actual == false && new_paths == expected_new_paths && cycle == expected_cycle && extended == true &&
           "Should detect cycle with impl and stop");
}

void test_process_single_dep_self_loop_ignored() {
    pair<string, string> dep = {"header1.h", "file1.cpp"};
    vector<string> path = {"file1.cpp"};
    vector<vector<string>> new_paths;
    vector<string> cycle;
    bool extended = false;
    auto actual = process_single_dep(dep, path, new_paths, cycle, extended);
    vector<vector<string>> expected_new_paths = {{"file1.cpp", "header1.h"}, {"file1.cpp", "file1.cpp"}};
    vector<string> expected_cycle = {"file1.cpp", "file1.cpp"};
    assert(actual == false && new_paths == expected_new_paths && cycle == expected_cycle && extended == true &&
           "Should detect cycle with impl even if it matches last_file");
}

void test_process_single_dep_empty_impl_no_cycle() {
    pair<string, string> dep = {"header1.h", ""};
    vector<string> path = {"file1.cpp"};
    vector<vector<string>> new_paths;
    vector<string> cycle;
    bool extended = false;
    auto actual = process_single_dep(dep, path, new_paths, cycle, extended);
    vector<vector<string>> expected_new_paths = {{"file1.cpp", "header1.h"}};
    vector<string> expected_cycle = {};
    assert(actual == true && new_paths == expected_new_paths && cycle == expected_cycle && extended == true &&
           "Should extend with header, empty impl does nothing");
}

void test_add_dep_paths_empty_paths() {
    depmap_t depmap = {{"file1.cpp", {{"header1.h", ""}}}};
    vector<vector<string>> paths = {};
    vector<string> cycle;
    auto actual = add_dep_paths(depmap, paths, cycle);
    vector<vector<string>> expected_paths = {};
    vector<string> expected_cycle = {};
    assert(actual == false && paths == expected_paths && cycle == expected_cycle &&
           "Should return false with empty paths, no extension");
}

void test_add_dep_paths_no_dependencies() {
    depmap_t depmap = {{"file2.cpp", {{"header2.h", ""}}}};
    vector<vector<string>> paths = {{"file1.cpp"}};
    vector<string> cycle;
    auto actual = add_dep_paths(depmap, paths, cycle);
    vector<vector<string>> expected_paths = {{"file1.cpp"}};
    vector<string> expected_cycle = {};
    assert(actual == false && paths == expected_paths && cycle == expected_cycle &&
           "Should keep path unchanged with no dependencies");
}

void test_add_dep_paths_single_path_extend_no_cycle() {
    depmap_t depmap = {{"file1.cpp", {{"header1.h", "impl1.cpp"}}}};
    vector<vector<string>> paths = {{"file1.cpp"}};
    vector<string> cycle;
    auto actual = add_dep_paths(depmap, paths, cycle);
    vector<vector<string>> expected_paths = {{"file1.cpp", "header1.h"}, {"file1.cpp", "impl1.cpp"}};
    vector<string> expected_cycle = {};
    assert(actual == true && paths == expected_paths && cycle == expected_cycle &&
           "Should extend single path with dependencies, no cycle");
}

void test_add_dep_paths_single_path_with_cycle() {
    depmap_t depmap = {{"file1.cpp", {{"header1.h", "file1.cpp"}}}};
    vector<vector<string>> paths = {{"file1.cpp"}};
    vector<string> cycle;
    auto actual = add_dep_paths(depmap, paths, cycle);
    vector<vector<string>> expected_paths = {{"file1.cpp", "header1.h"}, {"file1.cpp", "file1.cpp"}};
    vector<string> expected_cycle = {"file1.cpp", "file1.cpp"};
    assert(actual == false && paths == expected_paths && cycle == expected_cycle &&
           "Should detect cycle in single path and stop");
}

void test_add_dep_paths_multiple_paths_all_extend() {
    depmap_t depmap = {
        {"file1.cpp", {{"header1.h", "impl1.cpp"}}},
        {"file2.cpp", {{"header2.h", ""}}}
    };
    vector<vector<string>> paths = {{"file1.cpp"}, {"file2.cpp"}};
    vector<string> cycle;
    auto actual = add_dep_paths(depmap, paths, cycle);
    vector<vector<string>> expected_paths = {
        {"file1.cpp", "header1.h"}, {"file1.cpp", "impl1.cpp"},
        {"file2.cpp", "header2.h"}
    };
    vector<string> expected_cycle = {};
    assert(actual == true && paths == expected_paths && cycle == expected_cycle &&
           "Should extend all paths with dependencies, no cycle");
}

void test_add_dep_paths_multiple_paths_one_cycles() {
    depmap_t depmap = {
        {"file1.cpp", {{"header1.h", ""}}},
        {"file2.cpp", {{"file2.cpp", ""}}}
    };
    vector<vector<string>> paths = {{"file1.cpp"}, {"file2.cpp"}};
    vector<string> cycle;
    auto actual = add_dep_paths(depmap, paths, cycle);
    vector<vector<string>> expected_paths = {{"file1.cpp", "header1.h"}, {"file2.cpp", "file2.cpp"}};
    vector<string> expected_cycle = {"file2.cpp", "file2.cpp"};
    assert(actual == false && paths == expected_paths && cycle == expected_cycle &&
           "Should stop when one path creates a cycle");
}

void test_find_circulation_empty_depmap() {
    depmap_t depmap = {};
    auto actual = find_circulation(depmap);
    vector<string> expected = {};
    assert(actual == expected && "Should return empty vector for empty depmap");
}

void test_find_circulation_no_cycles() {
    depmap_t depmap = {
        {"file1.cpp", {{"header1.h", "impl1.cpp"}}},
        {"impl1.cpp", {{"header2.h", ""}}}
    };
    auto actual = find_circulation(depmap);
    vector<string> expected = {};
    assert(actual == expected && "Should return empty vector when no cycles exist");
}

void test_find_circulation_simple_cycle() {
    depmap_t depmap = {{"file1.cpp", {{"file1.cpp", ""}}}};
    auto actual = find_circulation(depmap);
    vector<string> expected = {"file1.cpp", "file1.cpp"};
    assert(actual == expected && "Should detect simple direct cycle");
}

void test_find_circulation_complex_cycle() {
    depmap_t depmap = {
        {"file1.cpp", {{"header1.h", "file2.cpp"}}},
        {"file2.cpp", {{"header2.h", "file1.cpp"}}}
    };
    auto actual = find_circulation(depmap);
    vector<string> expected = {"file1.cpp", "file2.cpp", "file1.cpp"};
    assert(actual == expected && "Should detect complex cycle across files");
}

void test_find_circulation_throw_on_cycle() {
    depmap_t depmap = {{"file1.cpp", {{"file1.cpp", ""}}}};
    bool threw = false;
    try {
        find_circulation(depmap, true);
    } catch (const exception& e) {
        threw = true;
        string expected_msg = "circular dependency found:\nfile1.cpp\n=>file1.cpp";
        assert(str_contains(e.what(), expected_msg) && "Should throw with correct cycle message");
    }
    assert(threw && "Should throw exception when throw_on_cycle is true");
}

void test_find_circulation_multiple_starting_points_one_cycle() {
    depmap_t depmap = {
        {"file1.cpp", {{"header1.h", ""}}},
        {"file2.cpp", {{"file2.cpp", ""}}}
    };
    auto actual = find_circulation(depmap);
    vector<string> expected = {"file2.cpp", "file2.cpp"};
    assert(actual == expected && "Should detect cycle from one starting point among many");
}

// Register tests
TEST(test_build_find_impfile_found_next_to_header);
TEST(test_build_find_impfile_found_in_srcdir);
TEST(test_build_find_impfile_not_found);
TEST(test_build_find_impfile_multiple_srcdirs);
TEST(test_build_scan_includes_basic);
TEST(test_build_scan_includes_with_implementation);
TEST(test_build_scan_includes_nested);
TEST(test_build_scan_includes_missing_include);
TEST(test_build_scan_includes_circular);
TEST(test_build_save_depcache_empty);
TEST(test_build_save_depcache_single_no_deps);
TEST(test_build_save_depcache_single_with_deps);
TEST(test_build_save_depcache_multiple_files);
TEST(test_build_load_depcache_empty_file);
TEST(test_build_load_depcache_single_file_no_deps);
TEST(test_build_load_depcache_single_file_with_header);
TEST(test_build_load_depcache_single_file_with_impl);
TEST(test_build_load_depcache_multiple_files);
TEST(test_build_load_depcache_invalid_format);
TEST(test_build_get_all_files_empty);
TEST(test_build_get_all_files_single_no_deps);
TEST(test_build_get_all_files_single_with_header);
TEST(test_build_get_all_files_single_with_impl);
TEST(test_build_get_all_files_multiple_files);
TEST(test_build_get_lstmtime_empty_vector);
TEST(test_build_get_lstmtime_single_file_vector);
TEST(test_build_get_lstmtime_multiple_files_vector);
TEST(test_build_get_lstmtime_missing_file_vector);
TEST(test_build_get_lstmtime_empty_depmap);
TEST(test_build_get_lstmtime_multiple_files_depmap);
TEST(test_build_get_lstmtime_missing_file_depmap);
TEST(test_build_compile_objfile_success);
TEST(test_build_compile_objfile_with_flags_and_idirs);
TEST(test_build_compile_objfile_dir_creation_failure);
TEST(test_build_compile_objfile_compilation_error);
TEST(test_build_link_outfile_success);
TEST(test_build_link_outfile_with_flags_and_libs);
TEST(test_build_link_outfile_dir_creation_failure);
TEST(test_build_link_outfile_linking_error);
TEST(test_build_get_alldeps_simple);
TEST(test_build_get_alldeps_transitive);
TEST(test_build_get_alldeps_circular);
TEST(test_build_get_alldeps_no_deps);
TEST(test_build_get_alldeps_empty_depmap);
TEST(test_build_get_alldepfmtime_no_deps);
TEST(test_build_get_alldepfmtime_direct_deps);
TEST(test_build_get_alldepfmtime_transitive_deps);
TEST(test_build_get_alldepfmtime_missing_dep);
TEST(test_build_config_constructor_missing_input);
TEST(test_build_config_constructor_input_not_found);
TEST(test_build_config_constructor_valid_input_no_config);
TEST(test_build_config_constructor_with_config);
TEST(test_build_config_constructor_with_named_config);
TEST(test_build_config_constructor_hash_uniqueness);
TEST(test_get_deps_multiple_dependencies);
TEST(test_get_deps_no_implementations);
TEST(test_get_deps_file_not_in_depmap);
TEST(test_get_deps_empty_depmap);
TEST(test_get_deps_single_dependency);
TEST(test_check_and_extend_path_no_cycle_empty_path);
TEST(test_check_and_extend_path_no_cycle_non_empty_path);
TEST(test_check_and_extend_path_cycle_detected);
TEST(test_check_and_extend_path_cycle_at_start);
TEST(test_check_and_extend_path_single_element_no_cycle);
TEST(test_process_single_dep_header_only_no_cycle);
TEST(test_process_single_dep_header_with_cycle);
TEST(test_process_single_dep_header_and_impl_no_cycle);
TEST(test_process_single_dep_impl_with_cycle);
TEST(test_process_single_dep_self_loop_ignored);
TEST(test_process_single_dep_empty_impl_no_cycle);
TEST(test_extend_paths_with_deps_no_dependencies);
TEST(test_extend_paths_with_deps_single_header_no_cycle);
TEST(test_extend_paths_with_deps_header_with_cycle);
TEST(test_extend_paths_with_deps_header_and_impl_no_cycle);
TEST(test_extend_paths_with_deps_impl_with_cycle);
TEST(test_extend_paths_with_deps_self_loop_ignored);
TEST(test_extend_paths_with_deps_multiple_headers_no_cycle);
TEST(test_add_dep_paths_empty_paths);
TEST(test_add_dep_paths_no_dependencies);
TEST(test_add_dep_paths_single_path_extend_no_cycle);
TEST(test_add_dep_paths_single_path_with_cycle);
TEST(test_add_dep_paths_multiple_paths_all_extend);
TEST(test_add_dep_paths_multiple_paths_one_cycles);
TEST(test_find_circulation_empty_depmap);
TEST(test_find_circulation_no_cycles);
TEST(test_find_circulation_simple_cycle);
TEST(test_find_circulation_complex_cycle);
TEST(test_find_circulation_throw_on_cycle);
TEST(test_find_circulation_multiple_starting_points_one_cycle);
#endif