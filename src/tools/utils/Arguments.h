#pragma once

#include <vector>
#include <string>
#include <utility>

#include "../str/str_starts_with.h"
#include "../str/parse.h"
#include "ERROR.h"

using namespace std;
using namespace tools::str;

namespace tools::utils {

    class Arguments {
        vector<string> args;

    public:
        using Key = pair<string, string>;

        // Constructor that initializes from command-line arguments
        Arguments(int argc, char* argv[]);

        const vector<string>& getArgsCRef() const;

        // Check if an argument exists
        bool has(const string& key, const string& prefix = "--") const;
        bool has(const Key& keys, const string& prefix = "--", const string& prefix_short = "-") const;
        bool has(size_t at) const;

        // Get the index of a specific argument
        long int indexOf(const string& arg, const string& prefix = "--") const;
        long int indexOf(const Key& keys, const string& prefix = "--", const string& prefix_short = "-") const;

        // Deprecated Bool getters
        bool getBool(const string& key) const;
        bool getBool(const string& key, bool defval) const;
        bool getBool(size_t at) const;
        bool getBool(size_t at, bool defval) const;

        // Deprecated String getters
        const string getString(const string& key) const;
        const string getString(const string& key, const string& defval) const;
        const string getString(size_t at) const;
        const string getString(size_t at, const string& defval) const;

        // Deprecated Int getters
        int getInt(const string& key) const;
        int getInt(const string& key, int defval) const;
        int getInt(size_t at) const;
        int getInt(size_t at, int defval) const;

        // Deprecated Double getters
        double getDouble(const string& key) const;
        double getDouble(const string& key, double defval) const;
        double getDouble(size_t at) const;
        double getDouble(size_t at, double defval) const;

        // =================================== Templated getters ===================================

        // Templated getter for key-based lookup
        template<typename T>
        T get(const string& key, const string& prefix = "--") const {

            // Special handling for boolean flags
            if constexpr (is_same_v<T, bool>) {
                long int idx = indexOf(key, prefix);

                // If the flag is missing, return false
                if (idx == -1) {
                    return false;
                }
                string arg = args[idx];
                string prefixed_key = prefix + key + "=";
                // If the argument is just the flag (e.g., "-v"), return true
                if (arg == prefix + key) {
                    return true;
                }
                // If the argument has a value (e.g., "-v=true"), parse it
                if (str_starts_with(arg, prefixed_key)) {
                    string value = arg.substr(prefixed_key.length());
                    if (value.empty()) throw ERROR("Missing value for argument: " + key);
                    return parse<T>(value);
                }
                // If the flag is present but no value follows, assume true
                return true;
            } else {
                long int idx = indexOf(key, prefix);

                // Non-boolean handling
                if (idx == -1) throw ERROR("Missing argument: " + key);
                string arg = args[idx];
                string prefixed_key = prefix + key + "=";
                if (str_starts_with(arg, prefixed_key)) {
                    // Extract value after "="
                    string value = arg.substr(prefixed_key.length());
                    if (value.empty()) throw ERROR("Missing value for argument: " + key);
                    return parse<T>(value);
                } else if (idx + 1 < (long int)(args.size())) {
                    // Value is in the next argument
                    return parse<T>(args[idx + 1]);
                } else {
                    throw ERROR("Missing value for argument: " + key);
                }
            }
        }

        // Templated getter for key-based lookup (long/short flags)
        template<typename T>
        T get(const Key& keys, const string& prefix = "--", const string& prefix_short = "-") const {
            if (has(keys.first, prefix)) return get<T>(keys.first, prefix);
            else if (has(keys.second, prefix_short)) return get<T>(keys.second, prefix_short);
            // For bool, return false if flag is missing; otherwise, throw
            if constexpr (is_same_v<T, bool>) {
                return false;
            } else {
                throw ERROR("Missing argument: " + prefix + keys.first + " (or " + prefix_short + keys.second + ")");
            }
        }

        // Templated getter with default value for key-based lookup
        template<typename T>
        T get(const string& key, const T& defval, const string& prefix = "--") const {
            return has(key, prefix) ? get<T>(key, prefix) : defval;
        }

        // Templated getter with default value for key-based lookup (long/short flags)
        template<typename T>
        T get(const Key& keys, const T& defval, const string& prefix = "--", const string& prefix_short = "-") const {
            return has(keys, prefix, prefix_short) ? get<T>(keys, prefix, prefix_short) : defval;
        }

        // Templated getter for positional lookup
        template<typename T>
        T get(size_t at) const {
            if (!has(at)) throw ERROR("Missing argument at: " + to_string(at));
            return parse<T>(args[at]);
        }

        // Templated getter with default value for positional lookup
        template<typename T>
        T get(size_t at, const T& defval) const {
            return !has(at) ? defval : parse<T>(args[at]);
        }
    };

} // namespace tools::utils