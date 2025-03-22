// to get coverage: 
// Executing task: g++ -std=c++20 /mnt/windows/llm/prompt/src/tools/build/compile.cpp -o /mnt/windows/llm/prompt/builds/compile -g -O0 -fsanitize=address -DTEST --coverage -fprofile-arcs -ftest-coverage -pedantic-errors -Werror -Wall -Wextra -Wunused -fno-elide-constructors 
// then: $ ./src/tools/build/gencov.sh ./src/tools/build/compile.cpp ./builds/compile

#include <mutex>

#include "../utils/Test.hpp"
#include "../utils/Settings.hpp"
#include "../utils/fnames.hpp"
#include "../utils/foreach.hpp"

#include "inc/build.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::build;


int safe_main(int argc, char* argv[]) {
    try {
        cout << "======= COMPILE =======" << endl;

        mutex mtx;
        Arguments args(argc, argv);
        build_config config(args);

        
        if (!file_exists(config.build_folder)) if (!mkdir(config.build_folder)) throw ERROR("Unable to create folder: " + config.build_folder);

        string outpath_hash = config.build_folder + "/" + config.hash;
        string outfname_hash = outpath_hash + "/" + remove_path(config.output_file);
        if (!file_exists(outpath_hash)) if (!mkdir(outpath_hash)) throw ERROR("Unable to create folder: " + outpath_hash);
        string depcachepath = outpath_hash + "/.depcache"; // TODO: to config

        if (config.verbose) {
            cout << "Input file ....: " << config.input_file << endl;
            cout << "Output file ...: " << config.output_file << endl;
            cout << "Hash ..........: " << config.hash << endl;
        }

        depmap_t depmap;
        ms_t lstmtime = filemtime_ms(config.input_file);
        if (!file_exists(depcachepath)) {
            if (config.verbose) cout << "Building dependency map for: " << config.input_file << endl;
            scan_includes(mtx, config.input_file, depmap, config.include_path, config.source_path, config.source_extensions, config.verbose);
            save_depcache(depcachepath, depmap);
        } else depmap = load_depcache(depcachepath);
        lstmtime = get_lstmtime(depmap);
        
        if (!file_exists(outfname_hash) || filemtime_ms(outfname_hash) < lstmtime) {
            vector<string> objfiles;

            #define COMPILE_OBJFILE(ifile) { \
                string objfile = replace_extension(str_replace(config.input_path, outpath_hash, ifile), config.object_extension); \
                if (!in_array(objfile, objfiles)) { \
                    objfiles.emplace_back(objfile); \
                    if (!file_exists(objfile) /*|| filemtime_ms(objfile) < filemtime_ms(ifile)*/ || filemtime_ms(objfile) < get_alldepfmtime(ifile, depmap)) \
                        compile_objfile(config.flags, ifile, objfile, config.include_path, config.verbose); \
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

            COMPILE_OBJFILE(config.input_file);

            if (config.verbose) cout << "Rebuilding executable: " << config.output_file << endl;
            link_outfile(config.flags, config.output_file, objfiles, config.libs, config.verbose);
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
