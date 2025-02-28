#pragma once

#include <vector>
#include <string>

#include "../ERROR.hpp"

using namespace std;
using namespace tools;

namespace tools::cmd {

    class Command {
    public:
        virtual ~Command() {}

        virtual vector<string> get_patterns() const UNIMP
        
        /**
         * @brief 
         * 
         * @param user 
         * @param args 
         * @return string
         */
        virtual string run(void*, const vector<string>&) UNIMP
    };    

}