#pragma once

#include <iostream>
#include <string>
#include <atomic>

using namespace std;

namespace tools::abstracts {

    template<typename T>
    class UserInterface {
    public:
        virtual bool readln(T& input) {
            lock_guard<mutex> lock(imutex);
            getline(cin, input);
            return false;
        }

        virtual T readln() {
            T input;
            readln(input);
            return input;
        }

        virtual void clearln() {
            lock_guard<mutex> lock(output_mutex);
            cout << "\33[2K\r" << flush;
        }

        virtual void print(const T& output) {
            lock_guard<mutex> lock(output_mutex);
            cout << output << flush;
        }
        
        virtual void println(const T& output) {
            print(output + "\n");
        }

        virtual void hide() { visible = false; }
        virtual void show() { visible = true; }
    protected:
        mutex imutex;
        mutex output_mutex;
        atomic<bool> visible = true;
    };

}