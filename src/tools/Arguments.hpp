#pragma once

#include <vector>
#include <string>
#include <algorithm>

#include "ERROR.hpp"

using namespace std;

namespace tools {

    class Arguments {
        vector<string> args;

    public:
        // Constructor that initializes from command-line arguments
        Arguments(int argc, char* argv[]) {
            for (int i = 0; i < argc; i++) 
                args.push_back(string(argv[i]));
        }

        // Check if an argument exists
        bool has(const string& arg) const {
            return find(args.begin(), args.end(), "--" + arg) != args.end();
        }

        // Get the index of a specific argument
        long int indexOf(const string& arg) const {
            auto it = find(args.begin(), args.end(), "--" + arg);
            return it != args.end() ? distance(args.begin(), it) : -1;
        }

        // Get a boolean value based on the presence of a flag
        bool getBool(const string& key) const {
            return has(key);
        }

        // Get a string value associated with a flag
        const string getString(const string& key) const {
            long int idx = indexOf(key);
            if (idx == -1 || idx + 1 >= (signed)args.size()) 
                throw ERROR("Missing value for argument: " + key);
            return args[(unsigned)(idx + 1)];
        }

        // Get a string value at position
        const string getString(size_t at) const {
            if (at >= args.size()) 
                throw ERROR("Missing argument at: " + at);
            return args[at];
        }

        // Get an integer value associated with a flag
        int getInt(const string& key) const {
            try {
                return stoi(getString(key));
            } catch (const exception &e) {
                throw ERROR("Invalid integer value at key: " + key + ", reason: " + e.what());
            }
        }
    };
    
}