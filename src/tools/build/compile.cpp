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

const string default_configs = R"({
    "coverage": {},
    "debug": {
        "flags": [
            "-std=c++20",
            "-g",
            "-O0", 
            "-fsanitize=address"
        ],
    },
    "test": {},
    "fast": {
        "flags": [
            "-std=c++20",
            "-Ofast",
            "-pedantic-errors",
            "-Werror",
            "-Wall", "-Wextra",
            "-Wunused",
            "-fno-elide-constructors"
        ]
    },
    "run": {},
    "coverage-debug": {},
    "coverage-test": {},
    "coverage-fast": {},
    "coverage-run": {},
    "debug-test": {},
    "debug-fast": {},
    "debug-run": {},
    "test-fast": {},
    "test-run": {},
    "fast-run": {},
    "coverage-debug-test": {},
    "coverage-debug-fast": {},
    "coverage-debug-run": {},
    "coverage-test-fast": {},
    "coverage-test-run": {},
    "coverage-fast-run": {},
    "debug-test-fast": {},
    "debug-test-run": {},
    "debug-fast-run": {},
    "test-fast-run": {},
    "coverage-debug-test-fast": {},
    "coverage-debug-test-run": {},
    "coverage-debug-fast-run": {},
    "coverage-test-fast-run": {},
    "debug-test-fast-run": {},
    "coverage-debug-test-fast-run": {}
})";

//     "_debug": {
//         "verbose": true,
//         "build-folder": "../builds",
//         "output-extension": ".gdb",
//         "object-extension": ".o",
//         "source-extension": [".cpp", ".c"],
//         "source-path": [],
//         "flags": [
//             "-std=c++20",
//             "-g",
//             "-O0", 
//             "-fsanitize=address"
//         ],
//         "include-path": [
//             "../libs/ggerganov/whisper.cpp/include",
//             "../libs/ggerganov/whisper.cpp/ggml/include"
//         ],
//         "libs": [
//             "libs/ggerganov/whisper.cpp/build/src/libwhisper.so.1.7.4", 
//             "-Wl,-rpath,libs/ggerganov/whisper.cpp/build/src",  // <-- Add this
//             "-lrt", "-lm", "-lasound", //"-ljack", 
//             "-lportaudio", //"-lwhisper",
//             "-lcurl",
//             "-pthread"
//         ]
//     },
//     "debug-with-tests": {
//         "verbose": true,
//         "build-folder": "../builds",
//         "output-extension": ".gdb",
//         "object-extension": ".o",
//         "source-extension": [".cpp", ".c"],
//         "source-path": [],
//         "flags": [
//             "-std=c++20",
//             "-g",
//             "-O0", 
//             "-fsanitize=address",
//             "-DTEST",

//             "-pedantic-errors",
//             "-Werror", // Treats all warnings as errors, forcing the compiler to flag suspicious code.
//             "-Wall", "-Wextra", // Enables extra warnings, including unused or suspicious constructs.
//             "-Wunused", // Warns if a function or variable is unused, which might hint at optimization skipping drop.
//             "-fno-elide-constructors"//, // Disables copy elision, which could hide template-related bugs.
//             // "-fsyntax-only", // Checks syntax without generating code, forcing full parsing.
//             // "-Wundefined-func-template", // (Clang): Warns about undefined template functions.
//         ],
//         "include-path": [
//             "../libs/ggerganov/whisper.cpp/include",
//             "../libs/ggerganov/whisper.cpp/ggml/include"
//         ],
//         "libs": [
//             "libs/ggerganov/whisper.cpp/build/src/libwhisper.so.1.7.4", 
//             "-Wl,-rpath,libs/ggerganov/whisper.cpp/build/src",  // <-- Add this
//             "-lrt", "-lm", "-lasound", //"-ljack", 
//             "-lportaudio", //"-lwhisper",
//             "-lcurl",
//             "-pthread"
//         ]
//     },
//     "fast-with-tests": {
//         "verbose": true,
//         "build-folder": "../builds",
//         "output-extension": "",
//         "object-extension": ".o",
//         "source-extension": [".cpp", ".c"],
//         "source-path": [],
//         "flags": [
//             "-std=c++20",
//             // "-g",
//             // "-O0", 
//             // "-fsanitize=address",
//             "-Ofast",
//             "-DTEST",
//             "-DTEST_FAILURE_DIES",

