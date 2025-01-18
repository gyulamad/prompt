#include <functional>
#include <map>
#include <thread>
#include <chrono>

#include "tools.hpp"
#include "Terminal.hpp"


class Commander {
private:
    TerminalIO& term;
    bool closing;
    string line;
    map<string, function<void(Commander&, const string&)>> handlers;

public:

    Commander(TerminalIO& term): 
        term(term), 
        closing(true), 
        line("") {}

    void readln(bool echo = true) {
        char c = term.getc();
        // if (c) cout << to_string(c) << endl;
        if (echo) cout << c << flush;
        line += c;
        if (c == 13 && !line.empty()) {
            if (echo) cout << endl;
            // Reverse iteration
            for (auto it = handlers.rbegin(); it != handlers.rend(); ++it) {
                cout << "[" << it->first << "][" << line << "]" << endl;
                if (str_starts_with(line, it->first)) {
                    cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
                    handlers[it->first](*this, line);
                    line = "";
                    return;
                }
            }
            cout << "Invalid command." << endl;  
            prompt();          
            line = "";
        }
    }

    void handle(string command, function<void(Commander&, const string&)> handler) {
        if (handlers.contains(command))
            throw ERROR("Command already registered: " + command);
        handlers[command] = handler;
    }

    void close() {
        closing = true;
    }

    bool tick() {
        this_thread::sleep_for(chrono::microseconds{10000});
        readln();
        return closing;
    }

    void prompt() {
        cout << "\r> " << flush;
    }

};


int main() {
    TerminalIO term;
    // TerminalEmulator temu;
    // string line;
    // char c;
    Commander commander(term);
    commander.handle("q", [](Commander& commander, const string&) {
        commander.close();
    });
    commander.prompt();
    while(commander.tick());

        // term.write(temu.read());
        // temu.write(term.read());
    //     char c = term.getc();

    //     cout << c << flush;
    //     if (c == 27) break;
    // }

    return 0;
}