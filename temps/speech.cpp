#include "../src/tools/Speech.hpp"

using namespace tools;

void test1 () {
    string secretf = "../builds/.prompt/hugging.key";
    if (!file_exists(secretf)) ERROR("no secret file");
    Speech speech(trim(file_get_contents(secretf)), "hu");
    string input = 
        //"AZ A BAJ HOGY SOSEM VESZEDÉSZRE HOGY MIKOR KEZDEKELL BESZÉLNI IDŐBEN ÉS MINDIG LEVÁGODA SZÖVEG ELEJÉT EZÉRT NEM TUDOM HOGY MI AZ ELSŐ BETŰ MERT AZ MINDIG LEMARAD PERSZE GOMBA PÖRKÜLT ÍZÉRŐL IS SÉMÁM BESZÉLHETÜNK HOGY HA AKARSZ";
        //"Hello! Hogy vagy ma faszikam? Most ide kellene irnom valamit ami hosszi de nincs ekezet a billentyumon"; 
        speech.rec();
    cout << input << endl; 
    speech.say(input);
}

void test2() {
    string secretf = "../builds/.prompt/hugging.key";
    if (!file_exists(secretf)) ERROR("no secret file");
    Speech speech(trim(file_get_contents(secretf)), "hu");

    string tellit = "AZ A BAJ HOGY SOSEM VESZEDÉSZRE HOGY MIKOR KEZDEKELL BESZÉLNI IDŐBEN ÉS MINDIG LEVÁGODA SZÖVEG ELEJÉT EZÉRT NEM TUDOM HOGY MI AZ ELSŐ BETŰ MERT AZ MINDIG LEMARAD PERSZE GOMBA PÖRKÜLT ÍZÉRŐL IS SÉMÁM BESZÉLHETÜNK HOGY HA AKARSZ";
    string input = speech.rec();
}

int main() {
    // test1();
    test2();
}