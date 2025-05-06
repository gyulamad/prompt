#include "io.h"
#include <sstream>
#include <limits>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

using namespace std;

namespace tools::utils {

    // Define mutexes
    mutex io_readln_mtx;
    mutex io_kbhit_mtx;
    mutex io_confirm_mtx;

    void write(const string& output) {
        cout << output << flush;
    }

    void writeln(const string& output) {
        cout << output << endl;
    }

    string readln(const string& prompt) {
        lock_guard<mutex> lock(io_readln_mtx);
        string input;
        write(prompt);
        getline(cin, input);
        return input;
    }

    bool kbhit() {
        lock_guard<mutex> lock(io_kbhit_mtx);
        struct termios oldt, newt;
        int ch;
        int oldf;

        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
        ch = getchar();
        fcntl(STDIN_FILENO, F_SETFL, oldf);

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

        if (ch != EOF) {
            ungetc(ch, stdin);
            return true;
        }

        return false;
    }

    bool kbhit_chk(int key) {
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

    bool confirm(const string& message, int def) {
        lock_guard<mutex> lock(io_confirm_mtx);
        while (kbhit()) getchar();

        def = tolower(def);
        int choice;

        cin.clear();
        while (true) {
            cout << message << " (" 
                      << (def == 'y' ? "Y/n" : "y/N") << "): ";
            choice = cin.get();

            if (choice == '\n' || choice == '\r') {
                cin.clear();
                return def == 'y';
            }
            cin.clear();

            choice = tolower(choice);
            if (choice == 'y') {
                return true;
            } else if (choice == 'n') {
                return false;
            }

            cout << "Please press 'y' or 'n'." << endl;
        }
    }

    string capture_cout(function<void()> func) {
        streambuf* original_cout_buffer = cout.rdbuf();
        stringstream buffer;
        cout.rdbuf(buffer.rdbuf());
        try {
            func();
        } catch (exception& e) {
            cout.rdbuf(original_cout_buffer);
            throw ERROR("Error in stdout capture: " + string(e.what()));
        }
        cout.rdbuf(original_cout_buffer);
        cout.clear();
        return buffer.str();
    }

    string capture_cerr(function<void()> func) {
        streambuf* original_cerr_buffer = cerr.rdbuf();
        stringstream buffer;
        cerr.rdbuf(buffer.rdbuf());
        try {
            func();
        } catch (exception& e) {
            cerr.rdbuf(original_cerr_buffer);
            throw ERROR("Error in stderr capture: " + string(e.what()));
        }
        cerr.rdbuf(original_cerr_buffer);
        cerr.clear();
        return buffer.str();
    }

    string capture_cout_cerr(function<void()> func) {
        streambuf* original_cout_buffer = cout.rdbuf();
        streambuf* original_cerr_buffer = cerr.rdbuf();
        stringstream buffer;
        cout.rdbuf(buffer.rdbuf());
        cerr.rdbuf(cout.rdbuf());
        try {
            func();
        } catch (exception& e) {
            cout.rdbuf(original_cout_buffer);
            cerr.rdbuf(original_cerr_buffer);
            throw ERROR("Error in stdout and stderr capture: " + string(e.what()));
        }
        cout.rdbuf(original_cout_buffer);
        cerr.rdbuf(original_cerr_buffer);
        cout.clear();
        cerr.clear();
        return buffer.str();
    }

} // namespace tools::utils

#ifdef TEST

#include "Test.h"
#include "assert.hpp"

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

#endif // TEST