//             "-pedantic-errors",
//             "-Werror", // Treats all warnings as errors, forcing the compiler to flag suspicious code.
//             "-Wall", "-Wextra", // Enables extra warnings, including unused or suspicious constructs.
//             "-Wunused", // Warns if a function or variable is unused, which might hint at optimization skipping drop.
//             "-fno-elide-constructors"//, // Disables copy elision, which could hide template-related bugs.
//             // "-fsyntax-only", // Checks syntax without generating code, forcing full parsing.
//             // "-Wundefined-func-template", // (Clang): Warns about undefined template functions.
//         ],
//         "include-path": [
//             "../libs/ggerganov/whisper.cpp/include",
//             "../libs/ggerganov/whisper.cpp/ggml/include"
//         ],
//         "libs": [
//             "libs/ggerganov/whisper.cpp/build/src/libwhisper.so.1.7.4", 
//             "-Wl,-rpath,libs/ggerganov/whisper.cpp/build/src",  // <-- Add this
//             "-lrt", "-lm", "-lasound", //"-ljack", 
//             "-lportaudio", //"-lwhisper",
//             "-lcurl",
//             "-pthread"
//         ]
//     },
//     "_fast": {
//         "verbose": true,
//         "build-folder": "../builds",
//         "output-extension": "",
//         "object-extension": ".o",
//         "source-extension": [".cpp", ".c"],
//         "source-path": [],
//         "flags": [
//             "-std=c++20",
//             // "-g",
//             // "-O0", 
//             // "-fsanitize=address",
//             "-Ofast",
//             // "-DTEST",
//             // "-DTEST_FAILURE_DIES",

//             "-pedantic-errors",
//             "-Werror", // Treats all warnings as errors, forcing the compiler to flag suspicious code.
//             "-Wall", "-Wextra", // Enables extra warnings, including unused or suspicious constructs.
//             "-Wunused", // Warns if a function or variable is unused, which might hint at optimization skipping drop.
//             "-fno-elide-constructors"//, // Disables copy elision, which could hide template-related bugs.
//             // "-fsyntax-only", // Checks syntax without generating code, forcing full parsing.
//             // "-Wundefined-func-template", // (Clang): Warns about undefined template functions.
//         ],
//         "include-path": [
//             "../libs/ggerganov/whisper.cpp/include",
//             "../libs/ggerganov/whisper.cpp/ggml/include"
//         ],
//         "libs": [
//             "libs/ggerganov/whisper.cpp/build/src/libwhisper.so.1.7.4", 
//             "-Wl,-rpath,libs/ggerganov/whisper.cpp/build/src",  // <-- Add this
//             "-lrt", "-lm", "-lasound", //"-ljack", 
//             "-lportaudio", //"-lwhisper",
//             "-lcurl",
//             "-pthread"
//         ]
//     }
// })";

struct build_config {
    bool verbose = true;
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

