#pragma once

#include <iostream>
#include <string>
#include <limits>
#include <atomic>
#include <mutex>
#include <sstream> // Required for stringstream
#include <functional> // Add this for function

#include <fcntl.h>
#include <unistd.h>
#include <termios.h> // For tcgetattr(), tcsetattr()

#include "ERROR.hpp"

using namespace std;

namespace tools::utils {

    // atomic<bool> io_input_active(false); // Shared boolean variable
    mutex io_readln_mtx; // Mutex to protect access to the variable
    mutex io_kbhit_mtx; // Mutex to protect access to the variable
    mutex io_confirm_mtx; // Mutex to protect access to the variable

    void write(const string& output = "") {
        cout << output << flush;
    }

    void writeln(const string& output = "") {
        cout << output << endl;
    }

    string readln(const string& prompt = "") {
        lock_guard<mutex> lock(io_readln_mtx); // Lock the mutex
        // if (io_input_active) return false; // Disable others if input is active
        // io_input_active = true;

        string input;
        write(prompt);
        getline(cin, input);
        return input;
    }

    // int kbhit() {
    //     struct timeval tv = {0, 0};
    //     fd_set fds;
    //     FD_ZERO(&fds);
    //     FD_SET(STDIN_FILENO, &fds);
    //     return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
    // }
    
    // Function to check if a key has been pressed (non-blocking)
    bool kbhit() {
        lock_guard<mutex> lock(io_kbhit_mtx); // Lock the mutex
        // if (io_input_active) return false; // Disable others if input is active
        // io_input_active = true;
    
        struct termios oldt, newt;
        int ch;
        int oldf;

        // Get the current terminal settings
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;

        // Disable canonical mode and echo
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        // Check if a key has been pressed
        oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
        ch = getchar();
        fcntl(STDIN_FILENO, F_SETFL, oldf);

        // Restore the terminal settings
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

        if (ch != EOF) {
            ungetc(ch, stdin); // Put the character back into the input buffer
            return true;
        }

        return false;
    }

    // Check for a specific keypress (e.g., ESC) non-blocking
    bool kbhit_chk(int key = 27) {
        lock_guard<mutex> lock(io_kbhit_mtx);

        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
        int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    
        int ch = getchar();
        cout << ch << endl;
    
        fcntl(STDIN_FILENO, F_SETFL, oldf);
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    
        return (ch == key);
    }


    bool confirm(const string& message, char def = 'y') {
        lock_guard<mutex> lock(io_confirm_mtx); // Lock the mutex

        while (kbhit()) getchar();

        // if (io_input_active) return false; // Disable others if input is active
        // io_input_active = true;

        def = tolower(def); // Normalize the default to lowercase
        char choice;
        
        cin.clear();
        while (true) {
            // Display the prompt with the default option
            cout << message << " (" 
                    << (def == 'y' ? "Y/n" : "y/N") << "): ";
            choice = cin.get();

            // Handle Enter (newline) input for default option
            if (choice == '\n' || choice == '\r') {
                cin.clear();
                return def == 'y';
            }
            cin.clear();

            //cin.clear();// ignore(numeric_limits<streamsize>::max(), '\n');

            // Clear the input buffer to handle extra characters after one key press
            // while (cin.get() != '\n') {
            //     cout << 
            //     usleep(30000);
            // }

            // Normalize choice to lowercase and evaluate
            choice = tolower(choice);
            if (choice == 'y') {
                return true;
            } else if (choice == 'n') {
                return false;
            }

            // Invalid input, prompt again
            cout << "Please press 'y' or 'n'." << endl;
            //cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }


    // Helper function to capture cout output
    string capture_cout(function<void()> func) {
        streambuf* original_cout_buffer = cout.rdbuf(); // Store original buffer
        stringstream buffer;
        cout.rdbuf(buffer.rdbuf()); // Redirect cout to buffer
        try {
            func(); // Call the function
        } catch (exception& e) {
            cout.rdbuf(original_cout_buffer); // Restore original cout
            throw ERROR("Error in stdout capture: " + string(e.what()));
        }
        cout.rdbuf(original_cout_buffer); // Restore original cout
        cout.clear(); // Clear any error flags
        return buffer.str();
    }

    // Helper function to capture stderr output
    string capture_cerr(function<void()> func) {
        streambuf* original_cerr_buffer = cerr.rdbuf(); // Store original buffer
        stringstream buffer;
        cerr.rdbuf(buffer.rdbuf()); // Redirect cerr to buffer
        try {
            func(); // Call the function
        } catch (exception& e) {
            cerr.rdbuf(original_cerr_buffer); // Restore original cerr
            throw ERROR("Error in stderr capture: " + string(e.what()));
        }
        cerr.rdbuf(original_cerr_buffer); // Restore original cerr
        cerr.clear(); // Clear any error flags
        return buffer.str();
    }
    
    // Helper function to capture both cout and cerr output
    string capture_cout_cerr(function<void()> func) {
        streambuf* original_cout_buffer = cout.rdbuf(); // Store original buffers
        streambuf* original_cerr_buffer = cerr.rdbuf();
        stringstream buffer;
        cout.rdbuf(buffer.rdbuf()); // Redirect cout to buffer
        cerr.rdbuf(cout.rdbuf()); // Redirect cerr to cout's redirected buffer
        try {
            func(); // Call the function
        } catch (exception& e) {
            cout.rdbuf(original_cout_buffer); // Restore original cout
            cerr.rdbuf(original_cerr_buffer); // Restore original cerr
            throw ERROR("Error in stdout and stderr capture: " + string(e.what()));
        }
        cout.rdbuf(original_cout_buffer); // Restore original cout
        cerr.rdbuf(original_cerr_buffer); // Restore original cerr
        cout.clear(); // Clear any error flags on cout
        cerr.clear(); // Clear any error flags on cerr
        return buffer.str();
    }

}

#ifdef TEST

#include "Test.hpp"

using namespace tools::utils;

void test_capture_cout_basic() {
    string actual = capture_cout([]() { cout << "Hello, world!"; });
    assert(actual == "Hello, world!" && "capture_cout: Basic cout capture failed");
}

void test_capture_cout_empty() {
    string actual = capture_cout([]() {});
    assert(actual.empty() && "capture_cout: Empty output should return an empty string");
}

void test_capture_cerr_basic() {
    string actual = capture_cerr([]() { cerr << "Error occurred!"; });
    assert(actual == "Error occurred!" && "capture_cerr: Basic cerr capture failed");
}

void test_capture_cerr_empty() {
    string actual = capture_cerr([]() {});
    assert(actual.empty() && "capture_cerr: Empty output should return an empty string");
}

void test_capture_cout_cerr_basic() {
    string actual = capture_cout_cerr([]() {
        cout << "Stdout message. ";
        cerr << "Stderr message.";
    });
    assert(actual == "Stdout message. Stderr message." && "capture_cout_cerr: Combined output capture failed");
}

void test_capture_cout_cerr_empty() {
    string actual = capture_cout_cerr([]() {});
    assert(actual.empty() && "capture_cout_cerr: Empty output should return an empty string");
}

// Register tests
TEST(test_capture_cout_basic);
TEST(test_capture_cout_empty);
TEST(test_capture_cerr_basic);
TEST(test_capture_cerr_empty);
TEST(test_capture_cout_cerr_basic);
TEST(test_capture_cout_cerr_empty);

#endif
