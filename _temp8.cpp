#include <iostream>
#include "Proc.hpp"
#include "tools.hpp"
int main() {
    Proc proc;
    cout << "[d1]" << endl;
    proc.writeln("espeak -vhu -s 200 \"A nap sugara aranyló,\nFényt hint a mezőre, haló.\nA szél susog a fák között,\nTitkokat suttog, édes, sőt.\n\nKék ég borul a táj fölé,\nMadár dalol a lombok vége.\nNyugalom száll a szívekre,\nCsoda a természet e szépségekre.\n\nA hold fényében ezüstös,\nA tó vizén csendes, hűvös.\nA csillagok ragyognak fent,\nEgy álomban messze, lent.\n\nA szél simogatja az arcot,\nBékesség árad a tájat, sőt.\nTitokzatos a sötét éj,\nDe szépségét mégis élvezem én.\" && echo");
    cout << "[d2]" << endl;
    while ((proc.read()).empty()) if (kbhit()) {
        cout << "[d3]" << endl;
        while (kbhit()) getchar(); // waiting for "done"
        cout << "[d4]" << endl;

        Proc pkiller;
        pkiller.writeln("pkill espeak");
        return 0;
    }
    cout << "[d5]" << endl;
    return 0;
}