    build_config(Arguments& args, const string& fsuffix = "compile.json") {
        if (verbose) cout << "Setup configuration..." << endl;

        if (!args.has(1)) throw ERROR("Input file argument is missing.");
        input_file = get_absolute_path(args.get<string>(1));
        if (!file_exists(input_file)) throw ERROR("Input file not found: " + input_file);
        if (verbose) cout << "Input file found: " << input_file << endl;

        input_path = get_absolute_path(get_path(input_file));

        string exename = get_absolute_path(args.get<string>(0));
        apply_cfg(args, exename + "/" + fsuffix);

        apply_cfg(args, input_path + "/" + fsuffix);
        apply_cfg(args, replace_extension(input_file, "." + fsuffix));
        if (args.has("config")) {
            string cfgfile = args.get<string>("config");
            apply_cfg(args, cfgfile);
            apply_cfg(args, replace_extension(input_file, "." + cfgfile + "." + fsuffix));
        }

        string outfname = replace_extension(remove_path(input_file), output_extension);
        output_file = get_absolute_path(build_folder + "/" + outfname);

        hash = get_hash(hash);
    }

private:
    void apply_cfg(Arguments& args, const string& cfgfile) {
        if (verbose) cout << "Looking for configuration at file: " + cfgfile + " ..." << flush;
        if (!file_exists(cfgfile)) {
            if (verbose) cout << " [not found]" << endl;
            return;
        }
        
        if (verbose) cout << " [found] loading into..." << flush;
        
        JSON cfg(tpl_replace("{{input-path}}", input_path, file_get_contents(cfgfile)));
        Settings settings(args, cfg);
        verbose = settings.get<bool>("verbose", verbose);

        #define SHOWCFG_STRING(var, label) if (verbose) cout << "\n\t" << label << ": " << var << flush;

        #define SHOWCFG_VECTOR(var, label) \
        if (verbose) { \
            cout << "\n\t" << label << ":" << flush; \
            for (const string& s: var) cout << "\n\t\t" << s << flush; \
        }

        #define LOADCFG_STRING(var, name, label) \
            var = settings.get<string>(name, var); \


        #define LOADCFG_STRING_PATH(var, name, label) \
            LOADCFG_STRING(var, name, label) \
            var = str_starts_with("/", var) ? var : get_absolute_path(input_path + "/" + var);

        #define LOADCFG_VECTOR(var, name, label) \
            var = array_merge(var, settings.get<vector<string>>(name, var)); \
            // SHOWCFG_VECTOR(var, label)

        #define LOADCFG_VECTOR_PATH(var, name, label) \
            LOADCFG_VECTOR(var, name, label) \
            foreach (var, [&](string& path) { path = str_starts_with("/", path) ? path : get_absolute_path(input_path + "/" + path); });


        #define LOADSHOWCFG_STRING(var, name, label) \
            LOADCFG_STRING(var, name, label) \
            SHOWCFG_STRING(var, label)

        #define LOADSHOWCFG_STRING_PATH(var, name, label) \
            LOADCFG_STRING_PATH(var, name, label) \
            SHOWCFG_STRING(var, label)


        #define LOADSHOWCFG_VECTOR(var, name, label) \
            LOADCFG_VECTOR(var, name, label) \
            SHOWCFG_VECTOR(var, label)

        #define LOADSHOWCFG_VECTOR_PATH(var, name, label) \
            LOADCFG_VECTOR_PATH(var, name, label) \
            SHOWCFG_VECTOR(var, label)

        LOADSHOWCFG_STRING_PATH(build_folder, "build-folder", "Build folder")
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

int safe_main(int argc, char* argv[]) {
    try {
        cout << "======= COMPILE =======" << endl;

        mutex mtx;
        Arguments args(argc, argv);
        build_config config(args);


        // string infile = get_absolute_path(args.get<string>(1));  
        // string cfgfile = args.get<string>("config", replace_extension(infile, ".compile.json"));
        // JSON allcfg(file_exists(cfgfile) ? file_get_contents(cfgfile) : default_configs);
        // vector<string> modes;
        // if (args.has(pair("coverage", "c"))) modes.push_back("coverage");
        // if (args.has(pair("debug", "d"))) modes.push_back("debug");
        // if (args.has(pair("fast", "f"))) modes.push_back("fast");
        // if (args.has(pair("test", "t"))) modes.push_back("test");
        // if (args.has(pair("run", "r"))) modes.push_back("run");
        // if (modes.empty()) modes.push_back("fast");
        // JSON conf = allcfg.get<JSON>(implode("-", modes));

        // Settings settings(args, conf);

        // string inpath = get_path(config.input_file);
        // string outpath = get_absolute_path(inpath + "/" + config.build_folder);
        if (!file_exists(config.build_folder)) if (!mkdir(config.build_folder)) throw ERROR("Unable to create folder: " + config.build_folder);
        // string outext = settings.get<string>("output-extension", "");
        // string objext = settings.get<string>("object-extension", ".o");
        // vector<string> impexts = settings.get<vector<string>>("source-extension", {".cpp", ".c"}); // ".cpp"; // TODO: make it array (".c", ".cpp") to config
        // string outfname = replace_extension(remove_path(config.input_file), config.output_extension);
        // string outfile = get_absolute_path(outpath + "/" + outfname);
        // string hash = config.hash;
        string outpath_hash = config.build_folder + "/" + config.hash;
        string outfname_hash = outpath_hash + "/" + remove_path(config.output_file);
        if (!file_exists(outpath_hash)) if (!mkdir(outpath_hash)) throw ERROR("Unable to create folder: " + outpath_hash);
        string depcachepath = outpath_hash + "/.depcache"; // TODO: to config
        // vector<string> flags = settings.get<vector<string>>("flags", {});
        // vector<string> libs = settings.get<vector<string>>("libs", {});
        // vector<string> idirs = settings.get<vector<string>>("include-path", {});
        // vector<string> sdirs = settings.get<vector<string>>("source-path", {});
        // foreach (idirs, [&](string& idir) { idir = get_absolute_path(inpath + "/" + idir); });
        // bool verbose = settings.get<bool>("verbose", true);

        if (config.verbose) {
            cout << "Input file ....: " << config.input_file << endl;
            cout << "Output file ...: " << config.output_file << endl;
            cout << "Hash ..........: " << config.hash << endl;
            // cout << "Config file ...: " << cfgfile << (file_exists(cfgfile) ? "" : " (not found)") << endl;
            // cout << "Modes .........: " << implode(", ", modes) << endl;
            // cout << "Include path...: " << endl;
            // foreach(idirs, [](const string& idir) { cout << "\t" << idir << endl; });
        }

        depmap_t depmap;
        ms_t lstmtime = filemtime_ms(config.input_file);
        if (!file_exists(depcachepath)) {
            if (config.verbose) cout << "Building dependency map for: " << config.input_file << endl;
            scan_includes(mtx, config.input_file, depmap, config.include_path, config.source_path, config.source_extensions, config.verbose);
            save_depcache(depcachepath, depmap);
        } else {
            depmap = load_depcache(depcachepath);
        }
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
