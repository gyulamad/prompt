
#include <mutex>

#include "../utils/Test.hpp"
#include "../utils/Config.hpp"
#include "../utils/fnames.hpp"
#include "../utils/foreach.hpp"

#include "./build.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::build;

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
        foreach (idirs, [&](string& idir) { idir = get_absolute_path(inpath + "/" + idir); });
        bool verbose = config.get<bool>(mode + ".verbose", false);

        if (verbose) {
            cout << "======= COMPILE =======" << endl;
            cout << "Mode ..........: " << mode << endl;
            cout << "Input file ....: " << infile << endl;
            cout << "Output file ...: " << outfile << endl;
            cout << "Hash ..........: " << hash << endl;
            cout << "Include path: " << endl;
            foreach(idirs, [](const string& idir) { cout << "\t" << idir << endl; });
        }

        depmap_t depmap;
        ms_t lstmtime = filemtime_ms(infile);
        if (!file_exists(depcachepath)) {
            if (verbose) cout << "Building dependency map: " << infile << endl;
            scan_includes(mtx, infile, depmap, idirs,/* lstmtime,*/ sdirs, impext, verbose);
            save_depcache(depcachepath, depmap);
        } else {
            depmap = load_depcache(depcachepath);
        }
        lstmtime = get_lstmtime(depmap);
        
        if (!file_exists(outfname_hash) || filemtime_ms(outfname_hash) < lstmtime) {
            vector<string> objfiles;

            #define COMPILE_OBJFILE(ifile) { \
                string objfile = replace_extension(str_replace(inpath, outpath_hash, ifile), objext); \
                if (!in_array(objfile, objfiles)) { \
                    objfiles.emplace_back(objfile); \
                    if (!file_exists(objfile) /*|| filemtime_ms(objfile) < filemtime_ms(ifile)*/ || filemtime_ms(objfile) < get_alldepfmtime(ifile, depmap)) \
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
