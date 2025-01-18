#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>

#include "Proc.hpp"

using namespace std;

int main() {
    vector<string> inputs = {
        "echo \"First line of input\"\n",
        "echo $((11 + 22))\n",
        "mkdir -p practice\n"
        "echo $((33 + 22))\n",
        "exit\n" // Command to terminate the child program
    };

    try {
        Proc proc("bash");
        for (const auto& input : inputs) {
            proc.write(input);
            sleep(1); // Simulate some delay for the child to process input

            if (proc.ready()) {
                string output = proc.read();
                cout << "Child says: " << output << flush;
            } else {
                cout << "No output available yet, continuing...\n";
            }
        }
        proc.kill();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
}
