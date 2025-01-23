#include <vector>

#include "../src/tools/io.hpp"

using namespace tools;

int main() {
    cout << "Start typing..." << endl;
    while (true) {
        if (kbhit()) {
            cout << "_________[ KEY HIT ]_________" << endl;
            string s;
            while (kbhit()) {
                char ch = getchar();
                cout << "Character: ";
                putchar(ch);
                s.push_back(ch);
                cout << endl;
                cout << "Value: " << to_string(ch) << endl;
            }
            cout << "_____________________________" << endl;
            if (s.size() > 1) {
                s[0] = '\\';
                cout << "String: " << s << endl;
                cout << "Length: " << s.size() << endl;
            }
            cout << endl;
        }
    }

    return 0;
}