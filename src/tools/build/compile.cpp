// to get coverage: 
// Executing task: g++ -std=c++20 /mnt/windows/llm/prompt/src/tools/build/compile.cpp -o /mnt/windows/llm/prompt/builds/compile -g -O0 -fsanitize=address -DTEST --coverage -fprofile-arcs -ftest-coverage -pedantic-errors -Werror -Wall -Wextra -Wunused -fno-elide-constructors 
// then: $ ./src/tools/build/gencov.sh ./src/tools/build/compile.cpp ./builds/compile

#include <mutex>

#include "../utils/foreach.hpp"
#include "../utils/Settings.hpp"
#include "../utils/Test.hpp"

#include "inc/build.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::build;


int safe_main(int argc, char* argv[]) {
    try {

        mutex mtx;
        
        Arguments args(argc, argv);
        if (args.get<bool>("verbose")) cout << "===================== COMPILE =====================" << endl;

        build_config config(args);


        if (config.verbose) {
            cout << "Input file ....: " << config.input_file << endl;
            cout << "Output file ...: " << config.output_file << endl;
            cout << "Hash ..........: " << config.hash << endl;
        }

        
        if (!file_exists(config.build_folder)) if (!mkdir(config.build_folder, true)) throw ERROR("Unable to create folder: " + config.build_folder);

        string outpath_hash = config.build_folder + "/" + config.hash;
        string outfname_hash = outpath_hash + "/" + remove_path(config.output_file);
        if (!file_exists(outpath_hash)) if (!mkdir(outpath_hash, true)) throw ERROR("Unable to create folder: " + outpath_hash);
        string depcachepath = outpath_hash + "/.depcache"; // TODO: to config

        

        depmap_t depmap;
        ms_t lstmtime = filemtime_ms(config.input_file);
        if (!file_exists(depcachepath)) {
            if (config.verbose) cout << "Building dependency map for: " << config.input_file << endl;
            scan_includes(mtx, config.input_file, depmap, config.include_path, config.source_path, config.source_extensions, config.verbose);
            save_depcache(depcachepath, depmap);
        } else depmap = load_depcache(depcachepath);
        lstmtime = get_lstmtime(depmap);
        
        if (!file_exists(outfname_hash) || filemtime_ms(outfname_hash) < lstmtime) {
            vector<pair<string, string>> failures;
            vector<string> objfiles;

            #define COMPILE_OBJFILE(ifile) { \
                string objfile = replace_extension(str_replace(config.input_path, outpath_hash, ifile), config.object_extension); \
                if (!in_array(objfile, objfiles)) { \
                    objfiles.emplace_back(objfile); \
                    if (!file_exists(objfile) || filemtime_ms(objfile) < filemtime_ms(ifile) || filemtime_ms(objfile) < get_alldepfmtime(ifile, depmap)) { \
                        pair<string, string> result = compile_objfile(config.flags, ifile, objfile, config.include_path, config.verbose); \
                        if (!result.second.empty()) failures.push_back(result); \
                    } \
                } \
            }

            vector<string> to_objfiles;
            
            foreach (depmap, [&](map<string, string>& depfiles, const string& /*incfile*/) {
                foreach (depfiles, [&](const string& impfile, const string& /*depfile*/) { 
                    string ifile = impfile;                  
                    if (ifile.empty()) {
                        return FE_CONTINUE;
                        // if (!config.make_temp_source) return FE_CONTINUE;
                        // if (!str_starts_with(depfile, config.input_path)) return FE_CONTINUE;
                        // ifile = replace_extension(str_replace(config.input_path, outpath_hash, depfile), config.temp_source_extension);
                        // if (config.verbose) cout << "Using temp source file: " << ifile << endl;
                        // if (!file_exists(ifile)) {
                        //     if (!mkdir(get_path(ifile), true)) throw ERROR("Unable to create path: " + get_path(ifile));
                        //     file_put_contents(ifile, "#include \"" + depfile + "\"", false, true);
                        // }
                    }
                    to_objfiles.push_back(ifile);
                    return FE_CONTINUE;
                });
            });

            vector<string> errors;
            vector<thread> threads;               
            foreach (to_objfiles, [&](const string& ifile) {
                threads.emplace_back([&]() {
                    try {                  
                        COMPILE_OBJFILE(ifile);
                    } catch (exception& e) {
                        lock_guard<mutex> lock(mtx);
                        errors.push_back(e.what());
                    }
                });
            });
            for (thread& t: threads) { if (t.joinable()) t.join(); }
            if (!errors.empty()) throw ERROR("Include error(s) detected:\n" + implode("\n", errors));

            COMPILE_OBJFILE(config.input_file);

            if (config.verbose) cout << "Rebuilding executable: " << config.output_file << endl;
            pair<string, string> result = link_outfile(config.flags, config.output_file, objfiles, config.libs, config.verbose);
            if (!result.second.empty()) failures.push_back(result);

            if (!failures.empty()) {
                foreach (failures, [](const pair<string, string>& fail) {
                    cout << fail.first << endl;
                    cerr << str_replace({
                        { "error:", ANSI_FMT(ANSI_FMT_ERROR, "error:") },
                        { "note:", ANSI_FMT(ANSI_FMT_NOTE, "note:") },
                        { "warning:", ANSI_FMT(ANSI_FMT_WARNING, "warning:") },
                    }, fail.second) << endl;
                });
                throw ERROR("Build failed, see more on standard error...");
            }
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
