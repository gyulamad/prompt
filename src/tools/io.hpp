#pragma once

#include <iostream>
#include <string>

#include <fcntl.h>
#include <unistd.h>
#include <termios.h> // For tcgetattr(), tcsetattr()

using namespace std;

namespace tools {

    void write(const string& output = "") {
        cout << output << flush;
    }

    void writeln(const string& output = "") {
        cout << output << endl;
    }

    string readln(const string& prompt = "") {
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

}
