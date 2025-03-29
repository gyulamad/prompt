#pragma once

#include <iostream>

using namespace std;

namespace tools::utils {

    class Printer {
    public:
        virtual void print(const string& text) {
            cout << text << flush;
        }
    
        virtual void println(const string& text) {
            print(text + "\n");
        }
    };

}