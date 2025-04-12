#pragma once

#include <iostream>
#include <string>
#include <atomic>

using namespace std;

namespace tools::abstracts {

    template<typename T>
    class UserInterface { // TODO: extend Printer (and maybe create a Reader too)
    public:
        virtual bool readln(T& input) {
            lock_guard<mutex> lock(input_mutex);
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
        
        virtual void print() {
            print("");
        }
        
        virtual void println(const T& output) {
            print(output + "\n");
        }

        virtual void println() {
            println("");
        }
        

        // virtual void set_prompt_visible(bool prompt_visible) { this->prompt_visible = prompt_visible; }
        // virtual void hide_prompt() { set_prompt_visible(false); }
        // virtual void show_prompt() { set_prompt_visible(true); }
        // virtual bool is_prompt_visible() const { return prompt_visible; }
        // virtual bool is_prompt_hidden() const { return !prompt_visible; }

    protected:
        mutex input_mutex;
        mutex output_mutex;

        // atomic<bool> prompt_visible = true;
    };

}