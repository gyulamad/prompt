#include <iostream>
#include <thread>

#include "../utils/Test.hpp"
#include "../utils/Config.hpp"
#include "../utils/fnames.hpp"
#include "../utils/vectors.hpp"
#include "../utils/time.hpp"
#include "../utils/foreach.hpp"
#include "../utils/execute.hpp"

using namespace std;
using namespace tools::utils;


typedef map<string, map<string, string>> depmap_t;

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

// float roll = 0, spd = 0.01;
void scan_includes(
    mutex& mtx,
    const string& srcfile, 
    depmap_t& depmap, 
    const vector<string>& idirs, 
    const vector<string>& sdirs, 
    // ms_t& lstmtime, 
    // map<string, string>& impmap, 
    const string& impext,
    vector<string>& visited,
    bool verbose
) {
    // Check if already visited
    {
        lock_guard<mutex> lock(mtx);
        if (in_array(srcfile, visited)) return;
        visited.emplace_back(srcfile);
    }

    if (verbose) cout << "scan: " + srcfile << endl;
    // cout << "\r\033[K[ ] scan: " << srcfile << flush;
    // ms_t _lstmtime = filemtime_ms(srcfile);
    // if (_lstmtime > lstmtime) lstmtime = _lstmtime;

    string srccode = file_get_contents(srcfile);
    vector<string> srclines = explode("\n", srccode);
    vector<string> matches;
    string srcpath = get_path(srcfile);
    vector<string> lookup;
    foreach(srclines, [&](const string& srcline) {
        // cout << "\r[" << "/-\\|"[(long)(roll += spd)%4] << "\r" << flush;
        if (srcline.empty() || !regx_match(R"(^\s*#)", srcline) || !regx_match(R"(^\s*#\s*include\s*\"\s*([^\"]+)\s*\")", srcline, &matches)) return;
        string incfile = get_absolute_path(srcpath + "/" + matches[1]);
        vector<string> tries = { incfile };
        if (!file_exists(incfile)) foreach (idirs, [&](const string& idir) {
            incfile = get_absolute_path(idir + "/" + remove_path(incfile));
            tries.push_back(incfile);
            if (file_exists(incfile)) return FE_BREAK;
            return FE_CONTINUE;
        });
        if (!file_exists(incfile))
            throw ERROR("Dependency include file not found, following are tried:\n\t" + implode("\n\t", tries) + "\nIncluded in " + srcfile);
        //cout << "found: " << incfile << " (in " << srcfile << ")" << endl;
        string impfile = find_impfile(incfile, sdirs, impext);
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
                scan_includes(mtx, incfile, depmap, idirs,/* lstmtime,*/ sdirs, impext, visited, verbose);
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
    const string& impext,
    bool verbose
) {
    vector<string> visited;
    scan_includes(mtx, incfile, depmap, idirs, sdirs, impext, visited, verbose);
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


void compile_objfile(const vector<string>& flags, const string& srcfile, const string& outfile, const vector<string>& idirs, bool verbose) {
    string outpath = get_path(outfile);
    if (!mkdir(outpath, true)) throw ERROR("Unable to create folder: " + outpath);
    string cmd = "g++" 
        + (!flags.empty() ? " " + implode(" ", flags) : "") 
        + " -c " + srcfile 
        + " -o " + outfile 
        + (!idirs.empty() ? " -I" + implode(" -I", idirs) : "");
    string output = execute(cmd, verbose); //""; cout << cmd << endl; // TODO: execute(cmd, true);
    if (!output.empty()) throw ERROR("Compile error at " + srcfile + "\n" + cmd + "\n" + output);
} 

void link_outfile(const vector<string>& flags, const string& outfile, const vector<string>& objfiles, const vector<string>& libs, bool verbose) {
    string outpath = get_path(outfile);
    if (!mkdir(outpath, true)) throw ERROR("Unable to create folder: " + outpath);
    string cmd = "g++" 
        + (!flags.empty() ? " " + implode(" ", flags) : "") 
        + (!objfiles.empty() ? " " + implode(" ", objfiles) : "")
        + " -o " + outfile
        + (!libs.empty() ? " " + implode(" ", libs) : "");
    string output = execute(cmd, verbose); //""; cout << cmd << endl; // TODO: execute(cmd, true);
    if (!output.empty()) throw ERROR("Link error: " + cmd + "\n" + output);
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
                    deps = array_merge(deps, get_alldeps(depf, depmap, visited));
                }
                if (!impf.empty()) {
                    deps.push_back(impf);
                    deps = array_merge(deps, get_alldeps(impf, depmap, visited));
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
    ms_t fmtime = filemtime(filename);
    vector<string> alldeps = get_alldeps(filename, depmap);
    foreach (alldeps, [&](const string& depf) {
        ms_t depfmtime = filemtime(depf);
        if (depfmtime > fmtime) fmtime = depfmtime;
    });
    return fmtime;
}

int safe_main(int argc, char* argv[]) {
    try {
        mutex mtx;
        Config config(argc, argv);
        string infile = get_absolute_path(config.get<string>(1));
        config.load(remove_extension(infile));
        string mode = config.get<string>("mode");
        string inpath = get_path(infile);
        string outpath = get_absolute_path(inpath + "/" + config.get<string>(mode + ".build-folder", inpath));
        if (!file_exists(outpath)) if (!mkdir(outpath)) throw ERROR("Unable to create folder: " + outpath);
        string outext = config.get<string>(mode + ".output-extension", "");
        string objext = config.get<string>(mode + ".object-extension", ".o");
        // map<string, string> impmap = config.get<map<string, string>>(mode + ".implementation-map", {}); // TODO: ?? impmap vs. idir!!!
        string impext = config.get<string>(mode + ".source-extension", ".cpp"); // ".cpp"; // TODO: make it array (".c", ".cpp") to config
        string outfname = remove_extension(remove_path(infile)) + outext;
        string outfile = get_absolute_path(outpath + "/" + outfname);
        string hash = config.hash();
        string outpath_hash = outpath + "/" + hash;
        string outfname_hash = outpath_hash + "/" + outfname;
        if (!file_exists(outpath_hash)) if (!mkdir(outpath_hash)) throw ERROR("Unable to create folder: " + outpath_hash);
        string depcachepath = outpath_hash + "/.depcache"; // TODO: to config
        vector<string> flags = config.get<vector<string>>(mode + ".flags", {});
        vector<string> libs = config.get<vector<string>>(mode + ".libs", {});
        vector<string> idirs = config.get<vector<string>>(mode + ".include-path");
        vector<string> sdirs = config.get<vector<string>>(mode + ".source-path");
        foreach(idirs, [&](string& idir) { idir = get_absolute_path(inpath + "/" + idir); });
        bool verbose = config.get<bool>(mode + ".verbose", false);

        if (verbose) {
            cout << "Mode ..........: " << mode << endl;
            cout << "Input file ....: " << infile << endl;
            cout << "Output file ...: " << outfile << endl;
            cout << "Hash ..........: " << hash << endl;
            cout << "Include path: " << endl;
            foreach(idirs, [](const string& idir) { cout << "\t" << idir << endl; });
        }

        depmap_t depmap;
        ms_t lstmtime = filemtime(infile);
        if (!file_exists(depcachepath)) {
            if (verbose) cout << "Building dependency map: " << infile << endl;
            scan_includes(mtx, infile, depmap, idirs,/* lstmtime,*/ sdirs, impext, verbose);
            save_depcache(depcachepath, depmap);
        } else {
            depmap = load_depcache(depcachepath);
        }
        lstmtime = get_lstmtime(depmap);
        
        if (!file_exists(outfname_hash) || filemtime(outfname_hash) < lstmtime) {
            vector<string> objfiles;

            #define COMPILE_OBJFILE(ifile) { \
                string objfile = replace_extension(str_replace(inpath, outpath_hash, ifile), objext); \
                if (!in_array(objfile, objfiles)) { \
                    objfiles.emplace_back(objfile); \
                    if (!file_exists(objfile) /*|| filemtime(objfile) < filemtime(ifile)*/ || filemtime(objfile) < get_alldepfmtime(ifile, depmap)) \
                        compile_objfile(flags, ifile, objfile, idirs, verbose); \
                } \
            }

            vector<string> errors;
            vector<thread> threads;
            foreach (depmap, [&](map<string, string>& depfiles, const string& /*incfile*/) {
                foreach (depfiles, [&](const string& impfile, const string& /*depfile*/) {  
                    if (impfile.empty()) return FE_CONTINUE;
                    threads.emplace_back([&]() {
                        try {                  
                            COMPILE_OBJFILE(impfile);
                        } catch (exception& e) {
                            lock_guard<mutex> lock(mtx);
                            errors.push_back(e.what());
                        }
                    });
                    return FE_CONTINUE;
                });
            });
            for (thread& t: threads) { if (t.joinable()) t.join(); }
            if (!errors.empty()) throw ERROR("Include error(s) detected:\n" + implode("\n", errors));

            COMPILE_OBJFILE(infile);

            if (verbose) cout << "Rebuilding executable: " << outfile << endl;
            link_outfile(flags, outfile, objfiles, libs, verbose);
        }

    } catch (exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    return run_tests() + safe_main(argc, argv);
}
