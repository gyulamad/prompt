#pragma once

#include <vector>
#include <string>

#include "../utils/ERROR.h"

using namespace std;
using namespace tools::utils;

namespace tools::cmd {

    class Command {
    public:
        Command(const string& prefix): prefix(prefix) {}
        virtual ~Command() {}

        virtual vector<string> getPatterns() const = 0;
        virtual string getName() const = 0;
        virtual string getDescription() const = 0;
        virtual string getUsage() const = 0;
        
        virtual const vector<string>& validate(const vector<string>& args) {
            // Override if need validation and "throw ERROR(<validation failure message to the caller>);" on error
            return args;
        }

        /**
         * @brief 
         * 
         * @param user 
         * @param args 
         */
        virtual void run(void*, const vector<string>&) = 0;
        
    protected:
        string prefix;
    };    

}