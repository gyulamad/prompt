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

        virtual vector<string> get_patterns() const UNIMP_THROWS
        
        /**
         * @brief 
         * 
         * @param user 
         * @param args 
         * @return string
         */
        virtual string run(void*, const vector<string>&) UNIMP_THROWS
    };    

}