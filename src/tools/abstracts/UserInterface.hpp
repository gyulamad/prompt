#pragma once

#include <iostream>
#include <string>
#include <atomic>

using namespace std;

namespace tools::abstracts {

    template<typename T>
    class UserInterface { // TODO: extend Printer (and maybe create a Reader too)
    public:

        // LCOV_EXCL_START
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
        // LCOV_EXCL_STOP

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

        virtual void clearln() {
            print("\33[2K\r");
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

#ifdef TEST

// #include "../tools/utils/Test.hpp"
#include <sstream>

using namespace std;
using namespace tools::abstracts;

void test_UserInterface_print() {
    UserInterface<string> ui;
    string output = capture_cout([&]() { ui.print("test"); });
    assert(output == "test" && "print() should output the given string");
}

void test_UserInterface_println() {
    UserInterface<string> ui;
    string output = capture_cout([&]() { ui.println("test"); });
    assert(output == "test\n" && "println() should output the given string with a newline");
}

void test_UserInterface_println_empty() {
    UserInterface<string> ui;
    string output = capture_cout([&]() { ui.println(""); });
    assert(output == "\n" && "println() with empty string should output a newline");
}

void test_UserInterface_clearln() {
    UserInterface<string> ui;
    string output = capture_cout([&]() { ui.clearln(); });
    assert(output == "\33[2K\r" && "clearln() should output the clear line sequence");
}

void test_UserInterface_print_empty() {
    UserInterface<string> ui;
    string output = capture_cout([&]() { ui.print(); });
    assert(output == "" && "print() with no arguments should output an empty string");
}

void test_UserInterface_println_no_args() {
    UserInterface<string> ui;
    string output = capture_cout([&]() { ui.println(); });
    assert(output == "\n" && "println() with no arguments should output a newline");
}


TEST(test_UserInterface_print);
TEST(test_UserInterface_println);
TEST(test_UserInterface_println_empty);
TEST(test_UserInterface_clearln);
TEST(test_UserInterface_print_empty);
TEST(test_UserInterface_println_no_args);

#endif
