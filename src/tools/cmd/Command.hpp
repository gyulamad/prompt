#pragma once

#include <vector>
#include <string>

#include "../utils/ERROR.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::cmd {

    class Command {
    public:
        virtual ~Command() {}

        virtual vector<string> getPatterns() const UNIMP_THROWS

        virtual string getUsage() const UNIMP_THROWS
        
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
        virtual void run(void*, const vector<string>&) UNIMP_THROWS
    };    

}