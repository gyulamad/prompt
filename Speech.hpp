#pragma once

#include <iostream>
#include <string>
// #include <fstream>

#include "Proc.hpp"
#include "tools.hpp"
#include "JSON.hpp"

using namespace std;

class Speech {
private:
    const string speechf;
    const string elogf;

    bool interrupted;

    string misschars() {
        interrupted = true;
        while (kbhit()) getchar();
        return "";
    }

public:
    Speech(
        const string& speechf = "/tmp/temp-speech.wav", 
        const string& elogf = "/dev/null"
    ): 
        speechf(speechf), 
        elogf(elogf)
    {}
    
    ~Speech() {}

    bool is_interrupted() { // TODO: depricated
        return interrupted;
    }

    void shtup() {
        Proc pkiller;
        pkiller.writeln("pkill espeak");
    }

    string rec(int timeout = 20) {
        Proc proc;
        const string secret = trim(file_get_contents("hugging.key"));

        interrupted = false;

        string output;

        while (true) {
// cout << "Delete: " << speechf << endl;
            proc.writeln("rm -f " + speechf + " && echo");
            while ((output = proc.read()).empty()) if (kbhit()) return misschars(); // waiting for "done"
// cout << "outp: " << output << endl;

            // recording the speech chunk
// cout << "Recording... (timeout " + to_string(timeout) + ")" << endl;
            proc.writeln(
                "timeout " + to_string(timeout) + " "
                "arecord -f cd -t wav -r 16000 -c 1 2>" + elogf + " | " // TODO error to a log that is "/dev/null" by default + add every parameter cusomizable
                "sox -t wav - -t wav " + speechf + " silence 1 0.1 2% 1 0.5 2% 2>/dev/null && "
                "echo \"\"");
            
            //cout << proc.read() << endl;
            while ((output = proc.read()).empty()) if (kbhit()) return misschars(); // waiting for "done"
// cout << "outp: " << output << endl;


            // skipp transcriptions if silence
// cout << "Silent?" << endl;
            proc.writeln("sox " + speechf + " -n stat 2>&1 | grep \"Maximum amplitude\" | awk '{print $3}'");
            while ((output = proc.read()).empty()) if (kbhit()) return misschars(); // waiting for sox to tell if silent?
// cout << "??" << output << endl;
            if (str_starts_with(output, "0.00")) continue;

            shtup();

            // transcribe
// cout << "transcribe..." << endl;
            proc.writeln(
                "curl https://api-inference.huggingface.co/models/jonatasgrosman/wav2vec2-large-xlsr-53-hungarian \
                    -X POST \
                    --data-binary '@" + speechf + "' \
                    -H 'Authorization: Bearer " + secret + "' -s");
            while ((output = proc.read()).empty()) if (kbhit()) return misschars(); // waiting for transcriptions
// cout << "resp: " << output << endl;

            // extract text

            JSON json(output);
            if (!json.isDefined("text")) {
                cerr << "STT transcibe failed:" + json.dump() << endl;
                return "";
                //break;  // TODO throw ERROR("STT transcibe failed:" + json.dump());
            }
            output = json.get<string>("text");

            if (!trim(output).empty()) break; // restart/break recording if it was silent
        }
        if (kbhit()) return misschars();
        return output;
    }

    string say(const string& text, int speed = 100, bool async = false) {        
        Proc proc;
        proc.writeln("espeak -vhu -s " + to_string(speed) + " \"" + esc(text) + "\"" + (async ? "" : " && echo"));
        if (!async) while ((proc.read()).empty()) if (kbhit()) {
            shtup();
            return misschars(); // waiting for "done"
        }
        return text;
    }

};